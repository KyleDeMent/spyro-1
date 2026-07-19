#include "42CC4.h"
#include "collision.h"
#include "dragon.h"
#include "math.h"
#include "moby_helpers.h"
#include "overlay_pointers.h"
#include "rand.h"
#include "spyro.h"
#include "variables.h"

#define MOBY_SHADOW_INIT(m)                                                    \
  (m)->m_ShadowDistance = -1;                                                  \
  VecCopy(&v, &moby->m_Position);                                              \
  v.z += 1024;                                                                 \
  func_8004D5EC(&v, 0x10000);                                                  \
  func_800533D0((m)); // Sets the shadow size to the distance

// We have to replace LEVEL with preprocessor LEVEL
// #ifdef DECOMP_NAME
// Moby *DECOMP_NAME(int pClass, Moby *pParent) {
// #else
Moby *NAME_OVERLAY_FUNCTION(SpawnMoby)(int pClass, Moby *pParent) {
  // #endif
  u_long idx;

  // Allocate a new moby
  Moby *moby = func_800524C4();

  moby->m_Class = pClass;

  if (pParent) {
    idx = pParent - g_LevelMobys;

    if (idx > 0xFF) {
      idx = 0;
    }

  } else {
    idx = 0;
  }

  moby->m_MobyIndex = idx;

  switch (pClass) {

  case MOBYCLASS_LIFE_ORB: { // Extra life orb
    MobyCollectableProps *lifeOrbProps = moby->m_Props;

    func_8003A720(moby); // Reset the Moby first

    lifeOrbProps->m_VelocityOrPickupPos.x = 0;
    lifeOrbProps->m_VelocityOrPickupPos.y = 0;
    lifeOrbProps->m_VelocityOrPickupPos.z = 140;
    lifeOrbProps->m_SpawnState = 0;
    lifeOrbProps->m_Ticks = 0;
    lifeOrbProps->m_BounceCount = 3;
    lifeOrbProps->m_RotX = 0;
    lifeOrbProps->m_RotY = 0;
    lifeOrbProps->m_RotationTicks = 0;
    lifeOrbProps->m_SparkleHandle = -1;

    moby->m_Substate = 2;
    moby->m_RenderRadius = 24;
    moby->m_UpdateDistance = 16;

    moby->m_Rotation.x = 32;
    moby->m_Rotation.y = 0;
    moby->m_Rotation.z = 0;

    if (pParent) {
      VecCopy(&moby->m_Position, &pParent->m_Position);
    }

    func_800526A8(moby); // Update collision

    moby->m_Renderer.raw |= 0x80; // Set shiny renderer
    moby->m_SpecularMetalColor[0] = 0;
    moby->m_SpecularMetalColor[1] = 0;
    moby->m_SpecularMetalColor[2] = 0;
    moby->m_SpecularMetalType = 1; // Set metal type
    break;
  }

  case MOBYCLASS_BUTTERFLY: { // Butterfly
    MobyButterflyProps *butterflyProps = (MobyButterflyProps *)moby->m_Props;

    func_8003A720(moby); // Reset the Moby first
    func_800526A8(moby); // Update collision
    VecCopy(&moby->m_Position, &pParent->m_Position);

    moby->m_Position.z += 512;
    VecCopy(&butterflyProps->m_AnchorPosition, &moby->m_Position);

    butterflyProps->m_VerticalSpeedTimer = 0;
    butterflyProps->m_TurnRetargetTimer = 0;
    butterflyProps->m_unk_14 = 1800;
    break;
  }
#if defined(HAS_FLIGHT_PLANES) || defined(HAS_THIEF_PLANE)
  case MOBYCLASS_17_FLIGHT_PLANE_RELATED_UNUSED: {
    MobyTimedProjectileProps *props;
    Vector3D v;
    func_8003A720(moby); // Reset the Moby first
    moby->m_Rotation.z = pParent->m_Rotation.z;
    moby->m_Position.x =
        pParent->m_Position.x + (COSINE_8(moby->m_Rotation.z) >> 4);
    moby->m_Position.y =
        pParent->m_Position.y + (SINE_8(moby->m_Rotation.z) >> 4);
    moby->m_Position.z = pParent->m_Position.z;
    v.x = g_Spyro.m_Position.x - moby->m_Position.x;
    v.y = g_Spyro.m_Position.y - moby->m_Position.y;
    v.z = g_Spyro.m_Position.z - moby->m_Position.z;
    VecScaleToLength(&v, VecMagnitude(&v, 1), 160);
    func_800526A8(moby); // Update collision

    props = moby->m_Props;
    VecCopy(&props->m_Velocity, &v);
    props->m_Lifetime = 80;
    break;
  }
#endif

#ifdef HAS_LASER_GNORC_ENEMY
  case MOBYCLASS_LASER_GNORC_GUN_FLASH: {
    MobyLaserGnorcGunFlashProps *props = moby->m_Props;

    func_8003A720(moby);
    props->m_Parent = pParent;
    func_80052D64(pParent, 0, &moby->m_Position);
    func_800526A8(moby);
    moby->m_Rotation.y = g_Camera.m_Rotation.y;
    moby->m_Rotation.z = g_Camera.m_Rotation.z + 0x400 >> 4;
    break;
  }
#endif

  case MOBYCLASS_DRAGON_EGG: { // Dragon Egg
    func_8003A720(moby);       // Reset the Moby first

    moby->m_RenderRadius = 32;
    moby->m_UpdateDistance = 255;

    if (pParent) {
      VecCopy(&moby->m_Position, &pParent->m_Position);
    }

    func_800529CC(moby);

    break;
  }
#ifdef HAS_FLIGHT_TRAINS
  case MOBYCLASS_FLIGHT_TRAIN_BARREL: { // Flight Train Barrel
    func_8003A720(moby);                // Reset the Moby first
    func_800526A8(moby);                // Update collision
    break;
  }
#endif

#if defined(HAS_GREEN_WIZARD_ENEMY) && LEVEL == 34
  case MOBYCLASS_CLOUDS: {
    MobyCloudsProps *props = moby->m_Props;
    Vector3D v; // used by MOBY_SHADOW_INIT

    func_8003A720(moby);

    MOBY_SHADOW_INIT(moby)

    switch (pParent->m_State) {
    case 72: {
      props->m_Mode = 1;
      props->m_Speed = 64;
      props->m_Distance = 3584;
      break;
    }
    case 82: {
      props->m_Mode = 2;
      props->m_Speed = 256;
      props->m_Distance = 8192;
      break;
    }
    case 92: {
      props->m_Mode = 3;
      props->m_Speed = 320;
      props->m_Distance = 8192;
      break;
    }
    }

    setXYZ(&moby->m_Position,
           pParent->m_Position.x + (COSINE_8(pParent->m_Rotation.z) << 1 >> 3),
           pParent->m_Position.y + (SINE_8(pParent->m_Rotation.z) << 1 >> 3),
           pParent->m_Position.z);

    props->m_Parent = pParent;
    props->m_Child = nullptr;

    moby->m_State = 0;

    // MOBY_ANIM_INIT(moby, 0)

    moby->m_AnimationState.m_PerFrameProgress =
        g_Models[moby->m_Class]->m_Animations[0]->m_ProgressPerTick;
    moby->m_AnimationState.m_NextAnimation = 0;
    moby->m_AnimationState.m_Animation = 0;
    moby->m_AnimationState.m_NextFrame = 0;
    moby->m_AnimationState.m_Frame = 0;

    func_800526A8(moby);

    break;
  }
#endif

#if defined(HAS_GREEN_WIZARD_ENEMY) || defined(HAS_BLUE_WIZARD_ENEMY)
  case MOBYCLASS_PROJECTILE_LIGHTNING_BOLT: {
    MobyLightningBoltProps *props = moby->m_Props;

    func_8003A720(moby);

    VecCopy(&moby->m_Position, &pParent->m_Position);

    if (pParent->m_Class == MOBYCLASS_PROJECTILE_LIGHTNING_BOLT) {
      moby->m_Rotation.z = pParent->m_Rotation.z;
      moby->m_State = 3;
      MOBY_ANIM_INIT(moby, 3)
      props->m_Timer = 3;
    } else {
      if (pParent->m_AnimationState.m_NextFrame > 5) {
        int yRot;
        moby->m_Position.x += Cos(pParent->m_Rotation.z << 4) >> 2;
        moby->m_Position.y += Sin(pParent->m_Rotation.z << 4) >> 2;
        moby->m_Position.z += 300;
        moby->m_Rotation.z = pParent->m_Rotation.z;

        yRot = Atan2(DISTANCE_TO_SPYRO(moby),
                     g_Spyro.m_Position.z - moby->m_Position.z, 0) -
                   pParent->m_Rotation.y &
               0xff;
        MAKE_SIGNED(yRot, 8)
        CLAMP_VAR(yRot, -16, 16)
        moby->m_Rotation.y = pParent->m_Rotation.y + yRot;

        props->m_Timer = 0x80;
        moby->m_State = 2;
        MOBY_ANIM_INIT(moby, 2)

        g_SpawnParticle(4, 7, &moby->m_Position, 0x10);
      } else if (pParent->m_AnimationState.m_NextFrame > 1) {
        moby->m_Position.z += 300;
        moby->m_Rotation.z = pParent->m_Rotation.z;
        moby->m_State = 1;
        MOBY_ANIM_INIT(moby, 1)
        props->m_Timer = 2;
      } else {
        moby->m_Position.z += 0x180;
        moby->m_Rotation.z = pParent->m_Rotation.z;
        props->m_Timer = 2;
      }
    }

    // init collision
    func_800526A8(moby);

    break;
  }
#endif

#ifdef HAS_BLOWHARD_ENEMY
  case MOBYCLASS_PROJECTILE_LIGHTNING_BOLT_VERTICAL: {
    MobyLightningBoltVerticalProps *props = moby->m_Props;

    func_8003A720(moby);
    VecCopy(&moby->m_Position, &pParent->m_Position);
    func_800526A8(moby);
    moby->m_Rotation.z = ANGLE_TO_MOBY(g_Camera);
    moby->m_Position.z -= 0x400;

    props->m_Timer = 64;
    props->m_Parent = pParent;
    props->m_SoundTimer = 1;
    break;
  }
#endif

#ifdef HAS_CUPID_ENEMY
  case MOBYCLASS_PROJECTILE_CUPID_ARROW: {
    MobyCupidArrowProps *props = moby->m_Props;
    int yRot;

    func_8003A720(moby);
    func_80052D64(pParent, 0, &moby->m_Position);

    moby->m_Rotation.x = 0;

    yRot = Atan2(DISTANCE_TO_SPYRO(moby),
                 g_Spyro.m_Position.z - moby->m_Position.z, 0);
    MAKE_SIGNED(yRot, 8)
    CLAMP_VAR(yRot, -16, 16)
    moby->m_Rotation.y = yRot;

    moby->m_Rotation.z = ANGLE_TO_MOBY(g_Spyro);

    func_800526A8(moby);

    props->m_Timer = 140;

    break;
  }
#endif

#ifdef HAS_PHOENIX
  case MOBYCLASS_PHOENIX: {
    MobyPhoenixProps *props = moby->m_Props;
    int yRot;

    func_8003A720(moby);
    func_80052D64(pParent, 0, &moby->m_Position);

    moby->m_Rotation.x = 0;

    yRot = Atan2(DISTANCE_TO_SPYRO(moby),
                 g_Spyro.m_Position.z - moby->m_Position.z, 0);
    MAKE_SIGNED(yRot, 8)
    CLAMP_VAR(yRot, -16, 16)
    moby->m_Rotation.y = yRot;

    moby->m_Rotation.z = ANGLE_TO_MOBY(g_Spyro);

    func_800526A8(moby);

    props->m_Timer = 240;

    break;
  }
#endif

  case MOBYCLASS_LIFE_STATUE:
  case MOBYCLASS_GEM_1:
  case MOBYCLASS_GEM_2:
  case MOBYCLASS_GEM_5:
  case MOBYCLASS_GEM_10:
  case MOBYCLASS_GEM_25: { // Collectables
    Vector3D v;            // Name from S2
    MobyCollectableProps *gemProps = (MobyCollectableProps *)moby->m_Props;

    func_8003A720(moby); // Reset the Moby first

    gemProps->m_VelocityOrPickupPos.x = 0;
    gemProps->m_VelocityOrPickupPos.y = 0;
    gemProps->m_VelocityOrPickupPos.z = 140; // Uh

    gemProps->m_SpawnState = 0;
    gemProps->m_Ticks = 0;

    gemProps->m_RotX = 0;
    gemProps->m_RotY = 0;

    gemProps->m_RotationTicks = 0;

    // Not sure about that
    if (pParent->m_Class == MOBYCLASS_GEM_SPAWNER) {
      gemProps->m_BounceCount = 2;
    } else {
      gemProps->m_BounceCount = 3;
    }

    gemProps->m_SparkleHandle = -1; // Unset the sparkle handle

    moby->m_Substate = 2;
    moby->m_RenderRadius = 24;
    moby->m_UpdateDistance = 64;

    moby->m_Rotation.x = 32;
    moby->m_Rotation.y = 0;
    moby->m_Rotation.z = 0;

    VecCopy(&moby->m_Position, &pParent->m_Position);
    func_800529CC(moby); // Set shaded moby

    MOBY_SHADOW_INIT(moby)

    moby->m_SpecularMetalColor[0] = 0;
    moby->m_SpecularMetalColor[1] = 0;
    moby->m_SpecularMetalColor[2] = 0;

    if (moby->m_Class == MOBYCLASS_LIFE_STATUE) {
      // Extra life statue
      moby->m_SpecularMetalType = 12;
    }

    if (moby->m_Class == MOBYCLASS_GEM_1) {
      moby->m_SpecularMetalType = 1;
    }

    if (moby->m_Class == MOBYCLASS_GEM_2) {
      moby->m_SpecularMetalType = 2;
    }

    if (moby->m_Class == MOBYCLASS_GEM_5) {
      moby->m_SpecularMetalType = 3;
    }

    if (moby->m_Class == MOBYCLASS_GEM_10) {
      moby->m_SpecularMetalType = 4;
    }

    if (moby->m_Class == MOBYCLASS_GEM_25) {
      moby->m_SpecularMetalType = 5;
    }

    break;
  }

#if defined(HAS_GUNNER_ENEMY) || defined(HAS_METALHEAD_POST)
  case MOBYCLASS_GUNNER_FRAG: {
    struct {
      Vector3D16 vec_0x0;
      Vector3D16 vec_0x6;
      int unk_0xC;
      int unk_0x10;
    } *props = moby->m_Props;
    MobyGunnerProps *parentProps;
    int angle;
    int rnd;
    int c;

    func_8003A720(moby);

    moby->m_RenderRadius = 32;
    VecCopy(&moby->m_Position, &pParent->m_Position);
    func_800526A8(moby);

    if (pParent->m_Class == MOBYCLASS_ENEMY_GNORC_GUNNER) {
      parentProps = (MobyGunnerProps *)pParent->m_Props;
      angle = (parentProps->m_KnockbackAngle + RandRange(-50, 50) & 0xff) << 4;
      rnd = rand() & 0x7FF;
      c = ABS2(Cos(rnd) >> 4);

      setXYZ(&props->vec_0x0, FIXED_MUL(c, Cos(angle)),
             FIXED_MUL(c, Sin(angle)), Sin(rnd) >> 4);

      props->unk_0xC = RandRange(35, 50);
      moby->m_ScaleOverride = 32 - RandRange(6, 20);

      props->vec_0x0.x += g_Spyro.m_Physics.m_Acceleration.x >> 6;
      props->vec_0x0.y += g_Spyro.m_Physics.m_Acceleration.y >> 6;
      props->vec_0x0.z += g_Spyro.m_Physics.m_Acceleration.z >> 6;
    } else {
      angle = rand() & 0xFFF;
      rnd = rand() & 0x7FF;

      setXYZ(&props->vec_0x0, FIXED_MUL(Cos(rnd) >> 5, Cos(angle)),
             FIXED_MUL(Cos(rnd) >> 5, Sin(angle)), Sin(rnd) >> 5);

      if (pParent->m_DamageFlags & 0x20000) {
        props->vec_0x0.x += g_Spyro.m_Physics.m_Acceleration.x >> 6;
        props->vec_0x0.y += g_Spyro.m_Physics.m_Acceleration.y >> 6;
        props->vec_0x0.z += g_Spyro.m_Physics.m_Acceleration.z >> 6;
      }

      props->unk_0xC = 64 - (rand() & 0xf);
    }

    moby->m_Position.x += props->vec_0x0.x * 4;
    moby->m_Position.y += props->vec_0x0.y * 4;
    moby->m_Position.z += props->vec_0x0.z * 4;

    setXYZ(&props->vec_0x6, rand() & 0xf, rand() & 0xf, rand() & 0xf);

    moby->m_Rotation.x += (char)props->vec_0x6.x * 4;
    moby->m_Rotation.y += (char)props->vec_0x6.y * 4;
    moby->m_Rotation.z += (char)props->vec_0x6.z * 4;

    props->unk_0x10 = pParent->m_Position.z - 64;

    *(int *)&moby->m_SpecularMetalColor = 0x1000000;
    moby->m_Renderer.raw |= 0x80; // Set shiny renderer
    break;
  }
#endif

#ifdef HAS_METAL_KNIGHT_ENEMY
  case MOBYCLASS_STATIC_KNIGHT_PIECE: {
    int rnd = RandRange(120, 150);
    int yRotRnd;

    struct {
      Vector3D16 vec_0x0;
      Vector3D16 vec_0x6;
      int unk_0xC;
      int unk_0x10;
    } *props = moby->m_Props;

    func_8003A720(moby);

    moby->m_RenderRadius = 32;
    VecCopy(&moby->m_Position, &pParent->m_Position);
    func_800526A8(moby);

    yRotRnd = pParent->m_Rotation.y + RandRange(-15, 15) & 0xff;

    setXYZ(&props->vec_0x0, FIXED_MUL(rnd, COSINE_8(yRotRnd)),
           FIXED_MUL(rnd, SINE_8(yRotRnd)), RandRange(100, 140));

    props->unk_0xC = RandRange(30, 50);

    setXYZ(&props->vec_0x6, RandRangeSigned(7, 12), RandRangeSigned(7, 12),
           RandRangeSigned(7, 12));

    props->unk_0x10 = pParent->m_Position.z - pParent->m_FloorDistance - 64;

    *(int *)&moby->m_SpecularMetalColor = 0x1000000;
    moby->m_Renderer.raw |= 0x80; // Set shiny renderer

    break;
  }
#endif

  case MOBYCLASS_SPARX: { // Sparx
    MobySparxProps *sparxProps = (MobySparxProps *)moby->m_Props;

    func_8003A720(moby); // Reset the Moby first
    func_800526A8(moby); // Update collision

    moby->m_Substate = 0;

    sparxProps->m_Timer = 0;
    sparxProps->m_IdleOffset.z = 0;
    sparxProps->m_IdleOffset.y = 0;
    sparxProps->m_IdleOffset.x = 0;
    sparxProps->glow = 0;
    sparxProps->m_MobyPickingUp = nullptr;

    if (pParent) {
      VecCopy(&moby->m_Position, &pParent->m_Position);
    }

    break;
  }

#ifdef HAS_LAMP_FOOL_ENEMY
  case MOBYCLASS_LAMP_FOOL_PARTIAL: {
    MobyLampFoolPartialProps *props = moby->m_Props;

    func_8003A720(moby);

    VecCopy(&moby->m_Position, &pParent->m_Position);
    moby->m_Rotation = pParent->m_Rotation;
    props->m_Parent = pParent;

    func_800526A8(moby);

    break;
  }
#endif

#ifdef HAS_FLIGHT_HELICOPTERS
  case MOBYCLASS_HELICOPTER_ROTORS: {
    MobyHeloRotorProps *props = moby->m_Props;

    func_8003A720(moby); // init

    // set rotor start pos
    VecCopy(&moby->m_Position, &pParent->m_Position);
    moby->m_Position.z += 0x400;

    // init collision
    func_800526A8(moby);

    props->m_ZVel = 200;
    props->m_VerticalTimer = 350;
    props->m_Angle =
        func_80038178(ANGLE_TO_SPYRO(*moby), g_Spyro.m_bodyRotation.z, 40, 64);
    props->m_AliveTimer = 120;

    break;
  }
#endif

#ifdef HAS_BARRIER_EFFECT
  case MOBYCLASS_BARRIER_EFFECT: {
    func_8003A720(moby); // Reset the Moby first
    VecCopy(&moby->m_Position, &g_Spyro.unk_0x17c);
    moby->m_Rotation.z = Atan2(g_Spyro.m_KnockbackDirection.x,
                               g_Spyro.m_KnockbackDirection.y, 0) +
                         64;
    func_800526A8(moby); // Update collision
    break;
  }
#endif

#ifdef HAS_FIREWORKS_CHEST
  case MOBYCLASS_FIREWORKS_CHEST_FRAG_2:
  case MOBYCLASS_FIREWORKS_CHEST_FRAG_3: {
    func_8003A720(moby);

    moby->m_RenderRadius = 32;
    moby->m_UpdateDistance = -1;

    func_800529CC(moby);

    moby->m_SpecularMetalColor[0] = '\0';
    moby->m_SpecularMetalColor[1] = '\0';
    moby->m_SpecularMetalColor[2] = '\0';
    moby->m_SpecularMetalType = 13;

    break;
  }
#endif

#ifdef HAS_ARMORED_TURTLE_ENEMY
  case MOBYCLASS_PROJECTILE_FIREBALL: {
    MobyFireballProps *props = moby->m_Props;
    int yRot;
    int zRot;

    func_8003A720(moby);
    func_80052D64(pParent, 0, &moby->m_Position);

    moby->m_Rotation.x = 0;

    yRot = Atan2(DISTANCE_TO_SPYRO(moby),
                 g_Spyro.m_Position.z - moby->m_Position.z, 0);
    MAKE_SIGNED(yRot, 8)
    CLAMP_VAR(yRot, -16, 16)
    moby->m_Rotation.y = yRot;

    moby->m_Rotation.z = ANGLE_TO_MOBY(g_Spyro);

    zRot = moby->m_Rotation.z - pParent->m_Rotation.z & 0xff;
    MAKE_SIGNED(zRot, 8)
    if (ABS(zRot) > 16)
      moby->m_Rotation.z = pParent->m_Rotation.z;

    func_800526A8(moby);

    props->m_Timer = 240;

    break;
  }
#endif

#ifdef HAS_BARREL_DISPENSER
  case MOBYCLASS_PROJECTILE_TNT_BARREL: {
    MobyTntBarrelProps *props = moby->m_Props;
    Vector3D v;

    func_8003A720(moby);

    VecCopy(&moby->m_Position, &pParent->m_Position);

    props->m_VelocityZ = -12;
    props->m_LinkedMoby = pParent;
    props->m_CollisionTriggered = 0;
    props->m_LinkedMobyIndex = -1;

    moby->m_FloorDistance = 461;

    func_800526A8(moby);

    moby->m_Renderer.raw = 0x10;
    moby->m_RenderRadius = 32;

    MOBY_SHADOW_INIT(moby)

    break;
  }

  case MOBYCLASS_PROJECTILE_STEEL_BARREL: {
    MobySteelBarrelProps *props = moby->m_Props;
    Vector3D v;

    func_8003A720(moby);

    VecCopy(&moby->m_Position, &pParent->m_Position);

    props->m_VelocityZ = -12;
    props->m_LinkedMoby = pParent;
    props->m_CollisionTriggered = 0;
    props->m_FlameHeat = 0;
    props->m_LinkedMobyIndex = -1;

    moby->m_FloorDistance = 461;

    func_800526A8(moby);

    moby->m_Renderer.raw = 0x90;
    moby->m_RenderRadius = 32;

    ((int *)&moby->m_SpecularMetalColor)[0] = 0xa18618;

    MOBY_SHADOW_INIT(moby)

    break;
  }
#endif

#ifdef HAS_WOODEN_CHEST
#define HAS_ANY_CHEST_FRAGMENTS
  // Wooden chest fragments
  case MOBYCLASS_WOODEN_CHEST_FRAG_1:
  case MOBYCLASS_WOODEN_CHEST_FRAG_2:
  case MOBYCLASS_WOODEN_CHEST_FRAG_3:
#endif

#ifdef HAS_SPRING_CHEST
#define HAS_ANY_CHEST_FRAGMENTS
  // Spring chest fragments
  case MOBYCLASS_SPRING_CHEST_FRAG_1:
  case MOBYCLASS_SPRING_CHEST_FRAG_2:
  case MOBYCLASS_SPRING_CHEST_FRAG_3:
#endif

#if defined(HAS_METAL_CHEST) || defined(HAS_LOCKED_CHEST) ||                   \
    defined(HAS_ARMORED_CHEST)
#define HAS_ANY_CHEST_FRAGMENTS
  case MOBYCLASS_METAL_CHEST_FRAG_LARGE: // Metal, locked and armored chest
                                         // fragments
  case MOBYCLASS_METAL_CHEST_FRAG_SMALL_1:
  case MOBYCLASS_METAL_CHEST_FRAG_SMALL_2:
#endif

#ifdef HAS_EXTRA_LIFE_CHEST
#define HAS_ANY_CHEST_FRAGMENTS
  case MOBYCLASS_EXTRA_LIFE_FRAG_1: // Extra life chest piece 1
  case MOBYCLASS_EXTRA_LIFE_FRAG_2: // Extra life chest piece 2
  case MOBYCLASS_EXTRA_LIFE_FRAG_3: // Extra life chest piece 3
#endif

#ifdef HAS_FLIGHT_CHESTS
#define HAS_ANY_CHEST_FRAGMENTS
  case MOBYCLASS_FLIGHT_CHEST_FRAG_1: // Flight chest fragments
  case MOBYCLASS_FLIGHT_CHEST_FRAG_2:
  case MOBYCLASS_FLIGHT_CHEST_FRAG_3:
#endif

#ifdef HAS_FIREWORKS_CHEST
#define HAS_ANY_CHEST_FRAGMENTS
  case MOBYCLASS_FIREWORKS_CHEST_FRAG_1:
#endif

#if LEVEL == 53
#define HAS_ANY_CHEST_FRAGMENTS
  case MOBYCLASS_HEAVY_DOOR_FRAG_HAUNTED_TOWERS:
#endif

#ifdef HAS_ANY_CHEST_FRAGMENTS

  {
    MobyFragmentProps *fragmentProps = moby->m_Props;

    int angle1;
    int angle2;

    func_8003A720(moby); // Reset the Moby first

    moby->m_RenderRadius = 32;
    VecCopy(&moby->m_Position, &pParent->m_Position);

    func_800526A8(moby); // Update collision

    angle2 = rand() & 0xFFF;
    angle1 = rand() & 0x7FF;

    fragmentProps->unk_0x00 = FIXED_MUL(Cos(angle1) >> 5, Cos(angle2));
    fragmentProps->unk_0x02 = FIXED_MUL(Cos(angle1) >> 5, Sin(angle2));
    fragmentProps->unk_0x04 = Sin(angle1) >> 5;

    // Charge damage
    if (pParent->m_DamageFlags & 0x20000) {
      fragmentProps->unk_0x00 += g_Spyro.m_Physics.m_Acceleration.x >> 6;
      fragmentProps->unk_0x02 += g_Spyro.m_Physics.m_Acceleration.y >> 6;
      fragmentProps->unk_0x04 += g_Spyro.m_Physics.m_Acceleration.z >> 6;
    }

    moby->m_Position.x += fragmentProps->unk_0x00 * 4;
    moby->m_Position.y += fragmentProps->unk_0x02 * 4;
    moby->m_Position.z += fragmentProps->unk_0x04 * 4;

    fragmentProps->unk_0x06 = rand() & 0xF;
    fragmentProps->unk_0x08 = rand() & 0xF;
    fragmentProps->unk_0x0A = rand() & 0xF;

    fragmentProps->unk_0x10 = pParent->m_Position.z - 64;
    fragmentProps->unk_0x0C = 64 - (rand() & 0xF);

    if (moby->m_Class >= MOBYCLASS_METAL_CHEST_FRAG_LARGE &&
        moby->m_Class <= MOBYCLASS_METAL_CHEST_FRAG_SMALL_2) {
      // Possible union
      ((int *)&moby->m_SpecularMetalColor)[0] = 0xa18618;
      moby->m_Renderer.raw |= 0x80; // Set shiny renderer
    }
    break;
  }
#endif
#ifdef HAS_DRAGON
  case MOBYCLASS_CRYSTAL_DRAGON_FRAGMENT: { // Dragon fragment
    Vector3D v;                             // Name from S2
    int randRes;

    MobyDragonFragmentProps *dragonFragmentProps =
        (MobyDragonFragmentProps *)moby->m_Props;

    func_8003A720(moby); // Reset the Moby first

    moby->m_RenderRadius = 32;
    moby->m_UpdateDistance = -1;

    func_800529CC(moby); // Set shaded moby

    moby->m_SpecularMetalColor[0] = 0;
    moby->m_SpecularMetalColor[1] = 0;
    moby->m_SpecularMetalColor[2] = 0;
    moby->m_SpecularMetalType = 14;

    if (g_DragonCutscene.m_State == 3) {
      moby->m_ScaleOverride = 20;
    } else if (g_DragonCutscene.m_State == 1) {
      moby->m_ScaleOverride = 48;
    }

    randRes = rand() & 7;
    v.x = D_8006F3A0[randRes][0];
    v.y = 0;
    v.z = D_8006F3A0[randRes][1];

    VecRotateByMatrix((MATRIX *)&pParent->m_RotationMatrix, &v, &v);

    v.x += -63 + (rand() & 127);
    v.y += -63 + (rand() & 127);
    v.z += -63 + (rand() & 127);

    VecAdd(&moby->m_Position, &pParent->m_Position, &v);

    VecCopy(&dragonFragmentProps->trajectory, &v);

    VecShiftRight(&dragonFragmentProps->trajectory, 2);

    dragonFragmentProps->trajectory.x += -127 + (rand() & 255);
    dragonFragmentProps->trajectory.y += -127 + (rand() & 255);
    dragonFragmentProps->trajectory.z += -127 + (rand() & 255);

    moby->m_Rotation.x = rand();
    moby->m_Rotation.y = rand();
    moby->m_Rotation.z = rand();

    dragonFragmentProps->unk_0x10 = rand() & 0xf;
    dragonFragmentProps->unk_0x11 = rand() & 0xf;
    dragonFragmentProps->unk_0x12 = rand() & 0xf;
    dragonFragmentProps->initZ = pParent->m_Position.z;
    dragonFragmentProps->m_Lifetime = (rand() & 3) + 16;
    break;
  }
#endif
#if defined(HAS_FLIGHT_PLANES) || defined(HAS_FLIGHT_HELICOPTERS) ||            \
    defined(HAS_THIEF_PLANE)
#define HAS_FLIGHT_METAL_FRAGS
  case MOBYCLASS_FLIGHT_METAL_FRAG_1:
  case MOBYCLASS_FLIGHT_METAL_FRAG_2:
#endif
#if defined(HAS_FLIGHT_PLANES) || defined(HAS_THIEF_PLANE)
#define HAS_FLIGHT_METAL_FRAGS
  case MOBYCLASS_FLIGHT_PLANE_FRAG:
#endif
#ifdef HAS_FLIGHT_METAL_FRAGS
  {
    MobyFragmentProps *props = moby->m_Props;
    int angle1, angle2;

    func_8003A720(moby);
    moby->m_RenderRadius = 32;
    VecCopy(&moby->m_Position, &pParent->m_Position);
    func_800526A8(moby);

    angle1 = rand() & 0xFFF;
    angle2 = rand() & 0x7FF;

    props->unk_0x00 = FIXED_MUL(Cos(angle2) >> 5, Cos(angle1 & 0xFFF));
    props->unk_0x02 = FIXED_MUL(Cos(angle2) >> 5, Sin(angle1 & 0xFFF));
    props->unk_0x04 = Sin(angle2) >> 5;

    props->unk_0x00 += g_Spyro.m_Physics.m_Acceleration.x >> 6;
    props->unk_0x02 += g_Spyro.m_Physics.m_Acceleration.y >> 6;
    props->unk_0x04 += g_Spyro.m_Physics.m_Acceleration.z >> 6;

    moby->m_Position.x += props->unk_0x00 * 4;
    moby->m_Position.y += props->unk_0x02 * 4;
    moby->m_Position.z += props->unk_0x04 * 4;

    props->unk_0x06 = rand() & 0xF;
    props->unk_0x08 = rand() & 0xF;
    props->unk_0x0A = rand() & 0xF;
    props->unk_0x10 = pParent->m_Position.z - 64;
    props->unk_0x0C = 64 - (rand() & 0xF);
    break;
  }
#endif
#undef HAS_FLIGHT_METAL_FRAGS

#if defined(HAS_FLIGHT_TRAINS) || defined(HAS_FLIGHT_BOATS)
  case MOBYCLASS_FLIGHT_TRAIN_BARREL_FRAG_1: // Flight Train Barrel Fragments
  case MOBYCLASS_FLIGHT_TRAIN_BARREL_FRAG_2:
  case MOBYCLASS_FLIGHT_TRAIN_BARREL_FRAG_3: {
    MobyFragmentProps *props = (MobyFragmentProps *)moby->m_Props;

    func_8003A720(moby); // Reset the Moby first

    moby->m_RenderRadius = 32;

    VecCopy(&moby->m_Position, &pParent->m_Position);
    func_800526A8(moby); // Update collision

    moby->m_Rotation.z = rand();

    props->unk_0x00 = COSINE_8(moby->m_Rotation.z & 0xFF) >> 7;
    props->unk_0x02 = SINE_8(moby->m_Rotation.z & 0xFF) >> 7;

    if ((rand() & 1)) {
      props->unk_0x04 = 90;
    } else {
      props->unk_0x04 = -90;
      moby->m_Rotation.x = 128;
    }

    moby->m_Position.x += props->unk_0x00 * 4;
    moby->m_Position.y += props->unk_0x02 * 4;
    moby->m_Position.z += props->unk_0x04 * 4;

    if (props->unk_0x04 < 20) {
      props->unk_0x04 = 20;
    }

    props->unk_0x06 = rand() & 0xF;
    props->unk_0x08 = rand() & 0xF;
    props->unk_0x0A = rand() & 0xF;

    props->unk_0x10 = pParent->m_Position.z - 64;

    props->unk_0x0C = 64 - (rand() & 0xF);
    break;
  }
#endif
#if defined(HAS_FLIGHT_TRAINS)
  case MOBYCLASS_FLIGHT_TRAIN_WHEELS: { // Flight Train wheels
    MobyFragmentProps *targetProps = (MobyFragmentProps *)moby->m_Props;
    MobyFlightTrainProps *parentProps =
        (MobyFlightTrainProps *)pParent->m_Props;
    int rnd;
    Vector3D v;
    func_8003A720(moby); // Reset
    moby->m_RenderRadius = 32;
    func_800526A8(moby); // Collision update

    VecCopy((Vector3D *)&targetProps->unk_0x00, &parentProps->unk_0x8);

    targetProps->unk_0x08 = 0;
    targetProps->unk_0x0A = 0;
    targetProps->unk_0x04 += 64;
    targetProps->unk_0x10 = pParent->m_Position.z - 64;
    targetProps->unk_0x0C = 64 - (rand() & 0xF);

    switch (rand() & 3) {
    case 0:
      v.x = 512;
      v.y = 512;
      moby->m_Rotation.z = 64;
      targetProps->unk_0x06 = 16;
      break;

    case 1:
      v.x = 512;
      v.y = -512;
      moby->m_Rotation.z = 192;
      targetProps->unk_0x06 = -16;
      break;

    case 2:
      v.x = -512;
      v.y = 512;
      moby->m_Rotation.z = 64;
      targetProps->unk_0x06 = 16;
      break;

    case 3:
      v.x = -512;
      v.y = -512;
      moby->m_Rotation.z = 192;
      targetProps->unk_0x06 = -16;
      break;
    }
    moby->m_Rotation.z += pParent->m_Rotation.z;
    v.z = 640;

    VecRotateByMatrix((MATRIX *)&pParent->m_RotationMatrix, &v, &v);

    VecAdd(&moby->m_Position, &v, &pParent->m_Position);

    break;
  }
#endif

  case MOBYCLASS_NUMBER_0: // Text 0-9
  case MOBYCLASS_NUMBER_1:
  case MOBYCLASS_NUMBER_2:
  case MOBYCLASS_NUMBER_3:
  case MOBYCLASS_NUMBER_4:
  case MOBYCLASS_NUMBER_5:
  case MOBYCLASS_NUMBER_6:
  case MOBYCLASS_NUMBER_7:
  case MOBYCLASS_NUMBER_8:
  case MOBYCLASS_NUMBER_9:
  case MOBYCLASS_SLASH:  // Text Slash
  case MOBYCLASS_PERIOD: // Hud .
  {
    MobyNumberProps *textProps = moby->m_Props;

    func_8003A720(moby); // Reset the Moby first
    func_800529CC(moby); // Set shaded moby

    moby->m_SpecularMetalColor[0] = 0;
    moby->m_SpecularMetalColor[1] = 0;
    moby->m_SpecularMetalColor[2] = 0;
    moby->m_SpecularMetalType = 2;

    textProps->unk_0x0 = 64;
    break;
  }

#if (LEVEL % 10) == 5
  case MOBYCLASS_FLIGHT_PLUS1: // Flight +1S moby?
  case MOBYCLASS_FLIGHT_PLUS2: // Flight +2S moby?
  case MOBYCLASS_FLIGHT_PLUS3: // Flight +3S moby?
  {
    if (g_FlightCourseRecords[g_Homeworld]) {
      func_800529CC(moby); // Set shaded moby
      func_80052568(moby);
      moby = nullptr;
    } else {
      func_8003A720(moby); // Reset the Moby first
      if (pParent != nullptr) {
        VecCopy(&moby->m_Position, &pParent->m_Position);
      }
      moby->m_RenderRadius = 64;
      func_800529CC(moby); // Set shaded moby
      moby->m_SpecularMetalColor[0] = 0;
      moby->m_SpecularMetalColor[1] = 0;
      moby->m_SpecularMetalColor[2] = 0;
      moby->m_SpecularMetalType = 2;
    }
    break;
  }
#endif

#ifdef HAS_FLIGHT_ARCHES
  case MOBYCLASS_FLIGHT_ARCH_BROKEN_BASE: { // Flight Gate Fragment
    func_8003A720(moby);                    // Reset the Moby first
    VecCopy(&moby->m_Position, &pParent->m_Position);
    moby->m_Rotation = pParent->m_Rotation;
    func_800526A8(moby); // Update collision
    moby->m_Renderer.raw = 0xbf;
    moby->m_SpecularMetalColor[0] = pParent->m_SpecularMetalColor[0];
    moby->m_SpecularMetalColor[1] = pParent->m_SpecularMetalColor[1];
    moby->m_SpecularMetalColor[2] = pParent->m_SpecularMetalColor[2];
    moby->m_SpecularMetalType = pParent->m_SpecularMetalType;
    moby->m_RenderRadius = pParent->m_RenderRadius;
    break;
  }

  case MOBYCLASS_FLIGHT_ARCH_FRAG_2: // Flight Gate Remaining Fragments
  case MOBYCLASS_FLIGHT_ARCH_FRAG_3:
  case MOBYCLASS_FLIGHT_ARCH_FRAG_4:
  case MOBYCLASS_FLIGHT_ARCH_FRAG_5:
  case MOBYCLASS_FLIGHT_ARCH_FRAG_6:
  case MOBYCLASS_FLIGHT_ARCH_FRAG_7:
  case MOBYCLASS_FLIGHT_ARCH_FRAG_8:
  case MOBYCLASS_FLIGHT_ARCH_FRAG_9:
  case MOBYCLASS_FLIGHT_ARCH_FRAG_10:
  case MOBYCLASS_FLIGHT_ARCH_FRAG_11:
  case MOBYCLASS_FLIGHT_ARCH_FRAG_12:
  case MOBYCLASS_FLIGHT_ARCH_FRAG_13: {
    func_8003A720(moby); // Reset the Moby first
    VecCopy(&moby->m_Position, &pParent->m_Position);
    moby->m_Rotation = pParent->m_Rotation;
    func_800526A8(moby); // Update collision
    moby->m_Renderer.raw = 0xbf;
    moby->m_SpecularMetalColor[0] = pParent->m_SpecularMetalColor[0];
    moby->m_SpecularMetalColor[1] = pParent->m_SpecularMetalColor[1];
    moby->m_SpecularMetalColor[2] = pParent->m_SpecularMetalColor[2];
    moby->m_SpecularMetalType = pParent->m_SpecularMetalType;
    break;
  }
#endif

#if (LEVEL % 10) == 5
#if defined(HAS_FLIGHT_RINGS) || defined(HAS_FLIGHT_ARCHES)
#define HAS_FLIGHT_HUD
  case MOBYCLASS_HUD_FLIGHT_RING:
  case MOBYCLASS_HUD_FLIGHT_ARCH:
#endif
#if defined(HAS_FLIGHT_CHESTS)
#define HAS_FLIGHT_HUD
  case MOBYCLASS_HUD_FLIGHT_CHEST:
#endif
#if defined(HAS_FLIGHT_PLANES) || defined(HAS_THIEF_PLANE)
#define HAS_FLIGHT_HUD
  case MOBYCLASS_HUD_FLIGHT_PLANE:
#endif
#ifdef HAS_FLIGHT_TRAINS
#define HAS_FLIGHT_HUD
  case MOBYCLASS_HUD_FLIGHT_BARREL:
#endif
#ifdef HAS_FLIGHT_LIGHTS
#define HAS_FLIGHT_HUD
  case MOBYCLASS_HUD_FLIGHT_LIGHT:
#endif
#ifdef HAS_FLIGHT_BOATS
#define HAS_FLIGHT_HUD
  case MOBYCLASS_HUD_FLIGHT_BOAT:
#endif
#ifdef HAS_FLIGHT_HELICOPTERS
#define HAS_FLIGHT_HUD
  case MOBYCLASS_HUD_FLIGHT_HELICOPTER:
#endif
#ifdef HAS_FLIGHT_HUD
  {
    func_8003A720(moby); // Reset the Moby first
    moby->m_RenderRadius = 0;
    moby->m_Position.x = -100;
    moby->m_Position.y = 30;
    moby->m_Position.z = 4096;

    func_800529CC(moby); // Set shaded moby
    moby->m_DepthOffset = 32;
    moby->m_SpecularMetalColor[0] = 0;
    moby->m_SpecularMetalColor[1] = 0;
    moby->m_SpecularMetalColor[2] = 0;
    moby->m_SpecularMetalType = 0;
    break;
  }
#endif
#undef HAS_FLIGHT_HUD
#endif

#ifdef HAS_VULTURES
  case MOBYCLASS_VULTURE_FRAG_1_MEAT:
  case MOBYCLASS_VULTURE_FRAG_2_FEATHER: {
    int r;
    int r2;
    struct {
      Vector3D16 vec_0x0;
      Vector3D16 vec_0x6;
      char unk_0xC;
      char unk_0xD;
      short unk_0xE;
      int unk_0x10;
    } *props = moby->m_Props;

    func_8003A720(moby);

    VecCopy(&moby->m_Position, &pParent->m_Position);

    func_800526A8(moby);

    r = rand() & 0xFFF;
    r2 = rand() & 0x7FF;
    CLAMP_VAR(r2, 0x80, 0x780)
    setXYZ(&props->vec_0x0, (Cos(r2) >> 5) * (Cos(r) >> 12),
           (Cos(r2) >> 5) * (Sin(r) >> 12), Sin(r2) >> 5);
    if (props->vec_0x0.z < 0x20)
      props->vec_0x0.z = 0x20;

    if (moby->m_Class == MOBYCLASS_VULTURE_FRAG_2_FEATHER) {
      props->vec_0x0.z += 0x20;
      moby->m_Rotation.z = rand();
      if ((rand() & 0xFFF) > 0x7FF) {
        moby->m_Rotation.y = (rand() & 0x1C) - 0x10;
        props->unk_0x10 = 1;
      }
    } else {
      CLAMP_VAR(props->vec_0x0.x, -0x60, 0x60);
      CLAMP_VAR(props->vec_0x0.y, -0x60, 0x60);
    }

    moby->m_Position.x += props->vec_0x0.x * 4;
    moby->m_Position.y += props->vec_0x0.y * 4;
    moby->m_Position.z += props->vec_0x0.z * 4;

    setXYZ(&props->vec_0x6, (rand() & 0x1F) - 0x10, (rand() & 0x1F) - 0x10,
           (rand() & 0x1F) - 0x10);
    props->unk_0xC = 50 - RandRange(0, 20);
    props->unk_0xD = 10;
    props->unk_0xE = 4;

    break;
  }
#endif

#ifdef HAS_ELDER_WIZARD_ENEMY
  case MOBYCLASS_ELDER_WIZARD_ARROWS: {
    MobyElderWizardArrowProps *props = moby->m_Props;
    int rotZ;
    int rotY;
    int dist;
    int floorZ;

    func_8003A720(moby);

    setXYZ(&moby->m_Position,
           pParent->m_Position.x + (COSINE_8(pParent->m_Rotation.z) >> 1),
           pParent->m_Position.y + (SINE_8(pParent->m_Rotation.z) >> 1),
           pParent->m_Position.z + 0x300);

    rotZ = ANGLE_TO_MOBY(g_Spyro) - pParent->m_Rotation.z & 0xff;
    MAKE_SIGNED(rotZ, 8)
    CLAMP_VAR(rotZ, -0x20, 0x20)
    moby->m_Rotation.z = pParent->m_Rotation.z + rotZ;

    dist = DISTANCE_TO_SPYRO(moby);
    floorZ = moby->m_Position.z - 356;
    rotY = Atan2(dist, g_Spyro.m_surfaceBelowSpyro - floorZ, 0) -
               pParent->m_Rotation.y &
           0xff;
    MAKE_SIGNED(rotY, 8)
    CLAMP_VAR(rotY, -0x30, 0x30)
    moby->m_Rotation.y = pParent->m_Rotation.y + rotY;

    props->m_Timer = 90;
    props->m_Parent = nullptr;

    func_800526A8(moby);
    break;
  }
#endif

#ifdef HAS_TORNADO_WIZARD_ENEMY
  case MOBYCLASS_TORNADO: {
    MobyTornadoProps *props = moby->m_Props;
    Vector3D v;
    void *parentProps = pParent->m_Props;

    func_8003A720(moby);
    VecCopy(&moby->m_Position, &pParent->m_Position);

    props->m_Parent = pParent;
    props->m_Path = *(PathData **)parentProps;
    VecNull(&props->m_Velocity);

    moby->m_RenderRadius = 32;

    func_800526A8(moby);

    MOBY_SHADOW_INIT(moby)

    break;
  }
#endif

#ifdef HAS_ELDER_WIZARD_ENEMY
  case MOBYCLASS_ELDER_WIZARD_ARROW_FRAG: {
    struct {
      int unk_0x0;
      int unk_0x4;
      int unk_0x8;
      int unk_0xC;
    } *props = moby->m_Props;

    func_8003A720(moby);

    VecCopy(&moby->m_Position, &pParent->m_Position);

    props->unk_0x0 = (rand() & 0x7E) - 0x3F;
    props->unk_0x4 = (rand() & 0x7E) - 0x3F;
    props->unk_0x8 = (rand() & 0x7E) - 0x10;
    props->unk_0xC = (rand() & 0x1F) + 0x20;

    func_800526A8(moby);
    break;
  }
#endif

#ifdef HAS_LASER_GNORC_ENEMY
  case MOBYCLASS_LASER_GNORC_GUN_RING: {
    MobyLaserGnorcGunRingProps *props = moby->m_Props;

    func_8003A720(moby);

    VecCopy(&moby->m_Position, &pParent->m_Position);

    props->m_Parent = pParent;
    props->m_Timer = 0;
    props->unk_08 = 0;

    moby->m_RenderRadius = 0;

    func_800526A8(moby);

    moby->m_WasDrawn = 0;

    break;
  }
#endif

#ifdef HAS_FAN_CHEST
  case MOBYCLASS_FAN_CHEST_TOP: { // Fan chest top
    func_8003A720(moby);          // Reset the Moby first
    moby->m_DepthOffset = 5;

    if (pParent) {
      VecCopy(&moby->m_Position, &pParent->m_Position);
    } else {
      VecCopy(&moby->m_Position, &g_Spyro.m_Position);
    }

    func_800526A8(moby); // Update collision
    break;
  }
#endif

#ifdef HAS_PORTAL_PATH
#define NEEDS_398_CODE
  case MOBYCLASS_PORTAL_PATH:
#endif
#ifdef HAS_FLIGHT_LIGHTS
#define NEEDS_398_CODE
  case MOBYCLASS_FLIGHT_LIGHT_ON:
#endif
#ifdef NEEDS_398_CODE
  {                      // ... Portal path Moby??
    func_8003A720(moby); // Reset the Moby first

    moby->m_RenderRadius = -1;

    moby->m_Position.x = 460;
    moby->m_Position.y = 40;
    moby->m_Position.z = 4096;

    func_800529CC(moby); // Set shaded moby

    moby->m_DepthOffset = 32;

    moby->m_SpecularMetalColor[0] = 0;
    moby->m_SpecularMetalColor[1] = 0;
    moby->m_SpecularMetalColor[2] = 0;
    moby->m_SpecularMetalType = 0;

    break;
  }
#endif
#undef NEEDS_398_CODE

#ifdef HAS_WATER
#define HAS_ANY_DROWNING
  case MOBYCLASS_WATER_BUBBLES:
  case MOBYCLASS_WATER_SPLASH:
#endif
#ifdef HAS_PURPLE_LAVA
#define HAS_ANY_DROWNING
  case MOBYCLASS_PURPLE_SPLASH:
  case MOBYCLASS_PURPLE_BUBBLES:
#endif
#ifdef HAS_LAVA
#define HAS_ANY_DROWNING
  case MOBYCLASS_LAVA_SPLASH:
  case MOBYCLASS_LAVA_BUBBLES:
#endif
#if defined(HAS_FRAG_WATER_SPLASH) || defined(HAS_FLIGHT_PLANES) ||            \
    defined(HAS_FLIGHT_HELICOPTERS) || defined(HAS_THIEF_PLANE)
#define HAS_ANY_DROWNING
  case MOBYCLASS_FRAG_WATER_SPLASH:
#endif
#ifdef HAS_ANY_DROWNING
  {
    int d, dt;

    func_8003A720(moby); // Reset the Moby first

    if (pParent) {
      VecCopy(&moby->m_Position, &pParent->m_Position);
    } else {
      VecCopy(&moby->m_Position, &g_Spyro.m_Position);
    }

    moby->m_Position.z += 512;

    d = func_8004D5EC(&moby->m_Position, 2048);

    dt = d - moby->m_Position.z;

    if (dt < 0)
      dt = -dt;

    if (dt < 2048) {
      moby->m_Position.z = d;
    } else {
      moby->m_Position.z -= 512;
    }

    func_800526A8(moby); // Update collision

    break;
  }
#endif

#ifdef HAS_CANNON_TARGET
  case MOBYCLASS_CANNON_TARGET_FRAG_1:
  case MOBYCLASS_CANNON_TARGET_FRAG_2:
  case MOBYCLASS_CANNON_TARGET_FRAG_3: {
    int r = RandRange(-2400, 1400);
    struct {
      Vector3D16 vec_0x0;
      Vector3D16 vec_0x6;
      int unk_0xC;
    } *props = moby->m_Props;

    func_8003A720(moby);
    VecCopy(&moby->m_Position, &pParent->m_Position);

    func_800526A8(moby);

    rand(); // why

    setXYZ(&props->vec_0x0, RandRange(-120, 120), RandRange(-120, 120),
           RandRange(50, 240));

    moby->m_Position.x += props->vec_0x0.x * 4;
    moby->m_Position.y += props->vec_0x0.y * 4;
    moby->m_Position.z += (props->vec_0x0.z * 4);
    moby->m_Position.z += r;

    moby->m_Position.x += props->vec_0x0.x * (-r + 1600) >> 10;
    moby->m_Position.y += props->vec_0x0.y * (-r + 1600) >> 10;

    setXYZ(&props->vec_0x6, rand() & 0xf, rand() & 0xf, rand() & 0xf);

    props->unk_0xC = 160;

    break;
  }
#endif

  // And then.. these?
  case MOBYCLASS_LETTER_APOSTROPHE:
  case MOBYCLASS_LETTER_A: // Text A-Z
  case MOBYCLASS_LETTER_B:
  case MOBYCLASS_LETTER_C:
  case MOBYCLASS_LETTER_D:
  case MOBYCLASS_LETTER_E:
  case MOBYCLASS_LETTER_F:
  case MOBYCLASS_LETTER_G:
  case MOBYCLASS_LETTER_H:
  case MOBYCLASS_LETTER_I:
  case MOBYCLASS_LETTER_J:
  case MOBYCLASS_LETTER_K:
  case MOBYCLASS_LETTER_L:
  case MOBYCLASS_LETTER_M:
  case MOBYCLASS_LETTER_N:
  case MOBYCLASS_LETTER_O:
  case MOBYCLASS_LETTER_P:
  case MOBYCLASS_LETTER_Q:
  case MOBYCLASS_LETTER_R:
  case MOBYCLASS_LETTER_S:
  case MOBYCLASS_LETTER_T:
  case MOBYCLASS_LETTER_U:
  case MOBYCLASS_LETTER_V:
  case MOBYCLASS_LETTER_W:
  case MOBYCLASS_LETTER_X:
  case MOBYCLASS_LETTER_Y:
  case MOBYCLASS_LETTER_Z: {
    func_8003A720(moby); // Reset the Moby first

    moby->m_RenderRadius = 32;
    moby->m_UpdateDistance = -1;

    func_800529CC(moby); // Set shaded moby
    break;
  }

#ifdef HAS_CANNONBALLS
  case MOBYCLASS_CANNONBALL_FRAG_1:
  case MOBYCLASS_CANNONBALL_FRAG_2: {
    struct {
      Vector3D16 vec_0x0;
      Vector3D16 vec_0x6;
      int unk_0xC;
    } *props = moby->m_Props;

    func_8003A720(moby);
    moby->m_RenderRadius = 32;

    VecCopy(&moby->m_Position, &pParent->m_Position);

    func_800526A8(moby);

    if (pParent->m_Class == MOBYCLASS_GRENADE_TWILIGHT_HARBOR) {
      setXYZ(&props->vec_0x0, RandRange(-100, 100), RandRange(-100, 100),
             RandRange(-60, 110));
    } else {
      setXYZ(&props->vec_0x0, RandRange(-150, 150), RandRange(-150, 150),
             RandRange(-70, 150));
    }

    moby->m_Position.x += props->vec_0x0.x * 4;
    moby->m_Position.y += props->vec_0x0.y * 4;
    moby->m_Position.z += props->vec_0x0.z * 4;

    setXYZ(&props->vec_0x6, rand() & 0xf, rand() & 0xf, rand() & 0xf);

    props->unk_0xC = RandRange(24, 35);

    break;
  }
#endif

#ifdef HAS_CHICKEN_FODDER
  case MOBYCLASS_CHICKEN_FRAG: { // Chicken fodder feather
    struct {
      short unk_0x00;
      short unk_0x02;
      short unk_0x04;

      short unk_0x06;
      short unk_0x08;
      short unk_0x0A;

      char unk_0x0C;
      char unk_0x0D;
      short unk_0x0E;
      int unk_0x10;
    } *chickenFeatherProps = moby->m_Props;
    int angle1;
    int angle2;

    func_8003A720(moby); // Reset the Moby first
    VecCopy(&moby->m_Position, &pParent->m_Position);
    func_800526A8(moby); // Update collision

    angle2 = rand() & 0xFFF;
    angle1 = rand() & 0x7FF;

    if (angle1 < 128) {
      angle1 = 128;
    }

    if (angle1 > 1920) {
      angle1 = 1920;
    }

    chickenFeatherProps->unk_0x00 = (Cos(angle1) >> 5) * (Cos(angle2) >> 12);
    chickenFeatherProps->unk_0x02 = (Cos(angle1) >> 5) * (Sin(angle2) >> 12);
    chickenFeatherProps->unk_0x04 = Sin(angle1) >> 5;

    if (chickenFeatherProps->unk_0x04 < 32) {
      chickenFeatherProps->unk_0x04 = 32;
    }

    chickenFeatherProps->unk_0x04 += 32;

    moby->m_Rotation.z = rand();

    if ((rand() & 0xFF) > 128) {
      moby->m_Rotation.y = (rand() & 28) - 16;
      chickenFeatherProps->unk_0x10 = 1;
      if (chickenFeatherProps->unk_0x04 > 32) {
        chickenFeatherProps->unk_0x04 = 32;
      }
    }

    moby->m_Position.x += chickenFeatherProps->unk_0x00 * 4;
    moby->m_Position.y += chickenFeatherProps->unk_0x02 * 4;
    moby->m_Position.z += chickenFeatherProps->unk_0x04 * 4;

    chickenFeatherProps->unk_0x06 = (rand() & 0x1F) - 16;
    chickenFeatherProps->unk_0x08 = (rand() & 0x1F) - 16;
    chickenFeatherProps->unk_0x0A = (rand() & 0x1F) - 16;
    chickenFeatherProps->unk_0x0C = 50 - RandRange(0, 20);
    chickenFeatherProps->unk_0x0D = 10;
    chickenFeatherProps->unk_0x0E = 4;

    break;
  }
#endif

  default: {
#ifdef HAS_DRAGON
    // dont like this
    char pad[8];
#endif

    func_8003A720(moby); // Reset the Moby first

    if (pParent) {
      VecCopy(&moby->m_Position, &pParent->m_Position);
    } else {
      VecCopy(&moby->m_Position, &g_Spyro.m_Position);
    }

    func_800526A8(moby); // Update collision
    break;
  }
  }

  return moby;
}
