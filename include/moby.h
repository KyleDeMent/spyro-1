#ifndef __MOBY_H
#define __MOBY_H

#include "graphics.h"
#include "matrix.h"
#include "vector.h"
#include <sys/types.h>

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

typedef struct {
  union {
    // TODO: Document the Spyro model format
    // struct {
    // } m_Props;
    u_int m_Data;
  } m;
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

// Data related

typedef enum {
  MOBYCLASS_PORTAL = 1,

  MOBYCLASS_EXIT_VORTEX = 9,

  MOBYCLASS_GEM_SPAWNER = 13,

  MOBYCLASS_LIFE_STATUE = 14,
  MOBYCLASS_LIFE_ORB = 15,
  MOBYCLASS_BUTTERFLY = 16,

  MOBYCLASS_DRAGON_EGG = 34,

  MOBYCLASS_HUD_SPYRO_HEAD = 58,

  MOBYCLASS_EXCLAMATION_MARK = 75,
  MOBYCLASS_LETTER_APOSTROPHE = 76,

  MOBYCLASS_FLIGHT_TRAIN_BARREL = 78,

  MOBYCLASS_GEM_1 = 83,
  MOBYCLASS_GEM_2 = 84,
  MOBYCLASS_GEM_5 = 85,
  MOBYCLASS_GEM_10 = 86,
  MOBYCLASS_GEM_25 = 87,

  MOBYCLASS_SPARX = 120, // Gets spawned in load scene

  MOBYCLASS_KEY = 173, // Both Hud and level key
  MOBYCLASS_LOCKED_CHEST = 174,

  MOBYCLASS_WOODEN_CHEST = 194,
  MOBYCLASS_METAL_CHEST = 195,

  MOBYCLASS_CRYSTAL_DRAGON = 250,
  MOBYCLASS_CRYSTAL_DRAGON_FRAGMENT = 251,

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

  MOBYCLASS_PERCENT = 272,
  MOBYCLASS_SLASH = 277,
  MOBYCLASS_QUESTION_MARK = 278,

  MOBYCLASS_FLIGHT_CHEST = 299,

  MOBYCLASS_FLIGHT_PLANE = 308,

  MOBYCLASS_PLUS = 317,
  MOBYCLASS_CARET = 321,
  MOBYCLASS_PERIOD = 327,

  MOBYCLASS_SPRING_CHEST = 329,

  MOBYCLASS_FLIGHT_GATE = 353,

  MOBYCLASS_FLIGHT_TRAIN_WHEELS = 362,

  MOBYCLASS_ARMORED_CHEST = 401,

  MOBYCLASS_FLIGHT_TRAIN = 407,
  MOBYCLASS_FLIGHT_WAGON = 408,

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

  MOBYCLASS_HUD_GEM_CHEST = 471,
  MOBYCLASS_HUD_DRAGON = 506,

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

// WIP Flight Moby Class 407 408 Flight Train/Wagon
typedef struct {
  PathData *m_Path;
  int unk_0x4;
  Vector3D unk_0x8;
  int unk_0x14;
} MobyFlightTrainProps;

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
  char m_VerticalSpeed;  //
  char m_TargetAngle;  // angle
  char m_TurnRetargetTimer;  // Char timer
  char m_VerticalSpeedTimer;  // Char timer
  short m_unk_14; // short timer
  short m_unk_16; // ?
} MobyButterflyProps;

typedef struct {
  Moby *m_Parent;
  short m_Len;
  short m_Index;
} MobyLetterProps;

typedef struct {
  int m_Timer;
  Vector3D16 m_IdleOffset;
  Glow *glow;
  Moby* m_MobyPickingUp;
} MobySparxProps;

extern Moby *g_Sparx;

typedef struct {
  int unk_0x0;
  // not sure what to call this, for input pGemValue=25 this is either "20" or
  // "5"
  int unk_0x4;
  Vector3D unk_0x8;
} MobyNumberProps;

// WIP
typedef struct {
  Vector3D unk_0x00;
  int unk_0x0C;
} Moby17Props;

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
  PathData *m_Path;       /* 0x00 */
  Vector3D m_FlyInOffset; /* 0x04 - 0x10 */
  int m_TargetLevelId;    /* 0x10 - which level to enter */
  int m_FlyInPresetIdx;   /* 0x14 - which fly-in preset */
  int unk_0x18;
} MobyExitVortexProps;
typedef struct {
  int m_Timer;     /* 0x00: Remaining lifetime frames */
  int m_Unused04;  /* 0x04 */
  int m_VelocityX; /* 0x08 */
  int m_VelocityY; /* 0x0C */
  int m_VelocityZ; /* 0x10 */
} MobyProjectileProps;

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
