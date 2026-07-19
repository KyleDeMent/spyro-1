#ifndef __MOBY_H
#define __MOBY_H

#include "graphics.h"
#include "matrix.h"
#include "vector.h"
#include <sys/types.h>

#define DISTANCE(p1, p2) OctDistance(&((p1).m_Position), &((p2).m_Position))
#define DISTANCE_TO_SPYRO(m) DISTANCE(*(m), g_Spyro)
#define MOBY_DISTANCE_TO(p2) DISTANCE(*moby, p2)
#define ANGLE_TO(p1, p2) Atan2((p1).x - (p2).x, (p1).y - (p2).y, 0)
#define ANGLE_TO_MOBY(p1) ANGLE_TO((p1).m_Position, moby->m_Position)
#define ANGLE_TO_MOBY_EXT(p1) ANGLE_TO(p1, moby->m_Position)
#define ANGLE_TO_SPYRO(p1) ANGLE_TO((p1).m_Position, g_Spyro.m_Position)

#define MAKE_SIGNED(var, bits)                                                 \
  {                                                                            \
    if ((var) > (1 << ((bits) - 1)))                                           \
      (var) -= (1 << (bits));                                                  \
  }

// Animation state naming comes from PS underground 1:08
typedef struct {
  /// @brief Which animation is currently playing
  u_char m_Animation;

  /// @brief Which animation you're interpolating to
  u_char m_NextAnimation;

  /// @brief The current frame
  u_char m_Frame;

  /// @brief The next frame which you're interpolating to
  u_char m_NextFrame;

  /// @brief The progress of the current frame used for interpolation
  /// 0 - 4096 (0.0 - 1.0)
  u_char m_FrameProgress;

  /// @brief The amount of progress to make per frame
  /// If you want a new frame every in-game frame, this would be 4096 (1.0)
  /// If you want a new frame every 2 in-game frames, this would be 2048 (0.5)
  /// etc..
  u_char m_PerFrameProgress;

  /// @brief Flags that are set by the animation system
  /// Still not entirely sure if it should be part of the animation state
  /// or just in the moby
  /// & 1 == Finished frame (?)
  /// & 2 == Finished animation (used commonly)
  u_char m_AnimationFlags;
} AnimationState;

// Double moby to define struct Moby inside
typedef struct Moby {
  /// @brief Specifies the properties of the moby, which is class specific
  void *m_Props;

  /// @brief The next moby in the collision chain that this Moby is a part of
  struct Moby *m_CollisionChainNext;

  /// @brief This Moby's active collision group (Which is part of it's model)
  void *m_CollisionGroup;

  /// @brief The Moby's position
  Vector3D m_Position;

  /// @brief The damage that has been applied to the Moby
  int m_DamageFlags; // TODO: Enum?

  /// @brief The distance of the shadow from the Moby, equal to the floor
  /// distance
  int m_ShadowDistance;

  /// @brief The Moby's rotation matrix
  SHORTMATRIX m_RotationMatrix;

  /// @brief The collision region that the Moby is in
  short m_CollisionRegion;

  /// @brief The Moby's class, which specifies which kind of Moby it is
  short m_Class;

  /// @brief The distance of the moby to the floor
  short m_FloorDistance;

  /// @brief Flag used to determine if the Moby has already dropped it's drop
  u_char m_DroppedFlag;

  /// @brief The range of the Moby's current collision
  u_char m_CollisionRange;

  /// @brief The Moby's current animation state
  AnimationState m_AnimationState;

  /// @brief The Pod that this Moby is a part of. This is a feature that they
  /// scrapped for S2/3, only to return for R&C It's used to determine which
  /// Moby's are part of the same group and utility functions exist to act on
  /// that group.
  u_char m_Pod;

  /// @brief The Moby's rotation
  Vector3D8 m_Rotation;

  /// @brief Offsets the sorting depth of the entire Moby
  char m_DepthOffset;

  /// @brief The Moby's current state, often used in Moby code
  u_char m_State;

  /// @brief The Moby's substate, less often used in Moby code
  u_char m_Substate;

  /// @brief Sector the Moby is located in, for culling purposes
  u_char m_SectorIndex;

  union {
    struct {
      /// @brief Stores the render distance of the Moby
      u_char m_Distance : 7;

      /// @brief And the renderer to be used
      u_char m_ShinyRenderer : 1;
    } flags;
    u_char raw;
  } m_Renderer;

  // We have to define it like this, we can't use
  // a bitfield because there's no 24 bit integer
  // type and the compiler aligns it to the
  // integer size used

  // Used for the specular color, which is 6 bits per channel (each value is
  // divided by 2)

  // For metal it's straight forward, 8 bits per channel

  u_char m_SpecularMetalColor[3];

  // Stores the type when shinyRenderer is enabled
  // If Shiny renderer: 0 = Specular 1/2 = metal (2 being rotated)
  // If shaded: color index
  u_char m_SpecularMetalType;

  /// @brief Radius of the Moby for clipping purposes (>> 2)
  u_char m_RenderRadius;

  // Stores whether the Moby got drawn last frame
  // If it has, the Moby receives more grace during culling
  // Also often used in Moby code, often enemies will not attack when they
  // are off-screen
  u_char m_WasDrawn;

  /// @brief The distance at which Mobys get updated (/1024)
  // Mobys often initialize this to 0xFF
  u_char m_UpdateDistance;

  /// @brief Moby class dropped by this Moby
  /// They AND this value with 0x7f (not sure why), which limits which Mobys
  /// can be used as a drop moby
  u_char m_DropMoby;

  /// @brief Sound channel the moby is occuping
  u_char m_SoundChannel;

  /// @brief Distance the last sound played was at
  u_char m_SoundDistance;

  // If dynamically allocated, this contains the index of the Moby that spawned
  // us used for keeping track of which index of the gem bitmask needs to be set
  // Ignored if the moby is not dynamically allocated
  u_char m_MobyIndex;

  // Used for resizing the Moby model dynamically, if set to 0 the regular size
  // is used
  u_char m_ScaleOverride;
} Moby;

// Moby models
typedef struct {
  // Vertex offset:    00000000000111111111111111111111
  // Collision model:  00000000111000000000000000000000
  // Frame sound:      11111111000000000000000000000000
  union {

    // Not sure if this will match, but there's only one way to find out
    struct {
      u_int m_VertexOffset : 21;
      u_int m_CollisionModel : 3;
      u_int m_FrameSound : 8;
    } m_Props;
    u_int m_Data;
  } m;

  u_short m_VertexColorOffset;
  u_char m_Shadow;
  u_char m_ShortOffset;
} AnimationFrame;

typedef union {
  struct {
    u_int frameDataOffset : 21;
    u_int nextNotCompressed : 1;
    u_int headNextNotCompressed : 1;
    u_int tailNextNotCompressed : 1;
    u_int soundForFrame : 8;
  } m_Props;
  u_int m_Data;
} SpyroAnimationFrame;

typedef struct {
  short m_NumFrames;
  u_short m_NumColors;

  u_char m_IsSpyroAnimation; // If 1, the other values of the int are unused
  u_char m_Scale;
  u_char m_ShortEncodeShift;
  u_char m_Radius;

  u_char m_VertCountHigh;
  u_char m_VertCountLow;
  u_char m_Padding2;
  u_char m_DepthScale;
  u_char m_ProgressPerTick;
  u_char m_Padding3;
  u_short m_Padding4;

  void *m_AnimationVertices; // Used when m_IsSpyroAnimation is set, otherwise
                             // matches m_faces
  void *m_Faces;
  void *m_Colors;
  void *m_LpFaces;
  void *m_LpColors;
  AnimationFrame m_Frames[1]; // To the size of frameCount
} AnimationHeader;

typedef struct {
  int m_NumAnimations; // >= 0 == Model
  u_char m_Sounds[16];
  void *m_CollisionModels[8];
  void *m_Data; // offset from this to the data, used to offset pointers inside
                // animations
  AnimationHeader *m_Animations[1];
} Model;

#define MODEL_COUNT 512

extern Model *g_Models[MODEL_COUNT];

#define SPYRO_MODEL (g_Models[0])

typedef struct {
  int m_NumAnimations; // < 0 == SimpleModel (always -1 in reality)
  void *m_Verts;
  void *m_Colors;
  void *m_Faces;
} SimpleModel;

// g_AnimationFinished
extern int D_80075794;

// Restart 'anim' from frame 0 at its natural model speed
#define MOBY_ANIM_RESTART(m, anim)                                             \
  (m)->m_AnimationState.m_FrameProgress = 0;                                   \
  (m)->m_AnimationState.m_PerFrameProgress =                                   \
      g_Models[(m)->m_Class]->m_Animations[(anim)]->m_ProgressPerTick;         \
  (m)->m_AnimationState.m_Animation = (anim);                                  \
  (m)->m_AnimationState.m_NextAnimation = (anim);                              \
  (m)->m_AnimationState.m_Frame = 0;                                           \
  (m)->m_AnimationState.m_NextFrame = 1;

// Start 'anim' at its natural model speed without resetting the frame fields
#define MOBY_ANIM_INIT(m, anim)                                                \
  (m)->m_AnimationState.m_FrameProgress = 0;                                   \
  (m)->m_AnimationState.m_PerFrameProgress =                                   \
      g_Models[(m)->m_Class]->m_Animations[(anim)]->m_ProgressPerTick;         \
  (m)->m_AnimationState.m_Animation = (anim);                                  \
  (m)->m_AnimationState.m_NextAnimation = (anim);

// Promote existing next anim to current, then set 'anim' as next anim
#define _MOBY_ANIM_ADVANCE(m, anim, speed)                                     \
  (m)->m_AnimationState.m_FrameProgress = (speed);                             \
  (m)->m_AnimationState.m_PerFrameProgress = (speed);                          \
  (m)->m_AnimationState.m_Animation = (m)->m_AnimationState.m_NextAnimation;   \
  (m)->m_AnimationState.m_NextAnimation = (anim);                              \
  (m)->m_AnimationState.m_Frame = (m)->m_AnimationState.m_NextFrame;           \
  (m)->m_AnimationState.m_NextFrame = 0;                                       \
  func_80037E98(m);
#define MOBY_ANIM_ADVANCE(m, anim) _MOBY_ANIM_ADVANCE(m, anim, 0x10)
#define MOBY_ANIM_ADVANCE_SLOW(m, anim) _MOBY_ANIM_ADVANCE(m, anim, 0x08)

// Advance to 'anim' if it is not already the next anim
#define MOBY_ANIM_CHANGE(m, anim)                                              \
  if ((m)->m_AnimationState.m_NextAnimation != (anim)) {                       \
    MOBY_ANIM_ADVANCE((m), (anim));                                            \
  }

// Advance to 'anim' if it is not already the next anim and clear D_80075794
#define MOBY_ANIM_CHANGE_CLEAR_FINISHED(m, anim)                               \
  if ((m)->m_AnimationState.m_NextAnimation != (anim)) {                       \
    D_80075794 = 0;                                                            \
    MOBY_ANIM_ADVANCE((m), (anim));                                            \
  }

// Set 'anim' as next without promoting the existing next anim
#define MOBY_ANIM_SET_NEXT(m, anim)                                            \
  if ((m)->m_AnimationState.m_NextAnimation != (anim)) {                       \
    (m)->m_AnimationState.m_FrameProgress = 0x10;                              \
    (m)->m_AnimationState.m_PerFrameProgress = 0x10;                           \
    (m)->m_AnimationState.m_NextAnimation = (anim);                            \
    (m)->m_AnimationState.m_NextFrame = 0;                                     \
    func_80037E98(m);                                                          \
  }

// Set state to 'anim' and advance to it if it is not already the next anim
#define MOBY_ANIM_CHANGE_WITH_STATE(m, anim)                                   \
  (m)->m_State = (anim);                                                       \
  MOBY_ANIM_CHANGE(m, anim)

// Restart 'anim' now if it is not already playing, and clear D_80075794
#define MOBY_IMMEDIATE_ANIM(m, anim)                                           \
  if ((m)->m_AnimationState.m_Animation != anim) {                             \
    D_80075794 = 0;                                                            \
    MOBY_ANIM_RESTART(m, anim)                                                 \
  }

// Data related

typedef enum {
  MOBYCLASS_PORTAL_TEXT = 1,

  MOBYCLASS_SWAMP_GRASS = 5, // beast makers
  MOBYCLASS_FLAG_BEASTMAKERS = 6,

  MOBYCLASS_EXIT_VORTEX = 9,
  MOBYCLASS_FODDER_SHEEP = 10,
  MOBYCLASS_WATERFALL = 11,

  MOBYCLASS_GEM_SPAWNER = 13,
  MOBYCLASS_LIFE_STATUE = 14,
  MOBYCLASS_LIFE_ORB = 15,
  MOBYCLASS_BUTTERFLY = 16,
  MOBYCLASS_17_FLIGHT_PLANE_RELATED_UNUSED = 17, // some kind of fireball thing
  MOBYCLASS_SUNNY_FLIGHT_STONE = 18,

  MOBYCLASS_ENEMY_BULL = 23, // town square

  MOBYCLASS_ENEMY_GNORC_DUDE = 28, // headphones dude misty bog

  MOBYCLASS_PROJECTILE_LONG_TONGUE = 31,
  MOBYCLASS_LASER_GNORC_GUN_FLASH =
      32, // disco ball flash when the laser gnorc shoots at you
  MOBYCLASS_EGG_THIEF = 33,
  MOBYCLASS_DRAGON_EGG = 34,
  MOBYCLASS_ENEMY_GNORC_GUNNER = 35, // beast makers lil gunner dude
  MOBYCLASS_FODDER_GOAT = 36,        // is it a goat? magic crafters main world
  MOBYCLASS_CLOUDS = 37,
  MOBYCLASS_PROJECTILE_LIGHTNING_BOLT =
      38, // Green wizard sends these at you in magic crafters
  MOBYCLASS_PROJECTILE_LIGHTNING_BOLT_VERTICAL = 39,

  MOBYCLASS_LIT_CAMPFIRE = 42, // tree tops

  MOBYCLASS_ENEMY_ARMORED_HORROR = 45, // jacques
  MOBYCLASS_ENEMY_GIANT_PANSIE = 46,   // jacques
  MOBYCLASS_PRESENT = 47,              // jacques
  MOBYCLASS_ENEMY_METALHEAD = 48,
  MOBYCLASS_TOASTY_ENTRANCE_CONTROL = 49,
  MOBYCLASS_ENEMY_DEVIL_DOG = 50,       // big red guy
  MOBYCLASS_ENEMY_DEVIL_DOG_SMALL = 51, // small yellow guy
  MOBYCLASS_ENEMY_CUPID = 52,
  MOBYCLASS_ENEMY_ARMORED_TURTLE = 53,
  MOBYCLASS_ENEMY_ARMORED_TURTLE_SMALL = 54,
  MOBYCLASS_PROJECTILE_CUPID_ARROW = 55,
  MOBYCLASS_PHOENIX = 56,
  MOBYCLASS_BANANAS = 57, // tree tops - bunch at anim=0, single at anim=1
  MOBYCLASS_HUD_SPYRO_HEAD = 58,

  MOBYCLASS_STICK_CAGE = 61,        // lofty
  MOBYCLASS_ENEMY_PUFFER_BIRD = 62, // lofty

  MOBYCLASS_TORCH_DREAMWEAVERS = 64,
  MOBYCLASS_METALHEAD_POST = 65,
  MOBYCLASS_ENEMY_PURPLE_ICE_GNORC = 66, // wizards peak
  MOBYCLASS_SPRING_CHEST_FRAG_1 = 67,
  MOBYCLASS_SPRING_CHEST_FRAG_2 = 68,
  MOBYCLASS_SPRING_CHEST_FRAG_3 = 69,
  MOBYCLASS_PROJECTILE_RING_METALHEAD = 70,

  MOBYCLASS_TORCH_PEACEKEEPERS = 74,
  MOBYCLASS_EXCLAMATION_MARK = 75,
  MOBYCLASS_LETTER_APOSTROPHE = 76,
  MOBYCLASS_PROJECTILE_RAYGUN_CIRCLES = 77, // dream weavers
  MOBYCLASS_FLIGHT_TRAIN_BARREL = 78,

  MOBYCLASS_LAMP_WIZARDPEAK = 80,

  MOBYCLASS_PROJECTILE_LIGHTNING_METALHEAD = 82,
  MOBYCLASS_GEM_1 = 83,
  MOBYCLASS_GEM_2 = 84,
  MOBYCLASS_GEM_5 = 85,
  MOBYCLASS_GEM_10 = 86,
  MOBYCLASS_GEM_25 = 87,
  MOBYCLASS_LOFTY_FAIRY = 88,

  MOBYCLASS_RAYGUN = 91, // dream weavers
  MOBYCLASS_ENEMY_FOOL =
      92, // the crazy arm guys you can shrink - dream weavers
  MOBYCLASS_GUNNER_FRAG = 93, // when you kill the gunner in terrace village

  MOBYCLASS_ENEMY_BANANA_BOY = 100,
  MOBYCLASS_ENEMY_STRONGARMS = 101, // the guy that rolls banana boy
  MOBYCLASS_ENEMY_ARMORED_BANANA_BOY = 102,

  MOBYCLASS_PROJECTILE_RAYGUN_LASER = 104, // dream weavers
  MOBYCLASS_FLAG_DREAMWEAVERS = 105,

  MOBYCLASS_FAIRY_HIGHCAVES =
      109, // the ones that bring you back up if you mess up the glide
  MOBYCLASS_DRAGON_PAD_FAIRY = 110,

  MOBYCLASS_ENEMY_SHEPHERD = 113,     //  Stone Hill
  MOBYCLASS_ENEMY_GNORC_SCARED = 114, // Artisans - scared gnorc

  MOBYCLASS_ENEMY_GNORC_WARRIOR = 115, // Dark Hollow - Big guy with club

  MOBYCLASS_SPARX = 120,     // Gets spawned in load scene
  MOBYCLASS_ENEMY_RAM = 121, // Stone Hill
  MOBYCLASS_LAMP_FOOL = 122, // dark passage

  MOBYCLASS_LAMP_FOOL_PARTIAL = 124, // dark passage

  MOBYCLASS_CLOCK_FOOL = 126, // dream weavers
  MOBYCLASS_TREE_DARKHOLLOW_1 = 127,
  MOBYCLASS_LAMP_STONEHILL = 128, // also used in magic crafters
  MOBYCLASS_LAMP_DARKHOLLOW = 129,
  MOBYCLASS_ENEMY_ARMORED_MONK = 130,

  MOBYCLASS_HEAVY_DOOR_HAUNTED_TOWERS_PART =
      132, // not super clear - not the actual door but there before you break
           // it
  MOBYCLASS_HEAVY_DOOR_FRAG_HAUNTED_TOWERS =
      133,                             // frag when you break the door
  MOBYCLASS_ENEMY_GRENADE_GNORC = 134, // haunted towers
  MOBYCLASS_GRENADE = 135,             // haunted towers
  MOBYCLASS_HEAVY_DOOR_HAUNTED_TOWERS = 136,

  MOBYCLASS_ENEMY_BOAR = 137, // beast makers
  MOBYCLASS_TREE_DARKHOLLOW_2 = 138,

  MOBYCLASS_TREE_DARKHOLLOW_3 = 141,
  MOBYCLASS_FAIRY_SUPERFLAME_HAUNTED_TOWERS = 142,
  MOBYCLASS_GNASTY_LASER_EXPLOSION = 143,

  MOBYCLASS_ENEMY_MACHINE_GUNNER = 145,         // twilight harbor
  MOBYCLASS_TENT = 146,                         // peace keepers
  MOBYCLASS_ENEMY_RED_BERET_GNORC_GUNNER = 147, // twilight harbor
  MOBYCLASS_ENEMY_MACHETE_GNORC = 148,          // twilight harbor
  MOBYCLASS_LAMP_ARTISANS = 149,
  MOBYCLASS_ENEMY_JACQUES = 150,
  MOBYCLASS_FIREWORKS_CHEST_FRAG_1 = 151,
  MOBYCLASS_FIREWORKS_CHEST_FRAG_2 = 152,
  MOBYCLASS_FIREWORKS_CHEST_FRAG_3 = 153,
  MOBYCLASS_BARREL_DISPENSER =
      154, // gnorc cove - tnt in anims 0,1 / steel in anims 3,4
  MOBYCLASS_PROJECTILE_FIREBALL = 155, // from the armored turtle
  MOBYCLASS_PROJECTILE_BULLET = 156,   // from twilight harbor gunner
  MOBYCLASS_FIRE_DARKHOLLOW = 157,

  MOBYCLASS_KEY_THIEF_GREEN = 159, // gnasty gnorc
  MOBYCLASS_FODDER_MUSHROOM = 160,
  MOBYCLASS_ENEMY_GNASTY_GNORC = 161,
  MOBYCLASS_FLIGHT_BOAT = 162,
  MOBYCLASS_FLIGHT_HELICOPTER = 163,

  MOBYCLASS_ENEMY_GNORC_SOLDIER = 165, // Little gnorc with sword and shield
  MOBYCLASS_ENEMY_GNORC_SENTRY =
      166, // Big gnorc you gotta flame from the back in Dark Hollow

  MOBYCLASS_PROJECTILE_RING_GNASTY_GNORC = 168,
  MOBYCLASS_PROJECTILE_GREEN_BALL = 169, // gnasty gnorc
  MOBYCLASS_ENEMY_STRONGARMS_METALHEAD = 170,
  MOBYCLASS_BRIDGE_CRANK = 171,
  MOBYCLASS_CASTLE_FLAG = 172,
  MOBYCLASS_KEY = 173, // Both Hud and level key
  MOBYCLASS_LOCKED_CHEST = 174,

  MOBYCLASS_GRENADE_TWILIGHT_HARBOR = 176,
  MOBYCLASS_ENEMY_THIEF_PLANE = 177,

  MOBYCLASS_ENEMY_KEY_THIEF_PURPLE = 179, // gnastys loot

  MOBYCLASS_THIEF_KEY = 181,

  MOBYCLASS_ENEMY_BIRD_WRANGLER = 184, // dry canyon

  MOBYCLASS_ENEMY_GNORC_MUSKETEER = 186, // dry canyon
  MOBYCLASS_BALLOONIST_10 = 187,
  MOBYCLASS_BALLOONIST_20 = 188,
  MOBYCLASS_BALLOONIST_30 = 189,
  MOBYCLASS_BALLOONIST_40 = 190,
  MOBYCLASS_BALLOONIST_50 = 191,
  MOBYCLASS_BALLOONIST_60 = 192,
  MOBYCLASS_FODDER_BUNNY = 193, // peace keepers fodder - what is this guy?
  MOBYCLASS_WOODEN_CHEST = 194,
  MOBYCLASS_METAL_CHEST = 195,

  MOBYCLASS_MUSKETEER_CANNONBALL = 197, // dry canyon
  MOBYCLASS_ENEMY_SNOW_GNORC = 198,
  MOBYCLASS_ENEMY_MECHANIC = 199,      // gnorc cove, has a wrench
  MOBYCLASS_ENEMY_DOCK_WORKER = 200,   // gnorc cove, construction helmet
  MOBYCLASS_ENEMY_DOCK_WORKER_2 = 201, // gnorc cove, gray helmet, heart undies
  MOBYCLASS_PROJECTILE_TNT_BARREL = 202,
  MOBYCLASS_ENEMY_METAL_KNIGHT = 203, // haunted towers
  MOBYCLASS_ENEMY_BLUE_WIZARD = 204,
  MOBYCLASS_LAMPPOST_ICECAVERN = 205,
  MOBYCLASS_CACTUS_MID = 206, // dry canyon
  MOBYCLASS_PROJECTILE_STEEL_BARREL = 207,
  MOBYCLASS_GNEXUS_DRAGON_HEAD_UNLOCKS = 208,
  MOBYCLASS_PROJECTILE_SNOWBALL = 209, // thrown by the snowball gnorc
  MOBYCLASS_METALHEAD_ELECTRIC_RING = 210,
  MOBYCLASS_ENEMY_SNOWBALL_GNORC = 211,
  MOBYCLASS_ENEMY_SKI_PATROL = 212,
  MOBYCLASS_FODDER_BAT = 213,          // ice cavern, is this actually fodder?
  MOBYCLASS_ENEMY_CANNON_PATROL = 214, // peace keepers
  MOBYCLASS_STATIC_KNIGHT_PIECE = 215, // anims are just individual pieces
  MOBYCLASS_ENEMY_FOOT_SOLDIER = 216,  // peace keepers
  MOBYCLASS_217 = 217,                 //?? in gnasty's loot, no model
  MOBYCLASS_FAT_LADY_CAULDRON = 218,

  MOBYCLASS_TREE_STONEHILL_1 = 222,
  MOBYCLASS_TREE_STONEHILL_2 = 223,
  MOBYCLASS_TREE_STONEHILL_3 = 224,
  MOBYCLASS_CANNON = 225,
  MOBYCLASS_ENEMY_PUEBLO = 226,
  MOBYCLASS_FAIRY_SUPERFLAME_HIGHCAVES = 227, // high caves

  MOBYCLASS_ENEMY_FAT_LADY = 229,
  MOBYCLASS_ENEMY_VULTURE = 230,       // dry canyon
  MOBYCLASS_ENEMY_ARMORED_GNORC = 231, // ice cavern
  MOBYCLASS_HELICOPTER_ROTORS = 232,

  MOBYCLASS_BARRIER_EFFECT = 234,

  MOBYCLASS_FODDER_RAT = 236,
  MOBYCLASS_CANNONBALL = 237,
  MOBYCLASS_FODDER_LIZARD = 238,  // cliff town
  MOBYCLASS_CACTUS_TALL = 239,    // dry canyon
  MOBYCLASS_LAMP_SUSPENDED = 240, // ice cavern
  MOBYCLASS_FAIRY_SUPERFLAME_HAUNTED_TOWERS_2 = 241,
  MOBYCLASS_CACTUS_SHORT = 242,    // dry canyon
  MOBYCLASS_FLAG_RED_DOUBLE = 243, // dry canyon

  MOBYCLASS_CRYSTAL_DRAGON = 250,
  MOBYCLASS_CRYSTAL_DRAGON_FRAGMENT = 251,

  MOBYCLASS_ENEMY_BEAST = 253, // alpine ridge

  MOBYCLASS_WOODEN_CHEST_FRAG_1 = 255,
  MOBYCLASS_WOODEN_CHEST_FRAG_2 = 256,
  MOBYCLASS_WOODEN_CHEST_FRAG_3 = 257,

  MOBYCLASS_NUMBER_0 = 260,
  MOBYCLASS_NUMBER_1,
  MOBYCLASS_NUMBER_2,
  MOBYCLASS_NUMBER_3,
  MOBYCLASS_NUMBER_4,
  MOBYCLASS_NUMBER_5,
  MOBYCLASS_NUMBER_6,
  MOBYCLASS_NUMBER_7,
  MOBYCLASS_NUMBER_8,
  MOBYCLASS_NUMBER_9, // 269
  MOBYCLASS_ENEMY_GREEN_DRUID = 270,
  MOBYCLASS_ENEMY_ARMORED_DRUID = 271,
  MOBYCLASS_PERCENT = 272,

  MOBYCLASS_SLASH = 277,
  MOBYCLASS_QUESTION_MARK = 278,

  MOBYCLASS_ENEMY_GREEN_WIZARD = 283,
  MOBYCLASS_ENEMY_ARMORED_DRUID_WIZARDPEAK = 284,
  MOBYCLASS_ENEMY_ELDER_WIZARD_WIZARDPEAK = 285,
  MOBYCLASS_SOUND = 286,

  MOBYCLASS_VULTURE_FRAG_1_MEAT = 288,    // when you kill a vulture
  MOBYCLASS_VULTURE_FRAG_2_FEATHER = 289, // when you kill a vulture

  MOBYCLASS_TREETOPS_THIEF_GREEN = 293,
  MOBYCLASS_TREETOPS_THIEF_RED = 294,
  MOBYCLASS_ELDER_WIZARD_ARROWS = 295,
  MOBYCLASS_GNORC_BALLOON =
      296, // the balloon for the gnorc balloonist in lofty

  MOBYCLASS_ENEMY_GNORC_BALLOONIST = 298, // lofty
  MOBYCLASS_FLIGHT_CHEST = 299,
  MOBYCLASS_WHIRLWIND = 300,
  MOBYCLASS_ENEMY_ELDER_WIZARD_HIGHCAVES =
      301, // not really sure why it's different
  MOBYCLASS_ENEMY_GREEN_DRUID_HIGHCAVES =
      302, // not really sure why it's different
  MOBYCLASS_ENEMY_TORNADO_WIZARD = 303,
  MOBYCLASS_TORNADO = 304, // from the tornado wizard guy
  MOBYCLASS_ENEMY_METAL_SPIDER = 305,

  MOBYCLASS_FLIGHT_PLANE = 308,
  MOBYCLASS_METAL_CHEST_FRAG_LARGE = 309,
  MOBYCLASS_METAL_CHEST_FRAG_SMALL_1 = 310,
  MOBYCLASS_METAL_CHEST_FRAG_SMALL_2 = 311,
  MOBYCLASS_FIREWORKS_CHEST = 312,

  MOBYCLASS_ENEMY_TOASTY = 314,
  MOBYCLASS_YELLOW_BEAST_SNACK = 315, // alpine ridge

  MOBYCLASS_PLUS = 317,
  MOBYCLASS_ELDER_WIZARD_ARROW_FRAG = 318,

  MOBYCLASS_CARET = 321,

  MOBYCLASS_FODDER_RESPAWN = 323,

  MOBYCLASS_ENEMY_ELDER_WIZARD = 326,
  MOBYCLASS_PERIOD = 327,
  MOBYCLASS_WALL_CONTROL = 328,
  MOBYCLASS_SPRING_CHEST = 329,

  MOBYCLASS_DRAGON_PAD_1 = 331, // all but peace keepers, magic crafters...
  MOBYCLASS_DRAGON_PAD_2 = 332, // peace keepers
  MOBYCLASS_DRAGON_PAD_3 = 333, // magic crafters

  MOBYCLASS_ENEMY_DOG = 335, // toasty
  MOBYCLASS_FLAMMABLE_FLOWER_SINGLE_ARTISANS = 336,

  MOBYCLASS_ENEMY_GNORC_GEM_THIEF =
      339, // The guy in Artisans that you have to hit 3 times

  MOBYCLASS_FLIGHT_RING = 340,

  MOBYCLASS_FLAMMABLE_FLOWER_TRIPLE_ARTISANS = 342,
  MOBYCLASS_LASER_GNORC_GUN_RING = 343,

  MOBYCLASS_FLIGHT_PLUS1 = 345,
  MOBYCLASS_FLIGHT_PLUS2 = 346,
  MOBYCLASS_FLIGHT_PLUS3 =
      347, // the +3 text inside the gates in the flight levels
  MOBYCLASS_BANNER = 348,
  MOBYCLASS_ENEMY_DOG_SHEPHERD = 349,
  MOBYCLASS_MOVING_WALL_SUNNY_FLIGHT = 350,
  MOBYCLASS_SINGLE_GEM_SPAWNER = 351,

  MOBYCLASS_FLIGHT_ARCH = 353,
  MOBYCLASS_TREE_ARTISANS_1 = 354,
  MOBYCLASS_TREE_ARTISANS_2 = 355,
  MOBYCLASS_TREE_ARTISANS_3 = 356,

  MOBYCLASS_FLIGHT_COURSE_DETAILS = 358,
  MOBYCLASS_FLIGHT_TRAIN_BARREL_FRAG_1 = 359,
  MOBYCLASS_FLIGHT_TRAIN_BARREL_FRAG_2 = 360,
  MOBYCLASS_FLIGHT_TRAIN_BARREL_FRAG_3 = 361,
  MOBYCLASS_FLIGHT_TRAIN_WHEELS = 362,
  MOBYCLASS_PURPLE_SPLASH = 363,
  MOBYCLASS_PURPLE_BUBBLES = 364,

  MOBYCLASS_ENEMY_KAMIKAZE = 370, // dr shemp
  MOBYCLASS_ENEMY_DR_SHEMP = 371,
  MOBYCLASS_ENEMY_FAT_LADY_SHEMP = 372, // dr shemp

  MOBYCLASS_FLIGHT_ARCH_BROKEN_BASE = 374,
  MOBYCLASS_FLIGHT_ARCH_FRAG_2 = 375,
  MOBYCLASS_FLIGHT_ARCH_FRAG_3 = 376,
  MOBYCLASS_FLIGHT_ARCH_FRAG_4 = 377,
  MOBYCLASS_FLIGHT_ARCH_FRAG_5 = 378,
  MOBYCLASS_FLIGHT_ARCH_FRAG_6 = 379,
  MOBYCLASS_FLIGHT_ARCH_FRAG_7 = 380,
  MOBYCLASS_FLIGHT_ARCH_FRAG_8 = 381,
  MOBYCLASS_FLIGHT_ARCH_FRAG_9 = 382,
  MOBYCLASS_FLIGHT_ARCH_FRAG_10 = 383,
  MOBYCLASS_FLIGHT_ARCH_FRAG_11 = 384,
  MOBYCLASS_FLIGHT_ARCH_FRAG_12 = 385,
  MOBYCLASS_FLIGHT_ARCH_FRAG_13 = 386,
  MOBYCLASS_HUD_FLIGHT_RING = 387,
  MOBYCLASS_HUD_FLIGHT_ARCH = 388, // gate icon in the hud when you collect
  MOBYCLASS_HUD_FLIGHT_CHEST = 389,
  MOBYCLASS_FAN_CHEST_BOTTOM = 390,
  MOBYCLASS_TOASTY_HEAD_SMOKE = 391,
  MOBYCLASS_FAN_CHEST_TOP = 392,
  MOBYCLASS_HUD_FLIGHT_PLANE = 393,
  MOBYCLASS_HUD_FLIGHT_HELICOPTER = 394,
  MOBYCLASS_ENEMY_MATADOR = 395, // town square
  MOBYCLASS_HUD_FLIGHT_BOAT = 396,
  MOBYCLASS_FLIGHT_LIGHT_OFF = 397,
  MOBYCLASS_PORTAL_PATH = 398,
  MOBYCLASS_FLIGHT_LIGHT_ON = 399,
  MOBYCLASS_FRAG_WATER_SPLASH = 400, // when moby frags hit the water
  MOBYCLASS_ARMORED_CHEST = 401,
  MOBYCLASS_FIREWORK = 402,          // cliff town
  MOBYCLASS_ENEMY_ATTACK_FROG = 403, // misty bog

  MOBYCLASS_WATER_BUBBLES = 405,

  MOBYCLASS_FLIGHT_TRAIN = 407,
  MOBYCLASS_FLIGHT_WAGON = 408,

  MOBYCLASS_FODDER_FROG = 412,    // dark hollow
  MOBYCLASS_FODDER_CHICKEN = 413, // town square
  MOBYCLASS_SPYRO_EMOTE = 414,    // gnasty gnorc

  MOBYCLASS_BALLOON = 416,
  MOBYCLASS_CANNON_TARGET = 417,

  MOBYCLASS_EXTRA_LIFE_CHEST = 421,
  MOBYCLASS_EXTRA_LIFE_CHEST_EYEBALLS = 422,
  MOBYCLASS_EXTRA_LIFE_FRAG_1 = 423,
  MOBYCLASS_EXTRA_LIFE_FRAG_2 = 424,
  MOBYCLASS_EXTRA_LIFE_FRAG_3 = 425,

  MOBYCLASS_LETTER_A = 426,
  MOBYCLASS_LETTER_B,
  MOBYCLASS_LETTER_C,
  MOBYCLASS_LETTER_D,
  MOBYCLASS_LETTER_E,
  MOBYCLASS_LETTER_F,
  MOBYCLASS_LETTER_G,
  MOBYCLASS_LETTER_H,
  MOBYCLASS_LETTER_I,
  MOBYCLASS_LETTER_J,
  MOBYCLASS_LETTER_K,
  MOBYCLASS_LETTER_L,
  MOBYCLASS_LETTER_M,
  MOBYCLASS_LETTER_N,
  MOBYCLASS_LETTER_O,
  MOBYCLASS_LETTER_P,
  MOBYCLASS_LETTER_Q,
  MOBYCLASS_LETTER_R,
  MOBYCLASS_LETTER_S,
  MOBYCLASS_LETTER_T,
  MOBYCLASS_LETTER_U,
  MOBYCLASS_LETTER_V,
  MOBYCLASS_LETTER_W,
  MOBYCLASS_LETTER_X,
  MOBYCLASS_LETTER_Y,
  MOBYCLASS_LETTER_Z, // 451

  MOBYCLASS_FODDER_TURKEY_453 = 453, // tree tops
  MOBYCLASS_CANNON_TARGET_FRAG_1 = 454,
  MOBYCLASS_CANNON_TARGET_FRAG_2 = 455,
  MOBYCLASS_CANNON_TARGET_FRAG_3 = 456,

  MOBYCLASS_CANNONBALL_FRAG_1 = 458,
  MOBYCLASS_CANNONBALL_FRAG_2 = 459,

  MOBYCLASS_ENEMY_FLOOR_ZAPPER = 461,

  MOBYCLASS_FODDER_TURKEY_466 = 466, // beast makers...
  MOBYCLASS_ENEMY_LASER_GNORC = 467, // purple guy

  MOBYCLASS_HUD_GEM_CHEST = 471,

  MOBYCLASS_TREE_BEASTMAKERS = 475,  // beast makers
  MOBYCLASS_ENEMY_KILLER_TREE = 476, // misty bog
  MOBYCLASS_WATER_SPLASH = 477,
  MOBYCLASS_FLIGHT_CHEST_FRAG_1 = 478,
  MOBYCLASS_FLIGHT_CHEST_FRAG_2 = 479,
  MOBYCLASS_FLIGHT_CHEST_FRAG_3 = 480,
  MOBYCLASS_FLIGHT_METAL_FRAG_1 = 481,
  MOBYCLASS_FLIGHT_METAL_FRAG_2 = 482,
  MOBYCLASS_FLIGHT_PLANE_FRAG = 483,
  MOBYCLASS_FAIRY_ARROW_DOWN = 484, // misty bog
  MOBYCLASS_FLIGHT_ARROW_FAIRY = 485,
  MOBYCLASS_TORCH_BEASTMAKERS = 486,
  MOBYCLASS_CHICKEN_CAGE = 487, // misty bog

  MOBYCLASS_HUD_FLIGHT_BARREL = 490, // barrel icon in the hud when you collect
  MOBYCLASS_HUD_FLIGHT_LIGHT = 491,

  MOBYCLASS_CACTUS_CHAR_BITS =
      493, // stuff that flies off when a cactus returns to normal
  MOBYCLASS_PROJECTILE_LIGHTNING_TERRACEVILLAGE = 494, // from the gunner
  MOBYCLASS_ENEMY_BLOWHARD = 495,

  MOBYCLASS_ENEMY_GREEN_DRUID_BLOWHARD = 497,

  MOBYCLASS_TREE_ALPINERIDGE = 499,
  MOBYCLASS_FLAG_CHECKERD = 500, // alpine ridge

  MOBYCLASS_TALL_GRASS = 501,
  MOBYCLASS_CHICKEN_FRAG = 502, // aka a feather? lol... when you blast a
                                // chicken fodder in town square
  MOBYCLASS_TREE_MAGICCRAFTERS_1 = 503,
  MOBYCLASS_TREE_MAGICCRAFTERS_2 = 504,

  MOBYCLASS_HUD_DRAGON = 506,
  MOBYCLASS_LAVA_SPLASH = 507,
  MOBYCLASS_LAVA_BUBBLES = 508,

  // These are the last two classes
  MOBYCLASS_DRAGON_CUTSCENE_DRAGON = 510,
  MOBYCLASS_DRAGON_CUTSCENE_SPYRO = 511,
} MobyClass;

typedef struct {
  u_char m_NodeCount;
  u_char m_CurrentNode;
  char unk_0x2[4];  // No clue, padding? Usually 0
  short m_Reversed; // Path traversal: -1 = normal, 1 = reversed
  struct {
    Vector3D m_Position;
    int unk_0xC; // Padding I assume
  } m_Nodes[1];
} PathData;

// Present in some fodder classes, as well as the class 214/216 Gnorcs
typedef struct {
  Vector3D m_Origin;
  u_char m_MoveSpeed;
  u_char m_TurnSpeed;
  u_char m_CollisionRadius;
  u_char m_TurnTimerMin;
  u_char m_TurnTimerMax;
  u_char m_RandomTurnMin;
  u_char m_RandomTurnMax;
  u_char m_WanderRadius;
  u_char m_TargetAngleOffsetLimit;
  u_char m_TargetAngleOffsetStep;
  u_char m_0x16;
  u_char m_TurnTimer;
  u_char m_TargetAngle;
  u_char m_TargetAngleOffsetDirection;
  u_char m_FleeRadius;
  u_char m_IsFleeing;
  u_char m_FleeDelay;
  u_char m_IgnoreMobyCollisionTimer;
  short m_TargetAngleOffset;
} MobyWanderState;

// WIP Flight Moby Class 407 408 Flight Train/Wagon
typedef struct {
  PathData *m_Path;
  int unk_0x4;
  Vector3D unk_0x8;
  int unk_0x14;
} MobyFlightTrainProps;

typedef struct {
  PathData *m_Path;
  int m_UsePathMode;
  int m_RollAngle;
  int m_RollVelocity;
  int m_ZVelocity;
  int m_KnockbackAngle;
  int m_TimerOrSpeed;
  Vector3D m_Velocity;
  int m_NodeIndex;
  int m_Initialized;
  int m_UnlockId;
  PathData *m_CurrentPath;
  PathData *m_PathA;
  PathData *m_PathB;
  PathData *m_PathC;
  int m_PathMode;
} MobyThiefPlaneProps;

typedef struct {
  PathData *m_Path;
  Vector3D unk_0x4;
  int m_Sidedness;
  int unk_0x14;
  int unk_0x18; // Some kinda link? not sure
} MobyPortalPathProps;

typedef struct {
  int m_SparxApproachSide;
  Vector3D m_AnchorPosition;
  char m_VerticalSpeed;      //
  char m_TargetAngle;        // angle
  char m_TurnRetargetTimer;  // Char timer
  char m_VerticalSpeedTimer; // Char timer
  short m_unk_14;            // short timer
  short m_unk_16;            // ?
} MobyButterflyProps;

typedef struct {
  int m_EggIndex;
  Vector3D16 m_StartPosition;
  Vector3D16 m_TargetPosition;
  short m_SpiralRadius;
  unsigned char m_Timer;
  unsigned char m_Angle;
} MobyEggProps;

typedef struct {
  Moby *m_Parent;
  short m_Len;
  short m_Index;
} MobyLetterProps;

typedef struct {
  int m_Timer;
  Vector3D16 m_IdleOffset;
  Glow *glow;
  Moby *m_MobyPickingUp;
} MobySparxProps;

extern Moby *g_Sparx;

typedef struct {
  int unk_0x0;
  // not sure what to call this, for input pGemValue=25 this is either "20" or
  // "5"
  int unk_0x4;
  Vector3D unk_0x8;
} MobyNumberProps;

typedef struct {
  int m_Mode;
  int m_Timer;
  int m_ActionTimer;
  int m_KnockbackAngle;
  int m_KnockbackSpeed;
  int m_KnockbackVelocity;
  int unk_18;
  PathData *m_Path;
  int m_ActivationRange;
  int m_RetargetTimer;
  int m_LinkedMobyIndex;
  short m_RotationVelocityX;
  short m_RotationVelocityY;
  Vector3D m_TriggerPosition;
} MobyGunnerProps;

typedef struct {
  void *unk_00;
  PathData *m_Path;
  int m_PathNodeIndices[2];
  int m_PathIndex;
  int m_TimerA;
  int m_GravityTimer;
  int m_BaseAnim;
  int m_LinkedMobyIndex;
  int m_RecheckTimer;
  int m_DeleteMobyIndex;
} MobyToastyProps;

typedef struct {
  int m_unk_0x00;
  int m_unk_0x04;
  int m_unk_0x08;
  PathData *m_Path;
  int m_unk_0x10;
  int m_unk_0x14;
  int m_unk_0x18;
  int m_unk_0x1C;
  int m_KnockbackSpeed; /* 0x20 - countdown / power level */
  int m_TargetAngle;    /* 0x24 - aim angle */
  int m_unk_0x28;
  int m_RetargetTimer; /* 0x2C - retarget delay */
  int m_unk_0x30;
} MobyEnemyProps;

typedef struct {
  int m_unk_0x00;
  int m_unk_0x04;
  int m_unk_0x08;
  PathData *m_Path;
  int m_unk_0x10;
  int m_unk_0x14;
  int m_unk_0x18;
  int m_unk_0x1C;
  int m_KnockbackSpeed; /* 0x20 - countdown / power level */
  int m_TargetAngle;    /* 0x24 - aim angle */
  int m_unk_0x28;
  int m_RetargetTimer; /* 0x2C - retarget delay */
  int m_unk_0x30;
} MobySheepProps;

typedef struct {
  int timer_0x0;
} Moby11Props;

// WIP
typedef struct {
  int m_BitPos;
  int m_MobyIdx;
} Moby18Props;

typedef struct {
  Moby *m_Parent;
  short m_Speed;
  short m_Distance;
  short m_TargetX;
  short m_TargetY;
  Moby *m_Child;
  u_char m_Mode;
} MobyCloudsProps;

typedef struct {
  short m_VelocityX;
  short m_VelocityY;
  short m_VelocityZ;
  short unk_06;
  short unk_08;
  short m_Timer;
  u_char m_RotationVelocityX;
  u_char m_RotationVelocityY;
  u_char m_RotationVelocityZ;
  u_char m_Angle;
} MobyBananaProps;

typedef struct {
  Vector3D m_Velocity;
  short m_Timer;
  u_char m_PodIdx;
  u_char m_Mode;
} MobyMetalheadLightningProjectileProps;

typedef struct {
  int m_ForwardSpeed;
  int m_VelocityZ;
  int m_AccelerationZ;
  int m_Value;
  short m_Lifetime;
  short m_PodIdx;
} MobyMetalheadRingProjectileProps;

typedef struct {
  int m_Height;
  int m_Active;
  int m_Duration;
  int m_BaseZ;
} MobyMetalheadElectricRingProps;

typedef struct {
  short m_Timer;
  short m_TimerActive;
  TracerPoint *m_TracerPoints;
  short m_ForwardSpeed;
  short m_VelocityZ;
  short m_SpawnTimer;
  short m_AccelerationZ;
  short m_Lifetime;
  u_char m_SpawnDelay;
  u_char m_TracerPointCount;
} MobyRaygunLaserProps;

typedef struct {
  PathData *m_Path;
  char m_Pad04[8];
  u_char m_StateMap0;
  u_char m_StateMap1;
  u_char m_StateMap2;
  u_char m_StateMap3;
  u_char m_StateMap4;
  u_char m_StateMap5;
  u_char m_StateMap6;
  u_char m_Pad13;
  int m_PathIndex;
  int m_Initialized;
  Vector3D m_MovePosition;
  Vector3D m_MoveVelocity;
  int m_Timer;
  int m_Unk38;
  Moby *m_Target;
  int m_Unk40;
  Moby *m_Child;
  int m_Unk48;
  int m_Cutscene;
  int m_AttackBlocked;
  int m_Unk54;
} MobyJacquesProps;

typedef struct {
  PathData *m_Path;
  int m_Mode;
  int m_PathDistance;
  int m_AngleOffset;
  int m_Timer;
  int m_PathNodeIndex;
  int m_LinkedMobyIndex;
  int m_PathAngle;
  int m_KnockbackSpeed;
  int m_KnockbackAngle;
} MobyGreenWizardProps;

typedef struct {
  int unk_00[4];
  int m_SoundIndex;
  PathData *m_Path;
  Vector3D m_PathPosition;
  Vector3D m_PathVelocity;
  PathData *m_Path0;
  PathData *m_Path1;
  PathData *m_Path2;
  PathData *m_CheckpointPath;
  PathData *m_FinalPath;
  int m_Phase;
  Moby *m_Child;
  int m_TargetZ;
  int m_CutsceneTimer;
  int unk_54;
  int m_Initialized;
  int unk_5C;
  int m_PathReady;
  int m_LinkedMobyIndex;
  int m_AttackTimer;
  int unk_6C[2];
  Vector3D m_TriggerPosition;
} MobyBlowhardProps;

typedef struct {
  int unk_0x0;
  int m_AnimChecked;
  int unk_0x8;
} Moby49Props; // toasty head?

typedef struct {
  PathData *m_Path;
  int m_Timer;
  int m_LvlMobyIdx;
  int m_TargetAngle;
  int m_Timer2;
  int m_ZVelocity;
} Moby114Props;

typedef struct {
  int m_Angle;
  int m_Timer;
  int m_AttackLatchOrTimer;
  int m_WaitTimer;
  int m_TargetMobyIndex;
} Moby115Props;

typedef struct {
  int m_UnlockId;
} Moby157Props;

typedef struct {
  PathData *m_Path;
  int m_Mode;
  int m_Timer;
  int m_KnockbackAngle;
  int m_KnockbackTimer;
  int m_KnockbackZVel;
  int m_ActionPhase;
  short m_RotVelX;
  short m_RotVelY;
  int m_ChasePhase;
  int m_AttackState;
  int m_LinkedMobyIndex;
} Moby165Props;

typedef struct {
  PathData *m_Path;
  int m_Mode;
  int m_CloseEnough;
  int m_UseWideDistance;
  int m_Timer;
} Moby166Props;

typedef struct {
  PathData *m_Path;        /* 0x00 */
  int m_UsePathMode;       /* 0x04 */
  int m_RollAngle;         /* 0x08 */
  int m_RollVelocity;      /* 0x0C */
  int m_ZVelocity;         /* 0x10 */
  int m_KnockbackAngle;    /* 0x14 */
  int m_TimerOrSpeed;      /* 0x18 */
  Vector3D m_Velocity;     /* 0x1C */
  int m_NodeIndex;         /* 0x28 */
  int m_Initialized;       /* 0x2C */
  int m_UnlockId;          /* 0x30 */
  PathData *m_CurrentPath; /* 0x34 */
  PathData *m_PathA;       /* 0x38 */
  PathData *m_PathB;       /* 0x3C */
  PathData *m_PathC;       /* 0x40 */
  int m_PathMode;          /* 0x44 */
} Moby177Props;

typedef struct {
  PathData *m_Path;        /* 0x00 */
  int m_LinkedMobyIndex;   /* 0x04 */
  int m_UseClosePathLogic; /* 0x08 */
  int m_PathMode;          /* 0x0C */
  int m_ZVelocity;         /* 0x10 */
  int m_KnockbackAngle;    /* 0x14 */
  int m_KnockbackTimer;    /* 0x18 */
  int m_CloseEnoughTimer;  /* 0x1C */
  int m_SoundTimer;        /* 0x20 */
  int m_LastSound;         /* 0x24 */
} Moby179Props;

typedef struct {
  short m_Timer;            /* 0x00 */
  u_char m_RotationTick;    /* 0x02 */
  u_char m_SparkleHandle;   /* 0x03 */
  int m_EndingType;         /* 0x04 */
  Vector3D m_TriggerRegion; /* 0x08 */
  int unk_0x14;
  int unk_0x18;
  int unk_0x1C;
  int m_MainState;         /* 0x20 */
  int m_LinkedMobyIndex;   /* 0x24 */
  int m_TargetMobyIndex;   /* 0x28 */
  int m_CutsceneFlagIndex; /* 0x2C */
} Moby181Props;

typedef struct {
  int m_Angle;
  int m_ZVel;
  int m_VerticalTimer;
  int m_AliveTimer;
} MobyHeloRotorProps;

typedef struct {
  int m_KeyMobyIndex;
  int m_Timer;
  int m_StoredRotX;
  int m_StoredRotY;
  int m_StoredPosZ;
  int m_FlameHeat;
} MobyLockedChestProps;

typedef struct {
  int m_Timer;      /* 0x00 */
  int m_StoredRotX; /* 0x04 */
  int m_StoredRotY; /* 0x08 */
  int m_StoredPosZ; /* 0x0C */
  int m_PullSpyro;  /* 0x10 */
} Moby312Props;

typedef struct {
  Moby *m_Child;    /* 0x00 */
  int m_Timer;      /* 0x04 */
  int m_StoredRotX; /* 0x08 */
  int m_StoredRotY; /* 0x0C */
  int m_StoredPosZ; /* 0x10 */
} Moby329Props;

typedef struct {
  int unk_0x0;
  int m_Timer;
  PathData *m_Path;
  int unk_0xC;
  int unk_0x10;
  int unk_0x14;
  Vector3D vec_0x18[1];
} Moby339Props;

typedef struct {
  Vector3D m_Min;
  Vector3D m_Max;
} MobyKamikazeBounds;

typedef struct {
  int m_PathMode;                         /* 0x00 */
  int m_AnimationTimer;                   /* 0x04 */
  int m_VerticalVelocity;                 /* 0x08 */
  int m_MovementAngle;                    /* 0x0C */
  int m_MovementSpeed;                    /* 0x10 */
  int unk_14;                             /* 0x14 */
  int m_ActivationRange;                  /* 0x18 */
  int m_ForwardSpeed;                     /* 0x1C */
  int m_ReturnAngle;                      /* 0x20 */
  int m_ActionTimer;                      /* 0x24 */
  int m_LinkedMobyIndex;                  /* 0x28 */
  int m_DeleteMobyIndex;                  /* 0x2C */
  int m_ReturnToPath;                     /* 0x30 */
  PathData *m_ChasePath;                  /* 0x34 */
  int m_AnimationCycleTimer;              /* 0x38 */
  PathData *m_ReturnPath;                 /* 0x3C */
  MobyKamikazeBounds m_BlockingBounds[2]; /* 0x40 */
} MobyKamikazeProps;

typedef struct {
  int m_PathAngle;          /* 0x00 */
  int unk_04;               /* 0x04 */
  int unk_08;               /* 0x08 */
  int unk_0C;               /* 0x0C */
  int m_Timer;              /* 0x10 */
  int unk_14;               /* 0x14 */
  int m_PathIndex;          /* 0x18 */
  int m_Variant;            /* 0x1C */
  PathData *m_Path;         /* 0x20 */
  PathData *m_CameraPath;   /* 0x24 */
  int m_PathNodeIndices[3]; /* 0x28 */
  int m_ActionPhase;        /* 0x34 */
  int m_IdleAnimCount;      /* 0x38 */
  int m_LinkedMobyIndex;    /* 0x3C */
  int m_DeathHandled;       /* 0x40 */
  int m_ParticleTimer;      /* 0x44 */
} MobyShempProps;

typedef struct {
  Moby *m_Child;   /* 0x00 */
  int m_RotAccum;  /* 0x04 */
  int m_Speed;     /* 0x08 */
  int m_Cooldown;  /* 0x0C */
  int m_FlameHeat; /* 0x10 */
} Moby390Props;

typedef struct {
  Vector3D m_Velocity; /* 0x00 */
  int m_Timer;         /* 0x0C */
} Moby392Props;

typedef struct {
  int m_Timer;      /* 0x00 */
  int m_StoredRotX; /* 0x04 */
  int m_StoredRotY; /* 0x08 */
  int m_StoredPosZ; /* 0x0C */
  int m_FlameHeat;  /* 0x10 */
} Moby401Props;

typedef struct {
  int m_LinkedMobyIndex; /* 0x00 */
  Vector3D m_Delta;      /* 0x04 */
  int m_Timer;           /* 0x10 */
  PathData *m_Path;      /* 0x14 */
  int m_InitDone;        /* 0x18 */
} Moby402Props;

typedef struct {
  int unk_0x00[8];
  int m_Timer;
  int m_KnockbackAngle;
  int m_KnockbackTimer;
} Moby412Props;

typedef struct {
  Moby *m_Child;
  int m_Timer;
  int m_StoredRotX;
  int m_StoredRotY;
  int m_StoredPosZ;
  int m_PlaceOnFloor;
} Moby421Props;

typedef struct {
  short unk_0x00;
  short unk_0x02;
  short unk_0x04;

  short unk_0x06;
  short unk_0x08;
  short unk_0x0A;

  int unk_0x0C;
  int unk_0x10;
} MobyFragmentProps;

typedef struct {
  Vector3D trajectory;
  int initZ;
  char unk_0x10;
  char unk_0x11;
  char unk_0x12;
  char m_Lifetime;
} MobyDragonFragmentProps;

typedef struct {
  Vector3D m_VelocityOrPickupPos;
  short m_CollisionIndex;
  u_char m_SpawnState;
  u_char m_Ticks;
  u_char m_BounceCount;
  u_char m_RotX;
  u_char m_RotY;
  u_char m_RotationTicks;
  u_char m_SparkleHandle;
} MobyCollectableProps;

typedef struct {
  Vector3D m_CameraPosition;
  Vector3D m_CameraRotation;
  int m_CutsceneId; // The ID of the cutscene to play when this Moby is
  int m_Rotation;
  int m_DragonPadLink;
  int m_OldDialogueId; // The ID of the *text* dialogue, used in prototypes
  Vector3D m_Angle;
  int m_IsUnskipable;       // Whether the cutscene can't be skipped
  int m_NameIndex;          // The index of the name in the dragon name table
  int m_CutsceneAudioPitch; // The pitch of the cutscene audio (for sample rate)
  int m_CutsceneTicks;      // The number of ticks the cutscene lasts
  int m_ShakeTimer;
  Vector3D m_AngleStorage; // Used in moby code
  int m_PosZStorage;       // Used in moby code
} RescuedDragonMobyProps;

typedef struct {
  int m_Initialized; /* 0x00 - one-time init flag */
} MobyHomeworldDoorProps;
typedef struct {
  int m_unk_0x00;
  int m_unk_0x04; /* Some flag for func_80038458 */
} MobyWoodenChestProps;
typedef struct {
  int m_HitTimer;   /* 0x00 - flash/recovery timer */
  int m_StoredRotX; /* 0x04 */
  int m_StoredRotY; /* 0x08 */
  int m_StoredPosZ; /* 0x0C */
  int m_unk_0x10;   /* 0x10 - check for func_80038458 trigger */
  int m_FlameTimer; /* 0x14 - flame heat tracker */
  int m_unk_0x18;
} MobyMetalChestProps;
typedef struct {
  int m_unk_0x00;
  int m_unk_0x04;
  int m_unk_0x08;
  int m_unk_0x0C;
  int m_unk_0x10;
  int m_unk_0x14;
  int m_unk_0x18;
  int m_unk_0x1C;
  int m_unk_0x20;    /* 0x20 - countdown / power level */
  int m_TargetAngle; /* 0x24 - aim angle */
  int m_unk_0x28;
  int m_RetargetTimer; /* 0x2C - retarget delay */
} MobyEnemyTurretProps;
typedef struct {
  int m_DragonId; /* 0x00 - dragon entry id */
  int m_unk_0x04;
  int m_unk_0x08;
  int m_unk_0x0C;
  int m_unk_0x10;
  int m_unk_0x14;
  int m_DragonIndex;   /* 0x18 */
  int m_DialogueId;    /* 0x1C - speech bubble id */
  int m_PadMobyIdx;    /* 0x20 - linked pad moby index */
  int m_DragonNameIdx; /* 0x24 - dragon name id */
  int m_unk_0x28;
  int m_unk_0x2C;
  int m_unk_0x30;
  int m_unk_0x34;
  int m_unk_0x38;
  int m_unk_0x3C;
  int m_unk_0x40;
  int m_HitTimer;   /* 0x44 - active hit/break animation timer */
  int m_StoredRotX; /* 0x48 */
  int m_StoredRotY; /* 0x4C */
  int m_StoredPosZ; /* 0x50 */
} MobyCrystalDragonProps;
typedef struct {
  int m_State; /* 0x00 - sub-state machine */
  int m_unk_0x04;
  PathData *m_PathPoints; /* 0x08 - per-state data array */
  int m_StateTimer;       /* 0x0C - timer driving state transitions */
  int m_PathState;        /* 0x10 - 0/1 idle/moving along path */
  int m_unk_0x14;
  int m_unk_0x18;  /* 0x18 - velocity component */
  int m_unk_0x1C;  /* 0x1C - velocity component */
  int m_unk_0x20;  /* 0x20 - velocity component */
  int m_PathTimer; /* 0x24 - countdown along path segment */
} MobyAmbientPathProps;

typedef struct {
  int m_ParentIndex;   // 0x00
  Vector3D m_Position; // 0x04 - x, y, z (z is at 0x0C)
  int m_InitDone;      // 0x10
  int m_HasParent;     // 0x14
  int m_RelativeMode;  // 0x18
  int m_SpawnTimer;    // 0x1C
  int m_PitchOffset;   // 0x20
  int m_GroundFlag;    // 0x24
} MobyGemSpawnerProps;
typedef struct {
  short m_VelocityX;
  short m_VelocityY;
  short m_VelocityZ;
  short m_AngularVelocityX;
  short m_AngularVelocityY;
  short m_AngularVelocityZ;
  int m_Lifetime;
  int m_KillBelowZ;
} MobyFragmentPhysicsProps;
typedef struct {
  int m_TargetClass;     /* 0x00 - which class to respawn */
  int m_RespawnTimer;    /* 0x04 - countdown */
  int m_RespawnInterval; /* 0x08 - reset value */
} MobyRespawnerProps;
typedef struct {
  int unk_0x00;
  int m_ParticleTimer;
  int unk_0x08;
  int m_HeightLimit;
} MobyExitVortexProps;
typedef struct {
  int m_Timer;     /* 0x00: Remaining lifetime frames */
  int m_Unused04;  /* 0x04 */
  int m_VelocityX; /* 0x08 */
  int m_VelocityY; /* 0x0C */
  int m_VelocityZ; /* 0x10 */
} MobyProjectileProps;

typedef struct {
  Moby *m_Parent;
  int m_Timer;
} MobyElderWizardArrowProps;

typedef struct {
  Vector3D m_Velocity;
  int m_Lifetime;
} MobyTimedProjectileProps;

typedef struct {
  Moby *m_Parent;
} MobyLaserGnorcGunFlashProps;

typedef struct {
  int m_Timer;
} MobyLightningBoltProps;

typedef struct {
  int m_Timer;
  Moby *m_Parent;
  int m_SoundTimer;
} MobyLightningBoltVerticalProps;

typedef struct {
  int m_Timer;
} MobyCupidArrowProps;

typedef struct {
  int m_Timer;
} MobyPhoenixProps;

typedef struct {
  Moby *m_Parent;
} MobyLampFoolPartialProps;

typedef struct {
  int m_Timer;
} MobyFireballProps;

typedef struct {
  short m_Timer;
  short m_CollisionTriggered;
  Moby *m_LinkedMoby;
  int unk_08;
  int m_VelocityZ;
  int m_LinkedMobyIndex;
} MobyTntBarrelProps;

typedef struct {
  short m_Timer;
  short m_CollisionTriggered;
  Moby *m_LinkedMoby;
  int m_Angle;
  int m_VelocityZ;
  int m_LinkedMobyIndex;
  short m_FlameHeat;
} MobySteelBarrelProps;

typedef struct {
  PathData *m_Path;
  Moby *m_Parent;
  Vector3D m_Velocity;
  short m_LinkedMobyIndex;
} MobyTornadoProps;

typedef struct {
  Moby *m_Parent;
  int m_Timer;
  int unk_08;
} MobyLaserGnorcGunRingProps;

// static_assert(sizeof(Moby) == 0x58, "Incorrect Moby size");

typedef struct {
  Tiledef shadow;
  int *shadow_list; // shadow queue?
} MobyShadow;

extern MobyShadow g_MobyShadows;

/// @brief Are any mobys in this pod still alive?
int func_8003B0DC(int pPod);

/// @brief Are all mobys in this pod in this state? (Unused)
int func_8003B160(int pPod, u_int pState);

extern u_short **g_MobyPods; // Pointer to the Moby pods data, the last Moby
                             // in the list has the top bit set.
extern int g_MobyPodCount;   // The number of pods in the current level

extern Moby *g_KeyMoby; // Pointer to the key Moby for this level

extern Moby *g_LevelMobys; // The Mobys in the current level

/// @brief Pointer to space for the Moby collision chain
extern void *g_MobyCollisionChain;

/// @brief Moby allocation pointer
extern Moby *g_MobyAllocPtr;

/// @brief Props allocation pointer
extern void *g_PropsAllocPtr;

/// @brief Start of the Mobys that are dynamically allocated
extern Moby *g_DynMobys;

/// @brief The current number of dynamically allocated Mobys
extern int g_DynMobyCount;

/// @brief The maximum number of dynamically allocated Mobys
extern int g_DynMobyMax;

/// @brief The end of dynamic Moby space (= start + max * (sizeof(Moby) + 24))
extern void *g_DynMobySpaceEnd;

extern Moby *g_HudMobys; // HUD Mobys

extern u_int D_8006E44C[17]; // Specular shaded color list

#endif // !__MOBY_H
