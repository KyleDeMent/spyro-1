#include "moby.h"

#include "42CC4.h"
#include "balloonist.h"
#include "camera.h"
#include "collision.h"
#include "common.h"
#include "cyclorama.h"
#include "environment.h"
#include "gamepad.h"
#include "gamestates/init.h"
#include "hud.h"
#include "loaders.h"
#include "math.h"
#include "moby_helpers.h"
#include "moby_lists.h"
#include "overlay_pointers.h"
#include "rand.h"
#include "renderers.h"
#include "sony_image.h"
#include "spu.h"
#include "spyro.h"
#include "variables.h"

#define ROTDEG(X) ((int)((X) / (360.0f / 256.0f)))

extern int D_80075794;
// g_FrameFinished
extern int D_800757F4;
// g_MobyLocalDeltaTime
extern int D_800756C4;
extern Moby *g_DynMobys;
extern Vector3D D_8006E5A0;
extern Vector3D D_8006E5B8;
extern Vector3D D_8006E5AC;
extern Vector3D D_8006E57C[3];
extern int D_80075808;
// Sparx Glow Vertices
extern int D_8006E390[9][2];
// Fix type
extern int D_8006E490;
extern int D_8006E494;
// Fix
extern int D_8006E330;
extern SphericalCoordsOffset D_8006CA24[];
extern int g_KeyFlag;
extern int g_ScreenBorderEnabled; // Is the screen border enabled
extern unsigned char D_800758D0[8];
extern int g_DynMobyMax;
extern int g_DynMobyCount;
extern signed char D_8006E638[32][2];

void func_level_60_8007D938(void) {

  Moby **mobyList;
  Moby *moby;
  Vector3D vec_38;
  Vector3D eggInterpStartPos;
  Vector3D eggInterpDelta;
  Vector3D vec_68;
  Vector3D eggCameraEffectPos;
  Vector3D vec_88;
  Vector3D collectableCollVerts[3];
  Vector3D collectableMovePos;
  Vector3D collectableSurfaceNormal;
  Vector3D collectablePickupDelta;
  Vector3D sparxChaseDelta;

  // Generate the moby update list (which looks at things such as the state and
  // distance)
  func_80051FEC();

  func_800522C0((Moby **)(g_SonyImage.u.m_Buf + 0x400), 0);

  if (g_DeltaTime > 2) {
    u_int val = 1 << 31;

    if (g_DeltaTime == 3)
      val |= 1;

    func_800522C0((Moby **)(g_SonyImage.u.m_Buf + 0x400), val);
  }

  mobyList = (Moby **)(g_SonyImage.u.m_Buf + 0x400);


  while (moby = *mobyList++) {
    int mobyClass, localDeltaTime;
    if (moby->m_State > 127)
      continue;

    localDeltaTime = g_DeltaTime;
    mobyClass = moby->m_Class;

    // Seems to be a macro
    D_80075794 = moby->m_AnimationState.m_AnimationFlags & 2; // iirc, animation finished
    D_800757F4 = moby->m_AnimationState.m_AnimationFlags & 1; // iirc, frame finished
    D_800756C4 = localDeltaTime;                              // Deltatime used in overlays, dunno why


    switch (mobyClass) {
    case 1: { // Portal text
      int shouldSpawnText, cameraPortalAngleDiff;
      int distance = OctDistance(&moby->m_Position, &g_Camera.m_Position);

      if (distance < (moby->m_State ? 12 : 11) * 1024) {
        shouldSpawnText = moby->m_State == 0;

        moby->m_State = 1;

        // Subtract the angle between us and the camera from the portal rotation
        cameraPortalAngleDiff = func_80017908(moby->m_Rotation.z, Atan2(g_Camera.m_Position.x - moby->m_Position.x, g_Camera.m_Position.y - moby->m_Position.y, 0));

        // You're facing the rotation of the portal
        if (cameraPortalAngleDiff <= ROTDEG(80)) {
          if (moby->m_Substate == 1) {
            // Retrigger
            shouldSpawnText = 1;
          }

          // Rotate the text by 180 degrees
          moby->m_Substate = 0;
        } else if (cameraPortalAngleDiff > ROTDEG(100)) { // You're facing against the rotation of the portal
          if (moby->m_Substate == 0) {
            // Retrigger
            shouldSpawnText = 1;
          }

          // Keep the portal rotation
          moby->m_Substate = 1;
        } else {
          // You're facing the portal in the 20 degree deadzone
          // Disable the text
          moby->m_State = 0;
        }

        if (moby->m_State && shouldSpawnText) {
          // Create the portal text
          func_8003C358(moby, 1);
        }
      } else {
        moby->m_State = 0;
      }

      break;
    }

    case 13: {
      Moby *parent;
      MobyGemSpawnerProps *spawnerProps;
      spawnerProps = (MobyGemSpawnerProps *)moby->m_Props;
      parent = &g_LevelMobys[spawnerProps->m_ParentIndex];

      if (spawnerProps->m_InitDone == 0) {
        if (spawnerProps->m_RelativeMode != 0) {
          VecSub(&spawnerProps->m_Position, &spawnerProps->m_Position, &parent->m_Position);
          moby->m_UpdateDistance = 0xFF;
        } else {
          moby->m_UpdateDistance = 0x20;
        }

        if (spawnerProps->m_HasParent != 0 && (parent->m_State >= 0x80 || (parent->m_DropMoby & 0x80))) {
          Moby *dst;
          Moby *src;
          int newIndex;

          parent->m_DropMoby = moby->m_DropMoby;
          func_80052568(moby);
          if (parent->m_State < 0x80) {
            func_80052568(parent);
          }

          dst = moby;
          src = parent;
          *dst = *src;
          func_800526A8(moby);
          moby->m_WasDrawn = 1;

          newIndex = moby - g_LevelMobys;

          for (parent = g_LevelMobys; parent < (Moby *)g_DynMobys; parent++) {
            MobyGemSpawnerProps *mp;
            if (parent == moby)
              continue;
            if (parent->m_State >= 0x80)
              continue;
            if (parent->m_Class != MOBYCLASS_GEM_SPAWNER)
              continue;
            mp = (MobyGemSpawnerProps *)parent->m_Props;
            if (mp->m_ParentIndex == spawnerProps->m_ParentIndex) {
              mp->m_ParentIndex = newIndex;
            }
          }

          moby->m_State = 0;
          moby->m_AnimationState.m_FrameProgress = 0;

          moby->m_AnimationState.m_PerFrameProgress = g_Models[moby->m_Class]->m_Animations[0]->m_ProgressPerTick;
          moby->m_AnimationState.m_Animation = 0;
          moby->m_AnimationState.m_NextAnimation = 0;
          moby->m_AnimationState.m_Frame = 0;
          moby->m_AnimationState.m_NextFrame = 1;
          break;
        } else {
          spawnerProps->m_InitDone = 1;
        }
      }

      if (spawnerProps->m_RelativeMode != 0) {
        VecCopy(&moby->m_Position, &parent->m_Position);
      }

      if (spawnerProps->m_SpawnTimer != 0) {
        if (TimerTick(&spawnerProps->m_SpawnTimer, 4) != 0) {
          if (spawnerProps->m_RelativeMode != 0) {
            VecAdd(&spawnerProps->m_Position, &spawnerProps->m_Position, &moby->m_Position);
          } else {
            VecCopy(&moby->m_Position, &parent->m_Position);
          }

          spawnerProps->m_Position.z += 0x400;

          if (spawnerProps->m_GroundFlag != 0) {
            func_8003ABC0(moby, 4, 0, nullptr);
          } else if (func_8004D5EC(&spawnerProps->m_Position, 0x1400) != 0) {
            spawnerProps->m_Position.z -= 0x400;
            func_8003ABC0(moby, 2, 0, &spawnerProps->m_Position);
          } else {
            func_8003ABC0(moby, 1, 0, nullptr);
          }

          func_8003B7C0(moby);

          spawnerProps->m_PitchOffset <<= 7;
          g_Spu.m_NextSoundOverrideFlags = 2;
          g_Spu.m_PitchOverride = g_Spu.m_SoundDefinitions[g_Spu.m_SoundTable->titlescreenMove].m_Pitch + spawnerProps->m_PitchOffset;
          PlaySound(g_Spu.m_SoundTable->titlescreenMove, moby, 8, &moby->m_SoundChannel);
          func_80052568(moby);
        }
      } else if (moby->m_Substate != 0 || parent->m_State >= 0x80) {
        spawnerProps->m_SpawnTimer = 9;
        spawnerProps->m_PitchOffset = 1;

        for (parent = g_LevelMobys; parent < moby; parent++) {
          MobyGemSpawnerProps *mp;
          if (parent->m_State >= 0x80)
            continue;
          if (parent->m_Class != MOBYCLASS_GEM_SPAWNER)
            continue;
          mp = (MobyGemSpawnerProps *) parent->m_Props;
          if (mp->m_ParentIndex == spawnerProps->m_ParentIndex) {
            spawnerProps->m_SpawnTimer += 9;
            spawnerProps->m_PitchOffset += 1;
          }
        }

        if (spawnerProps->m_PitchOffset == 1) {
          g_Spu.m_NextSoundOverrideFlags = 2;
          g_Spu.m_PitchOverride = g_Spu.m_SoundDefinitions[g_Spu.m_SoundTable->titlescreenMove].m_Pitch;
          PlaySound(g_Spu.m_SoundTable->titlescreenMove, moby, 8, &moby->m_SoundChannel);
        }
      }
      break;
    }
    case 16: { // Butterfly
      MobyButterflyProps* butterflyProps = moby->m_Props;

      if (g_Spyro.m_health < 3 && OctDistance(&moby->m_Position, &g_Spyro.m_Position) < 2400) {
        // We're in range and Spyro's health isn't full
        int mobyZ = moby->m_Position.z - moby->m_FloorDistance;
        if (ABS2(mobyZ - g_Spyro.m_Position.z) < 1800) {

          if (g_Sparx == nullptr) {             // If Sparx is not alive
            if (g_Spyro.m_health >= 0) {        // And Spyro isn't dead
              g_Sparx = g_SpawnMoby(120, moby); // Spawn Sparx

              // Play the sound
              func_8003851C(g_Sparx, 0, nullptr);

              g_Spyro.m_health = 1;

              func_80052568(moby); // Kill the Butterfly
              break;
            }
          } else if (g_Sparx->m_Substate != 99) {
            MobySparxProps *sparxProps = g_Sparx->m_Props;

            // Sparx is alive, and not chasing something already
            if (sparxProps->m_MobyPickingUp == nullptr) {
              // Set him to chase us
              if (func_800381BC(g_Spyro.m_bodyRotation.z, Atan2(moby->m_Position.x - g_Spyro.m_Position.x, moby->m_Position.y - g_Spyro.m_Position.y, 0)) < 0) {
                butterflyProps->m_SparxApproachSide = 1;
              } else {
                butterflyProps->m_SparxApproachSide = -1;
              }

              g_Sparx->m_Substate = 0;
              sparxProps->m_MobyPickingUp = moby;
              sparxProps->m_Timer = RandRange(150, 210);

              moby->m_State = 1;
            }
          }
        }
      }

      if (moby->m_RenderRadius & 0x80)
        break;

      switch (moby->m_State) {
      case 0: {
        // Timer running, exit
        // Wasn't drawn last frame, exit
        if (TimerTick(&butterflyProps->m_unk_14, 2) == 0 || moby->m_WasDrawn) {
          int refHeight, rotated, moved;

          if (TimerTick(&butterflyProps->m_TurnRetargetTimer, 1) != 0) {
            int r = RandRangeSigned(30, 90);
            if (rand() & 1) {
              r = -r;
            }

            butterflyProps->m_TargetAngle = func_80038074(butterflyProps->m_TargetAngle, r);
            butterflyProps->m_TurnRetargetTimer = RandRange(60, 140);
          }

          if (TimerTick(&butterflyProps->m_VerticalSpeedTimer, 1) != 0) {
            butterflyProps->m_VerticalSpeed = RandRangeSigned(10, 25);
            butterflyProps->m_VerticalSpeedTimer = RandRange(80, 140);
          }

          if (butterflyProps->m_VerticalSpeed >= 128) {
            moby->m_Position.z = moby->m_Position.z + (butterflyProps->m_VerticalSpeed - 256);
          } else {
            moby->m_Position.z = moby->m_Position.z + butterflyProps->m_VerticalSpeed;
          }

          refHeight = moby->m_Position.z - func_80038340(moby);

          if (refHeight < 300) {
            butterflyProps->m_VerticalSpeed = RandRange(10, 25);
            butterflyProps->m_VerticalSpeedTimer = RandRange(80, 140);
          }

          if (1024 < refHeight && refHeight < 2048 ) {
            butterflyProps->m_VerticalSpeed = -RandRange(10, 25);
            butterflyProps->m_VerticalSpeedTimer = RandRange(80, 140);
          }

          rotated = RotateMobyToAngle(moby, butterflyProps->m_TargetAngle, 4, 0xA, 1);
          moved = func_80039398(moby, 30, 0, 50, 1);

          if (rotated && moved) {
            butterflyProps->m_TargetAngle = func_80038074(butterflyProps->m_TargetAngle, RandRange(98, 158));
            butterflyProps->m_TurnRetargetTimer = RandRange(30, 80);
          }

          // Check distance to target position
          if (OctDistance(&moby->m_Position, &butterflyProps->m_AnchorPosition) > 2000) {
            // Too far from target, turn directly towards it
            int angle = Atan2(butterflyProps->m_AnchorPosition.x - moby->m_Position.x, butterflyProps->m_AnchorPosition.y - moby->m_Position.y, 0);

            butterflyProps->m_TargetAngle = angle;
            butterflyProps->m_TurnRetargetTimer = 0x28;
          }
        } else {
          func_80052568(moby);
        }
        break;
      }
      case 1: {
        func_80038638(moby, &g_Spyro.m_Position, 0x320, func_80038074(Atan2(moby->m_Position.x - g_Spyro.m_Position.x, moby->m_Position.y - g_Spyro.m_Position.y, 0), butterflyProps->m_SparxApproachSide * 8), 8, 0x82, 0xE, 0x80, 0xFF, 0xFF, 0, 0, 0);

        // If Sparx is not alive, or is chasing something
        if (g_Sparx == nullptr || g_Sparx->m_Substate == 99) {
          moby->m_State = 0;
        } else {
          int refHeight;

          // Timer
          if (TimerTick(&butterflyProps->m_VerticalSpeedTimer, 1)) {
            butterflyProps->m_VerticalSpeed = RandRangeSigned(5, 20);
            butterflyProps->m_VerticalSpeedTimer = RandRange(40, 80);
          }

          if (butterflyProps->m_VerticalSpeed >= 128) {
            moby->m_Position.z = (moby->m_Position.z) + (butterflyProps->m_VerticalSpeed - 256);
          } else {
            moby->m_Position.z = moby->m_Position.z + butterflyProps->m_VerticalSpeed;
          }

          refHeight = moby->m_Position.z - func_80038340(moby);

          if (refHeight < 200) {
            butterflyProps->m_VerticalSpeed = RandRange(5, 20);
            butterflyProps->m_VerticalSpeedTimer = RandRange(40, 80);
          }

          if (1000 < refHeight && refHeight < 2048) {
            butterflyProps->m_VerticalSpeed = -RandRange(5, 20);
            butterflyProps->m_VerticalSpeedTimer = RandRange(40, 80);
          }

        }
        break;
      }
      }

      break;
    }

    case 34: { // Dragon Egg
      struct {
        int m_EggIndex;
        Vector3D16 m_StartPosition;
        Vector3D16 m_TargetPosition;
        short m_SpiralRadius;
        unsigned char m_Timer;
        unsigned char m_Angle;
      } *eggProps = moby->m_Props;

      // If egg is in active collection states (1-4), prevent Spyro from certain
      // actions
      if (moby->m_State != 0 && moby->m_State < 5) {
        g_Spyro.m_ControlFlags = (0x80000000 | 0x00002000); // Set some flag

        // Skip if spyro isn't in a valid state
        if (g_Spyro.m_State != 0 && g_Spyro.m_State != 0xC) {
          moby->m_State = 5;
        }
      }

      // During spinning states (2-4), make spyro look at the Moby, and spawn
      // particle
      if (moby->m_State >= 2 && moby->m_State <= 4) {
        SetSpyroHeadLookTarget(&moby->m_Position);
        D_800758E4(1, 0xC, moby, (int *)0x602080); // Spawn particle
      }

      switch (moby->m_State) {
      case 1: { // Initialize collection
        g_ScreenBorderEnabled = 1;

        // Store starting position
        func_80017BFC(&eggProps->m_StartPosition, &moby->m_Position);

        // Calculate initial angle from egg to Spyro
        eggProps->m_Angle = (Atan2Fast(moby->m_Position.x - g_Spyro.m_Position.x, moby->m_Position.y - g_Spyro.m_Position.y) + 64);

        eggProps->m_SpiralRadius = 0;
        eggProps->m_Timer = 0;
        g_Hud.D_80077FDC = 1; // Show collection UI?

        moby->m_Rotation.x = 0;
        moby->m_Rotation.y = 8;
        moby->m_State++;
        break;
      }

      case 2: {
        eggProps->m_Timer += g_DeltaTime;
        if (eggProps->m_Timer < 128) {

          eggProps->m_SpiralRadius += g_DeltaTime * 4;
          eggProps->m_Angle += g_DeltaTime * 4;

          func_80017C24(&moby->m_Position, &eggProps->m_StartPosition);

          // Apply circular motion
          moby->m_Position.x += FIXED_MUL(Cos(eggProps->m_Angle * 16), eggProps->m_SpiralRadius);
          moby->m_Position.y += FIXED_MUL(Sin(eggProps->m_Angle * 16), eggProps->m_SpiralRadius);

          // Spin and rise
          moby->m_Position.z -= eggProps->m_Timer * 8;
          moby->m_Rotation.z += g_DeltaTime * 4;
        } else {

          VecCopy(&vec_38, &g_Spyro.m_Position);

            //todo here
          // v.x += (((Cos(g_Spyro.m_bodyRotation.z * 16)*181)*4 + (Sin(g_Spyro.m_bodyRotation.z * 16)*181)*4)) >> 12;
          {
            // TODO: Messed up match (Possible macro usage related to short
            // vectors)
            int cos = Cos(g_Spyro.m_bodyRotation.z * 16);
            int sin = Sin(g_Spyro.m_bodyRotation.z * 16);
            cos = ((cos * 181) * 4);
            sin = ((sin * 181) * 4);
            vec_38.x = vec_38.x + ((cos + sin) >> 12);
          }

          {
            // TODO: Messed up match (Possible macro usage related to short
            // vectors)
            int sin = Sin(g_Spyro.m_bodyRotation.z * 16);
            int cos = Cos(g_Spyro.m_bodyRotation.z * 16);
            sin = ((sin * 181) * 4);
            cos = ((cos * 181) * 4);
            vec_38.y = vec_38.y + ((sin - cos) >> 12);
          }

          vec_38.z += 768;

          func_80017BFC(&eggProps->m_TargetPosition, &vec_38);
          func_80017BFC(&eggProps->m_StartPosition, &moby->m_Position);

          eggProps->m_Timer = 0;
          moby->m_State++;
        }

        if (g_Pad.m_Down & PAD_CROSS) { // Skip button
          moby->m_State = 5;
        }
        break;
      }

      case 3: {
        eggProps->m_Timer += g_DeltaTime;
        if (eggProps->m_Timer < 64) {

          func_80017C24(&eggInterpStartPos, &eggProps->m_StartPosition);
          func_80017C24(&eggInterpDelta, &eggProps->m_TargetPosition);

          // Interpolate between start and target
          VecSub(&eggInterpDelta, &eggInterpDelta, &eggInterpStartPos);
          VecMult(&eggInterpDelta, &eggInterpDelta, eggProps->m_Timer);
          VecShiftRight(&eggInterpDelta, 6); // Divide by 64 for smooth interpolation
          VecAdd(&moby->m_Position, &eggInterpStartPos, &eggInterpDelta);

          moby->m_Rotation.z += g_DeltaTime * 4;
        } else {

          VecCopy(&vec_68, &g_Spyro.m_Position);

          vec_68.x += Cos(g_Spyro.m_bodyRotation.z * 0x10) >> 3;
          vec_68.y += Sin(g_Spyro.m_bodyRotation.z * 0x10) >> 3;
          vec_68.z += 0x300;

          func_80017BFC(&eggProps->m_StartPosition, &vec_68);

          eggProps->m_Angle = (g_Spyro.m_bodyRotation.z - 64);
          eggProps->m_SpiralRadius = 0x200;
          eggProps->m_Timer = 0;
          moby->m_State++;
        }

        if (g_Pad.m_Down & PAD_CROSS) { // Skip
          moby->m_State = 5;
        }
        break;
      }

      case 4: {
        eggProps->m_Timer += g_DeltaTime;
        if (eggProps->m_Timer < 128) {
          int deltaAngle = g_DeltaTime * 4;

          // Contract spiral
          eggProps->m_SpiralRadius -= deltaAngle;
          eggProps->m_Angle -= deltaAngle;

          func_80017C24(&moby->m_Position, &eggProps->m_StartPosition, deltaAngle);

          // Apply circular motion
          moby->m_Position.x += (Cos(eggProps->m_Angle * 0x10) * eggProps->m_SpiralRadius) >> 12;
          moby->m_Position.y += (Sin(eggProps->m_Angle * 0x10) * eggProps->m_SpiralRadius) >> 12;

          moby->m_Position.z -= eggProps->m_Timer * 4;
          moby->m_Rotation.z += g_DeltaTime * 4;
        } else {
          moby->m_State++;
        }

        if (g_Pad.m_Down & PAD_CROSS) { // Skip button
          moby->m_State = 5;
        }
        break;
      }

      case 5: {

        // Get position relative to camera for particle effect
        VecSub(&eggCameraEffectPos, &moby->m_Position, &g_Camera.m_Position);
        VecRotateByCam(&eggCameraEffectPos, &eggCameraEffectPos);
        D_800758E4(0x10, 0x4D, &eggCameraEffectPos, nullptr);

        g_ScreenBorderEnabled = 0;
        VecNull(&g_Spyro.m_HeadLookTarget);

        // TODO: ????
        func_8003B854(0, (Moby *)eggProps->m_EggIndex);

        // Update egg counters
        g_EggTotal++;
        g_LevelEggCount[g_LevelIndex]++;

        g_Hud.D_80077FDC = 0; // Hide collection UI?

        func_80052568(moby); // Delete egg moby

        break;
      }
      }

      break;
    }

    case 255:
    case 256: {
      MobyFragmentPhysicsProps *physicsProps = moby->m_Props;

      if (physicsProps->m_Lifetime > 0 && moby->m_WasDrawn) {
        if (func_8004BE4C(&moby->m_Position, 0x100, 0x100)) {
          int dot;
          func_80017330(&g_CollisionNormal, 0x1000);
          dot = physicsProps->m_VelocityX * g_CollisionNormal.x + physicsProps->m_VelocityY * g_CollisionNormal.y + physicsProps->m_VelocityZ * g_CollisionNormal.z >> 11;
          if (dot < 0) {
            VecScaleToLength(&g_CollisionNormal, 0x1000, (dot >> 2) - (dot));
            physicsProps->m_VelocityX += g_CollisionNormal.x;
            physicsProps->m_VelocityY += g_CollisionNormal.y;
            physicsProps->m_VelocityZ += g_CollisionNormal.z;
          }
        }

        moby->m_Position.x += physicsProps->m_VelocityX;
        moby->m_Position.y += physicsProps->m_VelocityY;
        physicsProps->m_VelocityZ -= 6;
        if (physicsProps->m_VelocityZ < -128)
          physicsProps->m_VelocityZ = -128;
        moby->m_Position.z += physicsProps->m_VelocityZ;

        moby->m_Rotation.x += physicsProps->m_AngularVelocityX;
        moby->m_Rotation.y += physicsProps->m_AngularVelocityY;
        moby->m_Rotation.z += physicsProps->m_AngularVelocityZ;
        if ((physicsProps->m_Lifetime & 3) == 0) {

          vec_68.x = rand() & 3;
          vec_68.y = rand() & 3;
          vec_68.z = 0x14;
          D_800758E4(1, 1, &moby->m_Position, &vec_68);
        }
        physicsProps->m_Lifetime--;
      } else {
        D_800758E4(8, 0x46, &moby->m_Position, (void *)0x10);
        func_80052568(moby);
      }
      break;
    }

    case 257: {
      MobyFragmentPhysicsProps *physicsProps = moby->m_Props;

      if (physicsProps->m_Lifetime != 0 && moby->m_WasDrawn && moby->m_Position.z > physicsProps->m_KillBelowZ) {
        // Apply velocity to position
        moby->m_Position.x += physicsProps->m_VelocityX;
        moby->m_Position.y += physicsProps->m_VelocityY;

        // Apply gravity to Z velocity
        physicsProps->m_VelocityZ -= 6;

        if (physicsProps->m_VelocityZ < -0x80) {
          physicsProps->m_VelocityZ = -0x80;
        }

        moby->m_Position.z += physicsProps->m_VelocityZ;

        // Apply angular velocity to rotation
        moby->m_Rotation.x += physicsProps->m_AngularVelocityX;
        moby->m_Rotation.y += physicsProps->m_AngularVelocityY;
        moby->m_Rotation.z += physicsProps->m_AngularVelocityZ;

        physicsProps->m_Lifetime--;
      } else {
        func_80052568(moby);
      }
      break;
    }

    case 14: // Life statue
    case 15: // Life orb
    case 83: // The gems
    case 84:
    case 85:
    case 86:
    case 87: {
      MobyCollectableProps *collectableProps = moby->m_Props;

      int distanceToSpyro = OctDistance(&moby->m_Position, &g_Spyro.m_Position);

      // Sparkle stuff
      if (collectableProps->m_Ticks < 0xFA) {
        collectableProps->m_Ticks += g_DeltaTime;
      }

      if (moby->m_Class != 15) {

        if (collectableProps->m_SparkleHandle != 255) {
          func_800529E4(moby, 4);

          if (moby->m_Class == 14) {
            VecRotateByMatrix((MATRIX *)&moby->m_RotationMatrix, &D_8006E5B8, &g_Sparkles[collectableProps->m_SparkleHandle].m_Position);
          } else {
            VecRotateByMatrix((MATRIX *)&moby->m_RotationMatrix, &D_8006E5A0, &g_Sparkles[collectableProps->m_SparkleHandle].m_Position);
          }

          VecAdd(&g_Sparkles[collectableProps->m_SparkleHandle].m_Position, &g_Sparkles[collectableProps->m_SparkleHandle].m_Position, &moby->m_Position);

          if (g_Sparkles[collectableProps->m_SparkleHandle].m_Life < 5) {
            collectableProps->m_SparkleHandle = 255;
          }
        } else {
          if (collectableProps->m_Ticks > 0xf7) {
            int handle = SpawnMobySparkle(moby, &D_8006E5A0);
            if (handle >= 0 && distanceToSpyro < 0x4000) {
              collectableProps->m_SparkleHandle = handle;
            }

            collectableProps->m_Ticks = (rand() & 0x1F) + 33;
          }
        }
      }

      if (!moby->m_WasDrawn && distanceToSpyro > 0x2000 && moby->m_Substate == 1 && collectableProps->m_SpawnState == 0) {
        break;
      }

      switch (moby->m_Substate) {
      case 0: {

        // Initialize
        VecCopy(&vec_38, &moby->m_Position);
        vec_38.z += 0x400;
        func_8004D5EC(&vec_38, 0x10000);
        collectableProps->m_RotX = -Atan2Fast(func_80017A38((g_CollisionNormal.x * g_CollisionNormal.x) + (g_CollisionNormal.z * g_CollisionNormal.z)), g_CollisionNormal.y);
        collectableProps->m_RotY = -Atan2Fast(g_CollisionNormal.z, g_CollisionNormal.x);

        // Smart compiler c:
        if (collectableProps->m_RotX != 0 || collectableProps->m_RotY != 0) {
          moby->m_Rotation.z = 0;
        }

        moby->m_Substate = 1;
        break;
      }
      case 1: {

        if (collectableProps->m_SpawnState != 0) {

          if (collectableProps->m_SpawnState == 1) {
            func_80038458(moby); // Place on floor
            func_800533D0(moby); // Set shadow
            if (moby->m_Class >= 83) {
              collectableProps->m_RotX = -Atan2Fast(func_80017A38((g_CollisionNormal.x * g_CollisionNormal.x) + (g_CollisionNormal.z * g_CollisionNormal.z)), g_CollisionNormal.y);
              collectableProps->m_RotY = -Atan2Fast(g_CollisionNormal.z, g_CollisionNormal.x);
            }

          } else if (collectableProps->m_SpawnState == 2) {

            // Get a collision polygon, unpack it
            ColTriUnpack(collectableProps->m_CollisionIndex, collectableCollVerts);

            // Sum the entire thing together
            VecAdd(&collectableCollVerts[0], &collectableCollVerts[0], &collectableCollVerts[1]);
            VecAdd(&collectableCollVerts[0], &collectableCollVerts[0], &collectableCollVerts[2]);

            // Set the moby's position to the average of that collision polygon
            moby->m_Position.x = collectableCollVerts[0].x / 3;
            moby->m_Position.y = collectableCollVerts[0].y / 3;
            moby->m_Position.z = collectableCollVerts[0].z / 3;

            func_800529E4(moby, UPDATE_PROP_CHAIN);
            func_80038458(moby); // Place on floor
            func_800533D0(moby); // Set shadow

          } else if (collectableProps->m_SpawnState == 3) {
            int colIndex;

            VecCopy(&vec_88, &moby->m_Position);
            vec_88.z += 0x400;

            func_800529E4(moby, UPDATE_PROP_CHAIN);

            // I think this should be >= 0, not > 0?
            if (func_8004D5EC(&vec_88, 0x1000) > 0) {
              colIndex = g_CollisionTriangleIndex;
              collectableProps->m_SpawnState = 2;
              collectableProps->m_CollisionIndex = colIndex;
            }
          }
        }

        // Need to be on seperate lines for some reason
        if (distanceToSpyro < 1434) {
          int zDist = (g_Spyro.m_Position.z - 356) - moby->m_Position.z;

          if (ABS2(zDist) < 512) {

            // Again.. seperate lines?
            if (g_Sparx != nullptr) {

              if (g_Spyro.m_airTime == 0) {

                // Check added after Tabloid/E3
                int res = func_80033E40(&g_Sparx->m_Position, &moby->m_Position);

                // Returns 1 if it didn't hit anything
                if (res) {
                  // From this part on, it's a jump to somewhere later
                  MobySparxProps *sparxProps = g_Sparx->m_Props;

                  if (sparxProps->m_MobyPickingUp == nullptr) {
                    if (!IsMobyPlayingSound(g_Sparx, g_Models[g_Sparx->m_Class]->m_Sounds[0])) {
                      g_Spu.m_NextSoundOverrideFlags = 1;
                      g_Spu.m_VolumeOverride.right = 0x2000;
                      g_Spu.m_VolumeOverride.left = 0x2000;
                      func_8003851C(g_Sparx, 0, 0);
                    }

                    g_Sparx->m_Substate = 4;
                    sparxProps->m_MobyPickingUp = moby;
                  }
                }
              }
            }
          }
        }

        break;
      }
      case 2: {
        int speed;

        VecMult(&collectableMovePos, &collectableProps->m_VelocityOrPickupPos, g_DeltaTime);
        VecShiftRight(&collectableMovePos, 1);

        speed = VecMagnitude(&collectableMovePos, 1);

        if (220 < speed) {
          VecScaleToLength(&collectableMovePos, speed, 220);
        }

        VecAdd(&collectableMovePos, &moby->m_Position, &collectableMovePos);

        // Apply gravity
        if (-220 < collectableProps->m_VelocityOrPickupPos.z) {
          collectableProps->m_VelocityOrPickupPos.z -= g_DeltaTime * 5;
        }

        if (collectableMovePos.z < 0) {
          // Fell off the world
          // If it's not a life statue or orb, collect it
          if (moby->m_Class != 14 && moby->m_Class != 15) {
            CollectItem(moby);
          }
          func_80052568(moby);
          // Easiest way to exit out
          continue;
        } else {
          collectableMovePos.z += 240;

          if (func_8004BE4C(&collectableMovePos, 240, 240)) {
            // Checks if the last collision was water
            if (func_80057380() == 0) {
              // If it's not a life statue or orb, collect it
              if (moby->m_Class != 14 && moby->m_Class != 15) {
                CollectItem(moby);
              }
              func_80052568(moby);
              // Easiest way to exit out
              continue;
            }

            VecCopy(&collectableSurfaceNormal, &g_CollisionNormal);
            VecCopy(&moby->m_Position, &g_CollisionPoint);
            moby->m_Position.z -= 0xF0;

            if (collectableProps->m_BounceCount == 0) {
              int groundHeight = func_8004D5EC(&collectableMovePos, 0x400);
              int groundAngle = (signed char)Atan2Fast(collectableSurfaceNormal.z, VecMagnitude(&collectableSurfaceNormal, 0));
              // Settle on ground if close and flat enough
              if ((collectableMovePos.z - 400) < groundHeight && groundAngle < 24) {
                moby->m_Substate = 1;
                VecCopy(&moby->m_Position, &collectableMovePos);
                moby->m_Position.z = groundHeight;

                if (moby->m_Class != 0xF) {

                  collectableProps->m_RotX = -Atan2Fast(func_80017A38((collectableSurfaceNormal.x * collectableSurfaceNormal.x) + (collectableSurfaceNormal.z * collectableSurfaceNormal.z)), collectableSurfaceNormal.y);
                  collectableProps->m_RotY = -Atan2Fast(collectableSurfaceNormal.z, collectableSurfaceNormal.x);

                  if (collectableProps->m_RotX != 0 || collectableProps->m_RotY != 0) {
                    moby->m_Rotation.z = 0;
                  }
                }

                func_800533D0(moby);
              } else {
                collectableProps->m_BounceCount++;
              }
            }

            if (collectableProps->m_BounceCount != 0) {
              g_Spu.m_NextSoundOverrideFlags = 1;
              g_Spu.m_VolumeOverride.left = 0x3CCC >> (3 - collectableProps->m_BounceCount);
              g_Spu.m_VolumeOverride.right = 0x3CCC >> (3 - collectableProps->m_BounceCount);

              // Bounce sound? I think
              PlaySound(g_Spu.m_SoundTable->pickupDing, moby, 8, &moby->m_SoundChannel);

              if (func_80017428(&collectableProps->m_VelocityOrPickupPos, &collectableSurfaceNormal, &collectableProps->m_VelocityOrPickupPos)) {
                collectableProps->m_BounceCount--;

                // Reduce velocity each bounce with randomness
                collectableProps->m_VelocityOrPickupPos.x = (collectableProps->m_VelocityOrPickupPos.x >> 3) + (rand() & 0x3F) - 32;
                collectableProps->m_VelocityOrPickupPos.y = (collectableProps->m_VelocityOrPickupPos.y >> 3) + (rand() & 0x3F) - 32;
                collectableProps->m_VelocityOrPickupPos.z = (collectableProps->m_VelocityOrPickupPos.z >> 2) + (rand() & 0xF);
              }
            }

          } else {
            // In air
            collectableMovePos.z -= 240;
            VecCopy(&moby->m_Position, &collectableMovePos);
            func_8004D5EC(&collectableMovePos, 0x10000);
            func_800533D0(moby);
          }
        }

        // Need to be on seperate lines for some reason
        if (distanceToSpyro < 1434) {
          int zDist = (g_Spyro.m_Position.z - 356) - moby->m_Position.z;

          if (ABS2(zDist) < 512) {

            // Again.. seperate lines?
            if (g_Sparx != nullptr) {

              if (g_Spyro.m_airTime == 0 && collectableProps->m_BounceCount < 3) {

                MobySparxProps *sparxProps = g_Sparx->m_Props;

                if (sparxProps->m_MobyPickingUp == nullptr) {
                  if (!IsMobyPlayingSound(g_Sparx, g_Models[g_Sparx->m_Class]->m_Sounds[0])) {
                    g_Spu.m_NextSoundOverrideFlags = 1;
                    g_Spu.m_VolumeOverride.right = 0x2000;
                    g_Spu.m_VolumeOverride.left = 0x2000;
                    func_8003851C(g_Sparx, 0, 0);
                  }

                  g_Sparx->m_Substate = 4;
                  sparxProps->m_MobyPickingUp = moby;
                }
              }
            }
          }
        }

        break;
      }
      case 3: { // After being picked up, move towards spyro
        if (collectableProps->m_Ticks >= 32) {
          // Snap to Spyro
          VecCopy(&moby->m_Position, &g_Spyro.m_Position);
        } else {

          // Interpolate toward Spyro
          VecSub(&collectablePickupDelta, &g_Spyro.m_Position, &collectableProps->m_VelocityOrPickupPos);
          VecShiftRight(&collectablePickupDelta, 5);

          if (VecMagnitude(&collectablePickupDelta, 1) > 480) {
            // Too far, snap
            VecCopy(&moby->m_Position, &g_Spyro.m_Position);
            collectableProps->m_Ticks = 32;
          } else {
            // Move toward Spyro
            VecMult(&collectablePickupDelta, &collectablePickupDelta, collectableProps->m_Ticks);
            VecAdd(&moby->m_Position, &collectableProps->m_VelocityOrPickupPos, &collectablePickupDelta);
            moby->m_Position.z += SINE_8(collectableProps->m_Ticks * 4) / 12;
          }
        }

        // Spin rapidly while being sucked in
        moby->m_Rotation.x = moby->m_Rotation.x - 7 + collectableProps->m_RotX;
        moby->m_Rotation.y = moby->m_Rotation.y - 7 + collectableProps->m_RotY;
        moby->m_Rotation.z = moby->m_Rotation.z - 7 + collectableProps->m_RotationTicks;
        break;
      }

      case 4: {
        continue;
      }
      }
      if (moby->m_Substate < 3) {
        int zdist;
        if (moby->m_Class == 14) {
          moby->m_Rotation.x = (COSINE_8(collectableProps->m_RotationTicks) >> 9) + collectableProps->m_RotX;
          moby->m_Rotation.y = (SINE_8(collectableProps->m_RotationTicks) >> 9) + collectableProps->m_RotY;
          collectableProps->m_RotationTicks += g_DeltaTime << 1;
        } else if (moby->m_Class == 15) {
          moby->m_Rotation.z += 8;
          moby->m_Rotation.y -= 6;
        } else {
          moby->m_Rotation.x = (COSINE_8(collectableProps->m_RotationTicks) >> 7) + collectableProps->m_RotX;
          moby->m_Rotation.y = (SINE_8(collectableProps->m_RotationTicks) >> 7) + collectableProps->m_RotY;
          collectableProps->m_RotationTicks += g_DeltaTime << 1;
        }
      }
      if ((collectableProps->m_Ticks > 32 && distanceToSpyro < 512 && (-384 < (g_Spyro.m_Position.z - moby->m_Position.z)) && ((g_Spyro.m_Position.z - moby->m_Position.z) < 512)) ||
          (moby->m_Substate == 3 && collectableProps->m_Ticks > 64)) {

        if (g_Sparx != nullptr) {
          MobySparxProps *sparxProps = g_Sparx->m_Props;

          if (sparxProps->m_MobyPickingUp == moby) {
            sparxProps->m_MobyPickingUp = nullptr;
          }
        }

        // Extra life
        if (moby->m_Class == 14) {
          PlaySound(g_Spu.m_SoundTable->gemPickup, moby, 16, nullptr);
          func_800529E4(moby, 4);
          D_800758E4(16, 70, (void *)&moby->m_Position, (void *)8);
          D_800758E4(16, 70, (void *)&moby->m_Position, (void *)0x10);

          // If life count is less than 99, increment it
          if (g_SpyroLifeCount < 99) {
            g_SpyroLifeCount++;
          }

          // Mark as collected and destroy
          func_8003B854(0, moby);
          func_80052568(moby);
        } else if (moby->m_Class == 15) { // Life orb
          PlaySound(g_Spu.m_SoundTable->gemPickup, moby, 16, nullptr);
          g_LifeOrbCount++;

          func_80052568(moby);
        } else {
          CollectItem(moby);
          func_80052568(moby);
        }
      }

      break;
    }

    case 110: { // Dragon Pad Fairy
      struct {
        int m_TriggerMobyLink; // Dragon moby link
        int m_ParentMobyLink;  // Pad moby link
        int m_Timer;
        int m_BaseZ;
        Vector3D m_TargetPosition;
        int m_TargetAngle;
        int m_RetargetTimer;
        int m_WasCollected;
      } *fairyProps = moby->m_Props;

      Moby *dragonMoby = &g_LevelMobys[fairyProps->m_TriggerMobyLink];
      Moby *padMoby = &g_LevelMobys[fairyProps->m_ParentMobyLink];

      switch (moby->m_State) {
      case 0: {
        moby->m_CollisionGroup = nullptr;
        if (OctDistance(&padMoby->m_Position, &g_Spyro.m_Position) > 2560) {
          moby->m_State = 1;
        }
        break;
      }

      case 1: {
        if (dragonMoby->m_State >= 0x80u) {
          // If dragon is dead
          moby->m_State = 2;
          fairyProps->m_Timer = 16;
          fairyProps->m_BaseZ = moby->m_Position.z;
        }
        break;
      }

      case 2: {
        if (fairyProps->m_Timer != 0) {
          fairyProps->m_Timer--;
        } else {
          moby->m_CollisionGroup = nullptr;

          if (OctDistance(&moby->m_Position, &g_Spyro.m_Position) < 0x2000) {
            if (ABS2(moby->m_Position.z - moby->m_FloorDistance - g_Spyro.m_Position.z) < 2048) {
              if (fairyProps->m_WasCollected == 0 || padMoby->m_State == 1) {
                // Spawn the fairy
                fairyProps->m_WasCollected = 0;
                moby->m_Position.z = fairyProps->m_BaseZ + 0x80;
                moby->m_Rotation.z = Atan2(g_Spyro.m_Position.x - moby->m_Position.x, g_Spyro.m_Position.y - moby->m_Position.y, 0) - 128;
                moby->m_RenderRadius = 0x20;
                moby->m_State = 3;
                moby->m_ScaleOverride = 0x60;

                // Spawn particle effect
                D_800758E4(0x18, 0x50, (void *)&moby->m_Position, (void *)0xC);

                func_800529E4(moby, 1);
                func_8004D5EC(&moby->m_Position, 0x1000);
                func_800533D0(moby);
                moby->m_ShadowDistance |= 0x80000000;
              }
            }
          }
        }
        break;
      }

      case 3: {
        if (moby->m_ScaleOverride >= 0x21) {
          moby->m_ScaleOverride -= 4;                  // Shrink
          moby->m_Rotation.z = moby->m_Rotation.z + 8; // Spin
        } else {
          moby->m_State = 4;
          fairyProps->m_Timer = 0;
          fairyProps->m_RetargetTimer = 30;
        }
        break;
      }

      case 4: { // Idle hovering
          int temp;
        fairyProps->m_Timer++;
        moby->m_Position.z = fairyProps->m_BaseZ + (Cos(fairyProps->m_Timer << 7) >> 6);

        // Check if pad is near Spyro and Spyro is idle
        if ((padMoby->m_State == 0 || (padMoby->m_State == 2 && padMoby->m_Substate >= 0x10)) && OctDistance(&padMoby->m_Position, &g_Spyro.m_Position) < 0x400 &&
            ABS2(padMoby->m_Position.z - padMoby->m_FloorDistance - g_Spyro.m_Position.z) < 0x200) {

          // Spyro must be idle or standing still
          if ((g_Spyro.m_State == 0 || g_Spyro.m_State == 0xD) && g_Spyro.m_idleTimer > 0) {
            moby->m_State = 6;
            fairyProps->m_WasCollected = 1;
            InitFairyCutscene(moby);
            break;
          }
        }

        // Check if should retarget or move away
        if (OctDistance(&moby->m_Position, &g_Spyro.m_Position) > 0x2400 || (ABS2(moby->m_Position.z - moby->m_FloorDistance - g_Spyro.m_Position.z) > 0xC00)) {
          // Too far from Spyro, despawn
          moby->m_State = 6;
        } else {
          fairyProps->m_RetargetTimer--;
          if (fairyProps->m_RetargetTimer == 0) {
            // Calculate new target position away from Spyro
            int baseDistance = (rand() & 0x3F) + 0x140; // 320-383
            char finalAngle = (Atan2(g_Spyro.m_Position.x - padMoby->m_Position.x,
               g_Spyro.m_Position.y - padMoby->m_Position.y, 0) + (rand() & 0xF) - 8);

            VecCopy(&fairyProps->m_TargetPosition, &padMoby->m_Position);

            moby->m_State = 5;

            // Calculate offset from parent
            fairyProps->m_TargetPosition.x -= (Cos(finalAngle * 0x10) * baseDistance) >> 12;
            fairyProps->m_TargetPosition.y -= (Sin(finalAngle * 0x10) * baseDistance) >> 12;

            fairyProps->m_TargetAngle = Atan2(g_Spyro.m_Position.x - fairyProps->m_TargetPosition.x, g_Spyro.m_Position.y - fairyProps->m_TargetPosition.y, 0);
            fairyProps->m_RetargetTimer = 0;
          }
        }
        break;
      }
      case 5: {
        fairyProps->m_Timer++;
        moby->m_Position.z = fairyProps->m_BaseZ + (Cos(fairyProps->m_Timer << 7) >> 6);

        if ((padMoby->m_State == 0 || (padMoby->m_State == 2 && padMoby->m_Substate >= 0x10)) && OctDistance(&padMoby->m_Position, &g_Spyro.m_Position) < 0x400 &&
            ABS2(padMoby->m_Position.z - padMoby->m_FloorDistance - g_Spyro.m_Position.z) < 512) {

          if ((g_Spyro.m_State == 0 || g_Spyro.m_State == 0xD) && g_Spyro.m_idleTimer > 0) {
            moby->m_State = 6;
            fairyProps->m_WasCollected = 1;
            InitFairyCutscene(moby);
            break;
          }
        }

        // Continue moving toward target
        if (OctDistance(&moby->m_Position, &g_Spyro.m_Position) > 0x2800 || ABS2(moby->m_Position.z - moby->m_FloorDistance - g_Spyro.m_Position.z) >= 0xC01) {
          moby->m_State = 6;
        } else {
          int angleDiff;
          int cosWeight1;
          int cosDelta;
          int totalWeight;

          fairyProps->m_RetargetTimer++;

          VecSub(&vec_88, &fairyProps->m_TargetPosition, &moby->m_Position);

          angleDiff = func_80017948(fairyProps->m_TargetAngle, moby->m_Rotation.z);

          // Smooth interpolation using cosine
          cosWeight1 = Cos((fairyProps->m_RetargetTimer - 1) << 7);
          cosDelta = cosWeight1 - Cos(fairyProps->m_RetargetTimer << 7);
          totalWeight = Cos((fairyProps->m_RetargetTimer - 1) << 7) + 0x1000;

          // Move toward target with smooth acceleration
          moby->m_Position.x += (vec_88.x * cosDelta) / totalWeight;
          moby->m_Position.y += ((vec_88.y * cosDelta) / totalWeight);
          moby->m_Rotation.z += (angleDiff * cosDelta) / totalWeight;

          func_8004D5EC(&moby->m_Position, 0x1000);
          func_800533D0(moby);

          // Reached target
          if (fairyProps->m_RetargetTimer == 0x10) {
            moby->m_State = 4;
            fairyProps->m_RetargetTimer = (rand() & 0x1F) + 0x1E; // 30-61
          }
        }

        break;
      }

      case 6: {
        if (moby->m_ScaleOverride < 96) {
          moby->m_ScaleOverride += 4;
          moby->m_Rotation.z = moby->m_Rotation.z + 8;
        } else {
          // Fully shrunk
          moby->m_RenderRadius = 0;
          moby->m_WasDrawn = 0;
          moby->m_State = 2;
          fairyProps->m_Timer = 16;

          // Spawn particle effect
          D_800758E4(0x18, 0x50, (void *)&moby->m_Position, (void *)0xC);

          moby->m_ShadowDistance &= 0x7FFFFFFF;
        }
        break;
      }
      }

      break;
    }
    case 120: { /* MOBYCLASS_SPARX */
      MobySparxProps *sparxProps = moby->m_Props;
      char pitchAngle;
      /* Spawn glow if Spyro has 2+ health, despawn it below */
      if (g_Spyro.m_health >= 2) {

        vec_88.x = 0;
        vec_88.y = 0x64;
        vec_88.z = 0;
        VecRotateByMatrix((MATRIX *)&moby->m_RotationMatrix, &vec_88, &vec_88);
        VecAdd(&vec_88, &vec_88, &moby->m_Position);
        D_800758E4(2, 0x42, &vec_88, nullptr);

        vec_88.x = 0;
        vec_88.y = -0x64;
        vec_88.z = 0;
        func_800170C0(&vec_88, &vec_88);
        VecAdd(&vec_88, &vec_88, &moby->m_Position);
        D_800758E4(2, 0x42, &vec_88, nullptr);
      }

      /* Glow management for full health */
      if (g_Spyro.m_health >= 3) {
        if (sparxProps->glow == nullptr) {
          Glow *g = func_80058AE8();
          sparxProps->glow = g;
          if (g != nullptr) {
            g->MobyPos = &moby->m_Position;
            sparxProps->glow->m_Radius = 0x40;
            sparxProps->glow->PosOffset.x = 0;
            sparxProps->glow->PosOffset.y = 0;
            sparxProps->glow->PosOffset.z = 0;
            sparxProps->glow->GlowColor.r = 0xC0;
            sparxProps->glow->GlowColor.g = 0xC0;
            sparxProps->glow->GlowColor.b = 0x60;
            sparxProps->glow->unk_0x00 = 9;
            sparxProps->glow->m_VertexTable = (int *)&D_8006E390;
          }
        } else {
          sparxProps->glow->m_Radius += 0x20;
          if (sparxProps->glow->m_Radius > 0x400) {
            sparxProps->glow->m_Radius = 0x400;
          }
        }
      } else {
        if (sparxProps->glow != nullptr) {
          func_80058B60(sparxProps->glow);
          sparxProps->glow = nullptr;
        }
      }

      /* If Spyro is dead, kill Sparx */
      if (g_Spyro.m_health <= 0) {
        Moby *target = sparxProps->m_MobyPickingUp;
        if (target != nullptr && target->m_Class == 16) {
          target->m_State = 0;
        }
        g_Sparx = nullptr;
        func_80052568(moby);
        break;
      }

      /* Detach if too far from Spyro */
      if (sparxProps->m_MobyPickingUp != nullptr && OctDistance(&moby->m_Position, &g_Spyro.m_Position) > 8192) {
        sparxProps->m_Timer = 0;
      }

      if (moby->m_Substate == 99)
        break;

      if (sparxProps->m_MobyPickingUp != nullptr) {
        switch (moby->m_Substate) {
        case 0: { /* Approach the moby being picked up */
          Moby *target = sparxProps->m_MobyPickingUp;
          MobyButterflyProps *bp = target->m_Props;
          int isPickupAnimPhase = moby->m_AnimationState.m_NextAnimation & 1;
          int dist;
          int targetSpeed;

          VecCopy(&vec_38, &target->m_Position);
          if (isPickupAnimPhase == 0) {
            vec_38.x -= COSINE_8(sparxProps->m_MobyPickingUp->m_Rotation.z) >> 4;
            vec_38.y -= SINE_8(sparxProps->m_MobyPickingUp->m_Rotation.z) >> 4;
          } else {
            vec_38.x -= COSINE_8(sparxProps->m_MobyPickingUp->m_Rotation.z) >> 6;
            vec_38.y -= SINE_8(sparxProps->m_MobyPickingUp->m_Rotation.z) >> 6;
          }

          /* Adjust pitch when on second-half of animation */
          if (isPickupAnimPhase) {
            int delta;
            if (moby->m_AnimationState.m_NextFrame >= 7) {
              moby->m_Substate++;
            }

            if (2 < moby->m_Rotation.y && moby->m_Rotation.y < 254) {
              delta = -moby->m_Rotation.y;
              if (delta > 2) {
                delta = 2;
              }
              if (delta < -2) {
                delta = -2;
              }
              moby->m_Rotation.y = moby->m_Rotation.y + delta;
            }
          }

          VecSub(&vec_38, &vec_38, &moby->m_Position);
          dist = VecMagnitude(&vec_38, 1);

          /* Check for trigger to kick into pickup animation */
          if (isPickupAnimPhase == 0) {
            if (TimerTick(sparxProps, 4) != 0 ||
                (sparxProps->m_Timer < 80 && func_80017908((moby->m_Rotation.z + (bp->m_SparxApproachSide * 110)) & 0xFF, ((g_Camera.m_Rotation.z >> 4) + 0x80) & 0xFF) < 8)) {
              if (moby->m_AnimationState.m_NextAnimation != moby->m_AnimationState.m_NextAnimation + 1) {
                D_80075794 = 0;
                moby->m_AnimationState.m_FrameProgress = 0x10;
                moby->m_AnimationState.m_PerFrameProgress = 0x10;
                moby->m_AnimationState.m_Animation = moby->m_AnimationState.m_NextAnimation;
                moby->m_AnimationState.m_NextAnimation = moby->m_AnimationState.m_NextAnimation + 1;
                moby->m_AnimationState.m_Frame = moby->m_AnimationState.m_NextFrame;
                moby->m_AnimationState.m_NextFrame = 0;
                func_80037E98(moby);
              }
            }
          }
          targetSpeed = isPickupAnimPhase ? 150 : 130;
          if (targetSpeed + 5 < dist) {
            VecScaleToLength(&vec_38, dist, targetSpeed + 5);
          } else {
            VecScaleToLength(&vec_38, dist, targetSpeed - 5);
          }
          VecAdd(&moby->m_Position, &moby->m_Position, &vec_38);
          RotateMobyToAngle(moby, Atan2(vec_38.x, vec_38.y, 0), 0xA, 0, 0);
          pitchAngle = Atan2(VecMagnitude(&vec_38, 0), vec_38.z, 0);
          moby->m_Rotation.y = pitchAngle;
          moby->m_Rotation.y = func_80038098(pitchAngle, 0, 0x30);
          break;
        }
        case 1: { /* Picked it up - heal/grant */
          g_Spyro.m_health++;
          PlaySound(g_Spu.m_SoundTable->gemPickup, moby, 0x10, nullptr);
          moby->m_Substate++;
          func_80052568(sparxProps->m_MobyPickingUp);
          break;
        }
        case 2: { /* Wait for animation completion */
          if (D_80075794 != 0) {
            if (moby->m_AnimationState.m_NextAnimation != moby->m_AnimationState.m_NextAnimation - 1) {
              D_80075794 = 0;
              moby->m_AnimationState.m_FrameProgress = 0x10;
              moby->m_AnimationState.m_PerFrameProgress = 0x10;
              moby->m_AnimationState.m_Animation = moby->m_AnimationState.m_NextAnimation;
              moby->m_AnimationState.m_NextAnimation = moby->m_AnimationState.m_NextAnimation - 1;
              moby->m_AnimationState.m_Frame = moby->m_AnimationState.m_NextFrame;
              moby->m_AnimationState.m_NextFrame = 0;
              func_80037E98(moby);
            }
            sparxProps->m_MobyPickingUp = nullptr;
            moby->m_Substate = 0;
          }
          break;
        }
        case 4: { /* Chasing a gem to gather it */
          MobyCollectableProps *cp = sparxProps->m_MobyPickingUp->m_Props;
          int dist;

          VecSub(&sparxChaseDelta, &sparxProps->m_MobyPickingUp->m_Position, &moby->m_Position);
          dist = VecMagnitude(&sparxChaseDelta, 1);

          if (dist < 512) {
            cp->m_Ticks = 0;
            cp->m_RotX = rand() & 0xE;
            cp->m_RotY = rand() & 0xE;
            cp->m_RotationTicks = rand() & 0xE;

            VecCopy(&cp->m_VelocityOrPickupPos, &sparxProps->m_MobyPickingUp->m_Position);
            sparxProps->m_MobyPickingUp->m_Substate = 3;
            /* select a particle data block based on class */

            if (sparxProps->m_MobyPickingUp->m_Class == 14)
              D_800758E4(1, 0xC, moby, (void *)D_8006E494);
            else if (sparxProps->m_MobyPickingUp->m_Class == 15)
              D_800758E4(1, 0xC, moby, (void *)D_8006E490);
            else
              D_800758E4(1, 0xC, moby, (void *)*(&D_8006E330 + sparxProps->m_MobyPickingUp->m_Class));

            sparxProps->m_MobyPickingUp = nullptr;
            if (!IsMobyPlayingSound(moby, g_Spu.m_SoundTable->pickupDing)) {
              g_Spu.m_NextSoundOverrideFlags = 1;
              g_Spu.m_VolumeOverride.right = 0x3600;
              g_Spu.m_VolumeOverride.left = 0x3600;
              PlaySound(g_Spu.m_SoundTable->pickupDing, moby, 8, &moby->m_SoundChannel);
            }
          } else {
            //i don't really get this, we already know dist >= 512 here...maybe some sort of inline
            int dcheck = 310;

            if (dcheck < dist)
              VecScaleToLength(&sparxChaseDelta, dist, 310);
            else
              VecScaleToLength(&sparxChaseDelta, dist, 290);

            VecAdd(&moby->m_Position, &moby->m_Position, &sparxChaseDelta);
            RotateMobyToAngle(moby, Atan2(sparxChaseDelta.x, sparxChaseDelta.y, 0), 0xA, 0, 0);
            pitchAngle = Atan2(VecMagnitude(&sparxChaseDelta, 0), sparxChaseDelta.z, 0);
            moby->m_Rotation.y = pitchAngle;
            moby->m_Rotation.y = func_80038098(pitchAngle, 0, 0x30);
          }
          break;
        }
        }
      } else {

        /* No moby being picked up - idle hover near Spyro */
        moby->m_RenderRadius = 0x10;
        if (sparxProps->m_Timer <= 0) {
          /* Pick a new offset target */
          vec_88.x = -0x258;
          vec_88.y = (rand() & 0x310) - 0x188;
          if (g_Camera.m_State == 3) {
            vec_88.z = (rand() & 0x7F) + 0x64;
          } else {
            vec_88.z = (rand() & 0x1FF) - 0xC8;
          }
          sparxProps->m_IdleOffset.x = vec_88.x;
          sparxProps->m_IdleOffset.y = vec_88.y;
          sparxProps->m_IdleOffset.z = vec_88.z;
          sparxProps->m_Timer = rand() & 0x7B;
        } else {
          sparxProps->m_Timer -= g_DeltaTime;
          if (g_Camera.m_State == 3) {
            vec_88.x = sparxProps->m_IdleOffset.x + (g_Spyro.m_Physics.m_SpeedAngle.m_Speed >> 2);
            vec_88.y = sparxProps->m_IdleOffset.y;
            vec_88.z = sparxProps->m_IdleOffset.z;
            if (vec_88.z < 0x64)
              vec_88.z = 0x64;
          } else {
            if (g_Camera.m_State == 0x80000009)
              vec_88.x = sparxProps->m_IdleOffset.x - 512;
            else {
              vec_88.x = sparxProps->m_IdleOffset.x;
            }
            vec_88.y = sparxProps->m_IdleOffset.y;
            vec_88.z = sparxProps->m_IdleOffset.z;
          }
          VecRotateByMatrix(&g_Spyro.m_RotationMatrix, &vec_88, &vec_88);
          VecAdd(&vec_88, &vec_88, &g_Spyro.m_Position);
          VecSub(&vec_88, &vec_88, &moby->m_Position);
          VecShiftRight(&vec_88, 2);
          VecAdd(&moby->m_Position, &moby->m_Position, &vec_88);

          if (func_8004BE4C(&moby->m_Position, 0x100, 0x100)) {
            VecCopy(&moby->m_Position, &g_CollisionPoint);
          }

          if (vec_88.z > 32)
            vec_88.z = 32;
          if (vec_88.z < -32)
            vec_88.z = -32;
          moby->m_Rotation.x = 0;
          moby->m_Rotation.y = vec_88.z;
          RotateMobyToAngle(moby, g_Spyro.m_bodyRotation.z, 4, 0, 0);
        }

        /* Pick animation based on health */
        switch (g_Spyro.m_health) {
        case 1:
          if (moby->m_AnimationState.m_NextAnimation != 0) {
            if (moby->m_AnimationState.m_Animation != 0) {
              D_80075794 = 0;
              moby->m_AnimationState.m_FrameProgress = 0;
              moby->m_AnimationState.m_PerFrameProgress = g_Models[moby->m_Class]->m_Animations[0]->m_ProgressPerTick;
              moby->m_AnimationState.m_Animation = 0;
              moby->m_AnimationState.m_NextAnimation = 0;
              moby->m_AnimationState.m_Frame = 0;
              moby->m_AnimationState.m_NextFrame = 1;
            }
          }
          break;
        case 2:
          if (moby->m_AnimationState.m_NextAnimation != 2) {
            if (moby->m_AnimationState.m_Animation != 2) {
              D_80075794 = 0;
              moby->m_AnimationState.m_FrameProgress = 0;
              moby->m_AnimationState.m_PerFrameProgress = g_Models[moby->m_Class]->m_Animations[2]->m_ProgressPerTick;
              moby->m_AnimationState.m_Animation = 2;
              moby->m_AnimationState.m_NextAnimation = 2;
              moby->m_AnimationState.m_Frame = 0;
              moby->m_AnimationState.m_NextFrame = 1;
            }
          }

          break;
        case 3:
          if (moby->m_AnimationState.m_NextAnimation != 4) {
            if (moby->m_AnimationState.m_Animation != 4) {
              D_80075794 = 0;
              moby->m_AnimationState.m_FrameProgress = 0;
              moby->m_AnimationState.m_PerFrameProgress = g_Models[moby->m_Class]->m_Animations[4]->m_ProgressPerTick;
              moby->m_AnimationState.m_Animation = 4;
              moby->m_AnimationState.m_NextAnimation = 4;
              moby->m_AnimationState.m_Frame = 0;
              moby->m_AnimationState.m_NextFrame = 1;
            }
          }
          break;
        }
      }
      func_800529E4(moby, 4);
      break;
    }
    case 173: { /* MOBYCLASS_KEY */
      struct {
        short m_SparkleTimer;      /* 0x00 */
        u_char m_RotationTick;      /* 0x02 - rotation tick (8-bit) */
        u_char m_SparkleHandle; /* 0x03 */
      } *keyProps = moby->m_Props;

      if (moby->m_Substate == 0) {
        moby->m_Rotation.x = COSINE_8(keyProps->m_RotationTick) >> 7;
        moby->m_Rotation.y = SINE_8(keyProps->m_RotationTick) >> 7;
        keyProps->m_RotationTick += g_DeltaTime * 2;

        if (OctDistance(&moby->m_Position, &g_Spyro.m_Position) < 512) {
          if (ABS2(moby->m_Position.z - moby->m_FloorDistance - g_Spyro.m_Position.z) < 512) {
            /* Pickup! */
            int i;
            PlaySound(g_Spu.m_SoundTable->gemPickup, moby, 0x10, nullptr);
            func_800529E4(moby, 4);
            for (i = 0; i < 6; i++) {
              D_800758E4(1, 0xC, moby, (void *)0x8080 + 0x01000000 * i);
            }
            g_KeyFlag = 1;
            moby->m_ScaleOverride = 0x40;
            moby->m_RenderRadius = 0;
            moby->m_UpdateDistance = 0;
            moby->m_WasDrawn = 0;
            moby->m_ShadowDistance = 0;
            moby->m_SectorIndex = 0xFF;
            moby->m_Substate = 2;
          }
        }

        /* Sparkles, similar to gems */
        if (keyProps->m_SparkleHandle != 0xFF) {
          func_800529E4(moby, 4);
          VecRotateByMatrix((MATRIX *)&moby->m_RotationMatrix, &D_8006E5AC, &g_Sparkles[keyProps->m_SparkleHandle].m_Position);
          VecAdd(&g_Sparkles[keyProps->m_SparkleHandle].m_Position, &g_Sparkles[keyProps->m_SparkleHandle].m_Position, &moby->m_Position);
          if (g_Sparkles[keyProps->m_SparkleHandle].m_Life < 5) {
            keyProps->m_SparkleHandle = 0xFF;
          }
        } else if (keyProps->m_SparkleTimer >= 248) {
          int handle = SpawnMobySparkle(moby, &D_8006E5AC);
          if (handle >= 0 && OctDistance(&moby->m_Position, &g_Spyro.m_Position) < 0x4000) {
            keyProps->m_SparkleHandle = handle;
          }
          keyProps->m_SparkleTimer = (rand() & 0x3F) + 24;
        }
        keyProps->m_SparkleTimer += g_DeltaTime;
      }
      break;
    }
    case 192: { /* MOBYCLASS_BALLOONIST */
      if (moby->m_State == 0) {
        if (OctDistance(&moby->m_Position, &g_Spyro.m_Position) >= 4096) {
          moby->m_State = 1;
        }
      } else if ((g_Spyro.m_State == 0 || g_Spyro.m_State == 1 || g_Spyro.m_State == 21  || g_Spyro.m_State == 2 || g_Spyro.m_State == 11) &&
                 OctDistance(&moby->m_Position, &g_Spyro.m_Position) < 0x780 &&
                 func_80017908(g_Spyro.m_bodyRotation.z, Atan2(moby->m_Position.x - g_Spyro.m_Position.x, moby->m_Position.y - g_Spyro.m_Position.y, 0)) < 0x20 &&
                 func_80017908(moby->m_Rotation.z, Atan2(g_Spyro.m_Position.x - moby->m_Position.x, g_Spyro.m_Position.y - moby->m_Position.y, 0)) < 0x28) {
        moby->m_State = 0;
        g_Spyro.m_ControlFlags = 0x80002000;
        func_8003DFA4();
        VecNull(&g_Spyro.m_HeadLookTarget);
        D_800757A0(moby);
        D_800777E8.m_DialogueId = 0x1E; /* TODO: actual struct member */
        D_800758D0[1] = 2;
        D_800758D0[2] = 2;
        D_800758D0[3] = 2;
        D_800758D0[4] = 2;
        D_800758D0[5] = 2;
      }
      break;
    }
    case 194: { /* MOBYCLASS_WOODEN_CHEST */
      struct {
        int m_unk_0x00;
        int m_PlaceOnFloor;
      } *chestProps = moby->m_Props;
      if (chestProps->m_PlaceOnFloor != 0) {
        func_80038458(moby);
      }

      if (moby->m_DamageFlags & 0xB0000) {
        int i;
        moby->m_SoundDistance = 0x20;
        func_8003851C(moby, 0, 0);

        /* Spawn fragments (limited by free dyn moby slots) */
        for (i = 0; i < 2; i++) {
          if (g_DynMobyMax - g_DynMobyCount < 0x15)
            break;
          g_SpawnMoby(0xFF, moby);
          g_SpawnMoby(0x100, moby);
        }
        for (i = 0; i < 6; i++) {
          if (g_DynMobyMax - g_DynMobyCount < 0x15)
            break;
          g_SpawnMoby(0x101, moby);
        }

        D_800758E4(5, 2, &moby->m_Position, nullptr);
        D_800758E4(0x10, 0x46, &moby->m_Position, (void *)0x20);
        func_8003ABC0(moby, 3, 0, 0);
        func_8003B7C0(moby);
        func_80052568(moby);
      }
      break;
    }
    case 195: { /* MOBYCLASS_METAL_CHEST */
      struct {
        int m_ShakeTimer;   /* 0x00 - flash/recovery timer */
        int m_StoredRotX; /* 0x04 */
        int m_StoredRotY; /* 0x08 */
        int m_StoredPosZ; /* 0x0C */
        int m_PlaceOnFloor;   /* 0x10 - check for func_80038458 trigger */
        int m_FlameTimer; /* 0x14 - flame heat tracker */
        int m_unk_0x18;
      } *chestProps = moby->m_Props;

      if (chestProps->m_PlaceOnFloor != 0) {
        func_80038458(moby);
      }

      if (chestProps->m_ShakeTimer != 0) {
        chestProps->m_ShakeTimer += g_DeltaTime;
        if (chestProps->m_ShakeTimer < 0x40) {
          /* shake animation */
          moby->m_Rotation.x = D_8006E638[chestProps->m_ShakeTimer >> 1][0] + chestProps->m_StoredRotX;
          moby->m_Rotation.y = D_8006E638[chestProps->m_ShakeTimer >> 1][1] + chestProps->m_StoredRotY;
          moby->m_Position.z = chestProps->m_StoredPosZ + (ABS2(D_8006E638[chestProps->m_ShakeTimer >> 1][0]) + ABS2(D_8006E638[chestProps->m_ShakeTimer >> 1][1])) * 6;
        } else {
          chestProps->m_ShakeTimer = 0;
          moby->m_Rotation.x = chestProps->m_StoredRotX;
          moby->m_Rotation.y = chestProps->m_StoredRotY;
          moby->m_Position.z = chestProps->m_StoredPosZ;
        }
      }

      if (moby->m_DamageFlags & (0x20000 | 0x80000)) {
        int i;
        moby->m_SoundDistance = 0x20;
        func_8003851C(moby, 0, 0);

        for (i = 0; i < 2; i++) {
          if (g_DynMobyMax - g_DynMobyCount < 21)
            break;
          g_SpawnMoby(0x136, moby);
          g_SpawnMoby(0x137, moby);
        }
        for (i = 0; i < 6; i++) {
          if (g_DynMobyMax - g_DynMobyCount < 21)
            break;
          g_SpawnMoby(0x135, moby);
        }
        D_800758E4(0x10, 0x46, &moby->m_Position, (void *)0x18);
        func_8003ABC0(moby, 3, 0, 0);
        func_8003B7C0(moby);
        func_80052568(moby);
      } else {
        chestProps->m_FlameTimer = ApplyFlameHeatExternal(moby, chestProps->m_FlameTimer);
        if ((moby->m_DamageFlags & 0x10000) && chestProps->m_ShakeTimer == 0) {
          chestProps->m_ShakeTimer = 1;
          chestProps->m_StoredRotX = moby->m_Rotation.x;
          chestProps->m_StoredRotY = moby->m_Rotation.y;
          chestProps->m_StoredPosZ = moby->m_Position.z;
        }
      }
      moby->m_DamageFlags = 0;

      break;
    }
    case 208: { /* Gnexus Dragon Head Unlocks */
      struct {
        int m_Initialized; /* 0x00 - one-time init flag */
      } *doorProps = moby->m_Props;
      if (doorProps->m_Initialized == 0) {
        /* Sync portal-locked flags from visited flags on first run */
        if (g_VisitedFlags[32] != 0) {
          func_8002B390(0, 0xFC, 0);
          if (g_Spyro.m_State == 0xF && g_Spyro.m_walkingState == 9)
            func_8002B444(0, 0x3B, 0);
        }
        if (g_VisitedFlags[33] != 0) {
          func_8002B390(1, 0xFC, 0);
          if (g_Spyro.m_State == 0xF && g_Spyro.m_walkingState == 9)
            func_8002B444(1, 0x3B, 0);
        }
        if (g_VisitedFlags[34] != 0) {
          func_8002B390(2, 0xFC, 0);
          if (g_Spyro.m_State == 0xF && g_Spyro.m_walkingState == 9)
            func_8002B444(2, 0x3B, 0);
        }
        doorProps->m_Initialized = 1;
      }

      if (((func_8002B3F4(0) >> 8) & 0xFF) == 0 && g_LevelVortexExitFlags[31] != 0 && g_LevelDragonCount[30] > 0) {
        func_8002B390(0, 0xFC, 0);
      }
      if (((func_8002B3F4(1) >> 8) & 0xFF) == 0 && g_LevelVortexExitFlags[32] != 0) {
        func_8002B390(1, 0xFC, 0);
      }
      if (((func_8002B3F4(2) >> 8) & 0xFF) == 0 && g_GemTotal >= 12000 && g_DragonTotal >= 80 && g_EggTotal >= 12) {
        func_8002B390(2, 0xFC, 0);
      }
      break;
    }
    case 236: { /* MOBYCLASS_FODDER_RAT */
      struct {
        int m_unk_0x00;
        int m_unk_0x04;
        int m_unk_0x08;
        int m_unk_0x0C;
        int m_unk_0x10;
        int m_unk_0x14;
        int m_unk_0x18;
        int m_unk_0x1C;
        int m_KnockbackSpeed;    /* 0x20 - countdown / power level */
        int m_TargetAngle; /* 0x24 - aim angle */
        int m_unk_0x28;
        int m_RetargetTimer; /* 0x2C - retarget delay */
      } *enemyProps = moby->m_Props;

      if ((moby->m_DamageFlags & (0x10000 | 0x20000 | 0x80000)) && moby->m_State != 1) {
        enemyProps->m_TargetAngle =
            func_80038178(Atan2(moby->m_Position.x - g_Spyro.m_Position.x, moby->m_Position.y - g_Spyro.m_Position.y, 0), g_Spyro.m_bodyRotation.z, 0x20, 0x40);
        if (moby->m_DamageFlags & 0x10000) {
          enemyProps->m_KnockbackSpeed  = 200;
        } else {
          enemyProps->m_KnockbackSpeed  = 400;
        }
        moby->m_DamageFlags = 0;
        func_8003ABC0(moby, 3, 0, 0);
        func_8003B7C0(moby);
        moby->m_State = 1;
        if (moby->m_AnimationState.m_NextAnimation != 1) {
          moby->m_AnimationState.m_FrameProgress = 0x10;
          moby->m_AnimationState.m_PerFrameProgress = 0x10;
          moby->m_AnimationState.m_Animation = moby->m_AnimationState.m_NextAnimation;
          moby->m_AnimationState.m_NextAnimation = 1;
          moby->m_AnimationState.m_Frame = moby->m_AnimationState.m_NextFrame;
          moby->m_AnimationState.m_NextFrame = 0;
          func_80037E98(moby);
        }
      } else {
        switch (moby->m_State) {
        case 0:
          func_80039AA8(moby, enemyProps);
          if (TimerTick(&enemyProps->m_RetargetTimer, 4) != 0) {
            moby->m_State = 2;
            if (moby->m_AnimationState.m_NextAnimation != 2) {
              moby->m_AnimationState.m_FrameProgress = 0x10;
              moby->m_AnimationState.m_PerFrameProgress = 0x10;
              moby->m_AnimationState.m_Animation = moby->m_AnimationState.m_NextAnimation;
              moby->m_AnimationState.m_NextAnimation = 2;
              moby->m_AnimationState.m_Frame = moby->m_AnimationState.m_NextFrame;
              moby->m_AnimationState.m_NextFrame = 0;
              func_80037E98(moby);
            }
          } else {
            func_800529E4(moby, 1);
          }
          break;
        case 1:
          if (enemyProps->m_KnockbackSpeed >= 16) {
            enemyProps->m_KnockbackSpeed -= 15;
            func_80039688(moby, enemyProps->m_TargetAngle, enemyProps->m_KnockbackSpeed, 0, 0x1F4, 5);
          }
          if (D_80075794 != 0) {
            func_800529E4(moby, 4);
            func_800385BC(moby, 0x10);
            func_80052568(moby);
          } else {
            func_800529E4(moby, 1);
          }
          break;
        case 2:
          if (D_80075794 != 0) {
            enemyProps->m_RetargetTimer = RandRange(90, 300);
            moby->m_State = 0;
            if (moby->m_AnimationState.m_NextAnimation != 0) {
              moby->m_AnimationState.m_FrameProgress = 0x10;
              moby->m_AnimationState.m_PerFrameProgress = 0x10;
              moby->m_AnimationState.m_Animation = moby->m_AnimationState.m_NextAnimation;
              moby->m_AnimationState.m_NextAnimation = 0;
              moby->m_AnimationState.m_Frame = moby->m_AnimationState.m_NextFrame;
              moby->m_AnimationState.m_NextFrame = 0;
              func_80037E98(moby);
            }
          } else {
            func_800529E4(moby, 1);
          }
          break;
        default:
          func_800529E4(moby, 1);
          break;
        }
      }
      break;
    }
    case 250: { /* MOBYCLASS_CRYSTAL_DRAGON */
      RescuedDragonMobyProps *dragonProps = moby->m_Props;

      if (moby->m_State == 0) {
        /* Init: link the pad and store rest pose */
        int padIdx = dragonProps->m_DragonPadLink;
        if (padIdx != -1) {
          g_LevelMobys[padIdx].m_State = 3;
        }
        moby->m_State = 1;
        dragonProps->m_AngleStorage.x = moby->m_Rotation.x;
        dragonProps->m_AngleStorage.y = moby->m_Rotation.y;
        dragonProps->m_AngleStorage.z = moby->m_Position.z;
      } else if (moby->m_State == 1) {
        dragonProps->m_ShakeTimer += g_DeltaTime;
        if (dragonProps->m_ShakeTimer > 256) {
          /* Recovery from hit */
          dragonProps->m_ShakeTimer = 0;
          moby->m_Rotation.x = dragonProps->m_AngleStorage.x;
          moby->m_Rotation.y = dragonProps->m_AngleStorage.y;
          moby->m_Position.z = dragonProps->m_AngleStorage.z;
          moby->m_DamageFlags = 0;
          func_800562A4(moby, 1);
        } else if (dragonProps->m_ShakeTimer >= 192) {

          /* Active rumble/break animation */
          if (!IsMobyPlayingSound(moby, g_Models[moby->m_Class]->m_Sounds[0])) {
            func_8003851C(moby, 0, 0);
          }
          moby->m_Rotation.x = D_8006E638[(dragonProps->m_ShakeTimer - 192) >> 1][0] + dragonProps->m_AngleStorage.x;
          moby->m_Rotation.y = D_8006E638[(dragonProps->m_ShakeTimer - 192) >> 1][1] + dragonProps->m_AngleStorage.y;
          moby->m_Position.z =
              dragonProps->m_AngleStorage.z + (ABS2(D_8006E638[(dragonProps->m_ShakeTimer - 192) >> 1][0]) + ABS2(D_8006E638[(dragonProps->m_ShakeTimer - 192) >> 1][1])) * 6;
        } else if (dragonProps->m_ShakeTimer >= 188) {
          dragonProps->m_AngleStorage.x = moby->m_Rotation.x;
          dragonProps->m_AngleStorage.y = moby->m_Rotation.y;
          dragonProps->m_AngleStorage.z = moby->m_Position.z;
        } else if (moby->m_DamageFlags != 0) {
          /* Got hit */
          dragonProps->m_ShakeTimer = 188;
          func_800562A4(moby, 1);
        } else {
          /* Periodic sparkles */
          if ((dragonProps->m_ShakeTimer >> 1) == 16 && (rand() & 3) == 0) {
            SpawnMobySparkle(moby, &D_8006E57C[0]);
          } else if ((dragonProps->m_ShakeTimer >> 1) == 48 && (rand() & 3) == 0) {
            SpawnMobySparkle(moby, &D_8006E57C[1]);
          } else if ((dragonProps->m_ShakeTimer >> 1) == 80 && (rand() & 3) == 0) {
            SpawnMobySparkle(moby, &D_8006E57C[2]);
          }
        }

        /* Check pickup distance */
        if (OctDistance(&moby->m_Position, &g_Spyro.m_Position) < 2048) {
          Vector3D dragonPickupDelta;

          VecSub(&dragonPickupDelta, &moby->m_Position, &g_Spyro.m_Position);
          dragonPickupDelta.z = (dragonPickupDelta.z * 3) >> 2;
          if (VecMagnitude(&dragonPickupDelta, 1) < 1088) {
            int rotZ;
            int cutsceneId;
            if (dragonProps->m_DragonPadLink != -1) {
              rotZ = g_LevelMobys[dragonProps->m_DragonPadLink].m_Rotation.z;
            } else {
              rotZ = dragonProps->m_Rotation;
            }
            func_8003B854(0, moby);
            CheckpointSave(moby, rotZ);

            if (dragonProps->m_OldDialogueId != -1) {
              /* Counted dragon */
              g_LevelDragonCount[g_LevelIndex]++;
              g_DragonTotal++;
              func_8002C914(dragonProps->m_OldDialogueId, 0);
              if (dragonProps->m_DragonPadLink != -1) {
                g_LevelMobys[dragonProps->m_DragonPadLink].m_State = 1;
              }
              func_80052568(moby);
            } else if (cutsceneId = dragonProps->m_CutsceneId, cutsceneId == dragonProps->m_OldDialogueId) {
              /* Already counted */
              g_LevelDragonCount[g_LevelIndex]++;
              g_DragonTotal++;
              if (dragonProps->m_DragonPadLink != cutsceneId) {
                g_LevelMobys[dragonProps->m_DragonPadLink].m_State = 1;
              }
              func_80052568(moby);
            } else {
              moby->m_State = 2;
              moby->m_Rotation.x = dragonProps->m_AngleStorage.x;
              moby->m_Rotation.y = dragonProps->m_AngleStorage.y;
              moby->m_Position.z = dragonProps->m_AngleStorage.z;
              func_8002C924(moby);
              VecNull(&g_Spyro.m_HeadLookTarget);
              g_Spyro.m_ControlFlags = 0x80000000 | 0x2000 | 0x100 | 0x40 | 0x4 | 0x2 | 0x1;
              if (g_Spyro.m_airTime != 0) {
                g_Spyro.m_fallingState = 6;
                g_Spyro.unk_0x208.x = g_Spyro.m_Physics.m_TrueVelocity.x >> 8;
                g_Spyro.unk_0x208.y = g_Spyro.m_Physics.m_TrueVelocity.y >> 8;
                if (g_Spyro.m_Physics.m_TrueVelocity.z > 0) {
                  g_Spyro.unk_0x208.z = 0;
                } else {
                  g_Spyro.unk_0x208.z = g_Spyro.m_Physics.m_TrueVelocity.z >> 6;
                }
              } else {
                g_Spyro.m_fallingState = 3;
                VecCopy(&g_Spyro.unk_0x208, &g_Spyro.m_Physics.m_TrueVelocity);
                VecShiftRight(&g_Spyro.unk_0x208, 6);
                g_Spyro.unk_0x208.z = 0;
                if (g_Spyro.unk_0x208.x != 0 || g_Spyro.unk_0x208.y != 0) {
                  VecScaleToLength(&g_Spyro.unk_0x208, VecMagnitude(&g_Spyro.unk_0x208, 0), 0x60);

                }
                g_Spyro.unk_0x208.z = 0;
              }
            }
          }
        }
      } else {
        /* state == 2: in cutscene */
        g_Spyro.m_ControlFlags = 0x80000000 | 0x2000;
      }
      break;
    }
    case 251: // Dragon fragment
      UpdateMobyDragonFragment(moby, g_DeltaTime);
      break;

    case 260:
    case 261:
    case 262:
    case 263:
    case 264:
    case 265:
    case 266:
    case 267:
    case 268:
    case 269: { // Digits

      struct {
        int m_Lifetime;     /* 0x00: Remaining lifetime frames */
        int m_Unused04;  /* 0x04 */
        int m_VelocityX; /* 0x08 */
        int m_VelocityY; /* 0x0C */
        int m_VelocityZ; /* 0x10 */
      } *digitProps = moby->m_Props;

      if (!(moby->m_RenderRadius & 0x80) && (moby->m_RenderRadius != 0)) {

        if (digitProps->m_Lifetime >= 1) {

          /* Update Rotation (Spinning effect) */
          moby->m_Rotation.z += 1;

          /* Apply Gravity */
          digitProps->m_VelocityZ -= 6;
          if (digitProps->m_VelocityZ < -0x80) {
            digitProps->m_VelocityZ = -0x80; /* Terminal velocity cap */
          }

          /* Apply Velocity to Position */
          moby->m_Position.x += digitProps->m_VelocityX;
          moby->m_Position.y += digitProps->m_VelocityY;
          moby->m_Position.z += digitProps->m_VelocityZ;

          /* Collision Logic */
          if (moby->m_Position.z > 1023) {
            /* Check for collision with world geometry */
            if (func_8004BE4C(&moby->m_Position, 0x100, 0x100)) {
              int dot;
              /* Snap to collision surface point */
              moby->m_Position.x = g_CollisionPoint.x;
              moby->m_Position.y = g_CollisionPoint.y;
              moby->m_Position.z = g_CollisionPoint.z;

              /* Calculate reflection/bounce based on surface normal */
              func_80017330(&g_CollisionNormal, 0x1000);

              /* Dot product of velocity and surface normal */
              dot = (digitProps->m_VelocityX * g_CollisionNormal.x + digitProps->m_VelocityY * g_CollisionNormal.y + digitProps->m_VelocityZ * g_CollisionNormal.z) >> 11;

              if (dot < 0) {
                /* Reflect velocity vector against the surface */
                VecScaleToLength(&g_CollisionNormal, 0x1000, -dot);
                digitProps->m_VelocityX += g_CollisionNormal.x;
                digitProps->m_VelocityY += g_CollisionNormal.y;
                digitProps->m_VelocityZ += g_CollisionNormal.z;
              }
            }
            digitProps->m_Lifetime--;
          } else {
              func_80052568(moby);
          }
        } else {
          /* Lifetime expired: Spawn final particles and likely despawn */
          D_800758E4(10, 0x47, &moby->m_Position, 0);
          func_80052568(moby);
        }
      }
      break;
    }

    case 286: { // sound related
      struct {
        int m_State; /* 0x00 - sub-state machine */
        int m_SoundDistance;
        PathData *m_PathPoints; /* 0x08 - per-state data array */
        int m_StateTimer;       /* 0x0C - timer driving state transitions */
        int m_PathState;        /* 0x10 - 0/1 idle/moving along path */
        int m_PathSpeed;
        Vector3D m_PathVelocity;
        int m_PathTimer; /* 0x24 - countdown along path segment */
      } *ambProps = moby->m_Props;
      int soundTableIndex;
      moby->m_SoundDistance = ambProps->m_SoundDistance;

      soundTableIndex = -1;
      switch (ambProps->m_State) {
      case 0:
        soundTableIndex = 6;
        break;
      case 1:
        if (TimerTick(&ambProps->m_StateTimer, 4) != 0) {
          ambProps->m_StateTimer = 0x258 - (rand() & 0x7F);
          soundTableIndex = (rand() & 7) + 7;
        }
        break;
      case 2: {
        switch (ambProps->m_PathState) {
        case 0:
          if (TimerTick(&ambProps->m_StateTimer, 4) != 0) {
            ambProps->m_StateTimer = 0x200 - (rand() & 0x7F);
            soundTableIndex = (rand() & 1) + 13;
            ambProps->m_PathState = 1;
          }
          break;

        case 1:
          if (ambProps->m_PathTimer-- > 0) {
            moby->m_Position.x += ambProps->m_PathVelocity.x;
            moby->m_Position.y += ambProps->m_PathVelocity.y;
            moby->m_Position.z += ambProps->m_PathVelocity.z;
          } else {
            /* Snap to next path node */
            int curr, nxt;
            moby->m_Position.x = ambProps->m_PathPoints->m_Nodes[(ambProps->m_PathPoints->m_CurrentNode + 1) % ambProps->m_PathPoints->m_NodeCount].m_Position.x;
            moby->m_Position.y = ambProps->m_PathPoints->m_Nodes[(ambProps->m_PathPoints->m_CurrentNode + 1) % ambProps->m_PathPoints->m_NodeCount].m_Position.y;
            moby->m_Position.z = ambProps->m_PathPoints->m_Nodes[(ambProps->m_PathPoints->m_CurrentNode + 1) % ambProps->m_PathPoints->m_NodeCount].m_Position.z;
            ambProps->m_PathState = 0;
            ambProps->m_PathPoints->m_CurrentNode = (ambProps->m_PathPoints->m_CurrentNode + 1) % ambProps->m_PathPoints->m_NodeCount;
            curr = ambProps->m_PathPoints->m_CurrentNode;
            nxt = (curr + 1) % ambProps->m_PathPoints->m_NodeCount;
            ambProps->m_PathTimer = func_80017D7C((int)&ambProps->m_PathPoints->m_Nodes[curr].m_Position, (int)&ambProps->m_PathPoints->m_Nodes[nxt].m_Position,
                                                  (void *)&ambProps->m_PathVelocity.x, ambProps->m_PathSpeed);
          }
          if (moby->m_SoundChannel == 0x7F && ambProps->m_PathTimer == (ambProps->m_PathTimer / 48) * 48) {
            soundTableIndex = 14;
          }
          break;
        }

        break;
      }
      case 3:
        if (TimerTick(&ambProps->m_StateTimer, 4) != 0) {
          ambProps->m_StateTimer = 624 - (rand() & 0x7F);
          soundTableIndex = (rand() & 1) + 15;
        }
        break;
      case 4:
        if (TimerTick(&ambProps->m_StateTimer, 4) != 0) {
          ambProps->m_StateTimer = 600 - (rand() & 0x7F);
          soundTableIndex = rand() % 4 + 21;
        }
        break;
      case 5:
        soundTableIndex = 24;
        break;
      case 6:
        soundTableIndex = 25;
        break;
      case 7:
        soundTableIndex = 18;
        break;
      case 8:
        soundTableIndex = 26;
        break;
      case 9:
        soundTableIndex = 54;
        break;
      }

      if (soundTableIndex >= 0) {
        u_char soundId = ((u_char *)g_Spu.m_SoundTable)[soundTableIndex];
        PlaySound(soundId, moby, 8, &moby->m_SoundChannel);
      }
      break;
    }

    case 309: { // Some fragment 1
      MobyFragmentPhysicsProps *physicsProps = moby->m_Props;
      int i;

      if (physicsProps->m_Lifetime != 0) {
        if (moby->m_WasDrawn) {
          Vector3D fragmentDustPos;

          /* Movement and Rotation (No Bouncing) */
          moby->m_Position.x += physicsProps->m_VelocityX;
          moby->m_Position.y += physicsProps->m_VelocityY;
          physicsProps->m_VelocityZ -= 6;
          if (physicsProps->m_VelocityZ < -0x80)
            physicsProps->m_VelocityZ = -0x80;
          moby->m_Position.z += physicsProps->m_VelocityZ;

          moby->m_Rotation.x += physicsProps->m_AngularVelocityX;
          moby->m_Rotation.y += physicsProps->m_AngularVelocityY;
          moby->m_Rotation.z += physicsProps->m_AngularVelocityZ;

          fragmentDustPos.x = (rand() & 0xFE) - 127;
          fragmentDustPos.y = (rand() & 0xFE) - 127;
          fragmentDustPos.z = (rand() & 0xFE) - 64;
          VecAdd(&fragmentDustPos, &fragmentDustPos, &moby->m_Position);
          D_800758E4(1, 0x42, &fragmentDustPos, (void *)1);
          physicsProps->m_Lifetime--;
        } else {
          if (moby->m_WasDrawn) {
            D_800758E4(8, 0x46, &moby->m_Position, (void *)0x10);
          }
          func_80052568(moby);
        }
      } else {
        func_80052568(moby);
      }
      break;
    }

    case 310: // Some fragment 2 / 3
    case 311: {
      int i;
      int dot;
      MobyFragmentPhysicsProps *physicsProps = moby->m_Props;

      if (physicsProps->m_Lifetime > 0 && moby->m_WasDrawn) {
        if (func_8004BE4C(&moby->m_Position, 0x100, 0x100)) {

          func_80017330(&g_CollisionNormal, 0x1000);
          dot = (physicsProps->m_VelocityX * g_CollisionNormal.x + physicsProps->m_VelocityY * g_CollisionNormal.y + physicsProps->m_VelocityZ * g_CollisionNormal.z) >> 11;
          // TODO: Permuter hack
          i = dot;
          if (dot < 0) {
            VecScaleToLength(&g_CollisionNormal, 0x1000, (i >> 2) - (dot));
            physicsProps->m_VelocityX += g_CollisionNormal.x;
            physicsProps->m_VelocityY += g_CollisionNormal.y;
            physicsProps->m_VelocityZ += g_CollisionNormal.z;
          }
        }

        /* Movement and Rotation */
        moby->m_Position.x += physicsProps->m_VelocityX;
        moby->m_Position.y += physicsProps->m_VelocityY;
        physicsProps->m_VelocityZ -= 6;
        if (physicsProps->m_VelocityZ < -0x80)
          physicsProps->m_VelocityZ = -0x80;
        moby->m_Position.z += physicsProps->m_VelocityZ;

        moby->m_Rotation.x += physicsProps->m_AngularVelocityX;
        moby->m_Rotation.y += physicsProps->m_AngularVelocityY;
        moby->m_Rotation.z += physicsProps->m_AngularVelocityZ;

        /* Spawn 3 dust particles */
        for (i = 0; i < 3; i++) {
          Vector3D vec_110;

          // TODO: Permuter hack
          i++;
          i--;
          vec_110.x = (rand() & 0xFE) - 127;
          vec_110.y = (rand() & 0xFE) - 127;
          vec_110.z = (rand() & 0xFE) - 64;
          VecAdd(&vec_110, &vec_110, &moby->m_Position);
          D_800758E4(1, 0x42, &vec_110, (void *)1);
        }
        physicsProps->m_Lifetime--;
      } else {
        if (moby->m_WasDrawn) {
          D_800758E4(8, 0x46, &moby->m_Position, (void *)0x10);
        }
        func_80052568(moby);
      }
      break;
    }

    case 323: { // Fodder Respawn
      struct {
        int m_TargetClass;     /* 0x00 - which class to respawn */
        int m_RespawnTimer;    /* 0x04 - countdown */
        int m_RespawnInterval; /* 0x08 - reset value */
      } *respProps = moby->m_Props;
      VecCopy(&moby->m_Position, &g_Spyro.m_Position);

      if (TimerTick(&respProps->m_RespawnTimer, 4) != 0) {
        Moby *m;
        respProps->m_RespawnTimer = respProps->m_RespawnInterval;

        for (m = g_LevelMobys; m < g_DynMobys; m++) {
          if (m->m_Class == respProps->m_TargetClass && m->m_State >= 0x80 && OctDistance(&g_Spyro.m_Position, (Vector3D *)m->m_Props) > 24576) {
            /* Respawn it */
            VecCopy(&m->m_Position, (Vector3D *)m->m_Props);
            func_800526A8(m);
            m->m_DamageFlags = 0;
            m->m_State = 0;
            m->m_DroppedFlag &= 0x7F;
            m->m_AnimationState.m_FrameProgress = 0;
            m->m_AnimationState.m_PerFrameProgress = g_Models[m->m_Class]->m_Animations[0]->m_ProgressPerTick;
            m->m_AnimationState.m_Animation = 0;
            m->m_AnimationState.m_NextAnimation = 0;
            m->m_AnimationState.m_Frame = 0;
            m->m_AnimationState.m_NextFrame = 1;
          }
        }
      }
      break;
    }
    case 331: { // Dragon pad
      switch (moby->m_State) {
      case 0: {
        if (OctDistance(&moby->m_Position, &g_Spyro.m_Position) > 2560) {
          moby->m_State = 1;
        }
        break;
      }

      case 1: {
        if (OctDistance(&moby->m_Position, &g_Spyro.m_Position) < 896 && ABS2(moby->m_Position.z - moby->m_FloorDistance - g_Spyro.m_Position.z) < 512) {

          // Save checkpoint
          CheckpointSave(moby, moby->m_Rotation.z);
          moby->m_State = 2;
          moby->m_AnimationState.m_Animation = 1;
          moby->m_Substate = 0;
        }
        break;
      }

      case 2: {
        moby->m_Substate += g_DeltaTime;

        if (moby->m_Substate >= 0x30) {
          moby->m_State = 0;
          moby->m_AnimationState.m_Animation = 0;
        }
        break;
      }
      }

      break;
    }
    case 405: // Water Bubbles (Blue)
    case 477: // Water Splash (Blue)
    {
      // Die if our animation has finished.
      if (D_80075794) {
        func_80052568(moby);
      }
      break;
    }
    case 398: {
      struct {
        PathData *m_Path;       /* 0x00 */
        Vector3D m_FlyInOffset; /* 0x04 - 0x10 */
        int m_TargetLevelId;    /* 0x10 - which level to enter */
        int m_ExitAngle;   /* 0x14 - which fly-in preset */
        int m_CameraPresetIndex;
      } *vortexProps = moby->m_Props;
      PathData *p;
      int mag;
      short progressed;
      switch (moby->m_State) {
      case 1: {
        Vector3D vortexPathMidpoint;

        g_Spyro.m_ControlFlags = (0x80000000 | 0x2000 | 0x4000 | 0x100 | 0x40 | 0x4 | 0x2 | 0x1);
        g_Spyro.unk_0x248 = moby->m_Substate;
        g_Spyro.unk_0x240 = vortexProps->m_Path;
        g_Spyro.unk_0x244 = 0x60;
        g_Spyro.m_fallingState = 5;
        g_Spyro.m_sortingDepth = 0x7F;

        /* Compute mid-point of vortex top */
        p = vortexProps->m_Path;
        VecAdd(&vortexPathMidpoint, &p->m_Nodes[0].m_Position, &p->m_Nodes[1].m_Position);
        VecShiftRight(&vortexPathMidpoint, 1);

        VecSub(&g_Spyro.unk_0x208, &vortexPathMidpoint, &g_Spyro.m_Position);
        VecAdd(&g_Spyro.unk_0x208, &g_Spyro.unk_0x208, &vortexProps->m_FlyInOffset);

        mag = VecMagnitude(&g_Spyro.unk_0x208, 1);
        if (mag > 128) {
          VecScaleToLength(&g_Spyro.unk_0x208, mag, 128);
        } else {
          /* Already at apex */
          if (moby->m_Substate != 0) {
            progressed = (vortexProps->m_Path->m_CurrentNode < (vortexProps->m_Path->m_NodeCount - 1));
          } else {
            progressed = vortexProps->m_Path->m_CurrentNode;
          }

          if (progressed) {
            moby->m_State = 2;
          }
        }

        /* Detect exit by water collision in front of camera */
        if (func_8004BE4C(&g_Camera.m_Position, 0x300, 0x300) != 0 && (g_SurfaceBelowFlags & 0x3F) < 0x3F && g_Environment.m_SurfaceData[g_SurfaceBelowFlags]->m_Type == 6) {
          moby->m_State = 0;
          g_Camera.unk_0xC0 = (0x80000000 | 0x10 | 0x2);
          g_Spyro.m_fallingState = 0xF;
          g_LoadStage = 0;
          g_LevelTransHudActive = 1;
          g_LevelTransTicks = 0;
          g_Gamestate = 1;
          g_StateSwitch = 1;
          g_Spyro.m_portalAngle.z = vortexProps->m_ExitAngle;
          func_80033F08(&g_Camera.m_Position);
          /* shift camera to follow */
          g_Camera.m_Simulation.m_Coords.azimuth += g_Spyro.m_Physics.m_SpeedAngle.m_RotZ;

          g_Spyro.m_Physics.m_SpeedAngle.m_RotZ += D_80075858;
          g_Spyro.m_bodyRotation.z = g_Spyro.m_Physics.m_SpeedAngle.m_RotZ >> 4;
          g_Camera.m_Sphere = g_Camera.m_Simulation;
          g_Camera.m_Sphere.m_Coords.azimuth -= (g_Spyro.m_bodyRotation.z) << 4;
          func_80034204(&g_Camera.m_Position);
          VecAdd(&g_Camera.m_Position, &g_Camera.m_Position, &g_Spyro.m_Position);
          func_800342F8();
          D_80075910 = D_800758FC;
          if (vortexProps->m_CameraPresetIndex >= 0) {
            g_Camera.m_SphericalPreset = &D_8006CA24[vortexProps->m_CameraPresetIndex];
          } else {
            g_Camera.m_SphericalPreset = nullptr;
          }
        } else {
          g_Spyro.m_ControlFlags |= 0x400;
        }
        break;
      }
      case 2: {
        /* identical to case 1 but with simpler flags */
        g_Spyro.m_ControlFlags = 0x80000000 | 0x4000 | 0x2000 | 0x100;
        g_Spyro.unk_0x240 = vortexProps->m_Path;
        g_Spyro.unk_0x244 = 0x60;
        g_Spyro.m_fallingState = 0xF;
        g_Spyro.m_sortingDepth = 0x7F;

        if (func_8004BE4C(&g_Camera.m_Position, 0x300, 0x300) != 0 && (g_SurfaceBelowFlags & 0x3F) < 0x3F && g_Environment.m_SurfaceData[g_SurfaceBelowFlags]->m_Type == 6) {
          /* same exit logic as case 1 */
          moby->m_State = 0;
          g_Camera.unk_0xC0 = 0x80000000 | 0x10 | 0x2;
          g_LoadStage = 0;
          g_LevelTransHudActive = 1;
          g_LevelTransTicks = 0;
          g_Gamestate = 1;
          g_StateSwitch = 1;
          g_Spyro.m_portalAngle.z = vortexProps->m_ExitAngle;
          func_80033F08(&g_Camera.m_Position);
          g_Camera.m_Simulation.m_Coords.azimuth += g_Spyro.m_Physics.m_SpeedAngle.m_RotZ;

          g_Spyro.m_Physics.m_SpeedAngle.m_RotZ += D_80075858;
          g_Spyro.m_bodyRotation.z = g_Spyro.m_Physics.m_SpeedAngle.m_RotZ >> 4;
          g_Camera.m_Sphere = g_Camera.m_Simulation;
          g_Camera.m_Sphere.m_Coords.azimuth -= g_Spyro.m_bodyRotation.z << 4;
          func_80034204(&g_Camera.m_Position);
          VecAdd(&g_Camera.m_Position, &g_Camera.m_Position, &g_Spyro.m_Position);
          func_800342F8();
          D_80075910 = D_800758FC;
          if (vortexProps->m_CameraPresetIndex >= 0) {
            g_Camera.m_SphericalPreset = &D_8006CA24[vortexProps->m_CameraPresetIndex];
          } else {
            g_Camera.m_SphericalPreset = nullptr;
          }
        } else {
          g_Spyro.m_ControlFlags |= 0x400;
        }
        break;
      }
      }
      break;
    }

    case 416: { // Balloon
      struct {
        int m_StartZ;
      } *balloonProps = moby->m_Props;

      if (moby->m_State == 0) {
        balloonProps->m_StartZ = moby->m_Position.z;
        moby->m_State = 1;
        break;
      }

      moby->m_Position.z = balloonProps->m_StartZ + (SINE_8(moby->m_Substate) >> 4);
      moby->m_Substate += g_DeltaTime;
      break;
    }

    case 76:
    case 426:
    case 427:
    case 428:
    case 429:
    case 430:
    case 431:
    case 432:
    case 433:
    case 434:
    case 435:
    case 436:
    case 437:
    case 438:
    case 439:
    case 440:
    case 441:
    case 442:
    case 443:
    case 444:
    case 445:
    case 446:
    case 447:
    case 448:
    case 449:
    case 450: // Letters
    case 451: {
      MobyLetterProps *letterProps = moby->m_Props;

      if (letterProps->m_Parent->m_State == 0 || letterProps->m_Parent->m_Substate != moby->m_State) {
        func_80052568(moby);
      } else if (letterProps->m_Parent->m_Class == 9) {
        if (OctDistance(&letterProps->m_Parent->m_Position, &g_Spyro.m_Position) < 0x400) {
          int oldval;
          oldval = COSINE_8(moby->m_Substate);
          moby->m_Substate += 8;
          moby->m_Rotation.z = moby->m_Rotation.z - (oldval * 3 >> 9) + (COSINE_8(moby->m_Substate) * 3 >> 9);
        } else {
          // TODO: used for multiple purposes? review name
          int anglespyro;
          Vector3D letterForwardOffset;
          Vector3D letterSideStep;
          Vector3D letterCenteringOffset;
          Vector3D letterWaveOriginOffset;

          moby->m_Substate += 8;
          anglespyro = Atan2(g_Spyro.m_Position.x - letterProps->m_Parent->m_Position.x, g_Spyro.m_Position.y - letterProps->m_Parent->m_Position.y, 0);
          letterForwardOffset.x = COSINE_8(anglespyro) * 3 >> 4;
          letterForwardOffset.y = SINE_8(anglespyro) * 3 >> 4;
          letterForwardOffset.z = 0;
          letterSideStep.x = COSINE_8(anglespyro + 64 & 0xFF) >> 5;
          letterSideStep.y = SINE_8((anglespyro + 64) & 0xFF) >> 5;
          letterSideStep.z = 0;
          anglespyro = (anglespyro + 0x80) & 0xFF;
          moby->m_Rotation.z = anglespyro + (COSINE_8(moby->m_Substate) * 3 >> 9);
          VecMult(&letterCenteringOffset, &letterSideStep, letterProps->m_Len - 1);
          VecShiftLeft(&letterSideStep, 1);
          anglespyro = 2;
          anglespyro = (letterProps->m_Len - 1) * anglespyro;
          letterCenteringOffset.z += COSINE_8(anglespyro) * 3 >> 3;
          letterWaveOriginOffset.x = letterForwardOffset.x * COSINE_8(anglespyro);
          letterWaveOriginOffset.y = letterForwardOffset.y * COSINE_8(anglespyro);
          letterWaveOriginOffset.z = 0;
          anglespyro = ((anglespyro - letterProps->m_Index * 4)) & 0xFF;
          VecMult(&letterSideStep, &letterSideStep, letterProps->m_Index);
          VecSub(&letterCenteringOffset, &letterCenteringOffset, &letterSideStep);
          moby->m_Position.x = letterForwardOffset.x * COSINE_8(anglespyro);
          moby->m_Position.y = letterForwardOffset.y * COSINE_8(anglespyro);
          moby->m_Position.z = 0;
          VecSub(&moby->m_Position, &moby->m_Position, &letterWaveOriginOffset);
          VecShiftRight(&moby->m_Position, 10);
          VecAdd(&moby->m_Position, &moby->m_Position, &letterForwardOffset);
          VecAdd(&moby->m_Position, &moby->m_Position, &letterProps->m_Parent->m_Position);
          VecSub(&moby->m_Position, &moby->m_Position, &letterCenteringOffset);
          moby->m_Position.z = moby->m_Position.z + (COSINE_8(anglespyro) * 3 >> 3) + 0x600;
        }
      } else {
        moby->m_Substate += 8;
        moby->m_Rotation.z = moby->m_FloorDistance + (COSINE_8(moby->m_Substate) * 3 >> 9);
      }
      break;
    }
    }
  }
}

INCLUDE_ASM("asm/nonmatchings/overlays/level_60", func_level_60_80083568);
