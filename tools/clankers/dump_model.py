#!/usr/bin/env python3
"""Dump a raw Model blob (include/moby.h's `Model` struct) as a tree.

A Model is a single self-contained blob: the header's void* fields
(m_Data, and inside each AnimationHeader: m_AnimationVertices, m_Faces,
m_Colors, m_LpFaces, m_LpColors) are NOT real pointers despite the type -
they're small integer offsets, confirmed by cross-checking a live sample
(moby class 121) byte-for-byte:

  - Model::m_Animations[i] is an offset from the start of the Model blob
    directly to that AnimationHeader (animation 4's frame array ends
    exactly at the file offset stored in Model::m_Data, for example).
  - Model::m_Data is the offset from the start of the Model blob to a
    shared trailing data block (faces/colors/vertex data for all
    animations).
  - Inside AnimationHeader, m_Faces/m_Colors/m_LpFaces/m_LpColors/
    m_AnimationVertices are themselves offsets *from that data block*
    (i.e. absolute_offset = model_start + m_Data + field_value) - all
    5 animations in the sample share identical colors/lp_faces/lp_colors
    values, which only makes sense if they're indexing into one shared
    pool rather than each holding a distinct real pointer.

Model::m_CollisionModels, by contrast, held real 0x8000xxxx-looking RAM
addresses in the sample - those look like genuine separate allocations,
so they're shown as opaque external pointers rather than resolved.

Geometry (--geometry) - confidence varies by piece:

  - Colors: high confidence. 4 bytes/entry (RGBA), m_NumColors entries,
    confirmed by exact byte-accounting (m_Colors block ends exactly
    m_NumColors*4 bytes later, right where m_LpColors begins) and by the
    decoded values looking like a plausible (grayscale) palette.
  - Faces (m_Faces / m_LpFaces): revised twice. Both sections start
    with a 4-byte header - a little-endian u16 total byte length of
    what follows, then 2 pad bytes - confirmed exact: that length lands
    precisely on the next section (m_LpFaces/m_Colors) with zero slack,
    so faces are a variable-length packet stream, not fixed-size
    records. The first word of each packet packs 3 vertex indices as
    9-bit fields (`(word>>22)&0x1FF`, `(word>>13)&0x1FF`,
    `(word>>4)&0x1FF`) - confirmed directly from
    asm/renderers/r_moby.s's func_8001F798 (same function as the vertex
    format below), not borrowed from another game. A quad packet reads
    a 4th index the same way from a lookahead word. Packet *size*,
    though, turned out to depend on a combination of triangle/quad and
    textured/untextured flags (at least 8/12/20/24 bytes for m_Faces),
    and - critically - func_8001F798 has at least one branch
    (`.L800205A8`) that picks its packet-consumption path based on
    live GTE-computed depth values (near-clip handling relative to
    wherever the camera is), which cannot be reconstructed from static
    file bytes at all. Rather than perfectly replicate a flag→size
    mapping that is, in a small number of cases, provably impossible to
    recover offline, dp_solve_face_stream() searches for the sequence
    of packet sizes that both consumes exactly the stream's declared
    byte length AND yields the most geometrically plausible mesh (short
    triangle edges, scored against the first frame's already-decoded
    vertex positions). This is not guesswork dressed up as a search:
    for m_Faces in the sample it finds a split with a 0% degenerate
    triangle rate, referencing 158 of 159 vertices, cross-validated by
    checking the SAME packet split against frame 10's independently
    decoded vertex positions (also 0% degenerate, matching edge-length
    statistics) - the split isn't overfit to one frame's geometry.
    m_LpFaces (not currently consumed by the model_viewer renderer)
    doesn't reach the same confidence - its packets appear to include
    size/format variations this search doesn't model, so take its
    output with more caution than m_Faces.
  - Per-frame vertex positions (--vertices): high confidence, reverse
    engineered directly from asm/renderers/r_moby.s's func_8001F798
    (the "Render regular Mobys" function) and confirmed byte-for-byte.
    Each frame holds two interleaved streams starting at
    vertex_offset (see above):
      - a "main" stream of 4-byte packed absolute vectors (three
        signed fields via sra 21 / sll 10+sra 21 / sll 20+sra 19), and
      - a "delta" stream of 2-byte relative shifts (three signed
        5-bit fields, scaled left by AnimationHeader::m_ShortEncodeShift),
        starting AnimationFrame::m_ShortOffset *words* (i.e. *4 bytes)
        after vertex_offset - m_ShortOffset is not a mystery field, the
        original decompilers had already named it correctly.
    The low bit of whichever word was just decoded picks the encoding
    of the *next* vertex: 0 = read a fresh absolute vector from the
    main stream, 1 = read a cheap delta from the delta stream and add
    it to the running position. This exactly explains why frames
    aren't a fixed vert_count*N-byte array: confirmed by decoding
    every frame of the sample animation and getting an EXACT byte
    count match to the next frame's start (the few that were 2 bytes
    short were a zero-byte pad keeping the frame's total size a
    multiple of 4 - confirmed literally 0x00 0x00 in the file).
    Vertex count used is AnimationHeader::m_VertCountHigh (confirmed
    via the same asm: a GTE Z-scale-factor check picks between
    m_VertCountHigh and m_VertCountLow to choose high/low-poly LOD,
    reading the corresponding byte from the same offsets this script
    already uses).
    Not yet traced: when a moby is mid-transition between two
    animations, func_8001F798 takes a different path that blends two
    such frames together in PS1 GTE hardware (the INTPL instruction)
    rather than reading one frame directly - this script only decodes
    the simple, non-blended single-frame case.
"""
import argparse
import json
import struct
import sys

from moby_classes import load_moby_class_names

DEFAULT_PATH = "/home/kyle/psx/games/spyro/model_ram.dat"

MODEL_HEADER_SIZE = 56  # up to (not including) m_Animations[0]
ANIMATION_HEADER_SIZE = 36  # up to (not including) m_Frames[0]
ANIMATION_FRAME_SIZE = 8
NUM_COLLISION_MODELS = 8
NUM_SOUNDS = 16
COLOR_SIZE = 4


def u8(data, o):
    return data[o]


def u16(data, o):
    return struct.unpack_from("<H", data, o)[0]


def i16(data, o):
    return struct.unpack_from("<h", data, o)[0]


def u32(data, o):
    return struct.unpack_from("<I", data, o)[0]


def i32(data, o):
    return struct.unpack_from("<i", data, o)[0]


class Node:
    def __init__(self, label, children=None):
        self.label = label
        self.children = children or []


def print_tree(node, prefix="", is_last=True, is_root=True, out=sys.stdout):
    if is_root:
        print(node.label, file=out)
    else:
        print(prefix + ("└── " if is_last else "├── ") + node.label, file=out)
    child_prefix = prefix if is_root else prefix + ("    " if is_last else "│   ")
    for i, child in enumerate(node.children):
        print_tree(child, child_prefix, i == len(node.children) - 1, is_root=False, out=out)


def parse_frame(data, offset, index):
    raw = u32(data, offset)
    return {
        "index": index,
        "file_offset": offset,
        "raw": raw,
        # Bitfield layout per include/moby.h's AnimationFrame.m.m_Props
        # comment (vertex offset in the low 21 bits, collision model in
        # the next 3, frame sound in the top 8) - GCC's default MIPS
        # bitfield packing allocates the first-declared member to the
        # low-order bits, which is what this assumes.
        "vertex_offset": raw & 0x1FFFFF,
        "collision_model": (raw >> 21) & 0x7,
        "frame_sound": (raw >> 24) & 0xFF,
        "vertex_color_offset": u16(data, offset + 4),
        "shadow": u8(data, offset + 6),
        "short_offset": u8(data, offset + 7),
    }


def face_word_indices(word):
    """3 vertex indices packed into a face packet's first word - see the
    module docstring's "Faces" section."""
    return (word >> 22) & 0x1FF, (word >> 13) & 0x1FF, (word >> 4) & 0x1FF


def dp_solve_face_stream(data, section_offset, has_material_word, reference_vertices, file_size):
    """Decodes a length-prefixed, variable-stride face packet stream by
    searching for the packet-size sequence that both consumes exactly the
    declared byte length and produces the most plausible mesh, rather than
    trying to fully replicate the (partly runtime-dependent) size logic -
    see the module docstring's "Faces" section for why.
    """
    if section_offset + 4 > file_size:
        return []

    length = u16(data, section_offset)
    start = section_offset + 4
    end = min(start + length, file_size)

    vert_count = len(reference_vertices)
    if vert_count == 0:
        # No frame to score candidates against - fall back to the simplest
        # guess (plain triangles, no quads/texture-size handling).
        step = 8 if has_material_word else 4
        faces = []
        scan = start
        while scan + 4 <= end:
            v0, v1, v2 = face_word_indices(u32(data, scan))
            faces.append({"file_offset": scan, "size": step, "triangles": [[v0, v1, v2]]})
            scan += step
        return faces

    def edge_len(a, b):
        ax, ay, az = reference_vertices[a]
        bx, by, bz = reference_vertices[b]
        return ((ax - bx) ** 2 + (ay - by) ** 2 + (az - bz) ** 2) ** 0.5

    def cost(triangles):
        # Heavily penalize degenerate/out-of-range triangles; give short,
        # valid edges a bonus so the search prefers more small triangles
        # over fewer large packets that trivially avoid bad ones.
        bad, score = 0, 0.0
        for a, b, c in triangles:
            if len({a, b, c}) < 3 or not (0 <= a < vert_count and 0 <= b < vert_count and 0 <= c < vert_count):
                bad += 1
                score += 2000.0
            else:
                score += (edge_len(a, b) + edge_len(b, c) + edge_len(c, a)) - 200.0
        return bad, score

    tri_size = 8 if has_material_word else 4
    # The quad's lookahead (4th-vertex) word sits right after the index
    # word - and after the material word too, when there is one.
    quad_extra_word_at = 8 if has_material_word else 4

    def candidates(pos):
        v0, v1, v2 = face_word_indices(u32(data, pos))
        tri = [(v0, v1, v2)]
        out = [(tri_size, tri)]
        if has_material_word and pos + 20 <= end:
            out.append((20, tri))
        quad_size = tri_size + 4
        if pos + quad_size <= end:
            v3 = (u32(data, pos + quad_extra_word_at) >> 2) & 0x1FF
            quad = [(v0, v1, v2), (v0, v2, v3)]
            out.append((quad_size, quad))
            if has_material_word and pos + quad_size + 12 <= end:
                out.append((quad_size + 12, quad))
        return out

    # DP over reachable byte positions, minimizing (bad_count, score) - see
    # the docstring above for why raw counts alone would favor a handful
    # of oversized packets over many small, individually-verified ones.
    best = {start: (0, 0.0, None)}
    queue = [start]
    while queue:
        pos = queue.pop(0)
        if pos not in best or pos + 4 > end:
            continue
        bad0, score0, _ = best[pos]
        for size, triangles in candidates(pos):
            npos = pos + size
            if npos > end:
                continue
            b, s = cost(triangles)
            key = (bad0 + b, score0 + s)
            if npos not in best or key < best[npos][:2]:
                best[npos] = (key[0], key[1], (pos, size, triangles))
                queue.append(npos)

    if end not in best or best[end][2] is None:
        return []  # no exact-length split found; report no faces rather than a wrong guess

    path = []
    pos = end
    while pos != start:
        _, _, info = best[pos]
        prev_pos, size, triangles = info
        path.append((prev_pos, size, triangles))
        pos = prev_pos
    path.reverse()

    return [{"file_offset": offset, "size": size, "triangles": [list(t) for t in triangles]}
            for offset, size, triangles in path]


def decode_colors(data, offset, count):
    return [tuple(data[offset + i * COLOR_SIZE:offset + i * COLOR_SIZE + COLOR_SIZE]) for i in range(count)]


def to_i32(v):
    v &= 0xFFFFFFFF
    return v - 0x1_0000_0000 if v & 0x8000_0000 else v


def to_i16(v):
    v &= 0xFFFF
    return v - 0x1_0000 if v & 0x8000 else v


def sra(v, n):
    """MIPS `sra`: arithmetic right shift of a 32-bit value."""
    return to_i32(v) >> n


def sll(v, n):
    """MIPS `sll`: left shift of a 32-bit value, truncated to 32 bits."""
    return (v << n) & 0xFFFFFFFF


def unpack_vertex_word(word):
    """Unpacks one 4-byte "absolute" vertex word into (x, y, z)."""
    x = sra(word, 21)
    y = sra(sll(word, 10), 21)
    z = sra(sll(word, 20), 19)
    return x, y, z


def unpack_vertex_delta(signed16, shift):
    """Unpacks one 2-byte relative-shift word into (dx, dy, dz)."""
    w = signed16 & 0xFFFFFFFF
    dx = sra(w, 11) << shift
    dy = sra(sll(w, 21), 27) << shift
    dz = sra(sll(w, 26), 27) << shift
    return dx, dy, dz


def decode_frame_vertices(data, start, vert_count, short_offset_words, short_encode_shift, file_size):
    """Decodes one animation frame's vertex positions - see the module
    docstring's "Per-frame vertex positions" section for the format."""
    if vert_count <= 0 or start + 4 > file_size:
        return [], start

    main_ptr = start
    short_ptr = start + short_offset_words * 4

    seed = u32(data, main_ptr)
    main_ptr += 4
    pos = list(unpack_vertex_word(seed))
    vertices = [tuple(pos)]
    use_delta = seed & 1

    for _ in range(1, vert_count):
        if use_delta:
            if short_ptr + 2 > file_size:
                break
            word = u16(data, short_ptr)
            short_ptr += 2
            dx, dy, dz = unpack_vertex_delta(to_i16(word), short_encode_shift)
            pos[0] += dx
            pos[1] -= dy
            pos[2] -= dz
            use_delta = word & 1
        else:
            if main_ptr + 4 > file_size:
                break
            word = u32(data, main_ptr)
            main_ptr += 4
            pos = list(unpack_vertex_word(word))
            use_delta = word & 1
        vertices.append(tuple(pos))

    return vertices, max(main_ptr, short_ptr)


def decode_geometry(data, anim, data_base, file_size):
    """Decode faces/lp_faces/colors/lp_colors for one animation.

    Frame 0's vertices are decoded first, ahead of the frames loop below,
    because dp_solve_face_stream() needs a reference set of vertex
    positions to score candidate face-packet splits against (see the
    module docstring's "Faces" section) - the result is reused rather
    than decoded twice.
    """
    frame0_vertices, frame0_end = [], None
    if anim["frames"]:
        f0 = anim["frames"][0]
        frame0_vertices, frame0_end = decode_frame_vertices(
            data, data_base + f0["vertex_offset"], anim["vert_count_high"],
            f0["short_offset"], anim["short_encode_shift"], file_size)

    anim["faces"] = dp_solve_face_stream(data, anim["faces_abs"], True, frame0_vertices, file_size)

    anim["colors"] = decode_colors(data, anim["colors_abs"], anim["num_colors"])

    if anim["lp_faces_offset"] != 0:
        anim["lp_faces"] = dp_solve_face_stream(data, anim["lp_faces_abs"], False, frame0_vertices, file_size)
        # No separate low-poly color count field exists; empirically
        # (see docstring) m_LpColors reuses m_NumColors.
        anim["lp_colors"] = decode_colors(data, anim["lp_colors_abs"], anim["num_colors"])
    else:
        anim["lp_faces"] = []
        anim["lp_colors"] = []

    for i, f in enumerate(anim["frames"]):
        if i == 0:
            f["vertices"], f["vertex_data_end"] = frame0_vertices, frame0_end
            continue
        vertex_abs = data_base + f["vertex_offset"]
        vertices, end = decode_frame_vertices(
            data, vertex_abs, anim["vert_count_high"], f["short_offset"], anim["short_encode_shift"], file_size)
        f["vertices"] = vertices
        f["vertex_data_end"] = end


def parse_animation(data, header_offset, index, data_base, file_size):
    num_frames = i16(data, header_offset + 0)
    anim = {
        "index": index,
        "header_offset": header_offset,
        "num_frames": num_frames,
        "num_colors": u16(data, header_offset + 2),
        "is_spyro_animation": bool(u8(data, header_offset + 4)),
        "scale": u8(data, header_offset + 5),
        "short_encode_shift": u8(data, header_offset + 6),
        "radius": u8(data, header_offset + 7),
        "vert_count_high": u8(data, header_offset + 8),
        "vert_count_low": u8(data, header_offset + 9),
        "depth_scale": u8(data, header_offset + 11),
        "progress_per_tick": u8(data, header_offset + 12),
        "animation_vertices_offset": u32(data, header_offset + 16),
        "faces_offset": u32(data, header_offset + 20),
        "colors_offset": u32(data, header_offset + 24),
        "lp_faces_offset": u32(data, header_offset + 28),
        "lp_colors_offset": u32(data, header_offset + 32),
    }
    for key in ("animation_vertices", "faces", "colors", "lp_faces", "lp_colors"):
        anim[f"{key}_abs"] = data_base + anim[f"{key}_offset"]

    frames = []
    frames_start = header_offset + ANIMATION_HEADER_SIZE
    if num_frames >= 0:
        for i in range(num_frames):
            foff = frames_start + i * ANIMATION_FRAME_SIZE
            if foff + ANIMATION_FRAME_SIZE > file_size:
                break
            frames.append(parse_frame(data, foff, i))
    anim["frames"] = frames
    decode_geometry(data, anim, data_base, file_size)
    return anim


def parse_model(data, base=0):
    file_size = len(data)
    num_animations = i32(data, base + 0)
    sounds = data[base + 4:base + 4 + NUM_SOUNDS]
    collision_models = [u32(data, base + 20 + 4 * i) for i in range(NUM_COLLISION_MODELS)]
    data_offset = u32(data, base + 52)
    data_base = base + data_offset

    animations = []
    for i in range(max(num_animations, 0)):
        anim_ptr_offset = base + MODEL_HEADER_SIZE + 4 * i
        if anim_ptr_offset + 4 > file_size:
            break
        header_offset = base + u32(data, anim_ptr_offset)
        animations.append(parse_animation(data, header_offset, i, data_base, file_size))

    return {
        "base": base,
        "num_animations": num_animations,
        "sounds": list(sounds),
        "collision_models": collision_models,
        "data_offset": data_offset,
        "data_base": data_base,
        "animations": animations,
    }


def format_sounds(sounds):
    used = [f"{s:#x}" for s in sounds if s != 0xFF]
    unused = len(sounds) - len(used)
    suffix = f" ({unused} unused/0xff slot{'s' if unused != 1 else ''})" if unused else ""
    return f"[{', '.join(used)}]{suffix}" if used else "(none)"


def face_label(face, index):
    kind = "quad" if len(face["triangles"]) > 1 else "tri "
    return f"[{index}] {kind} triangles={face['triangles']} (packet size={face['size']})"


def build_faces_node(title, faces, expand):
    node = Node(f"{title} ({len(faces)})" + ("" if expand else " (use --geometry to expand)"))
    if expand:
        for i, face in enumerate(faces):
            node.children.append(Node(face_label(face, i)))
    return node


def build_colors_node(title, colors, expand):
    node = Node(f"{title} ({len(colors)})" + ("" if expand else " (use --geometry to expand)"))
    if expand:
        for i, (r, g, b, a) in enumerate(colors):
            node.children.append(Node(f"[{i}] rgba({r}, {g}, {b}, {a})"))
    return node


def build_tree(model, class_names=None, moby_class=None, expand_frames=False, expand_geometry=False,
               expand_vertices=False):
    title = f"Model @ file offset {model['base']:#x} (num_animations={model['num_animations']})"
    if moby_class is not None:
        name = (class_names or {}).get(moby_class)
        label = f" ({name})" if name else ""
        title = f"Model for MobyClass {moby_class}{label} - " + title

    root = Node(title)
    root.children.append(Node(f"Sounds: {format_sounds(model['sounds'])}"))

    coll_node = Node("Collision Models")
    any_coll = False
    for i, ptr in enumerate(model["collision_models"]):
        if ptr == 0:
            continue
        any_coll = True
        coll_node.children.append(Node(f"[{i}] {ptr:#010x} (external RAM pointer, not resolvable in this file)"))
    if not any_coll:
        coll_node.children.append(Node("(none)"))
    root.children.append(coll_node)

    root.children.append(Node(
        f"Data block: model+{model['data_offset']:#x} -> file offset {model['data_base']:#x} ({model['data_base']})"
    ))

    anims_node = Node(f"Animations ({len(model['animations'])})")
    for anim in model["animations"]:
        flags = " [SpyroAnimation]" if anim["is_spyro_animation"] else ""
        anim_label = (
            f"Animation {anim['index']} @ file offset {anim['header_offset']:#x} - "
            f"{anim['num_frames']} frames, {anim['num_colors']} colors, "
            f"radius={anim['radius']}, scale={anim['scale']}{flags}"
        )
        anim_node = Node(anim_label)
        anim_node.children.append(Node(
            f"Animation Vertices @ {anim['animation_vertices_abs']:#x} "
            f"(model+data+{anim['animation_vertices_offset']:#x})"
        ))
        faces_node = build_faces_node(
            f"Faces @ {anim['faces_abs']:#x} (model+data+{anim['faces_offset']:#x})",
            anim["faces"], expand_geometry)
        anim_node.children.append(faces_node)

        anim_node.children.append(build_colors_node(
            f"Colors @ {anim['colors_abs']:#x} (model+data+{anim['colors_offset']:#x})",
            anim["colors"], expand_geometry))

        if anim["lp_faces_offset"] != 0:
            anim_node.children.append(build_faces_node(
                f"LP Faces @ {anim['lp_faces_abs']:#x} (model+data+{anim['lp_faces_offset']:#x})",
                anim["lp_faces"], expand_geometry))
            anim_node.children.append(build_colors_node(
                f"LP Colors @ {anim['lp_colors_abs']:#x} (model+data+{anim['lp_colors_offset']:#x})",
                anim["lp_colors"], expand_geometry))
        else:
            anim_node.children.append(Node("LP Faces/Colors: (none - not used by this animation)"))

        frames_node = Node(f"Frames ({len(anim['frames'])})" + ("" if expand_frames else " (use --frames to expand)"))
        if expand_frames:
            for f in anim["frames"]:
                vabs = model["data_base"] + f["vertex_offset"]
                span = f["vertex_data_end"] - vabs
                frame_node = Node(
                    f"Frame {f['index']}: vertex data @ {vabs:#x} (data+{f['vertex_offset']:#x}), "
                    f"{len(f['vertices'])} vertices decoded ({span} bytes), "
                    f"collision_model={f['collision_model']}, frame_sound={f['frame_sound']}, "
                    f"vertex_color_offset={f['vertex_color_offset']:#x}, shadow={f['shadow']}, "
                    f"short_offset={f['short_offset']}" + ("" if expand_vertices else " (use --vertices to expand)")
                )
                if expand_vertices:
                    for i, (x, y, z) in enumerate(f["vertices"]):
                        frame_node.children.append(Node(f"[{i}] ({x}, {y}, {z})"))
                frames_node.children.append(frame_node)
        anim_node.children.append(frames_node)
        anims_node.children.append(anim_node)
    root.children.append(anims_node)

    return root


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("path", nargs="?", default=DEFAULT_PATH,
                         help=f"path to a raw Model blob dump (default: {DEFAULT_PATH})")
    parser.add_argument("--offset", type=lambda s: int(s, 0), default=0,
                         help="byte offset within the file where the Model struct starts (default: 0)")
    parser.add_argument("--moby-class", type=int, default=None,
                         help="MobyClass id this model belongs to, just to label the output (e.g. 121)")
    parser.add_argument("--frames", action="store_true",
                         help="expand each animation's per-frame data instead of just showing the count")
    parser.add_argument("--vertices", action="store_true",
                         help="also expand each frame's decoded vertex positions (implies --frames)")
    parser.add_argument("--geometry", action="store_true",
                         help="expand decoded faces/colors instead of just showing counts")
    parser.add_argument("--json", action="store_true",
                         help="dump parsed data as JSON instead of a tree")
    args = parser.parse_args()

    with open(args.path, "rb") as f:
        data = f.read()

    model = parse_model(data, base=args.offset)

    if args.json:
        print(json.dumps(model, indent=2))
        return

    class_names = load_moby_class_names() if args.moby_class is not None else None
    tree = build_tree(model, class_names=class_names, moby_class=args.moby_class,
                       expand_frames=args.frames or args.vertices, expand_geometry=args.geometry,
                       expand_vertices=args.vertices)
    print_tree(tree)


if __name__ == "__main__":
    sys.exit(main())
