#ifndef __CREDITS_H_
#define __CREDITS_H_

#include "vector.h"

void CreditsUpdate(void);
void CreditsDraw(void);
void func_credits_8007C338(void);

extern int g_CreditsMobyCount; // todo credits moby count

typedef struct {
  char* m_Ptr;
  short unk_0x04;
  short m_Timer;
  short m_Timer2;
  short unk_0x0A;
} CreditsStringEntry;

typedef struct {
  int m_Length;
  int m_DataOffset;
  int m_Count;
  //CreditsStringEntry[m_Count]
  //followed by null term strings
} CreditsStrings;

extern CreditsStrings *g_CreditsStrings;

extern CreditsStringEntry *g_CreditsStringEntries;

typedef struct {
  char unk_0x0[4];
  char unk_0x4;
  Vector3D8 m_Rotation;
  u_char rand_0x8;
  u_char rand_0x9;
  u_char rand_0xa;
  u_char unk_0xb;
  short unk_0xc;
  short unk_0xe;
  short unk_0x10;
  Vector3D16 m_Position;
  short x_displayEntry;
  short m_MobyClass;
} CreditsMoby;

extern CreditsMoby *g_CreditsMobys;

#endif