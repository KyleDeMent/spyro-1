#include "moby.h"
#include "42CC4.h"
#include "abs.h"
#include "collision.h"
#include "common.h"
#include "loaders.h"
#include "math.h"
#include "moby_helpers.h"
#include "moby_lists.h"
#include "spyro.h"
#include "rand.h"

// From psyq
extern int rand(void);
extern void srand(unsigned int);


extern struct {
  u_char m_Buf[0x1c00];
  u_int m_WorldNearQueued[256];
  u_int m_WorldFarQueued[256];
  Moby *m_ShadedMobys[256];
  char unk_0x2800[2464];
} g_SonyImage;

extern int D_80075794;
extern int D_800757F4;

typedef struct {
  int unk_0x0;
  int unk_0x4;
  int unk_0x8;
  u_char* unk_0xC;
  //...
  int unk_0x1C;
  int unk_0x20;
} MobyProps10;

typedef struct {
  int unk_0x0;
  int unk_0x4;
  int unk_0x8;
  u_char* unk_0xC;
  //...
  int unk_0x30;
} SubProps;

void func_level_10_8007D9C8(void) {
  Moby **lst;
  Moby *list_moby;
  Moby *pod_moby;
  MobyProps10 *props10;

  func_80051FEC(); //build moby-needs-update list
  func_800522C0(g_SonyImage.m_Buf + 0x400, 0); //update moby anims
  if(g_DeltaTime > 2) {
    func_800522C0(g_SonyImage.m_Buf + 0x400, g_DeltaTime == 3 ? 0x80000001 : 0x80000000); //update moby anims
  }

  lst = (Moby**)(g_SonyImage.m_Buf + 0x400);
  for(list_moby = *lst; list_moby; ++list_moby)
  {
    if(list_moby->m_State >= 0x80)
      continue;

    D_80075794 = list_moby->m_AnimationState.m_AnimationFlags & 2;
    D_800757F4 = list_moby->m_AnimationState.m_AnimationFlags & 1;
    D_800756C4 = g_DeltaTime;

    switch (list_moby->m_Class)
    {
    case 10: //0xA
      props10 = list_moby->m_Props;
      if ((list_moby->m_DamageFlags & 0xB0000) && list_moby->m_State != 3) {
        list_moby->m_DamageFlags = 0;
        props10->unk_0x1C = Atan2(list_moby->m_Position.x - g_Spyro.m_Position.x, list_moby->m_Position.y - g_Spyro.m_Position.y, 0);
        props10->unk_0x20 = (list_moby->m_DamageFlags & 0x10000) ? 400 : 200;
        func_8003ABC0(list_moby, 3, 0, 0);
        func_8003B7C0(list_moby);  // Mark moby killed
        list_moby->m_State = 3;
        list_moby->m_AnimationState.m_FrameProgress = 0;
        list_moby->m_AnimationState.m_Animation = 3;
        list_moby->m_AnimationState.m_NextAnimation = 3;
        list_moby->m_AnimationState.m_Frame = 0;
        list_moby->m_AnimationState.m_NextFrame = 1;
        list_moby->m_AnimationState.m_PerFrameProgress = D_80076378[list_moby->m_Class]->m_Animations[3]->m_ProgressPerTick;
      } else {
        switch (list_moby->m_State) {
        case 0:
          func_80038458(list_moby);
          if (OctDistance(&list_moby->m_Position, &g_Spyro.m_Position) >= 0x1400) {
            if (list_moby->m_AnimationState.m_NextAnimation != 0) {
              D_80075794 = 0;
              list_moby->m_AnimationState.m_Animation = list_moby->m_AnimationState.m_NextAnimation;
              list_moby->m_AnimationState.m_Frame = list_moby->m_AnimationState.m_NextFrame;
              list_moby->m_AnimationState.m_FrameProgress = 16;
              list_moby->m_AnimationState.m_PerFrameProgress = 16;
              list_moby->m_AnimationState.m_NextAnimation = 0;
              list_moby->m_AnimationState.m_NextFrame = 0;
              func_80037E98(list_moby);
            } else {
              func_800529E4(list_moby, UPDATE_PROP_CHAIN);
            }
          } else {
            if (ABS(list_moby->m_Position.z - g_Spyro.m_Position.z) < 0x400 && list_moby->m_Pod != 0xFF) {
              for(pod_moby = *lst; pod_moby; ++pod_moby) {
                SubProps *sub_props = pod_moby->m_Props;
                if (pod_moby->m_Pod == list_moby->m_Pod && pod_moby->m_State == 0 && sub_props->unk_0xC[1] == props10->unk_0xC[1]) {
                  pod_moby->m_State = 1;
                  sub_props->unk_0x30 = RandRange(6, 40);
                }
              }
            }
            list_moby->m_State = 1;
          }
          break;
        case 1:
          uv22 = Atan2(g_Spyro.m_Position.x - (list_moby->m_Position).x, g_Spyro.m_Position.y - (list_moby->m_Position).y, 0);
          uint uv20 = props10->unk_0xC[0];
          uv36 = (props10->unk_0xC[1] + 1) % uv20;
          iv11 = (props10->unk_0xC[1] - 1 + uv20) % uv20;
          break;
        }
      }
      break;
    }
  }
}

Moby *artisans_SpawnMoby(int pClass, Moby *pParent) {
  Vector3D tmp_vec;
  Vector3D tmp_vec2;
  uint moby_index;
  Moby *new_moby;
  int floor_position;
  uint rnd;
  int r1;
  int r2;
  MobyPropsCollectable *orb_props;
  MobyPropsCollectable *gem_props;
  MobyPropsButterfly *butterfly_props;
  MobyPropsSparx *sparx_props;
  MobyProps255 *props255;
  MobyPropsDragonFragment *fragment_props;
  MobyPropsDigit *props_digit;
  char padding[8];

  new_moby = func_800524C4();
  new_moby->m_Class = pClass;

  if (pParent) {
    moby_index = (pParent - D_80075828);
    if (moby_index >= 256)
      moby_index = 0;
  } else {
    moby_index = 0;
  }

  new_moby->m_MobyIndex = moby_index;

  switch (pClass) {
  case MOBYCLASS_LIFE_ORB:
    orb_props = new_moby->m_Props;

    // Initialize moby
    MobyInitialize(new_moby);

    // Initialize collectable props
    orb_props->unk_0x0 = 0;
    orb_props->unk_0x4 = 0;
    orb_props->unk_0x8 = 0x8C;
    orb_props->unk_0xE = 0;
    orb_props->unk_0xF = 0;
    orb_props->unk_0x10 = 3;
    orb_props->unk_0x11 = 0;
    orb_props->unk_0x12 = 0;
    orb_props->unk_0x13 = 0;
    orb_props->unk_0x14 = 0xFF;

    new_moby->m_Substate = 2;
    new_moby->m_RenderRadius = 0x18;
    new_moby->m_UpdateDistance = 0x10;
    new_moby->m_Rotation.x = 0x20;
    new_moby->m_Rotation.y = 0;
    new_moby->m_Rotation.z = 0;
    if (pParent) {
      VecCopy(&new_moby->m_Position, &pParent->m_Position);
    }

    // Initialize moby collision
    MobyCollisionEnable(new_moby);

    new_moby->m_SpecularMetalColor[0] = 0;
    new_moby->m_SpecularMetalColor[1] = 0;
    new_moby->m_SpecularMetalColor[2] = 0;
    new_moby->m_SpecularMetalType = 1;
    new_moby->m_Renderer.raw |= 0x80;
    break;
  case MOBYCLASS_BUTTERFLY:
    butterfly_props = new_moby->m_Props;

    MobyInitialize(new_moby);

    MobyCollisionEnable(new_moby);

    VecCopy(&new_moby->m_Position, &pParent->m_Position);
    new_moby->m_Position.z += 0x200;
    VecCopy(&butterfly_props->vec_0x4, &new_moby->m_Position);

    butterfly_props->unk_0x13 = 0;
    butterfly_props->unk_0x12 = 0;
    butterfly_props->unk_0x14 = 0x708;
    break;
  case 34: // 0x22
    MobyInitialize(new_moby);

    new_moby->m_RenderRadius = 0x20;
    new_moby->m_UpdateDistance = 0xFF;
    if (pParent)
      VecCopy(&new_moby->m_Position, &pParent->m_Position);

    MobyCollisionDisable(new_moby);
    break;
  case MOBYCLASS_EXTRA_LIFE:
  case MOBYCLASS_GEM_1:
  case MOBYCLASS_GEM_2:
  case MOBYCLASS_GEM_5:
  case MOBYCLASS_GEM_10:
  case MOBYCLASS_GEM_25:
    gem_props = new_moby->m_Props;

    MobyInitialize(new_moby);

    gem_props->unk_0x0 = 0;
    gem_props->unk_0x4 = 0;
    gem_props->unk_0x8 = 0x8C;
    gem_props->unk_0xE = 0;
    gem_props->unk_0xF = 0;
    gem_props->unk_0x11 = 0;
    gem_props->unk_0x12 = 0;
    gem_props->unk_0x13 = 0;
    if (pParent->m_Class == 13)
      gem_props->unk_0x10 = 2;
    else
      gem_props->unk_0x10 = 3;
    gem_props->unk_0x14 = 0xFF;

    new_moby->m_Substate = 2;
    new_moby->m_RenderRadius = 0x18;
    new_moby->m_UpdateDistance = 0x40;
    setXYZ(&new_moby->m_Rotation, 0x20, 0, 0);
    VecCopy(&new_moby->m_Position, &pParent->m_Position);

    MobyCollisionDisable(new_moby);

    new_moby->m_ShadowDistance = -1;

    VecCopy(&tmp_vec, &new_moby->m_Position);
    tmp_vec.z = tmp_vec.z + 0x400;
    floor_position = func_8004D5EC(&tmp_vec, 0x10000);

    MobyUpdateShadow(new_moby);

    new_moby->m_SpecularMetalColor[0] = 0;
    new_moby->m_SpecularMetalColor[1] = 0;
    new_moby->m_SpecularMetalColor[2] = 0;

    if (new_moby->m_Class == MOBYCLASS_EXTRA_LIFE)
      new_moby->m_SpecularMetalType = 0xC;
    if (new_moby->m_Class == MOBYCLASS_GEM_1)
      new_moby->m_SpecularMetalType = 1;
    if (new_moby->m_Class == MOBYCLASS_GEM_2)
      new_moby->m_SpecularMetalType = 2;
    if (new_moby->m_Class == MOBYCLASS_GEM_5)
      new_moby->m_SpecularMetalType = 3;
    if (new_moby->m_Class == MOBYCLASS_GEM_10)
      new_moby->m_SpecularMetalType = 4;
    if (new_moby->m_Class == MOBYCLASS_GEM_25)
      new_moby->m_SpecularMetalType = 5;
    break;
  case MOBYCLASS_SPARX:
    sparx_props = new_moby->m_Props;

    MobyInitialize(new_moby);

    MobyCollisionEnable(new_moby);

    new_moby->m_Substate = 0;

    sparx_props->unk_0x0 = 0;
    sparx_props->unk_0x8 = 0;
    sparx_props->unk_0x6 = 0;
    sparx_props->unk_0x4 = 0;
    sparx_props->unk_0xC = 0;
    sparx_props->unk_0x10 = 0;

    if (pParent)
      VecCopy(&new_moby->m_Position, &pParent->m_Position);
    break;
  case 255: // 0xFF
  case 256: // 0x100
  case 257: // 0x101
  case 423: // 0x1A7
  case 424: // 0x1A8
  case 425: // 0x1A9
    props255 = new_moby->m_Props;

    MobyInitialize(new_moby);

    new_moby->m_RenderRadius = 0x20;
    VecCopy(&new_moby->m_Position, &pParent->m_Position);

    MobyCollisionEnable(new_moby);

    r1 = rand() & 0xFFF;
    r2 = rand() & 0x7FF;
    props255->unk_0x0 = (Cos(r2) >> 5) * (Cos(r1)) >> 12;
    props255->unk_0x2 = (Cos(r2) >> 5) * (Sin(r1)) >> 12;
    props255->unk_0x4 = Sin(r2) >> 5;
    if (pParent->m_DamageFlags & 0x20000) {
      props255->unk_0x0 += g_Spyro.m_Physics.m_Acceleration.x >> 6;
      props255->unk_0x2 += g_Spyro.m_Physics.m_Acceleration.y >> 6;
      props255->unk_0x4 += g_Spyro.m_Physics.m_Acceleration.z >> 6;
    }
    setXYZ(&new_moby->m_Position,
           new_moby->m_Position.x + props255->unk_0x0 * 4,
           new_moby->m_Position.y + props255->unk_0x2 * 4,
           new_moby->m_Position.z + props255->unk_0x4 * 4);

    props255->unk_0x6 = rand() & 0xF;
    props255->unk_0x8 = rand() & 0xF;
    props255->unk_0xA = rand() & 0xF;
    props255->unk_0x10 = pParent->m_Position.z - 0x40;
    props255->unk_0xC = 0x40 - (rand() & 0xF);

    if (new_moby->m_Class >= 309 && new_moby->m_Class <= 311) {
      // ??? not sure why this block is in here
      // perhaps these blocks were #defined to include them in many levels?
      *(uint *)new_moby->m_SpecularMetalColor = 0xA18618;
      new_moby->m_Renderer.raw |= 0x80;
    }
    break;
  case MOBYCLASS_CRYSTAL_DRAGON_FRAGMENT:
    fragment_props = new_moby->m_Props;

    MobyInitialize(new_moby);

    new_moby->m_RenderRadius = 0x20;
    new_moby->m_UpdateDistance = 0xFF;

    MobyCollisionDisable(new_moby);

    new_moby->m_SpecularMetalColor[0] = 0;
    new_moby->m_SpecularMetalColor[1] = 0;
    new_moby->m_SpecularMetalColor[2] = 0;
    new_moby->m_SpecularMetalType = 14;

    if (D_80077058 == 3)
      new_moby->m_ScaleOverride = 0x14;
    else if (D_80077058 == 1)
      new_moby->m_ScaleOverride = 0x30;

    rnd = rand();
    setXYZ(&tmp_vec2, D_8006F3A0[rnd & 0x7].x, 0, D_8006F3A0[rnd & 0x7].y);
    VecRotateByMatrix((MATRIX *)&pParent->m_RotationMatrix, &tmp_vec2,
                      &tmp_vec2);
    tmp_vec2.x += (rand() & 0x7F) - 0x3F;
    tmp_vec2.y += (rand() & 0x7F) - 0x3F;
    tmp_vec2.z += (rand() & 0x7F) - 0x3F;
    VecAdd(&new_moby->m_Position, &pParent->m_Position, &tmp_vec2);
    VecCopy(&fragment_props->vec_0x0, &tmp_vec2);
    VecShiftRight(&fragment_props->vec_0x0, 2);
    fragment_props->vec_0x0.x += (rand() & 0xFF) - 0x7F;
    fragment_props->vec_0x0.y += (rand() & 0xFF) - 0x7F;
    fragment_props->vec_0x0.z += (rand() & 0xFF) - 0x7F;

    setXYZ(&new_moby->m_Rotation, rand(), rand(), rand());

    fragment_props->unk_0x10 = rand() & 0xF;
    fragment_props->unk_0x11 = rand() & 0xF;
    fragment_props->unk_0x12 = rand() & 0xF;
    fragment_props->unk_0xC = pParent->m_Position.z;
    fragment_props->unk_0x13 = (rand() & 0x3) + 0x10;
    break;
  case MOBYCLASS_NUMBER_0:
  case MOBYCLASS_NUMBER_1:
  case MOBYCLASS_NUMBER_2:
  case MOBYCLASS_NUMBER_3:
  case MOBYCLASS_NUMBER_4:
  case MOBYCLASS_NUMBER_5:
  case MOBYCLASS_NUMBER_6:
  case MOBYCLASS_NUMBER_7:
  case MOBYCLASS_NUMBER_8:
  case MOBYCLASS_NUMBER_9:
  case 277: // 0x115
  case 327: // 0x147
    props_digit = new_moby->m_Props;

    MobyInitialize(new_moby);

    MobyCollisionDisable(new_moby);

    new_moby->m_SpecularMetalType = 2;
    new_moby->m_SpecularMetalColor[0] = 0;
    new_moby->m_SpecularMetalColor[1] = 0;
    new_moby->m_SpecularMetalColor[2] = 0;

    props_digit->unk_0x0 = 0x40;
    break;
  case 398: // 0x18E
    MobyInitialize(new_moby);

    new_moby->m_RenderRadius = 0xFF;
    setXYZ(&new_moby->m_Position, 0x1CC, 0x28, 0x1000);

    MobyCollisionDisable(new_moby);

    new_moby->m_DepthOffset = 0x20;
    new_moby->m_SpecularMetalColor[0] = 0;
    new_moby->m_SpecularMetalColor[1] = 0;
    new_moby->m_SpecularMetalColor[2] = 0;
    new_moby->m_SpecularMetalType = 0;
    break;
  case 405: // 0x195
  case 477: // 0x1DD
    MobyInitialize(new_moby);

    if (pParent)
      VecCopy(&new_moby->m_Position, &pParent->m_Position);
    else
      VecCopy(&new_moby->m_Position, &g_Spyro.m_Position);

    new_moby->m_Position.z += 0x200;
    floor_position = func_8004D5EC(&new_moby->m_Position, 0x800);
    if (abs(floor_position - new_moby->m_Position.z) < 0x800)
      new_moby->m_Position.z = floor_position;
    else
      new_moby->m_Position.z -= 0x200;

    MobyCollisionEnable(new_moby);
    break;
  case MOBYCLASS_LETTER_APOSTROPHE: // 0x4C 76
  case MOBYCLASS_LETTER_A:
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
  case MOBYCLASS_LETTER_Z:
    MobyInitialize(new_moby);

    new_moby->m_RenderRadius = 0x20;
    new_moby->m_UpdateDistance = 0xFF;

    MobyCollisionDisable(new_moby);
    break;
  default:
    MobyInitialize(new_moby);

    if (pParent)
      VecCopy(&new_moby->m_Position, &pParent->m_Position);
    else
      VecCopy(&new_moby->m_Position, &g_Spyro.m_Position);

    MobyCollisionEnable(new_moby);
    break;
  }
  return new_moby;
}