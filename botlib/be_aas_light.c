/*
 * be_aas_light.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x1000D450..0x1000D770.
 */

#include "botlib_port.h"
#include "l_libvar.h"
#undef VectorNegate
#include "be_ea.h"
#include "q2files.h"
#include "aasfile.h"
#include "be_aas_def.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "be_ai_def.h"
#include "be_interface.h"
#include "struct_sizes_asserts.h"
#include "be_aas_light.h"
#include "be_aas_bspq2.h"
#include "be_aas_main.h"
#include "be_interface.h"
#include "l_memory.h"

// gladiator.dll: 1000D450..1000D487
// gladi386.so:   00017CAC..00017CFE
bsp_pointlight_t *sub_1000D450()
{
  bsp_pointlight_t *v0; // esi

  v0 = aasworld.oldestcache;
  if ( aasworld.oldestcache )
  {
    aasworld.oldestcache = aasworld.oldestcache->next;
    if ( aasworld.oldestcache )
      aasworld.oldestcache->prev = 0;
  }
  if ( !v0 )
    botimport.Print(PRT_MESSAGE, "WARNING: empty light heap\n");
  return v0;
}
// gladiator.dll: 1000D4A0..1000D4C7
// gladi386.so:   00017D00..00017D42
void __cdecl sub_1000D4A0(bsp_pointlight_t *a1)
{
  if ( aasworld.oldestcache )
    aasworld.oldestcache->prev = a1;
  a1->prev = 0;
  a1->next = aasworld.oldestcache;
  aasworld.oldestcache = a1;
}
// gladiator.dll: 1000D4E0..1000D52E
// gladi386.so:   00017D44..00017DE0
void __cdecl sub_1000D4E0(float a1)
{
  bsp_pointlight_t *v1; // ecx
  bsp_pointlight_t *v2; // esi
  bsp_pointlight_t *v3; // eax

  v1 = aasworld.newestcache;
  if ( aasworld.newestcache )
  {
    do
    {
      v2 = v1->next;
      if ( v1->endtime < a1 )
      {
        if ( v2 )
          v2->prev = v1->prev;
        v3 = v1->prev;
        if ( v3 )
          v3->next = v1->next;
        else
          aasworld.newestcache = v1->next;
        sub_1000D4A0(v1);
      }
      v1 = v2;
    }
    while ( v2 );
  }
}
// gladiator.dll: 1000D550..1000D5CE
// gladi386.so:   00017DE0..00017EBF
int __cdecl BotAddPointLight(vec3_t origin, int ent, float radius, float r, float g, float b, float time, float decay)
{
  bsp_pointlight_t *v8; // esi

  v8 = sub_1000D450();
  if ( !v8 )
    return 0;
  /* Faithful quirk: copies the free-list entry's stale xyz back into the
   * caller's origin buffer. */
  VectorCopy(v8->origin, origin);
  v8->ent       = ent;
  v8->radius    = radius;
  v8->color[0]  = r;
  v8->color[1]  = g;
  v8->color[2]  = b;
  v8->endtime   = time;
  v8->decay     = decay;
  v8->starttime = AAS_Time();
  v8->prev      = 0;
  v8->next      = aasworld.newestcache;
  if ( aasworld.newestcache )
    aasworld.newestcache->prev = v8;
  aasworld.newestcache = v8;
  return 0;
}
// gladiator.dll: 1000D5F0..1000D718
// gladi386.so:   00017EC0..00018046
/* Lightmap RGB at the impact point of (start..end), plus the BotAddPointLight
 * cache contribution.  Returns (R+G+B)/3 plus accumulated radius slack. */
int __cdecl AAS_BSPTraceLight(intptr_t start, intptr_t end, intptr_t endpos, int *red, int *green, int *blue)
{
  float *v6; // ebx
  int v7; // edi
  bsp_pointlight_t *v8; // esi
  float v9; // st7
  vec3_t v12; // [esp+10h] [ebp-Ch] BYREF
  /* 32-bit keeps the original's no-init form; 64-bit must zero-init because
   * sub_10007150 leaves these unwritten when there is no BSP light data. */
#if BOTLIB_NEED_SIDEBAND
  int rs = 0, gs = 0, bs_ = 0;
#else
  int rs, gs, bs_;
#endif

  v6 = (float *)endpos;
  if ( sub_10007150(start, end, endpos, (_DWORD *)&rs, (_DWORD *)&gs, (_DWORD *)&bs_) )
    v7 = (rs + gs + bs_) / 3;
  else
    v7 = 255;
  v8 = aasworld.newestcache;
  for ( ; v8; v8 = v8->next )
  {
    VectorSubtract(v6, v8->origin, v12);
    v9 = v8->radius - VectorLength(v12);
    if ( v9 > 0.0f )
    {
      v7 = (int)((float)v7 + v9);
      rs  = (int)((float)rs  + v8->color[0]);
      gs  = (int)((float)gs  + v8->color[1]);
      bs_ = (int)((float)bs_ + v8->color[2]);
    }
  }
  if ( red )
    *red = rs;
  if ( green )
    *green = gs;
  if ( blue )
    *blue = bs_;
  return v7;
}
// gladiator.dll: 1000D770..1000D7B4
// gladi386.so:   00018048..000181F8
/* Lightmap intensity at `origin`, traced 4096 units straight down.  The channel
 * out-pointers may be NULL. */
int __cdecl AAS_PointLight(float *origin, int *red, int *green, int *blue)
{
  vec3_t v7; // [esp+0h] [ebp-18h] BYREF
  char v8[12]; // [esp+Ch] [ebp-Ch] BYREF

  VectorCopy(origin, v7);
  v7[2] = origin[2] - 4096.0f;
  return AAS_BSPTraceLight((intptr_t)origin, (intptr_t)v7, (intptr_t)v8, red, green, blue);
}
