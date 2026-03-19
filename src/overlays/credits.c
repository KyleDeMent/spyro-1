#include "credits.h"

#include "buffers.h"
#include "camera.h"
#include "cd.h"
#include "common.h"
#include "cutscene.h"
#include "cyclorama.h"
#include "environment.h"
#include "gamepad.h"
#include "gamestates/draw.h"
#include "libetc.h"
#include "loaders.h"
#include "memory.h"
#include "moby_helpers.h"
#include "music.h"
#include "rand.h"
#include "renderers.h"
#include "sony_image.h"
#include "specular_and_metal.h"
#include "titlescreen.h"
#include "wad.h"

INCLUDE_ASM("asm/nonmatchings/overlays/credits", CreditsUpdate);

extern struct {
  int post;
  int pre;
} D_80075950;

extern int D_80075958;

// todo terminators

typedef struct {
  Vector3D pos;
  int lower_spacing;
  int spacing;
  int lower_y_add;
  int lower_z_pos;
} CreditsUpdateState;

void CreditsUpdate(void) {
  if (g_CreditsStage == -1) {
    int i;
    RECT rc;
    char *buf_spot = func_credits_8007C338;
    int strings_data_offset;

    if (!g_CreditsBuffer) {
      g_CreditsMobys = (CreditsMoby *)buf_spot;
      g_CreditsMobyCount = 0;
      for (i = 0; i < 100; ++i) {
        g_CreditsMobys[i].unk_0x4 = 0;
      }
      g_CreditsStringEntries =
          (CreditsStringEntry *)(buf_spot + sizeof(CreditsMoby) * 100);
      g_CreditsBuffer = (char *)buf_spot + 0x2800;
      g_Buffers.m_CopyBuf = (char *)buf_spot + 0x2800;
      g_Buffers.m_DiscCopyBuf = (char *)buf_spot + 0x2800;
    }

    // Load "level" header
    CDLoadSync(g_CdState.m_WadSector, g_Buffers.m_DiscCopyBuf, 0x800,
               g_WadHeader.m_Credits1Data[g_CreditsSequence].m_Offset, 600);
    Memcpy(&g_LevelHeader, g_Buffers.m_DiscCopyBuf, sizeof(LevelHeader));

    // Load vram data
    CDLoadSync(g_CdState.m_WadSector, g_Buffers.m_DiscCopyBuf, 0x60000,
               g_LevelHeader.m_VramSramOffset +
                   g_WadHeader.m_Credits1Data[g_CreditsSequence].m_Offset,
               600);
    setRECT(&rc, 512, 0, 512, 384);
    LoadImage(&rc, g_Buffers.m_DiscCopyBuf);

    // Load SPU data
    CDLoadSync(g_CdState.m_WadSector, g_Buffers.m_DiscCopyBuf,
               g_LevelHeader.m_VramSramSize - 0x60000,
               g_LevelHeader.m_VramSramOffset +
                   g_WadHeader.m_Credits1Data[g_CreditsSequence].m_Offset +
                   0x60000,
               600);
    SpuSetTransferStartAddr(0x1010);
    SpuWrite(g_Buffers.m_DiscCopyBuf, 0x7EFF0 /*mistake?*/);
    while (!SpuIsTransferCompleted(0))
      ;

    // Load scene data
    CDLoadSync(g_CdState.m_WadSector, g_Buffers.m_DiscCopyBuf,
               g_LevelHeader.m_SceneSize,
               g_LevelHeader.m_SceneOffset +
                   g_WadHeader.m_Credits1Data[g_CreditsSequence].m_Offset,
               600);
    g_Buffers.m_ModelData = func_80012D58(g_Buffers.m_DiscCopyBuf, 1);
    g_Buffers.m_LevelLayout = g_Buffers.m_ModelData;
    g_Cyclorama = g_NewCyclorama;
    g_Buffers.m_LevelLayoutOffset = g_LevelHeader.m_LayoutOffset;
    g_Buffers.m_LevelLayoutSize = g_LevelHeader.m_LayoutSize;
    g_PortalCount = 0;

    CDLoadSync(g_CdState.m_WadSector, g_Buffers.m_ModelData,
               g_Buffers.m_LevelLayoutSize,
               g_Buffers.m_LevelLayoutOffset +
                   g_WadHeader.m_Credits1Data[g_CreditsSequence].m_Offset,
               600);
    g_CreditsDataPtr = g_Buffers.m_LevelLayout;

    g_Spu.m_SoundDefinitions = &g_CutsceneSoundDef;
    g_Spu.m_SoundDefinitions->m_Addr = 0x1010;
    g_Spu.m_SoundDefinitions->m_LoopAddr = -1;
    g_Spu.m_SoundDefinitions->unk_0x8 = 0x50;
    g_Spu.m_SoundDefinitions->m_Pitch = 1625;
    g_Spu.m_SoundDefinitions->m_PitchVariance = 0;
    g_Spu.m_SoundDefinitions->m_PitchMultiplier = 0;
    g_Spu.m_SoundDefinitions->m_VarianceType = 0;
    g_Spu.m_NextSoundOverrideFlags = 0x1;
    g_Spu.m_VolumeOverride.left = 0x3FFF;
    g_Spu.m_VolumeOverride.right = 0x3FFF;
    PlaySound(0, nullptr, 0x10, nullptr);

    // v1 = g_CreditsDataPtr->m_StringsDataOffset
    // v0 = g_Buffers.m_LevelLayout
    // a0 = g_CreditsStringEntries
    // v0 = g_Buffers.m_LevelLayout + g_CreditsDataPtr->m_StringsDataOffset
    // a1 = v0 + 0x1C
    // v0 =
    g_CreditsStrings =
        (g_Buffers.m_LevelLayout + g_CreditsDataPtr->m_StringsDataOffset);
    g_CreditsTotalEntries = g_CreditsStrings->m_Count;

    Memcpy(g_CreditsStringEntries, g_CreditsStrings + 1,
           g_CreditsStrings->m_Length - sizeof(CreditsStrings));

    for (i = 0; i < g_CreditsTotalEntries; ++i) {
      g_CreditsStringEntries[i].m_Ptr = (char *)g_CreditsStringEntries +
                                        (int)g_CreditsStringEntries[i].m_Ptr +
                                        strings_data_offset;
    }

    g_CreditsStage = 0;
    ++g_CreditsSequence;
  }

  if (g_CreditsDataPtr && g_CreditsStage >= 0) {
    int tickEnd;
    g_CreditsDataPtr->m_Tick += g_DeltaTime;
    tickEnd = g_CreditsDataPtr->m_CamDataCount * 2;

    if (g_CreditsDataPtr->m_Tick < tickEnd) {
      if (g_CreditsDataPtr->m_Tick < 32 ||
          tickEnd - g_CreditsDataPtr->m_Tick < 32) {
        g_Fade = 16 - ((g_CreditsDataPtr->m_Tick < 32)
                           ? g_CreditsDataPtr->m_Tick
                           : (tickEnd - g_CreditsDataPtr->m_Tick)) >>
                 1;
      } else {
        g_Fade = 0;
      }

      if (g_Fade > 15)
        g_Fade = 15;
      if (g_Fade < 0)
        g_Fade = 0;

      setXYZ(&g_Camera.m_Position,
             g_CreditsDataPtr->m_CamData[g_CreditsDataPtr->m_Tick >> 1]
                 .m_Position.x,
             g_CreditsDataPtr->m_CamData[g_CreditsDataPtr->m_Tick >> 1]
                 .m_Position.y,
             g_CreditsDataPtr->m_CamData[g_CreditsDataPtr->m_Tick >> 1]
                 .m_Position.z);
      setXYZ(&g_Camera.m_Rotation,
             g_CreditsDataPtr->m_CamData[g_CreditsDataPtr->m_Tick >> 1]
                 .m_Rotation.x,
             g_CreditsDataPtr->m_CamData[g_CreditsDataPtr->m_Tick >> 1]
                 .m_Rotation.y,
             g_CreditsDataPtr->m_CamData[g_CreditsDataPtr->m_Tick >> 1]
                 .m_Rotation.z);
    } else {
      g_CreditsStage += 1;
    }
  }

  if (g_CreditsMobyCount == 0 && g_CreditsEntryIndex >= g_CreditsTotalEntries) {
    g_Spu.m_SoundDefinitions->unk_0x8 -= 10;
    g_CreditsTimer += g_DeltaTime;
    if (g_Spu.m_SoundDefinitions->unk_0x8 < 0)
      g_Spu.m_SoundDefinitions->unk_0x8 = 0;

    g_Fade = g_CreditsTimer;
    if (g_Fade > 15)
      g_Fade = 15;
    if (g_Fade == 15)
      g_CreditsStage = 99;
  }

  if (g_CreditsMobyCount > 0 || (g_CreditsTotalEntries > 0 &&
                                 g_CreditsEntryIndex < g_CreditsTotalEntries)) {
    int ii;
    int j;
    int displayEntry;
    int iv12;
    int isUpper;
    SpecularUpdate(3);

    isUpper = 1;

    for (j = g_CreditsEntryIndex; j <= g_CreditsDisplayedCount; ++j) {
      displayEntry = -1;
      if (TimerTick(&g_CreditsStringEntries[j].m_Timer, 2)) {
        g_CreditsStringEntries[j].unk_0x0A = 2;
        if (g_CreditsStringEntries[j].m_Timer2 == 0 && j == g_CreditsEntryIndex)
          g_CreditsEntryIndex = j + 1;
      }

      if (g_CreditsStringEntries[0].unk_0x0A == 0) {
        displayEntry = 0;
      } else if (TimerTick(&g_CreditsStringEntries[j].m_Timer2, 2) &&
                 g_CreditsStringEntries[j + 1].unk_0x0A == 0 &&
                 j < g_CreditsTotalEntries - 1 /*checking this after...ok?*/) {
        displayEntry = j + 1;
      }

      iv12 = 0;
      if (displayEntry != -1) {
        CreditsUpdateState cus;
        char *pch;
        int line_mid;
        CreditsStringEntry *entry = &g_CreditsStringEntries[displayEntry];
        int colon_pass;
        int last_spacing;
        entry->unk_0x0A = 1;
        if (displayEntry > g_CreditsDisplayedCount)
          g_CreditsDisplayedCount = displayEntry;
        if (g_CreditsDisplayedCount > g_CreditsTotalEntries)
          g_CreditsDisplayedCount = g_CreditsTotalEntries;
        setXYZ(&cus.pos, 256, g_CreditsStringEntries[displayEntry].unk_0x04,
               4787);
        cus.spacing = 14;
        cus.lower_y_add = 1;
        cus.lower_z_pos = 0x1600;
        line_mid = 256;
        colon_pass = 0;
        last_spacing = 16;
        for (pch = entry->m_Ptr; *pch; ++pch) {
          if (*pch != ' ') {
            CreditsMoby *outMoby;

            while (1) {
              // if (iv12 < g_CreditsMobyCount) {
              while (iv12 < g_CreditsMobyCount && g_CreditsMobys[iv12].unk_0x4)
                ++iv12;
              // for (; iv12 < g_CreditsMobyCount; ++iv12) {
              //   if (g_CreditsMobys[iv12].unk_0x4 == '\0')
              //     break;
              // }
              // }

              if (iv12 < g_CreditsMobyCount) {
                outMoby = &g_CreditsMobys[iv12];
              } else {
                iv12 = g_CreditsMobyCount;
                ++g_CreditsMobyCount;
                outMoby = &g_CreditsMobys[iv12];
              }

              if (*pch >= 'A' && *pch <= 'Z')
                isUpper = 1;
              if (*pch >= 'a' && *pch <= 'z')
                isUpper = 0;
              outMoby->unk_0x4 = 1;
              outMoby->x_displayEntry = displayEntry;
              if (g_CreditsMobyCount > 100) {
                outMoby = nullptr;
                exit(0);
              }

              setXYZ(&outMoby->m_Position, cus.pos.x, cus.pos.y, cus.pos.z);
              if (!isUpper) {
                outMoby->m_Position.y += cus.lower_y_add;
                outMoby->m_Position.z = cus.lower_z_pos;
              }

              if (*pch >= '0' && *pch <= '9') {
                outMoby->m_MobyClass = MOBYCLASS_NUMBER_0 + *pch - '0';
              } else if (*pch >= 'A' && *pch <= 'Z') {
                outMoby->m_MobyClass = MOBYCLASS_LETTER_A + *pch - 'A';
              } else if (*pch >= 'a' && *pch <= 'z') {
                outMoby->m_MobyClass = MOBYCLASS_LETTER_A + *pch - 'a';
              } else if (*pch == '!') {
                outMoby->m_MobyClass = MOBYCLASS_EXCLAMATION_MARK;
              } else if (*pch == ',') {
                outMoby->m_MobyClass = MOBYCLASS_LETTER_APOSTROPHE;
              } else if (*pch == '.') {
                outMoby->m_MobyClass = MOBYCLASS_PERIOD;
              } else if (*pch == '-') {
                outMoby->m_MobyClass = MOBYCLASS_SLASH;
              } else if (*pch == ':') {
                // kind of insane, drawing two periods here for the colon
                outMoby->m_MobyClass = MOBYCLASS_PERIOD;
                outMoby->m_Position.z += 1400;
                if (colon_pass == 0) {
                  // this was the first pass to draw the bottom part
                  // loop around and draw another period up top
                  colon_pass = 1;
                  continue;
                } else {
                  // second pass, move it up and move on
                  colon_pass = 0;
                  outMoby->m_Position.y -= 6;
                }
              } else {
                // none of the above, just put in a dot
                outMoby->m_MobyClass = MOBYCLASS_LETTER_APOSTROPHE;
                outMoby->m_Position.y -= cus.spacing * 2 / 3;
              }

              if (!isUpper) {
                cus.pos.x += cus.spacing;
              } else {
                cus.pos.x += last_spacing;
              }
              break;
            }
          } else {
            cus.pos.x += 10;
          }
        }

        for (ii = 0; ii < g_CreditsMobyCount; ++ii) {
          if (g_CreditsMobys[ii].x_displayEntry == displayEntry) {
            short lx =
                g_CreditsMobys[ii].m_Position.x - (cus.pos.x - line_mid >> 1);
            short lz = (g_CreditsMobys[ii].m_Position.z * -2) / 30;
            short lx2 = (lx - 256) * -3 / 30;
            g_CreditsMobys[ii].m_Position.x = lx;
            g_CreditsMobys[ii].unk_0xc = lx2;
            g_CreditsMobys[ii].unk_0x10 = lz;
            g_CreditsMobys[ii].m_Position.x += (lx2 * -30);
            g_CreditsMobys[ii].m_Position.z += (lz * -30);
            g_CreditsMobys[ii].rand_0x8 = RandRangeSigned(4, 15);
            g_CreditsMobys[ii].rand_0x9 = RandRangeSigned(4, 15);
            g_CreditsMobys[ii].rand_0xa = RandRangeSigned(4, 15);
            g_CreditsMobys[ii].m_Rotation.x = g_CreditsMobys[ii].rand_0x8 * -30;
            g_CreditsMobys[ii].m_Rotation.y = g_CreditsMobys[ii].rand_0x9 * -30;
            g_CreditsMobys[ii].m_Rotation.z = g_CreditsMobys[ii].rand_0xa * -30;
            g_CreditsMobys[ii].unk_0xb = 30;
          }
        }
      }
    }

    // line 357
    {
      int ii;
      for (ii = 0; ii < g_CreditsMobyCount; ++ii) {
        if (g_CreditsStringEntries[g_CreditsMobys[ii].x_displayEntry]
                .unk_0x0A == 2) {
          if (g_CreditsMobys[ii].unk_0x4 == 2)
            g_CreditsMobys[ii].unk_0x4 = 3;
          else if (g_CreditsMobys[ii].unk_0x4 == 4)
            g_CreditsMobys[ii].unk_0x4 = 0;
        }
      }
    }

    // line 373
    {
      int ii;
      for (ii = 0; ii < g_CreditsMobyCount; ++ii) {
        if (g_CreditsMobys[ii].unk_0x4 == 1) {
          g_CreditsMobys[ii].m_Position.x += g_CreditsMobys[ii].unk_0xc;
          g_CreditsMobys[ii].m_Position.z += g_CreditsMobys[ii].unk_0x10;

          g_CreditsMobys[ii].m_Rotation.x += g_CreditsMobys[ii].rand_0x8;
          g_CreditsMobys[ii].m_Rotation.y += g_CreditsMobys[ii].rand_0x9;
          g_CreditsMobys[ii].m_Rotation.z += g_CreditsMobys[ii].rand_0xa;

          g_CreditsMobys[ii].unk_0xb -= 1;
          if (g_CreditsMobys[ii].unk_0xb == 0) {
            g_CreditsMobys[ii].unk_0x4 = 2;
            g_CreditsMobys[ii].unk_0xc = RandRangeSigned(3, 11);
            g_CreditsMobys[ii].unk_0xe = -RandRange(6, 10);
            g_CreditsMobys[ii].unk_0x10 = RandRangeSigned(-35, 35);
            g_CreditsMobys[ii].rand_0x8 = RandRangeSigned(4, 9);
            g_CreditsMobys[ii].rand_0x9 = RandRangeSigned(4, 9);
            g_CreditsMobys[ii].rand_0xa = RandRangeSigned(4, 9);
          }
        } else if (g_CreditsMobys[ii].unk_0x4 == 3) {
          g_CreditsMobys[ii].m_Position.x += g_CreditsMobys[ii].unk_0xc;
          g_CreditsMobys[ii].m_Position.y += g_CreditsMobys[ii].unk_0xe;
          g_CreditsMobys[ii].m_Position.z += g_CreditsMobys[ii].unk_0x10;

          g_CreditsMobys[ii].m_Rotation.x += g_CreditsMobys[ii].rand_0x8;
          g_CreditsMobys[ii].m_Rotation.y += g_CreditsMobys[ii].rand_0x9;
          g_CreditsMobys[ii].m_Rotation.z += g_CreditsMobys[ii].rand_0xa;

          g_CreditsMobys[ii].unk_0xe += 1;
          if (g_CreditsMobys[ii].unk_0xe > 20)
            g_CreditsMobys[ii].unk_0xe = 20;
          if (g_CreditsMobys[ii].m_Position.y > 280)
            g_CreditsMobys[ii].unk_0x4 = 4;
        }
      }
    }

    // line 443
    // todo cull state==0 mobys?
    {
      while (g_CreditsMobyCount > 0 &&
             g_CreditsMobys[g_CreditsMobyCount - 1].unk_0x4 == 0)
        --g_CreditsMobyCount;
    }
  }

  if (g_CreditsStage == -1)
    return;

  if (g_CreditsStage > 0) {
    CDLoadTime();
    if (g_CdState.m_IsReading != 0 || CdSync(1, 0) != CdlComplete)
      return;
  }

  switch (g_CreditsStage) {
  case 0: {
    CDLoadAsync(g_CdState.m_WadSector, g_CreditsStrings, 0x800,
                g_WadHeader.m_Credits1Data[g_CreditsSequence].m_Offset, 600);
    g_CreditsStage += 1;
    break;
  }
  case 1: {
    Memcpy(&g_LevelHeader, g_CreditsStrings, sizeof(LevelHeader));
    CDLoadAsync(g_CdState.m_WadSector, g_CreditsStrings, 0x20000,
                g_LevelHeader.m_VramSramOffset +
                    g_WadHeader.m_Credits1Data[g_CreditsSequence].m_Offset,
                600);
    g_CreditsStage += 1;
    break;
  }
  case 2: {
    RECT rc;
    setRECT(&rc, 0x180, 0x200, 0x200, 0x80);
    DrawSync(0);
    LoadImage(&rc, (u_long *)g_CreditsStrings);
    DrawSync(0);
    D_80075958 =
        g_LevelHeader.m_SceneSize + g_LevelHeader.m_LayoutSize + 0x40000;
    CDLoadAsync(g_CdState.m_WadSector, g_CreditsStrings, D_80075958,
                g_LevelHeader.m_VramSramOffset +
                    g_WadHeader.m_Credits1Data[g_CreditsSequence].m_Offset +
                    0x20000,
                600);
    g_CreditsStage += 1;
    break;
  }
  case 3: {
    D_8007576C = 0;
    break;
  }
  case 4: {
    RECT rc;
    setRECT(&rc, 0x200, 0x80, 0x200, 0x100);
    g_Buffers.m_CopyBuf = g_CreditsBuffer;
    g_Buffers.m_DiscCopyBuf = g_CreditsBuffer;
    DrawSync(0);
    LoadImage(&rc, (u_long *)g_CreditsStrings);
    setRECT(&rc, 0x200, 0x180, 0x200, 0x80);
    MoveImage(&rc, 0x200, 0);
    DrawSync(0);
    Memcpy(g_CreditsBuffer, (char *)g_CreditsStrings + 0x40000,
           D_80075958 - 0x40000);
    g_CreditsStrings = g_CreditsBuffer + D_80075958 - 0x40000;
    g_Buffers.m_ModelData = func_80012D58(g_Buffers.m_DiscCopyBuf, 1);
    g_Cyclorama = g_NewCyclorama;
    g_Buffers.m_LevelLayout =
        (char *)g_Buffers.m_DiscCopyBuf + g_LevelHeader.m_SceneSize;
    g_CreditsDataPtr = (CreditsData *)((char *)g_Buffers.m_DiscCopyBuf +
                                       g_LevelHeader.m_SceneSize);
    g_Buffers.m_LevelLayoutSize = g_LevelHeader.m_LayoutSize;
    g_Buffers.m_LevelLayoutOffset = g_LevelHeader.m_LayoutOffset;
    if (g_CreditsSequence % 10 < 9) {
      g_CreditsSequence = 0;
    } else {
      g_CreditsSequence = 3;
    }
    g_CreditsSequence += 1;
    break;
  }
  }
}

void CreditsDraw(void) {
  if (g_CreditsStage == -1) {
    RECT rc;
    DrawSync(0);
    VSync(0);
    setRECT(&rc, 0, g_CurDB != g_DB ? 248 : 8, 512, 224);
    MoveImage(&rc, 0, 256 - rc.y);
    DrawSync(0);
  } else {
    int i;
    g_DB[0].m_DrawEnv.r0 = g_Cyclorama.m_BackgroundColor.r;
    g_DB[0].m_DrawEnv.g0 = g_Cyclorama.m_BackgroundColor.g;
    g_DB[0].m_DrawEnv.b0 = g_Cyclorama.m_BackgroundColor.b;
    g_DB[1].m_DrawEnv.r0 = g_Cyclorama.m_BackgroundColor.r;
    g_DB[1].m_DrawEnv.g0 = g_Cyclorama.m_BackgroundColor.g;
    g_DB[1].m_DrawEnv.b0 = g_Cyclorama.m_BackgroundColor.b;
    for (i = 0; i < g_CreditsMobyCount; ++i) {
      CreditsMoby *mb = &g_CreditsMobys[i];
      g_HudMobys--;
      Memset(g_HudMobys, 0, sizeof(Moby));

      g_HudMobys->m_Position.x = mb->m_Position.x;
      g_HudMobys->m_Position.y = mb->m_Position.y;
      g_HudMobys->m_Position.z = mb->m_Position.z;

      g_HudMobys->m_Rotation.x = mb->m_Rotation.x;
      if (mb->m_MobyClass == MOBYCLASS_SLASH) {
        g_HudMobys->m_Rotation.x += 0x29;
        g_HudMobys->m_Position.z = 0x1C00;
      }
      g_HudMobys->m_Rotation.y = mb->m_Rotation.y;
      g_HudMobys->m_Rotation.z = mb->m_Rotation.z;

      g_HudMobys->m_Class = mb->m_MobyClass;
      g_HudMobys->m_SpecularMetalType = 11;
      g_HudMobys->m_RenderRadius = 0xFF;
      g_HudMobys->m_DepthOffset = 0x7F;
    }

    if (g_Fade)
      func_800190D4(2, g_Fade << 4, g_Fade << 4, g_Fade << 4);

    g_SonyImage.m_ShadedMobys[0] = nullptr;
    func_80018880(); // copy to shaded mobys
    Memset(g_SonyImage.u.m_Draw.m_Moby, 0, sizeof(g_SonyImage.u.m_Draw.m_Moby));
    func_80022A2C(); // render shaded mobys

    Memset16(g_SonyImage.u.m_Buf, 0, sizeof(g_SonyImage.u.m_Buf));
    g_Environment.m_CullingDistance = 0x28000;
    func_800258F0(-1);
    func_8004EBA8(-1, &g_Camera.m_ViewMatrix, &g_Camera.m_ProjectionMatrix);
    DrawSync(0);

    if (D_80075784)
      VSync(0);

    D_80075950.pre = VSync(-1);

    while (D_80075950.pre - D_80075950.post < 2) {
      VSync(0);
      D_80075950.pre = VSync(-1);
    }

    D_80075950.post = VSync(-1);

    PutDispEnv(&g_CurDB->m_DispEnv);
    PutDrawEnv(&g_CurDB->m_DrawEnv);
    DrawOTag(func_80016784(0x800));
  }
}

void func_credits_8007C338(void) {}
