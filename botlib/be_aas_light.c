/*
 * be_aas_light.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x1000D450..0x1000D770; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * Its own interface is in the matching .h; botlib_local.h, which that header
 * pulls in, carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs, the side-band scheme and the externs for
 * botlib.c's remaining globals).
 */

#include "be_aas_light.h"
#include "be_aas_main.h"

//----- (1000D450) --------------------------------------------------------
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
//----- (1000D4A0) --------------------------------------------------------
bsp_pointlight_t *__cdecl sub_1000D4A0(bsp_pointlight_t *a1)
{
  bsp_pointlight_t *result; // eax

  result = a1;
  if ( aasworld.oldestcache )
    aasworld.oldestcache->prev = a1;
  a1->prev = 0;
  a1->next = aasworld.oldestcache;
  aasworld.oldestcache = a1;
  return result;
}
//----- (1000D4E0) --------------------------------------------------------
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
//----- (1000D550) --------------------------------------------------------
int __cdecl BotAddPointLight(vec3_t origin, int ent, float radius, float r, float g, float b, float time, float decay)
{
  bsp_pointlight_t *v8; // esi

  v8 = sub_1000D450();
  if ( v8 )
  {
    /* Quirk preserved: the original copies the free-list entry's stale xyz back
     * into the caller's origin buffer. */
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
  }
  return 0;
}
//----- (1000D5F0) --------------------------------------------------------
/* AAS_BSPTraceLight — trace (start..end) through the BSP, returning the
 * lightmap RGB at the impact point in *red/*green/*blue and the position in
 * *endpos.  After the static trace it walks the BotAddPointLight cache
 * (aasworld.newestcache), adding each light whose radius exceeds its distance
 * to endpos.  Returns (R+G+B)/3 plus accumulated radius slack — the "how lit
 * is this point" scalar callers like BotEntityVisible use.
 *
 * Q3 stubs this out in be_aas_bspq3.c; this Q2 implementation is the original. */
int __cdecl AAS_BSPTraceLight(intptr_t start, intptr_t end, intptr_t endpos, int *red, int *green, int *blue)
{
  float *v6; // ebx
  int v7; // edi
  bsp_pointlight_t *v8; // esi
  float v9; // st7
  int i; // [esp+Ch] [ebp-10h]
  vec3_t v12; // [esp+10h] [ebp-Ch] BYREF
  /* Dedicated int locals for sub_10007150's RGB outputs, since the parameter
   * slots are intptr_t here.  32-bit keeps the original's no-init form; 64-bit
   * zero-inits, because sub_10007150 leaves them unwritten when there is no BSP
   * light data. */
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
  for ( i = v7; v8; v8 = v8->next )
  {
    VectorSubtract(v6, v8->origin, v12);
    v9 = v8->radius - VectorLength(v12);
    if ( v9 > 0.0f )
    {
      v7 = (__int64)((float)i + v9);
      i = v7;
      rs  = (__int64)((float)rs  + v8->color[0]);
      gs  = (__int64)((float)gs  + v8->color[1]);
      bs_ = (__int64)((float)bs_ + v8->color[2]);
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
//----- (1000D770) --------------------------------------------------------
/* AAS_PointLight — lightmap intensity at `origin`, from an AAS_BSPTraceLight
 * 4096 units straight down.  The three channel out-pointers may be NULL when
 * only the average-RGB return value is wanted. */
int __cdecl AAS_PointLight(float *origin, int *red, int *green, int *blue)
{
  /* int[3]: raw bit copies for X/Y, float bit pattern stored at [2]. */
  int v7[3]; // [esp+0h] [ebp-18h] BYREF
  char v8[12]; // [esp+Ch] [ebp-Ch] BYREF

  v7[0] = *(_DWORD *)&origin[0];
  v7[1] = *(_DWORD *)&origin[1];
  *(float *)&v7[2] = origin[2] - 4096.0f;
  return AAS_BSPTraceLight((intptr_t)origin, (intptr_t)v7, (intptr_t)v8, red, green, blue);
}
