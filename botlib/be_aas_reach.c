/*
 * be_aas_reach.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10010F60..0x10018C70.
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
#include "be_aas_reach.h"
#include "be_aas_bspq2.h"
#include "be_aas_entity.h"
#include "be_aas_main.h"
#include "be_aas_move.h"
#include "be_aas_sample.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"

/* STATIC, and the two names were swapped.  Real reaches both of these GOTOFF
 * (0x5d558 / 0x5d55c, past the end of the GOT and inside .bss) with no
 * `R_386_GLOB_DAT` and no .dynsym entry, which is what a file-static looks
 * like under -fPIC; ours exported them, which is a 2-instruction different
 * access form -- the same class as game/'s LEGACY_STATIC case.  The two names
 * were also swapped against the cvars they cache: the variable called
 * `...reachabilitydelay` held LibVar("framereachability") and was used as the
 * per-cycle AREA COUNT, while `...framereachability` held
 * LibVar("reachability_delay") and was used as the millisecond budget.  Names
 * exchanged; behaviour unchanged.  (globwiring.py, 2026-08-16.) */
static libvar_t *libvar_framereachability;  /* LibVar("framereachability", "20") */
static libvar_t *libvar_reachabilitydelay;  /* LibVar("reachability_delay", "100") */
aas_reachabilitynode_t **areareachability;   /* per-area linked-list-head array */
int reach_ladder; // weak
int reach_elevator; // weak
intptr_t reachabilityheap; // pool base
int reach_jump; // weak
int reach_grapple; // weak
int reach_waterjump; // weak
int reach_teleport; // weak
int reach_barrier; // weak
int reach_swim; // weak
int reach_equalfloor; // weak
intptr_t nextreachability; // free-list head
int reach_walkoffledge; // weak
int reach_rocketjump; // weak
int reach_step; // weak
int reach_walk; // weak
/* The four reachability-type counters Gladiator never increments.  Recovered
 * from gladi386.so's .dynsym, which exports all four as 4-byte .bss OBJECTs
 * (reach_rampjump @0x60918, reach_bfgjump @0x6091c, reach_doublejump
 * @0x60920, reach_strafejump @0x60930) interleaved with the ones above.  They
 * have NO reference anywhere in the real image either -- the generators for
 * those four travel types are the Q3-era ones Gladiator does not have -- so
 * only the name, size and neighbourhood are evidence; the `int` type is by
 * analogy with the eleven siblings, every one of which is a counter here.
 * (globwiring.py / dataaudit.py, 2026-08-16.) */
int reach_rampjump;    // weak -- unreferenced in BOTH images
int reach_bfgjump;     // weak -- unreferenced in BOTH images
int reach_doublejump;  // weak -- unreferenced in BOTH images
int reach_strafejump;  // weak -- unreferenced in BOTH images

// gladiator.dll: 10010F60..10010FA3
// gladi386.so:   0001DAFC..0001DBC0
/* The reach free-list is 65536 fixed-size nodes: 48 B each with the next-ptr at
 * +44 on 32-bit, 56 B with it at +48 on 64-bit.  Keep the original byte-offset
 * walk (so MSVC6 still emits the +44 / stride-48 form) but derive the link slot
 * from offsetof(..,next) and store through intptr_t*.
 *
 * `nextoffset` must stay a genuinely separate loop-carried accumulator even
 * though it moves in lockstep with `offset`: gcc 2.7.2.3 -O6 -funroll-loops
 * unrolls this 5x and keeps a distinct register for it, and MSVC6 /O2 folds the
 * redundancy back out, so the split is free on PE and required on ELF.
 *
 * Do NOT add a third variable (a 0..65535 node index decoupled from the byte
 * offsets).  It closes the last few ELF insns, but MSVC6 then recognises a small
 * fixed trip count and rewrites the loop into a countdown needing an extra
 * callee-saved register — an unavoidable PE regression.  The residual ELF gap is
 * the accepted cost. */
#define AAS_REACHABILITYHEAP_NODES 65536

int AAS_SetupReachabilityHeap()
{
  intptr_t result;
  int offset, nextoffset;

  result = (intptr_t)GetClearedMemory(AAS_REACHABILITYHEAP_NODES * sizeof(aas_reachabilitynode_t));
  reachabilityheap = result;
  nextoffset = (int)sizeof(aas_reachabilitynode_t);
  for ( offset = 0; offset < (AAS_REACHABILITYHEAP_NODES - 1) * (int)sizeof(aas_reachabilitynode_t); offset += (int)sizeof(aas_reachabilitynode_t) )
  {
    *(intptr_t *)(offset + result + offsetof(aas_reachabilitynode_t, next)) = result + nextoffset;
    nextoffset += (int)sizeof(aas_reachabilitynode_t);
    result = reachabilityheap;
  }
  *(intptr_t *)(reachabilityheap + (AAS_REACHABILITYHEAP_NODES - 1) * (int)sizeof(aas_reachabilitynode_t) + offsetof(aas_reachabilitynode_t, next)) = 0;
  nextreachability = reachabilityheap;
  return result;
}

// gladiator.dll: 10010FD0..10010FDD
// gladi386.so:   0001DBC0..0001DBE0
void AAS_ShutDownReachabilityHeap()
{
  FreeMemory((void *)reachabilityheap);
}
/* Pop a node off the reach free chain, raising AAS_MAX_REACHABILITYSIZE when the
 * successor is NULL.  A DIFFERENT free list from the entity-link one at
 * aasworld.freelinks (16-byte stride, link at +8). */
int numlreachabilities;               /* 0x1006677C reachabilities allocated (be_aas_reach.c; was dword_1006677C) */

// gladiator.dll: 10010FF0..1001102A
// gladi386.so:   0001DBE0..0001DC2E
void *AAS_AllocReachability(void)
{
  aas_reachabilitynode_t *head;

  if ( !nextreachability )
    return NULL;
  if ( !((aas_reachabilitynode_t *)nextreachability)->next )
    AAS_Error("AAS_MAX_REACHABILITYSIZE");
  /* Original re-reads head here in case AAS_Error trashed eax. */
  head = (aas_reachabilitynode_t *)nextreachability;
  nextreachability = (intptr_t)head->next;
  ++numlreachabilities;
  return head;
}

// gladiator.dll: 10011040..10011075
// gladi386.so:   0001DC30..0001DC7C
int __cdecl AAS_AreaReachability(int areanum)
{
  if ( areanum < 0 || areanum >= aasworld.numareas )
  {
    AAS_Error("AAS_AreaReachability: areanum %d out of range", areanum);
    return 0;
  }
  return aasworld.areasettings[areanum].numreachableareas;
}

// gladiator.dll: 10011090..100111D0
// gladi386.so:   0001DC7C..0001DE37
float __cdecl AAS_FaceArea(aas_face_t *face)
{
  int i;
  int edgenum;
  int side;
  float total;
  vec_t *v;
  vec3_t d1;
  vec3_t d2;
  vec3_t cross;
  aas_edge_t *edge;

  edgenum = aasworld.edgeindex[face->firstedge];
  side = edgenum < 0;
  edge = &aasworld.edges[abs(edgenum)];
  v = aasworld.vertexes[edge->v[side]];

  total = 0.0f;
  for ( i = 1; i < face->numedges - 1; i++ )
  {
    edgenum = aasworld.edgeindex[face->firstedge + i];
    side = edgenum < 0;
    edge = &aasworld.edges[abs(edgenum)];
    VectorSubtract(aasworld.vertexes[edge->v[side]], v, d1);
    VectorSubtract(aasworld.vertexes[edge->v[!side]], v, d2);
    CrossProduct(d1, d2, cross);
    total += 0.5 * VectorLength(cross);
  }
  return total;
}

// gladiator.dll: 10011220..10011319
// gladi386.so:   0001DE38..0001DF70
float __cdecl AAS_AreaVolume(int areanum)
{
  aas_area_t *area;
  aas_face_t *face;
  aas_edge_t *edge;
  aas_plane_t *plane;
  int i;
  int facenum;
  int edgenum;
  float d;
  float a;
  vec3_t corner;
  float volume;

  area = &aasworld.areas[areanum];
  facenum = aasworld.faceindex[area->firstface];
  face = &aasworld.faces[abs(facenum)];
  edgenum = aasworld.edgeindex[face->firstedge];
  edge = &aasworld.edges[abs(edgenum)];
  VectorCopy(aasworld.vertexes[edge->v[0]], corner);
  volume = 0.0f;
  for ( i = 0; i < area->numfaces; ++i )
  {
    facenum = abs(aasworld.faceindex[area->firstface + i]);
    face = &aasworld.faces[facenum];
    plane = &aasworld.planes[face->planenum];
    d = -(DotProduct(corner, plane->normal)
          - plane->dist);
    a = AAS_FaceArea(face);
    volume += d * a;
  }
  return volume * 0.33333334f;
}

// gladiator.dll: 10011360..100113CE
// gladi386.so:   0001DF70..0001DFF0
float __cdecl AAS_AreaGroundFaceArea(int areanum)
{
  float total; // st7
  int i; // edi
  aas_area_t *area; // esi
  aas_face_t *face;

  total = 0.0f;
  area = &aasworld.areas[areanum];
  for ( i = 0; i < area->numfaces; i++ )
  {
    face = &aasworld.faces[abs(aasworld.faceindex[area->firstface + i])];
    if ( !(face->faceflags & 4) )
      continue;
    total += AAS_FaceArea(face);
  }
  return total;
}

// gladiator.dll: 100113F0..100114D7
// gladi386.so:   0001DFF0..0001E12B
void __cdecl AAS_FaceCenter(int facenum, vec3_t center)
{
  int i; // esi
  aas_face_t *face; // edi
  aas_edge_t *edge;
  float scale; // [esp+0h] [ebp-10h]

  face = &aasworld.faces[facenum];
  VectorClear(center);
  for ( i = 0; i < face->numedges; i++ )
  {
    edge = &aasworld.edges[abs(aasworld.edgeindex[face->firstedge + i])];
    VectorAdd(center, aasworld.vertexes[edge->v[0]], center);
    VectorAdd(center, aasworld.vertexes[edge->v[1]], center);
  }
  scale = 0.5 / face->numedges;
  VectorScale((float *)center, scale, (float *)center);
}

// gladiator.dll: 10011520..10011546
// gladi386.so:   0001E12C..0001E18E
int AAS_FallDamageDistance()
{
  float maxzvelocity, gravity, t;

  maxzvelocity = sqrt(30 * 10000);
  gravity = libvar_sv_gravity->value;
  t = maxzvelocity / gravity;
  return 0.5 * gravity * t * t;
}

// gladiator.dll: 10011560..1001157F
// gladi386.so:   0001E190..0001E1CD
float __cdecl AAS_MaxJumpHeight(float phys_jumpvel)
{
  float phys_gravity;

  phys_gravity = libvar_sv_gravity->value;
  return 0.5 * phys_gravity * (phys_jumpvel / phys_gravity) * (phys_jumpvel / phys_gravity);
}

// gladiator.dll: 10011590..100115BC
// gladi386.so:   0001E1D0..0001E223
float __cdecl AAS_MaxJumpDistance(float phys_jumpvel)
{
  float phys_gravity, phys_maxvelocity, t;

  phys_gravity = libvar_sv_gravity->value;
  phys_maxvelocity = libvar_sv_maxvelocity->value;
  t = sqrt(450.0 / (0.5 * phys_gravity));
  return phys_maxvelocity * (t + phys_jumpvel / phys_gravity);
}

// gladiator.dll: 100115D0..100115F2
// gladi386.so:   0001E224..0001E25B
int __cdecl AAS_AreaCrouch(int areanum)
{
  if ( !(aasworld.areasettings[areanum].presencetype & 2) )
    return 1;
  else
    return 0;
}

// gladiator.dll: 10011610..1001162F
// gladi386.so:   0001E25C..0001E293
int __cdecl AAS_AreaSwim(int areanum)
{
  /* Keep the if/else 0-or-1 form (Q3's `if (areaflags & AREA_LIQUID)`): MSVC's
   * bit-test idiom emits a byte-narrowed test, whereas an arithmetic `(x&4)>>2`
   * gets reassociated to `(x>>2)&1` on a full DWORD load. */
  if ( aasworld.areasettings[areanum].areaflags & 4 )
    return 1;
  else
    return 0;
}

// gladiator.dll: 10011640..1001165F
// gladi386.so:   0001E294..0001E2CB
/* Q3's AAS_AreaLiquid — a byte-identical duplicate of AAS_AreaSwim above, as
 * in be_aas_reach.c, where both share the same AREA_LIQUID body. */
int __cdecl AAS_AreaLiquid(int areanum)
{
  if ( aasworld.areasettings[areanum].areaflags & 4 )
    return 1;
  else
    return 0;
}

// gladiator.dll: 10011670..1001168B
// gladi386.so:   0001E2CC..0001E2FA
int __cdecl AAS_AreaGrounded(int areanum)
{
  return aasworld.areasettings[areanum].areaflags & 1;
}

// gladiator.dll: 100116A0..100116BB
// gladi386.so:   0001E2FC..0001E32A
int __cdecl AAS_AreaLadder(int areanum)
{
  return aasworld.areasettings[areanum].areaflags & 2;
}

// gladiator.dll: 100116D0..100116EE
// gladi386.so:   0001E32C..0001E37E
/* Returns (int)(10 * sv_jumpvel / sv_gravity) — a crude jump/hang-time
 * estimate in tics.  DEAD in Gladiator, preserved by /INCREMENTAL. */
unsigned __int16 __cdecl sub_100116D0(void)
{
  return libvar_sv_jumpvel->value / (libvar_sv_gravity->value * 0.1);
}

// gladiator.dll: 10011700..10011729
// gladi386.so:   0001E380..0001E3BB
qboolean __cdecl AAS_ReachabilityExists(int area1num, int area2num)
{
  aas_reachabilitynode_t *r;

  r = areareachability[area1num];
  while ( r )
  {
    if ( *(_DWORD *)r == area2num )
      return 1;
    r = r->next;
  }
  return 0;
}

// gladiator.dll: 10011740..10011814
// gladi386.so:   0001E3BC..0001E4B3
BOOL __cdecl AAS_NearbySolidOrGap(vec3_t start, vec3_t end)
{
  int areanum; // eax
  /* One vec3_t, so VectorMA's writes land in the same slot the original bumps by
   * 16.0 after the first AAS_PointAreaNum. */
  vec3_t dir; // [esp+10h] [ebp-Ch] BYREF
  vec3_t testpoint; // [esp+4h] [ebp-18h] BYREF

  dir[0] = *end - *start;
  dir[1] = end[1] - start[1];
  dir[2] = 0;
  VectorNormalize(dir);
  VectorMA(end, 48.0f, dir, testpoint);
  areanum = AAS_PointAreaNum(testpoint);
  if ( !areanum )
  {
    testpoint[2] = testpoint[2] + 16.0f;
    areanum = AAS_PointAreaNum(testpoint);
    if ( !areanum )
      return 1;
  }
  VectorMA(end, 64.0, dir, testpoint);
  areanum = AAS_PointAreaNum(testpoint);
  if ( areanum && !AAS_AreaSwim(areanum) && !AAS_AreaGrounded(areanum) )
    return 1;
  return 0;
}

// gladiator.dll: 10011860..10011A51
// gladi386.so:   0001E4B4..0001E778
int __cdecl AAS_Reachability_Swim(int area1num, int area2num)
{
  int i;
  int j;
  int face1num;
  int face2num;
  int side1;
  aas_area_t *area1;
  aas_area_t *area2;
  aas_areasettings_t *areasettings;
  aas_reachabilitynode_t *lreach;
  aas_face_t *face1;
  aas_plane_t *plane;
  vec3_t start;

  if ( !AAS_AreaSwim(area1num) || !AAS_AreaSwim(area2num) )
    return 0;
  if ( (aasworld.areasettings[area2num].presencetype & 2) == 0 )
    return 0;
  area1 = &aasworld.areas[area1num];
  area2 = &aasworld.areas[area2num];
  for ( i = 0; i < 3; ++i )
  {
    if ( area1->mins[i] > area2->maxs[i] + 10.0f )
      return 0;
    if ( area1->maxs[i] < area2->mins[i] - 10.0f )
      return 0;
  }
  for ( i = 0; i < area1->numfaces; ++i )
  {
    face1num = aasworld.faceindex[area1->firstface + i];
    side1 = face1num < 0;
    face1num = abs(face1num);
    for ( j = 0; j < area2->numfaces; ++j )
    {
      face2num = abs(aasworld.faceindex[area2->firstface + j]);
      if ( face1num == face2num )
      {
        AAS_FaceCenter(face1num, start);
        if ( sub_10003080(start) & 0x38 )   /* water-edge contents check */
        {
          face1 = &aasworld.faces[face1num];
          areasettings = &aasworld.areasettings[area1num];
          lreach = (aas_reachabilitynode_t *)AAS_AllocReachability();
          if ( !lreach )
            return 0;
          lreach->reach.areanum = area2num;
          lreach->reach.facenum = face1num;
          lreach->reach.edgenum = 0;
          VectorCopy(start, lreach->reach.start);
          /* Indexed, not float* arithmetic — aas_plane_t's stride is 20 bytes. */
          plane = &aasworld.planes[face1->planenum ^ side1];
          VectorMA(lreach->reach.start, 2.0f, plane->normal, lreach->reach.end);
          lreach->reach.traveltype = 8;
          lreach->reach.traveltime = 1;
          if ( AAS_AreaVolume(area2num) < 800.0f )
            lreach->reach.traveltime += 200;
          lreach->next = areareachability[area1num];
          areareachability[area1num] = lreach;
          ++reach_swim;
          return 1;
        }
      }
    }
  }
  return 0;
}

// gladiator.dll: 10011AE0..10012081
// gladi386.so:   0001E778..0001EF99
int __cdecl AAS_Reachability_EqualFloorHeight(int area1num, int area2num)
{
  int i;
  int j;
  int edgenum;
  int edgenum1;
  int edgenum2;
  int foundreach;
  int side;
  float height;
  float bestheight;
  float length;
  float bestlength;
  vec3_t start;
  vec3_t end;
  vec3_t normal;
  vec3_t invgravity;
  vec3_t down = { 0, 0, -1 };
  vec3_t edgevec;
  aas_area_t *area1;
  aas_area_t *area2;
  aas_face_t *face1;
  aas_face_t *face2;
  aas_edge_t *edge;
  aas_plane_t *plane2;
  /* Q3's local aas_lreachability_t included the next pointer; keeping the
   * 48-byte node shape preserves the original 32-bit frame layout here. */
  aas_reachabilitynode_t lr;
  aas_reachabilitynode_t *lreach;

  if ( !AAS_AreaGrounded(area1num) || !AAS_AreaGrounded(area2num) )
    return 0;

  area1 = &aasworld.areas[area1num];
  area2 = &aasworld.areas[area2num];

  for ( i = 0; i < 2; ++i )
  {
    if ( area1->mins[i] > area2->maxs[i] + 10.0f )
      return 0;
    if ( area1->maxs[i] < area2->mins[i] - 10.0f )
      return 0;
  }

  if ( area2->mins[2] > area1->maxs[2] )
    return 0;

  VectorCopy(down, invgravity);
  VectorNegate(invgravity);

  bestheight = 99999.0f;
  bestlength = 0.0f;
  foundreach = 0;

  for ( i = 0; i < area1->numfaces; ++i )
  {
    face1 = &aasworld.faces[abs(aasworld.faceindex[area1->firstface + i])];
    if ( !(face1->faceflags & 4) )
      continue;

    for ( j = 0; j < area2->numfaces; ++j )
    {
      face2 = &aasworld.faces[abs(aasworld.faceindex[area2->firstface + j])];
      if ( !(face2->faceflags & 4) )
        continue;

      for ( edgenum1 = 0; edgenum1 < face1->numedges; ++edgenum1 )
      {
        for ( edgenum2 = 0; edgenum2 < face2->numedges; ++edgenum2 )
        {
          if ( abs(aasworld.edgeindex[face1->firstedge + edgenum1]) !=
               abs(aasworld.edgeindex[face2->firstedge + edgenum2]) )
            continue;

          edgenum = aasworld.edgeindex[face1->firstedge + edgenum1];
          side = edgenum < 0;
          edge = &aasworld.edges[abs(edgenum)];

          VectorAdd(aasworld.vertexes[edge->v[0]],
                    aasworld.vertexes[edge->v[1]], start);
          length = VectorLength(start);
          VectorScale(start, 0.5f, start);
          VectorCopy(start, end);

          VectorSubtract(aasworld.vertexes[edge->v[side]],
                         aasworld.vertexes[edge->v[!side]], edgevec);
          plane2 = &aasworld.planes[face2->planenum];
          CrossProduct(edgevec, plane2->normal, normal);
          VectorNormalize(normal);
          VectorMA(end, 5.0f, normal, end);
          VectorMA(start, 0.1f, normal, start);
          end[2] += 0.125;

          height = DotProduct(invgravity, start);
          if ( !AAS_NearbySolidOrGap(start, end) )
            height += 200.0f;

          if ( height < bestheight || (height < bestheight + 1.0f && length > bestlength) )
          {
            bestheight = height;
            bestlength = length;
            lr.reach.areanum = area2num;
            lr.reach.facenum = 0;
            lr.reach.edgenum = edgenum;
            VectorCopy(start, lr.reach.start);
            VectorCopy(end, lr.reach.end);
            lr.reach.traveltype = 2;
            lr.reach.traveltime = 1;
            foundreach = 1;
          }
        }
      }
    }
  }

  /* Q3's `if (foundreach) { … return qtrue; } return qfalse;` — the trailing
   * `return 0` is its own exit block in the original.  Written as the negative
   * guard `if (!foundreach) return 0;` gcc cross-jumps that exit into the
   * `!lreach` one and the function comes out 7 instructions short. */
  if ( foundreach )
  {
    lreach = (aas_reachabilitynode_t *)AAS_AllocReachability();
    if ( !lreach )
      return 0;

    lreach->reach.areanum = lr.reach.areanum;
    lreach->reach.facenum = lr.reach.facenum;
    lreach->reach.edgenum = lr.reach.edgenum;
    VectorCopy(lr.reach.start, lreach->reach.start);
    VectorCopy(lr.reach.end, lreach->reach.end);
    lreach->reach.traveltype = lr.reach.traveltype;
    lreach->reach.traveltime = lr.reach.traveltime;
    lreach->next = areareachability[area1num];
    areareachability[area1num] = lreach;

    if ( !AAS_AreaCrouch(area1num) && AAS_AreaCrouch(area2num) )
      lreach->reach.traveltime += 300;
    if ( !AAS_NearbySolidOrGap(lreach->reach.start, lreach->reach.end) )
      lreach->reach.traveltime += 100;
    if ( AAS_AreaGroundFaceArea(lreach->reach.areanum) < 500.0f )
      lreach->reach.traveltime += 100;

    ++reach_equalfloor;
    return 1;
  }
  return 0;
}

// gladiator.dll: 10012200..1001367A
// gladi386.so:   0001EF9C..00020BB5
int __cdecl AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge(int area1num, int area2num)
{
  aas_area_t *area1; // ebp
  int v3; // eax
  int v4; // edx
  int i; // [esp+F4h] [ebp-DCh]
  aas_face_t *groundface1; // ebp
  int j; // ebp
  int k; // [esp+ECh] [ebp-E4h]
  int v13; // esi
  aas_edge_t *edge1; // ecx
  int v18; // ebx
  aas_area_t *v19; // eax — base pointer alias of area2 (was int, must hold 64-bit ptr)
  int v20; // rax — int + abs(); see asm_matching/idioms
  aas_face_t *groundface2; // edi
  int edge1num; // eax
  int edge2num; // rax — int + abs()
  aas_edge_t *edge2; // ecx
  /* Q3 declares this; IDA lost it because both compilers CSE'd the subscript into
   * an address they already had.  Restoring it stops gcc272 re-evaluating
   * `planes[planenum ^ !faceside1]` once per dot-product term. */
  aas_plane_t *plane;
  /* The original keeps these on the x87 stack at 80-bit, so they are long
   * double with explicit operand casts in the chain expressions. */
  float v25; // st7
  int ground_bestarea2groundedgenum; // [esp+DCh] [ebp-F4h]
  float v28; // st7
  int ground_foundreach; // [esp+E8h] [ebp-E8h]
  aas_reachabilitynode_t *v44; // eax
  int v46; // ecx
  aas_reachabilitynode_t *v50; // esi
  aas_reachabilitynode_t *v54; // eax
  aas_reachabilitynode_t *v56; // esi
  char *v57; // eax
  int v58; // edx
  int water_foundreach; // [esp+F0h] [ebp-E0h]
  int side1; // edi
  int area1swim; // [esp+13Ch] [ebp-94h]
  /* NOTHING at [esp+118h]: IDA's `aas_face_t *v126` there was MSVC's SPILL SLOT
   * for `groundface1`, not a source variable.  Q3 declares no such local, and
   * carrying it made our frame 4 bytes too big in BOTH originals.  Dropping it
   * makes both frame sizes exact (PE 0x1c0, ELF 0x1f4).
   * DO NOT REINTRODUCE IT.  The PE's differing-line count goes UP because MSVC6
   * then permutes the frame differently, but instruction parity is untouched at
   * 1237/1237 and that permutation is unreachable from C.  A deliberate
   * fidelity-over-byte-metric deviation. */
  int faceside1; // [esp+120h] [ebp-B0h]
  int groundface1num; // eax
  float dist; // [esp+24h] [ebp-1ACh]
  float dist1; // st7
  float v66; // [esp+28h] [ebp-1A8h]
  float dist2; // [esp+10h] [ebp-1C0h]
  float ortdot; // st7
  float x1; // [esp+14h] [ebp-1BCh]
  float x2; // [esp+10h] [ebp-1C0h]
  float x3; // [esp+18h] [ebp-1B8h]
  vec3_t edgev1; // canonical Q3 edge endpoint locals
  vec3_t edgev2;
  vec3_t edgev3;
  float x4; // [esp+1Ch] [ebp-1B4h]
  vec3_t edgev4;
  vec3_t tmpv;
  float y1; // [esp+60h] [ebp-170h]
  int v114; // [esp+E0h] [ebp-F0h]
  aas_area_t *area2; // [esp+E4h] [ebp-ECh] — second area's char* base (was int — truncates ptr)
  float y2; // [esp+80h] [ebp-150h]
  float y3; // [esp+5Ch] [ebp-174h]
  float y4; // [esp+64h] [ebp-16Ch]
  float length; // [esp+28h] [ebp-1A8h]
  vec3_t p1area1;
  vec3_t p1area2;
  float ground_bestlength; // [esp+110h] [ebp-C0h]
  float water_bestlength; // [esp+114h] [ebp-BCh]
  float ground_bestdist; // [esp+20h] [ebp-1B0h]
  vec3_t p2area1;
  float water_bestdist; // [esp+A8h] [ebp-128h]
  vec3_t p2area2;
  /* vec3_t: CrossProduct's destination. */
  vec3_t normal; // [esp+ACh] [ebp-124h] BYREF — was v101/v102/v103 mixed triplet
  /* vec3_t: CrossProduct writes 3 contiguous floats here. */
  vec3_t ort; // [esp+9Ch] [ebp-134h] BYREF
  /* vec3_t: VectorMA's destination and the AAS_PointAreaNum trace point — the z
   * decrement below has to land in the same slot. */
  float edgevec[3]; // [esp+17Ch] [ebp-54h] BYREF
  /* vec3_t: passed by reference to VectorScale, which writes 3 floats. */
  vec3_t start; // [esp+84h] [ebp-14Ch] BYREF — was start/v92/v93 int triplet
  /* Same root cause as start/v92/v93 — VectorScale destination. */
  vec3_t end; // [esp+90h] [ebp-140h] BYREF — was end/v95/v96 int triplet
  float dir[3]; // [esp+164h] [ebp-6Ch] BYREF
  /* vec3_t: VectorMA's destination. */
  vec3_t ground_beststart; // [esp+104h] [ebp-CCh] BYREF — was ground_beststart/v122/v123 mixed triplet
  /* vec3_t: VectorMA's destination. */
  vec3_t ground_bestend; // [esp+D0h] [ebp-100h] BYREF — was ground_bestend/v111/v112 mixed triplet
  float ground_bestnormal[3]; // [esp+F8h] [ebp-D8h] BYREF
  vec3_t water_beststart; // [esp+140h..148h] [ebp-90h..-88h] — one contiguous vec3, as Q3 declares it
  float water_bestend[3]; // [esp+14Ch] [ebp-84h] BYREF
  float water_bestnormal[3]; // [esp+158h] [ebp-78h] BYREF
  vec3_t up; // [esp+130h] [ebp-A0h] BYREF — world up axis (0,0,1) for CrossProduct
  vec3_t testpoint; // [esp+170h] [ebp-60h] BYREF — was int testpoint[2] + float v142
  aas_trace_t trace; // [esp+188h] [ebp-48h] (was int v144[9] + char v145[36] hidden return buffer)

  up[0] = 0.0f;
  up[1] = 0.0f;
  up[2] = 1.0f;
  if ( !AAS_AreaGrounded(area1num) && !AAS_AreaSwim(area1num) )
    return 0;
  if ( !AAS_AreaGrounded(area2num) && !AAS_AreaSwim(area2num) )
    return 0;
  {
    area1 = &aasworld.areas[area1num];
    area2 = &aasworld.areas[area2num];
    v3 = AAS_AreaSwim(area1num);
    area1swim = v3;
    /* if the areas are not near anough in the x-y direction */
    for ( v4 = 0; v4 < 2; ++v4 )
    {
      if ( area1->mins[v4] > area2->maxs[v4] + 10.0f )
        return 0;
      if ( area1->maxs[v4] < area2->mins[v4] - 10.0f )
        return 0;
    }
    {
      {
        ground_foundreach = 0;
        ground_bestdist = 99999.0f;
        ground_bestlength = 0.0f;
        water_foundreach = 0;
        water_bestdist = 99999.0f;
        water_bestlength = 0.0f;
        for ( i = 0; i < area1->numfaces; ++i )
        {
          groundface1num = aasworld.faceindex[i + area1->firstface];
          faceside1 = groundface1num < 0;
          groundface1 = &aasworld.faces[abs(groundface1num)];
          /* Comma expression, not a hoisted statement: both originals compute the
           * plane address only after `area1swim` short-circuits, so the assignment
           * has to stay inside the `&&`. */
          if ( (groundface1->faceflags & 4) != 0
            || area1swim
            && (plane = &aasworld.planes[groundface1->planenum ^ (!faceside1)],
                DotProduct(plane->normal, up)) >= 0.7 )
          {
            for ( k = 0; k < groundface1->numedges; ++k )
            {
                edge1num = aasworld.edgeindex[k + groundface1->firstedge];
                side1 = edge1num < 0;
                if ( (groundface1->faceflags & 4) == 0 )
                  side1 = side1 == faceside1;
                v13 = abs(edge1num);
                edge1 = &aasworld.edges[v13];
                VectorCopy(aasworld.vertexes[edge1->v[!side1]], edgev1);
                VectorCopy(aasworld.vertexes[edge1->v[side1]], edgev2);
                VectorSubtract(edgev2, edgev1, edgevec);
                CrossProduct(edgevec, up, normal);
                VectorNormalize(normal);
                v18 = 0;
                dist = DotProduct(normal, edgev1);
                if ( area2->numfaces > 0 )
                {
                  v19 = area2;
                  do
                  {
                    v20 = aasworld.faceindex[v18 + v19->firstface];
                    groundface2 = &aasworld.faces[abs(v20)];
                    if ( (groundface2->faceflags & 4) != 0 )
                    {
                      for ( j = 0; j < groundface2->numedges; ++j )
                      {
                        edge2num = aasworld.edgeindex[j + groundface2->firstedge];
                        edge2 = &aasworld.edges[abs(edge2num)];
                        VectorCopy(aasworld.vertexes[edge2->v[0]], edgev3);
                        VectorCopy(aasworld.vertexes[edge2->v[1]], edgev4);
                        v25 = DotProduct(normal, edgev3) - dist;
                        if ( v25 >= -0.1 && v25 <= 0.1 )
                        {
                          v25 = DotProduct(normal, edgev4) - dist;
                          if ( v25 >= -0.1 && v25 <= 0.1 )
                          {
                            CrossProduct(up, normal, ort);
                            ortdot = DotProduct(ort, ort);
                            y1 = edgev1[2];
                            y2 = edgev2[2];
                            y3 = edgev3[2];
                            y4 = edgev4[2];
                            x1 = DotProduct(edgev1, ort) / ortdot;
                            x2 = DotProduct(edgev2, ort) / ortdot;
                            x3 = DotProduct(edgev3, ort) / ortdot;
                            x4 = DotProduct(edgev4, ort) / ortdot;
                            if ( x1 > (float)x2 )
                            {
                              v28 = x1;
                              x1 = x2;
                              x2 = v28;
                              v28 = y1;
                              y1 = y2;
                              y2 = v28;
                              VectorCopy(edgev1, tmpv);
                              VectorCopy(edgev2, edgev1);
                              VectorCopy(tmpv, edgev2);
                            }
                            if ( x3 > (float)x4 )
                            {
                              v28 = x3;
                              x3 = x4;
                              x4 = v28;
                              v28 = y3;
                              y3 = y4;
                              y4 = v28;
                              VectorCopy(edgev3, tmpv);
                              VectorCopy(edgev4, edgev3);
                              VectorCopy(tmpv, edgev4);
                            }
                            if ( x2 > (float)x3 && x4 > (float)x1 )
                            {
                              /* if the two lines fully overlap */
                              if ( (x1 - 0.5 < x3 && x4 < x2 + 0.5)
                                  && (x3 - 0.5 < x1 && x2 < x4 + 0.5) )
                              {
                                dist1 = y3 - y1;
                                dist2 = y4 - y2;
                                VectorCopy(edgev1, p1area1);
                                VectorCopy(edgev2, p2area1);
                                VectorCopy(edgev3, p1area2);
                                VectorCopy(edgev4, p2area2);
                              }
                              else
                              {
                                /* if the points are equal */
                                if ( x1 > x3 - 0.1 && x1 < x3 + 0.1 )
                                {
                                  dist1 = y3 - y1;
                                  VectorCopy(edgev1, p1area1);
                                  VectorCopy(edgev3, p1area2);
                                }
                                else if ( x1 < (float)x3 )
                                {
                                  v66 = y1 + (x3 - x1) * (y2 - y1) / (x2 - x1);
                                  dist1 = y3 - v66;
                                  VectorCopy(edgev3, p1area1);
                                  p1area1[2] = v66;
                                  VectorCopy(edgev3, p1area2);
                                }
                                else
                                {
                                  v66 = y3 + (x1 - x3) * (y4 - y3) / (x4 - x3);
                                  dist1 = v66 - y1;
                                  VectorCopy(edgev1, p1area1);
                                  VectorCopy(edgev1, p1area2);
                                  p1area2[2] = v66;
                                }
                                /* if the points are equal */
                                if ( x2 > x4 - 0.1 && x2 < x4 + 0.1 )
                                {
                                  dist2 = y4 - y2;
                                  VectorCopy(edgev2, p2area1);
                                  VectorCopy(edgev4, p2area2);
                                }
                                else if ( x2 < (float)x4 )
                                {
                                  v66 = y3 + (x2 - x3) * (y4 - y3) / (x4 - x3);
                                  dist2 = v66 - y2;
                                  VectorCopy(edgev2, p2area1);
                                  VectorCopy(edgev2, p2area2);
                                  p2area2[2] = v66;
                                }
                                else
                                {
                                  v66 = y1 + (x4 - x1) * (y2 - y1) / (x2 - x1);
                                  dist2 = y4 - v66;
                                  VectorCopy(edgev4, p2area1);
                                  p2area1[2] = v66;
                                  VectorCopy(edgev4, p2area2);
                                }
                              }
                              /* if both distances are pretty much equal
                               * then we take the middle of the points */
                              if ( dist1 > dist2 - 1 && dist1 < dist2 + 1 )
                              {
                                dist = dist1;
                                VectorAdd(p1area1, p2area1, start);
                                VectorScale(start, 0.5f, start);
                                VectorAdd(p1area2, p2area2, end);
                                VectorScale(end, 0.5f, end);
                              }
                              else if ( dist1 < (float)dist2 )
                              {
                                dist = dist1;
                                VectorCopy(p1area1, start);
                                VectorCopy(p1area2, end);
                              }
                              else
                              {
                                dist = dist2;
                                VectorCopy(p2area1, start);
                                VectorCopy(p2area2, end);
                              }
                              VectorSubtract(p2area2, p1area2, dir);
                              length = VectorLength(dir);
                              if ( (groundface1->faceflags & 4) != 0 )
                              {
                                if ( dist < ground_bestdist || ground_bestdist + 1.0f > dist && length > (float)ground_bestlength )
                                {
                                  ground_bestdist = dist;
                                  ground_bestlength = length;
                                  VectorCopy(start, ground_beststart);
                                  /* [0] keeps the int-view first-component copy form used
                                   * elsewhere in this file. */
                                  *(int *)ground_bestnormal = *(int *)&normal[0];
                                  ground_bestnormal[1] = normal[1];
                                  ground_bestnormal[2] = normal[2];
                                  ground_foundreach = 1;
                                  ground_bestarea2groundedgenum = v13;
                                  VectorCopy(end, ground_bestend);
                                }
                              }
                              else if ( dist < water_bestdist || water_bestdist + 1.0f > dist && length > (float)water_bestlength )
                              {
                                water_bestdist = dist;
                                water_bestlength = length;
                                VectorCopy(start, water_beststart);
                                /* Same bit-pattern preservation as ground_bestnormal above. */
                                *(int *)water_bestnormal = *(int *)&normal[0];
                                water_bestnormal[1] = normal[1];
                                water_bestnormal[2] = normal[2];
                                water_foundreach = 1;
                                v114 = v13;
                                *(int *)water_bestend = *(int *)&end[0];
                                water_bestend[1] = end[1];
                                water_bestend[2] = end[2];
                              }
                            }
                          }
                        }
                      }
                    }
                    v19 = area2;
                    ++v18;
                  }
                  while ( v18 < area2->numfaces );
                }
            }
          }
        }
        if ( ground_foundreach && ground_bestdist >= 0.0f && ground_bestdist < (float)libvar_sv_step->value )
        {
          v44 = AAS_AllocReachability();
          if ( !v44 )
            return 0;
          v46 = ground_bestarea2groundedgenum;
          v44->reach.areanum = area2num;
          v44->reach.facenum = 0;
          v44->reach.edgenum = v46;
          VectorMA(ground_beststart, 0.1f, (float *)ground_bestnormal, v44->reach.start);
          VectorMA(ground_bestend, 5.0f, (float *)ground_bestnormal, v44->reach.end);
          v44->reach.traveltype = 2;
          v44->reach.traveltime = 1;
          if ( !AAS_AreaCrouch(area1num) && AAS_AreaCrouch(area2num) )
            v44->reach.traveltime += 300;
          v44->next = areareachability[area1num];
          areareachability[area1num] = v44;
          if ( !AAS_NearbySolidOrGap(v44->reach.start, v44->reach.end) )
            v44->reach.traveltime += 400;
          /* The thunk at 0x10001be0 goes to the ground-face area sum, NOT to
           * AAS_AreaReachability. */
          if ( AAS_AreaGroundFaceArea(v44->reach.areanum) < 500.0f )
            v44->reach.traveltime += 400;
          ++reach_step;
          return 1;
        }
        if ( water_foundreach )
        {
          VectorMA((float *)water_bestend, -2.0f, (float *)water_bestnormal, testpoint);
          testpoint[2] = testpoint[2] - libvar_sv_maxwaterjump->value;
          if ( (aasworld.areasettings[AAS_PointAreaNum(testpoint)].areaflags & 4) != 0 )
          {
            if ( libvar_sv_maxwaterjump->value + 24.0f > water_bestdist )
            {
              if ( (aasworld.areasettings[area1num].presencetype & 2) != 0
                && (aasworld.areasettings[area2num].presencetype & 2) != 0 )
              {
                v50 = AAS_AllocReachability();
                if ( !v50 )
                  return 0;
                v50->reach.edgenum = v114;
                VectorCopy(water_beststart, v50->reach.start);
                v50->reach.areanum = area2num;
                v50->reach.facenum = 0;
                VectorMA((float *)water_bestend, 15.0f, (float *)water_bestnormal, v50->reach.end);
                v50->reach.traveltype = 9;
                v50->reach.traveltime = 700;
                v50->next = areareachability[area1num];
                areareachability[area1num] = v50;
                ++reach_waterjump;
                return 1;
              }
            }
          }
        }
        if ( ground_foundreach )
        {
          if ( ground_bestdist > 0.0f && ground_bestdist < (float)libvar_sv_maxbarrier->value )
          {
            if ( !water_foundreach || ground_bestdist - water_bestdist < 16.0f )
            {
              if ( !AAS_AreaCrouch(area1num) && !AAS_AreaCrouch(area2num) )
              {
                v54 = AAS_AllocReachability();
                if ( !v54 )
                  return 0;
                v54->reach.edgenum = ground_bestarea2groundedgenum;
                v54->reach.areanum = area2num;
                v54->reach.facenum = 0;
                VectorMA(ground_beststart, 0.1f, (float *)ground_bestnormal, v54->reach.start);
                VectorMA(ground_bestend, 5.0f, (float *)ground_bestnormal, v54->reach.end);
                v54->reach.traveltype = 4;
                v54->reach.traveltime = 400;
                v54->next = areareachability[area1num];
                areareachability[area1num] = v54;
                ++reach_barrier;
                return 1;
              }
            }
          }
          if ( ground_bestdist < 0.0f )
          {
            if ( -libvar_sv_step->value < ground_bestdist )
            {
              v56 = AAS_AllocReachability();
              if ( !v56 )
                return 0;
              v56->reach.edgenum = ground_bestarea2groundedgenum;
              v56->reach.areanum = area2num;
              v56->reach.facenum = 0;
              VectorMA(ground_beststart, 0.1f, (float *)ground_bestnormal, v56->reach.start);
              VectorMA(ground_bestend, 5.0f, (float *)ground_bestnormal, v56->reach.end);
              v56->reach.traveltype = 2;
              v56->reach.traveltime = 1;
              v56->next = areareachability[area1num];
              areareachability[area1num] = v56;
              ++reach_walk;
              return 1;
            }
            v114 = -AAS_FallDamageDistance();
            if ( (float)(int)v114 < ground_bestdist || AAS_AreaSwim(area2num) )
            {
              VectorMA(ground_bestend, 2.0f, (float *)ground_bestnormal, ground_bestend);
              VectorCopy(ground_bestend, start);
              start[2] = ground_beststart[2];
              VectorCopy(ground_bestend, end);
              end[2] += 4.0f;
              trace = AAS_TraceClientBBox(start, end, 2, -1);
              if ( !trace.startsolid && trace.fraction >= 1.0 )
              {
                trace.endpos[2] = trace.endpos[2] + 1.0f;
                if ( AAS_PointAreaNum(trace.endpos) == area2num )
                {
                  v57 = AAS_AllocReachability();
                  if ( v57 )
                  {
                    v58 = ground_bestarea2groundedgenum;
                    *(_DWORD *)v57 = area2num;
                    *(_DWORD *)(v57 + 4) = 0;
                    *(_DWORD *)(v57 + 8) = v58;
                    *(float *)(v57 + 12) = ground_beststart[0];
                    *(float *)(v57 + 16) = ground_beststart[1];
                    *(float *)(v57 + 20) = ground_beststart[2];
                    *(float *)(v57 + 24) = ground_bestend[0];
                    *(float *)(v57 + 28) = ground_bestend[1];
                    *(float *)(v57 + 32) = ground_bestend[2];
                    *(_DWORD *)(v57 + 36) = 7;
                    *(_WORD *)(v57 + 40) = 100;
                    ((aas_reachabilitynode_t *)v57)->next = areareachability[area1num];
                    areareachability[area1num] = (aas_reachabilitynode_t *)v57;
                    ++reach_walkoffledge;
                    return 1;
                  }
                }
              }
            }
          }
        }
        return 0;
      }
    }
  }
  return 0;
}

// gladiator.dll: 10013BA0..10013BD5
// gladi386.so:   00020BB8..00020BFE
/* Euclidean distance |v2 - v1|: a tail-call to VectorLength, returning what it
 * leaves on ST(0).  Not void — the decompiler read it that way because the body
 * looks like a fire-and-forget call. */
float __cdecl VectorDistance(vec3_t v1, vec3_t v2)
{
  vec3_t dir; // [esp+0h] [ebp-Ch] BYREF

  VectorSubtract(v2, v1, dir);
  return VectorLength(dir);
}

// gladiator.dll: 10013BF0..10013C46
// gladi386.so:   00020C00..00020C67
/* Returns 1 iff (v - v1) . (v - v2) <= 0, i.e. v lies within the segment [v1,v2],
 * endpoints included.  AAS_Reachability_Jump uses it to decide whether a
 * projected vertex lands inside the OTHER edge. */
int __cdecl VectorBetweenVectors(vec3_t v, vec3_t v1, vec3_t v2)
{
  float ab[3], ac[3];

  VectorSubtract(v, v1, ab);
  VectorSubtract(v, v2, ac);
  return DotProduct(ab, ac) <= 0.0f;
}

// gladiator.dll: 10013C70..10013CA4
// gladi386.so:   00020C68..00020CAA
void __cdecl VectorMiddle(vec3_t v1, vec3_t v2, vec3_t middle)
{
  VectorAdd(v1, v2, middle);
  VectorScale((float *)middle, 0.5, (float *)middle);
}

// gladiator.dll: 10013CC0..10014AC7
// gladi386.so:   00020CAC..0002243A
int AAS_Reachability_Jump(int area1num, int area2num)
{
  aas_area_t *area1; // ebx — a real pointer, not a float-typed slot holding its bits
  aas_area_t *area2; // esi
  int i; // edx
  int v8; // ecx
  int v9; // eax
  __int64 face1num; // rax
  aas_face_t *face1; // edi
  int v13; // eax
  int j; // ebp
  __int64 face2num; // rax
  int edge1num; // edx
  __int64 edge2num; // rax
  aas_edge_t *edge2; // ecx
  float *v1; // edi
  float *v2; // ebx
  int v23; // ecx
  float *v3; // esi
  float *v4; // ebp
  aas_face_t *face2; // ecx
  /* v26..v39 are x87 80-bit FPU temporaries, and the spelling depends on whether
   * the target HAS an x87 stack — i.e. on FLT_EVAL_METHOD, not on the compiler
   * brand.  Where float expressions are evaluated in 80-bit registers (MSVC6/x86
   * and any i386 gcc using the 387), a plain `float` temp already carries full
   * register precision and `long double` would force a widening convert the
   * original never had.  Where they are not (aarch64 / x86-64, gcc -mfpmath=sse),
   * `long double` plus per-operand casts reproduces the wide intermediates.
   *
   * `_MSC_VER` alone is the wrong predicate: gcc 2.7.2.3's reg-stack pass ABORTS
   * on the `long double` version here — eight 80-bit temporaries live at once
   * overflow the 8-deep x87 stack it models — and the 1999 build compiled this
   * file, so the original cannot have been `long double`. */
#if defined(_MSC_VER) || (defined(__i386__) && !defined(__SSE_MATH__))
  float v26; // st7
  float v27; // st6
  float v28; // st5
  float v29; // st4
  float v30; // st7
  float v31; // st6
  float v32; // st5
  float v33; // st4
#else
  long double v26; // st7
  long double v27; // st6
  long double v28; // st5
  long double v29; // st4
  long double v30; // st7
  long double v31; // st6
  long double v32; // st5
  long double v33; // st4
#endif
  aas_plane_t *plane1; // eax
#if defined(_MSC_VER) || (defined(__i386__) && !defined(__SSE_MATH__))
  float v36; // st7
  float v37; // st6
#else
  long double v36; // st7
  long double v37; // st6
#endif
  aas_plane_t *plane2; // ecx
#if defined(_MSC_VER) || (defined(__i386__) && !defined(__SSE_MATH__))
  float v39; // st7
#else
  long double v39; // st7
#endif
  int v40; // edx
  int v43; // eax
  int v44; // ecx
  int traveltype; // ebx
  int v46; // edi
  double v48; // st7
  aas_reachabilitynode_t *lreach; // esi
  float v51; // [esp+0h] [ebp-1CCh]
  float bestdist; // [esp+1Ch] [ebp-1B0h]
  /* vec3_t: passed to AAS_HorizontalVelocityForJump and the other vec3
   * helpers. */
  vec3_t beststart; // [esp+20h..0x2B] [ebp-1ACh..-1A4h] BYREF
  vec3_t bestend;   // [esp+2Ch..0x37] [ebp-1A0h..-198h] BYREF
  float phys_jumpvel; // [esp+38h] [ebp-194h]
  vec3_t dir; // [esp+3Ch..44h] [ebp-190h..188h] BYREF — vec3 difference; v60[0..2] = old v60/v61/v62
  vec3_t teststart; // [esp+48h..0x53] [ebp-184h..-17Ch] BYREF — trace start
  int maxjumpheight; // [esp+5Ch] [ebp-170h]
  float speed; // [esp+60h] [ebp-16Ch] BYREF
  /* The four edge-projection results used to pick the best face pair.  All must be
   * vec3_t: VectorDistance / VectorMiddle / VectorBetweenVectors read three
   * components, and garbage Y/Z makes every reach WALKOFFLEDGE. */
  vec3_t v70_vec; // [esp+64h..6Ch] [ebp-168h..-160h] BYREF — v70/v71/v72 collapsed (edge2 proj A)
  vec3_t v73_vec; // [esp+70h..78h] [ebp-15Ch..-154h] BYREF — v73/v74/v75 collapsed (edge1 proj B)
  vec3_t v76_vec; // [esp+7Ch..84h] [ebp-150h..-148h] BYREF — v76/v77/v78 collapsed (edge2 proj B)
  int v79; // [esp+88h] [ebp-144h]
  vec3_t v80_vec; // [esp+8Ch..94h] [ebp-140h..-138h] BYREF — v80/v81/v82 collapsed (edge1 proj A)
  vec3_t dir1; // [esp+98h..A0h] — edge1 direction (Q3's dir1); [2] is never written
  vec3_t dir2; // [esp+A4h..ACh] — edge2 direction (Q3's dir2); [2] is never written
  int l; // [esp+B0h] [ebp-11Ch]
  int k; // [esp+B4h] [ebp-118h]
  float maxjumpdistance; // [esp+B8h] [ebp-114h]
  float v90; // [esp+BCh] [ebp-110h]
  char *v91; // [esp+C0h] [ebp-10Ch]
  vec3_t testend; // [esp+C8h..D0h] [ebp-104h..0FCh] BYREF — vec3 trace destination
  aas_edge_t *edge1; // [esp+D4h] [ebp-F8h]
  /* vec3_t: VectorScale writes 3 floats and ClientMovementPrediction reads 3;
   * the original frame leaves exactly 12 bytes here. */
  vec3_t cmdmove; // [esp+D8h..E3h] [ebp-F4h..-E8h] BYREF
  aas_trace_t trace; // [esp+E4h] [ebp-E8h] (was int v99[9] + char v100[36] hidden return buffer)
  aas_clientmove_t move2; // [esp+12Ch] [ebp-A0h] BYREF

  if ( AAS_AreaGrounded(area1num) && AAS_AreaGrounded(area2num) && !AAS_AreaCrouch(area1num) && !AAS_AreaCrouch(area2num) )
  {
    area1 = &aasworld.areas[area1num];
    area2 = &aasworld.areas[area2num];
    /* AAS_MaxJumpDistance / AAS_MaxJumpHeight return their values on the FPU stack;
     * the decompiler dropped both, substituting the area number. */
    phys_jumpvel = libvar_sv_jumpvel->value;
    maxjumpdistance = (float)AAS_MaxJumpDistance(phys_jumpvel);
    *(float *)&maxjumpheight = (float)AAS_MaxJumpHeight(phys_jumpvel);

    /* Q3 be_aas_reach.c's own x-y proximity test. */
    for (i = 0; i < 2; i++)
    {
      if ( maxjumpdistance + area2->maxs[i] < area1->mins[i]
        || area2->mins[i] - maxjumpdistance > area1->maxs[i] )
        return 0;
    }

    if ( *(float *)&maxjumpheight + area1->maxs[2] < area2->mins[2] )
      return 0;
    {
      v8 = area1->numfaces;
      v9 = 0;
      bestdist = 999999.0;
      *(float *)&maxjumpheight = 0.0;
      if ( v8 > 0 )
      {
      do
      {
        face1num = aasworld.faceindex[v9 + area1->firstface];
        face1num = (HIDWORD(face1num) ^ face1num) - HIDWORD(face1num);
        face1 = &aasworld.faces[face1num];
        if ( (face1->faceflags & 4) != 0 )
        {
          v13 = area2->numfaces;
          j = 0;
          if ( v13 > 0 )
          {
            do
            {
              face2num = aasworld.faceindex[j + area2->firstface];
              face2 = &aasworld.faces[((HIDWORD(face2num) ^ face2num) - HIDWORD(face2num))];
              if ( (face2->faceflags & 4) != 0 )
              {
                for ( k = 0; k < face1->numedges; k++ )
                {
                    edge1num = k + face1->firstedge;
                    l = 0;
                    edge1 = &aasworld.edges[abs(aasworld.edgeindex[edge1num])];
                    if ( face2->numedges > 0 )
                    {
                      while ( 1 )
                      {
                        edge2num = aasworld.edgeindex[l + face2->firstedge];
                        edge2 = &aasworld.edges[((HIDWORD(edge2num) ^ edge2num) - HIDWORD(edge2num))];
                        v1 = (float *)(&aasworld.vertexes[edge1->v[0]]);
                        v2 = (float *)(&aasworld.vertexes[edge1->v[1]]);
                        HIDWORD(edge2num) = edge2->v[0];
                        v23 = edge2->v[1];
                        dir1[0] = *v2 - *v1;
                        v3 = (float *)(&aasworld.vertexes[HIDWORD(edge2num)]);
                        v4 = (float *)(&aasworld.vertexes[v23]);
                        dir1[1] = v2[1] - v1[1];
                        dir2[0] = *v4 - *v3;
                        dir2[1] = v4[1] - v3[1];
                        if ( dir2[0] != 0 )
                        {
                          v26 = (float)dir2[1] / (float)dir2[0];
                          v27 = (float)v3[1] - v26 * (float)*v3;
                          v28 = v27 * (float)dir2[1] + v26 * (float)dir2[0];
                          v29 = ((float)dir2[0] * (float)*v1 + (float)dir2[1] * (float)v1[1] - v28) / (float)dir2[0];
                          v70_vec[0] = (float)v29;
                          v70_vec[1] = (float)(v29 * v26 + v27);
                          {
                            float tmp = (float)dir2[1] * (float)v2[1];
                            tmp = tmp + (float)dir2[0] * (float)*v2;
                            tmp = (tmp - v28) / (float)dir2[0];
                            v76_vec[0] = (float)tmp;
                            v76_vec[1] = (float)(tmp * v26 + v27);
                          }
                        }
                        else
                        {
                          v70_vec[0] = *v3;
                          v70_vec[1] = v1[1];
                          v76_vec[0] = *v3;
                          v76_vec[1] = v2[1];
                        }
                        if ( dir1[0] != 0 )
                        {
                          v30 = (float)dir1[1] / (float)dir1[0];
                          v31 = (float)v1[1] - v30 * (float)*v1;
                          v32 = v31 * (float)dir1[1] + v30 * (float)dir1[0];
                          v33 = ((float)dir1[0] * (float)*v3 + (float)dir1[1] * (float)v3[1] - v32) / (float)dir1[0];
                          v80_vec[0] = (float)v33;
                          v80_vec[1] = (float)(v33 * v30 + v31);
                          {
                            float tmp = (float)dir1[1] * (float)v4[1];
                            tmp = tmp + (float)dir1[0] * (float)*v4;
                            tmp = (tmp - v32) / (float)dir1[0];
                            v73_vec[0] = (float)tmp;
                            v73_vec[1] = (float)(tmp * v30 + v31);
                          }
                        }
                        else
                        {
                          v80_vec[0] = *v1;
                          v80_vec[1] = v3[1];
                          v73_vec[0] = *v1;
                          v73_vec[1] = v4[1];
                        }
                        v70_vec[2] = 0.0f;
                        v76_vec[2] = 0.0f;
                        v80_vec[2] = 0.0f;
                        v73_vec[2] = 0.0f;
                        plane1 = &aasworld.planes[face1->planenum];
                        v79 = 0;
                        v36 = (float)v70_vec[1] * (float)aasworld.planes[face2->planenum].normal[1];
                        v37 = (float)v70_vec[0] * (float)aasworld.planes[face2->planenum].normal[0];
                        plane2 = &aasworld.planes[face2->planenum];
                        v70_vec[2] = (float)(((float)plane2->dist - (v36 + v37)) / (float)plane2->normal[2]);
                        v76_vec[2] = (float)(((float)plane2->dist - ((float)v76_vec[1] * (float)plane2->normal[1] + (float)v76_vec[0] * (float)plane2->normal[0])) / (float)plane2->normal[2]);
                        v80_vec[2] = (float)(((float)plane1->dist - ((float)v80_vec[0] * (float)plane1->normal[0] + (float)v80_vec[1] * (float)plane1->normal[1])) / (float)plane1->normal[2]);
                        v39 = ((float)plane1->dist - ((float)v73_vec[0] * (float)plane1->normal[0] + (float)v73_vec[1] * (float)plane1->normal[1])) / (float)plane1->normal[2];
                        v73_vec[2] = (float)v39;
                        /* Is the projection of v21 onto edge2's line (stored
                         * in v70_vec) between edge2's endpoints v24, v25? */
                        if ( VectorBetweenVectors(v70_vec, v3, v4) )
                        {
                          v39 = VectorDistance(v1, v70_vec);
                          if ( v39 <= bestdist - 0.5 || v39 >= bestdist + 0.5 )
                          {
                            if ( v39 < bestdist )
                            {
                              VectorCopy(v1, beststart);
                              bestdist = v39;
                              VectorCopy(v70_vec, bestend);
                            }
                          }
                          else
                          {
                            VectorMiddle(beststart, v1, beststart);
                            VectorMiddle(bestend, v70_vec, bestend);
                          }
                          v79 = 1;
                        }
                        /* Is the projection of v22 onto edge2's line (stored
                         * in v76_vec) between edge2's endpoints v24, v25? */
                        if ( VectorBetweenVectors(v76_vec, v3, v4) )
                        {
                          v39 = VectorDistance(v2, v76_vec);
                          if ( v39 <= bestdist - 0.5 || v39 >= bestdist + 0.5 )
                          {
                            if ( v39 < bestdist )
                            {
                              VectorCopy(v2, beststart);
                              bestdist = v39;
                              VectorCopy(v76_vec, bestend);
                            }
                          }
                          else
                          {
                            VectorMiddle(beststart, v2, beststart);
                            VectorMiddle(bestend, v76_vec, bestend);
                          }
                          v79 = 1;
                        }
                        /* Is the projection of v24 onto edge1's line (stored
                         * in v80_vec) between edge1's endpoints v21, v22? */
                        if ( VectorBetweenVectors(v80_vec, v1, v2) )
                        {
                          v39 = VectorDistance(v3, v80_vec);
                          if ( v39 <= bestdist - 0.5 || v39 >= bestdist + 0.5 )
                          {
                            if ( v39 < bestdist )
                            {
                              VectorCopy(v80_vec, beststart);
                              bestdist = v39;
                              VectorCopy(v3, bestend);
                            }
                          }
                          else
                          {
                            VectorMiddle(beststart, v80_vec, beststart);
                            VectorMiddle(bestend, v3, bestend);
                          }
                          v79 = 1;
                        }
                        /* Is the projection of v25 onto edge1's line (stored
                         * in v73_vec) between edge1's endpoints v21, v22? */
                        if ( !VectorBetweenVectors(v73_vec, v1, v2) )
                          break;
                        v39 = VectorDistance(v4, v73_vec);
                        if ( v39 <= bestdist - 0.5 || v39 >= bestdist + 0.5 )
                        {
                          if ( v39 < bestdist )
                          {
                            bestdist = v39;
                            v40 = *(int *)&v73_vec[2];
                            beststart[0] = v73_vec[0];
                            beststart[1] = v73_vec[1];
LABEL_60:
                            *(int *)&beststart[2] = v40;
                            VectorCopy(v4, bestend);
                          }
                        }
                        else
                        {
                          VectorMiddle(beststart, v73_vec, beststart);
                          VectorMiddle(bestend, v4, bestend);
                        }
LABEL_61:
                        if ( ++l >= face2->numedges )
                          goto LABEL_62;
                      }
                      if ( v79 )
                        goto LABEL_61;
                      v39 = VectorDistance(v1, v3);
                      if ( v39 < bestdist )
                      {
                        VectorCopy(v1, beststart);
                        bestdist = v39;
                        VectorCopy(v3, bestend);
                      }
                      v39 = VectorDistance(v1, v4);
                      if ( v39 < bestdist )
                      {
                        VectorCopy(v1, beststart);
                        bestdist = v39;
                        VectorCopy(v4, bestend);
                      }
                      v39 = VectorDistance(v2, v3);
                      if ( v39 < bestdist )
                      {
                        VectorCopy(v2, beststart);
                        bestdist = v39;
                        VectorCopy(v3, bestend);
                      }
                      v39 = VectorDistance(v2, v4);
                      if ( v39 >= bestdist )
                        goto LABEL_61;
                      bestdist = v39;
                      beststart[0] = *v2;
                      beststart[1] = v2[1];
                      v40 = *(int *)&v2[2];
                      goto LABEL_60;
                    }
LABEL_62:
                    ;
                  }
              }
              v43 = area2->numfaces;
              ++j;
            }
            while ( j < v43 );
          }
        }
        v44 = area1->numfaces;
        v9 = ++maxjumpheight;
      }
      while ( maxjumpheight < v44 );
      }
      if ( bestdist > 4 )
      {
        if ( bestdist < (float)maxjumpdistance )
        {
          v90 = beststart[2] - bestend[2];
          v91 = (char *)AAS_FallDamageDistance();
          if ( (float)(int)v91 >= v90 )
          {
            if ( AAS_HorizontalVelocityForJump(0.0, beststart, bestend, &speed) )
            {
              traveltype = 7;
              speed = speed * 1.2;
            }
            else
            {
              if ( !AAS_HorizontalVelocityForJump(phys_jumpvel, beststart, bestend, &speed) )
                return 0;
              traveltype = 5;
            }
            VectorSubtract(bestend, beststart, dir);
            dir[2] = 0.0f;
            if ( VectorLength(dir) >= 10 )
            {
              VectorSubtract(bestend, beststart, dir);
              VectorNormalize(dir);
              VectorMA(beststart, 1.0, dir, teststart);
              VectorCopy(teststart, testend);
              testend[2] = testend[2] - 100.0f;
              trace = AAS_TraceClientBBox(teststart, testend, 2, -1);
              if ( !trace.startsolid
                && (trace.fraction >= 1
                 || aasworld.planes[trace.planenum].normal[2] < 0.7
                 || teststart[2] - trace.endpos[2] > libvar_sv_maxbarrier->value) )
              {
                VectorMA(bestend, -1.0, dir, teststart);
                VectorCopy(teststart, testend);
                testend[2] = testend[2] - 100.0f;
                trace = AAS_TraceClientBBox(teststart, testend, 2, -1);
                if ( !trace.startsolid
                  && (trace.fraction >= 1
                   || aasworld.planes[trace.planenum].normal[2] < 0.7
                   || teststart[2] - trace.endpos[2] > libvar_sv_maxbarrier->value) )
                {
                  /* Horizontal velocity vector (Z explicitly zeroed):
                   * .text 0x100148e9-0x1001490e — same pattern as block 1. */
                  VectorSubtract(bestend, beststart, dir);
                  dir[2] = 0.0f;
                  VectorNormalize(dir);
                  VectorScale(dir, speed, cmdmove);
                  /* The cmdmove[2] override is required: without a Z impulse for
                   * traveltype 5, every JUMP candidate's landing point misses
                   * area2num and no JUMP reach is produced. */
                  cmdmove[2] = (traveltype == 5) ? libvar_sv_jumpvel->value : 0.0f;
                  move2 = AAS_ClientMovementPrediction(
                                    -1,
                                    beststart,
                                    2,
                                    1,
                                    vec3_origin,
                                    cmdmove,    /* now vec3_t */
                                    3,
                                    30,
                                    0.1,
                                    61,
                                    0);
                  if ( move2.frames < 30 && (move2.stopevent & 0x38) == 0 )
                  {
                    /* Probe loop: pull the candidate test-point back along the trace
                     * direction at scale 0, -8, -16, -24, -32 and accept the first
                     * that lands in area2num.  The counter must be a plain int —
                     * through a float slot the bit pattern is lost to x87 qNaN
                     * canonicalisation and only scale=0 ever works. */
                    int probe_scale;
                    v46 = 0;
                    probe_scale = 0;
                    while ( 1 )
                    {
                      v51 = (float)probe_scale;
                      VectorMA(move2.endpos, v51, dir, teststart);
                      v48 = teststart[2] + 0.125;
                      teststart[2] = v48;
                      if ( AAS_PointAreaNum(teststart) == area2num )
                        break;
                      probe_scale -= 8;
                      v46 += 8;
                      if ( probe_scale < -32 )
                        return 0;
                    }
                    if ( v46 <= 32 )
                    {
                      lreach = AAS_AllocReachability();
                      if ( lreach )
                      {
                        lreach->reach.areanum = area2num;
                        lreach->reach.facenum = 0;
                        lreach->reach.edgenum = 0;
                        VectorCopy(beststart, lreach->reach.start);
                        VectorCopy(bestend, lreach->reach.end);
                        lreach->reach.traveltype = traveltype;
                        /* traveltime = dist * 240 / sv_maxwalkvelocity + 600, where dist
                         * is VectorDistance's FPU return value. */
                        lreach->reach.traveltime = (__int64)(VectorDistance(bestend, beststart) * 240 / libvar_sv_maxwalkvelocity->value + 600);
                        lreach->next = areareachability[area1num];
                        areareachability[area1num] = lreach;
                        ++reach_jump;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return 0;
}

// gladiator.dll: 10014E60..100158FD
// gladi386.so:   0002243C..00023415
/* DELIBERATE DEVIATION: the six reach.facenum writes here store abs() of the
 * faceindex value.  The original stored it raw and signed (the sign encodes face
 * orientation for plane-flip selection) and AAS_Optimize then did a signed
 * [base+idx*4] lookup, producing garbage face numbers on every ladder reach in
 * the optimized .aas and an unmapped read under MinGW.  Downstream only needs the
 * magnitude, and Q3's later AAS_Optimize abs()es defensively for the same reason.
 */
int AAS_Reachability_Ladder(int area1num, int area2num)
{
  aas_area_t *area1; // edi
  aas_area_t *area2; // ebx
  int v5; // eax
  int v6; // esi
  aas_face_t *ladderface1; // ebp
  aas_face_t *face1; // ecx
  int v9; // eax
  int j; // edi
  aas_face_t *face2; // esi
  int v12; // edx
  int k; // eax
  int v14; // edi
  int edge1num; // ebp
  int *v17; // ebx
  float face2area; // st7
  int v20; // eax
  unsigned int v21; // edi
  aas_edge_t *sharededge; // ecx
  BOOL firstv; // edx
  aas_plane_t *plane1; // ebx
  aas_plane_t *plane2; // esi
  BOOL ladderface2vertical; // eax
  aas_reachabilitynode_t *v32; // eax (was int — alloc'd reach slot pointer)
  int v34; // edx
  aas_reachabilitynode_t *v35; // eax
  int v37; // ecx
  aas_reachabilitynode_t *v39; // eax
  int v41; // ecx
  aas_reachabilitynode_t *v42; // eax
  int v43; // ecx
  int i; // edi
  unsigned int v45; // esi
  float v52; // st7
  int v53; // edi
  int v54; // ebx
  aas_area_t *v55; // eax
  int v56; // esi
  int *v57; // ebp
  aas_face_t *v58; // eax
  aas_reachabilitynode_t *v59; // eax
  int v60; // ecx
  aas_reachabilitynode_t *lreach; // esi
  int v62; // edx
  int v65; // [esp+10h] [ebp-108h]
  BOOL v66; // [esp+10h] [ebp-108h]
  int sharededgenum; // [esp+14h] [ebp-104h]
  vec3_t area1point; // [esp+18h] [ebp-100h] BYREF — v68/v69/v70 collapsed
  int ladderface1num; // [esp+24h] [ebp-F4h]
  vec3_t area2point; // [esp+28h] [ebp-F0h] BYREF — v72/v73/v74 collapsed
  aas_face_t *ladderface2; // [esp+34h] [ebp-E4h]
  vec3_t v1; // [esp+38h] [ebp-E0h] — v76/v77/v78 collapsed (first edge vertex)
  vec3_t v2; // [esp+44h] [ebp-D4h] — v79/v80/v81 collapsed (second edge vertex)
  int l; // [esp+50h] [ebp-C8h]
  aas_face_t *v83; // [esp+54h] [ebp-C4h]
  int ladderface2num; // [esp+58h] [ebp-C0h]
  char *v85; // [esp+5Ch] [ebp-BCh] (was int — aas_area_t * stash)
  int absladderface1num;
  int absladderface2num;
  vec3_t lowestpoint; // [esp+60h] [ebp-B8h] BYREF — v86/v87/v88 collapsed
  int v89; // [esp+6Ch] [ebp-ACh]
  float bestface2area; // [esp+70h] [ebp-A8h]
  vec3_t mid; // [esp+74h] [ebp-A4h] BYREF — v91/v92/v93 collapsed
  aas_area_t *v94; // [esp+80h] [ebp-98h]
  float face1area; // [esp+88h] [ebp-90h]
  float bestface1area; // [esp+90h] [ebp-88h]
  vec3_t dir; // [esp+B8h] [ebp-60h] BYREF — v106
  vec3_t end; // [esp+C4h] [ebp-54h] BYREF — v107
  int face1num; // [esp+A4h] [ebp-74h]
  float maxjumpheight; // [esp+A8h] [ebp-70h]
  float phys_jumpvel;
  vec3_t start; // [esp+94h] [ebp-84h] BYREF — v99[2]+v100 collapsed
  int face2num; // [esp+A0h] [ebp-78h]
  aas_trace_t trace; // [esp+D0h] [ebp-48h] (was int v108[9] + char v109[36] hidden return buffer)
  vec3_t sharededgevec; // [esp+ACh] [ebp-6Ch] BYREF — v104[2]+v105 collapsed

  if ( AAS_AreaLadder(area1num) )
  {
    if ( AAS_AreaLadder(area2num) )
    {
      /* AAS_MaxJumpHeight's FPU return is required here; the decompiler dropped it
       * and substituted the area number, breaking ladder-reach filtering. */
      phys_jumpvel = libvar_sv_jumpvel->value;
      maxjumpheight = (float)AAS_MaxJumpHeight(phys_jumpvel);
      area1 = &aasworld.areas[area1num];
      area2 = &aasworld.areas[area2num];
      v5 = area1->numfaces;
      v6 = 0;
      ladderface1 = 0;
      v94 = area1;
      v85 = (char *)area2;
      ladderface2 = 0;
      bestface1area = -9999.0;
      bestface2area = -9999.0;
      v65 = 0;
      if ( v5 > 0 )
      {
        do
        {
          face1num = aasworld.faceindex[v6 + area1->firstface];
          face1 = &aasworld.faces[abs(face1num)];
          v83 = face1;
          if ( (face1->faceflags & 2) != 0 )
          {
            v9 = area2->numfaces;
            j = 0;
            v89 = 0;
            if ( v9 > 0 )
            {
              do
              {
                face2num = aasworld.faceindex[j + area2->firstface];
                face2 = &aasworld.faces[abs(face2num)];
                if ( (face2->faceflags & 2) != 0 )
                {
v12 = face1->numedges;
k = 0;
if ( v12 > 0 )
{
while ( 1 )
{
  edge1num = aasworld.edgeindex[k + face1->firstedge];
  v14 = face2->numedges;
  l = 0;
  if ( v14 > 0 )
  {
    v17 = &aasworld.edgeindex[face2->firstedge];
    while ( 1 )
    {
      if ( abs(edge1num) == abs(*v17) )
        break;
      ++v17;
      if ( ++l >= v14 )
        goto LABEL_16;
    }
    face1area = AAS_FaceArea(v83);
    face2area = AAS_FaceArea(face2);
    if ( face1area > (float)bestface1area && face2area > bestface2area )
    {
      bestface1area = face1area;
      bestface2area = face2area;
      ladderface1 = v83;
      ladderface2 = face2;
      ladderface1num = face1num;
      ladderface2num = face2num;
      sharededgenum = edge1num;
    }
  }
LABEL_16:
  if ( l != face2->numedges )
    break;
  face1 = v83;
  ++k;
  if ( k >= v83->numedges )
    goto LABEL_20;
}
face1 = v83;
LABEL_20:
j = v89;
area2 = (aas_area_t *)v85;
}
                }
                v20 = area2->numfaces;
                v89 = ++j;
              }
              while ( j < v20 );
              v6 = v65;
            }
            area1 = v94;
          }
          v65 = ++v6;
        }
        while ( v6 < area1->numfaces );
        if ( ladderface1 )
        {
          if ( ladderface2 )
          {
            absladderface1num = abs(ladderface1num);
            absladderface2num = abs(ladderface2num);
            v21 = abs(sharededgenum);
            sharededge = &aasworld.edges[v21];
            firstv = sharededgenum < 0;
            VectorCopy(aasworld.vertexes[sharededge->v[firstv]], v1);
            VectorCopy(aasworld.vertexes[sharededge->v[!firstv]], v2);
            VectorAdd(v2, v1, area1point);
            VectorScale(area1point, 0.5, area1point);
            VectorCopy(area1point, area2point);
            plane1 = &aasworld.planes[(ladderface1->planenum ^ (ladderface1num < 0))];
            plane2 = &aasworld.planes[(ladderface2->planenum ^ (ladderface2num < 0))];
            VectorSubtract(v2, v1, sharededgevec);
            CrossProduct(plane1->normal, sharededgevec, dir);
            VectorNormalize(dir);
            VectorMA(area1point, -32.0, dir, area1point);
            VectorMA(area2point, 32.0, dir, area2point);
            /* No `(float)` cast around abs(): the original folds `n < 0.1` on a
   * non-negative int into `test edx,edx`, so it never converts back to float.
   * The cast forced a `fild` plus a real x87 compare at all four sites. */
  v66 = abs((int)plane1->normal[2]) < 0.1;
            ladderface2vertical = abs((int)plane2->normal[2]) < 0.1;
            if ( v66 )
            {
              if ( ladderface2vertical
                && plane2->normal[0] * plane1->normal[0] + plane2->normal[2] * plane1->normal[2] + plane2->normal[1] * plane1->normal[1] > 0.7
                && abs((int)sharededgevec[2]) < 0.7 )
              {
                v32 = AAS_AllocReachability();
                if ( v32 )
                {
                  v34 = absladderface1num;
                  v32->reach.areanum = area2num;
                  v32->reach.facenum = v34;
                  v32->reach.edgenum = v21;
                  VectorCopy(area1point, v32->reach.start);
                  VectorMA(area2point, -3.0, plane1->normal, v32->reach.end);
                  v32->reach.traveltype = 6;
                  v32->reach.traveltime = 10;
                  v32->next = areareachability[area1num];
                  areareachability[area1num] = v32;
                  ++reach_ladder;
                  v35 = AAS_AllocReachability();
                  if ( v35 )
                  {
                    v37 = absladderface2num;
                    v35->reach.areanum = area1num;
                    v35->reach.facenum = v37;
                    v35->reach.edgenum = v21;
                    VectorCopy(area2point, v35->reach.start);
                    VectorMA(area1point, -3.0, plane1->normal, v35->reach.end);
                    v35->reach.traveltype = 6;
                    v35->reach.traveltime = 10;
                    v35->next = areareachability[area2num];
                    areareachability[area2num] = v35;
                    ++reach_ladder;
                    return 1;
                  }
                }
              }
              else if ( (ladderface2->faceflags & 4) != 0 )
              {
                v39 = AAS_AllocReachability();
                if ( v39 )
                {
                  v41 = absladderface1num;
                  v39->reach.areanum = area2num;
                  v39->reach.facenum = v41;
                  v39->reach.edgenum = v21;
                  VectorCopy(area1point, v39->reach.start);
                  VectorCopy(area2point, v39->reach.end);
                  v39->reach.end[2] = area2point[2] + 16;
                  VectorMA(v39->reach.end, -15.0, plane1->normal, v39->reach.end);
                  v39->reach.traveltype = 6;
                  v39->reach.traveltime = 10;
                  v39->next = areareachability[area1num];
                  areareachability[area1num] = v39;
                  ++reach_ladder;
                  v42 = AAS_AllocReachability();
                  if ( v42 )
                  {
                    v43 = absladderface2num;
                    v42->reach.areanum = area1num;
                    v42->reach.facenum = v43;
                    v42->reach.edgenum = v21;
                    VectorCopy(area2point, v42->reach.start);
                    VectorCopy(area1point, v42->reach.end);
                    v42->reach.traveltype = 7;
                    v42->reach.traveltime = 10;
                    v42->next = areareachability[area2num];
                    areareachability[area2num] = v42;
                    ++reach_walkoffledge;
                    return 1;
                  }
                }
              }
              else
              {
                lowestpoint[2] = 99999.0;
                for ( i = 0; i < ladderface1->numedges; ++i )
                {
                  v45 = abs(aasworld.edgeindex[i + ladderface1->firstedge]);
                  VectorCopy(aasworld.vertexes[aasworld.edges[v45].v[0]], v1);
                  VectorCopy(aasworld.vertexes[aasworld.edges[v45].v[1]], v2);
                  VectorAdd(v1, v2, mid);
                  VectorScale(mid, 0.5, mid);
                  if ( mid[2] < (float)lowestpoint[2] )
                  {
                    VectorCopy(mid, lowestpoint);
                    v66 = v45;
                  }
                }
                /* v85 is the slot the original re-uses here (it also stashes area2
                 * further up). */
                v85 = (char *)&aasworld.planes[ladderface1->planenum];
                VectorMA(lowestpoint, 5.0, ((aas_plane_t *)v85)->normal, start);
                v52 = start[2];
                end[0] = start[0];
                end[1] = start[1];
                start[2] = start[2] + 5;
                end[2] = v52 - 100;
                trace = AAS_TraceClientBBox(start, end, 2, -1);
                trace.endpos[2] = trace.endpos[2] + 1.0f;
                v53 = AAS_PointAreaNum(trace.endpos);
                v54 = 0;
                v55 = &aasworld.areas[v53];
                v56 = v55->numfaces;
                if ( v56 > 0 )
                {
                  v57 = &aasworld.faceindex[v55->firstface];
                  do
                  {
                    v58 = &aasworld.faces[abs(*v57)];
                    if ( (v58->faceflags & 2) != 0 )
                    {
                      plane2 = &aasworld.planes[v58->planenum];
                      if ( abs((int)plane2->normal[2]) < 0.1 )
                        break;
                    }
                    ++v54;
                    ++v57;
                  }
                  while ( v54 < v56 );
                }
                if ( v54 >= v55->numfaces
                  && v53 != area1num
                  && !AAS_ReachabilityExists(area1num, v53)
                  && !AAS_ReachabilityExists(v53, area1num)
                  && start[2] - trace.endpos[2] < maxjumpheight )
                {
                  v59 = AAS_AllocReachability();
                  if ( v59 )
                  {
                    v60 = absladderface1num;
                    v59->reach.areanum = v53;
                    v59->reach.facenum = v60;
                    v59->reach.edgenum = v66;
                    VectorCopy(lowestpoint, v59->reach.start);
                    VectorCopy(trace.endpos, v59->reach.end);
                    v59->reach.traveltype = 6;
                    v59->reach.traveltime = 10;
                    v59->next = areareachability[area1num];
                    areareachability[area1num] = v59;
                    ++reach_ladder;
                    lreach = AAS_AllocReachability();
                    if ( lreach )
                    {
                      v62 = absladderface1num;
                      lreach->reach.areanum = area1num;
                      lreach->reach.facenum = v62;
                      lreach->reach.edgenum = v66;
                      VectorCopy(trace.endpos, lreach->reach.start);
                      VectorMA(lowestpoint, -5.0, (float *)v85, lreach->reach.end);
                      lreach->reach.traveltype = 5;
                      lreach->reach.traveltime = 10;
                      lreach->reach.end[2] = lreach->reach.end[2] + 10;
                      lreach->next = areareachability[v53];
                      areareachability[v53] = lreach;
                      ++reach_jump;
                      return 1;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return 0;
}

// gladiator.dll: 10015BB0..10015FCC
// gladi386.so:   00023418..000238CD
int AAS_Reachability_Teleport()
{
  bsp_entity_t *v0; // eax — entity list head
  bsp_entity_t *ent; // ebx — current entrance entity
  const char *classname; // eax
  const char *target; // ebp
  bsp_entity_t *dest; // edi — current destination-search entity
  const char *v5; // eax
  const char *targetname; // eax
  int area2num; // ebp
  aas_link_t *areas; // ebx
  aas_link_t *i; // edi
  int area1num; // esi
  aas_reachabilitynode_t *lreach; // eax
  /* vec3 order is the ELF original's frame layout, not IDA's listing order: gcc 2.7
   * fills the address-taken group top-down in DECLARATION order, and declprobe.py
   * measured ours as maxs > destorigin > mins > origin > v29 > end > v30 while
   * slotmap.py wants origin where maxs is and end where v29 is. */
  vec3_t origin; // [ebp-84h] BYREF — teleport entrance origin (VectorForBSPEpairKey output)
  vec3_t destorigin; // [ebp-9Ch] BYREF — teleport destination origin (VectorForBSPEpairKey output)
  vec3_t mins; // [ebp-90h] BYREF — entrance bbox lower bound (UpdateEntityLinks)
  vec3_t maxs; // [ebp-A8h] BYREF — entrance bbox upper bound (UpdateEntityLinks)
  bsp_entity_t *v26; // [esp+5Ch] [ebp-74h] — list head saved for AAS_FreeBSPEntities
  vec3_t end; // [esp+64h] [ebp-6Ch] BYREF
  vec3_t v29; // [esp+70h] [ebp-60h] BYREF
  vec3_t v30; // [esp+7Ch] [ebp-54h] BYREF
  aas_trace_t trace; // [esp+88h] [ebp-48h] (was int v31[9] + char v32[36] hidden return buffer)

  v0 = AAS_ParseBSPEntities();
  ent = v0;
  if ( v0 )
  {
    v26 = v0;
    while ( 1 )
    {
      classname = (const char *)AAS_ValueForBSPEpairKey(ent, "classname");
      /* Flat early-out guards with `continue`, not nested if/else: nesting lets
       * cl.exe cross-jump-merge the error-print cold blocks, whereas the original
       * keeps each Print inline. */
      if ( !classname || strcmp(classname, "misc_teleporter") )
        goto cont;
      if ( !AAS_VectorForBSPEpairKey(ent, "origin", origin) )
      {
        /* `target` intentionally reused: it is function-scoped, so on this
         * early-out it still holds the PREVIOUS entity's target string (or garbage
         * on the first iteration).  Both oracles confirm there is no separate
         * variable — this print reads target's own stack slot. */
        botimport.Print(PRT_ERROR, "teleporter (%s) without origin\n", target);
        goto cont;
      }
      target = (const char *)AAS_ValueForBSPEpairKey(ent, "target");
      if ( !target )
      {
        botimport.Print(PRT_ERROR, "teleporter at %1.0f %1.0f %1.0f without target\n", origin[0], origin[1], origin[2]);
        goto cont;
      }
      for ( dest = v26; dest; dest = dest->next )
      {
        v5 = (const char *)AAS_ValueForBSPEpairKey(dest, "classname");
        if ( v5 )
        {
          if ( !strcmp(v5, "misc_teleporter_dest") )
          {
            targetname = (const char *)AAS_ValueForBSPEpairKey(dest, "targetname");
            if ( targetname )
            {
              if ( !strcmp(targetname, target) )
                break;
            }
          }
        }
      }
      if ( !dest )
      {
        botimport.Print(PRT_ERROR, "teleporter without destination (%s)\n", target);
        goto cont;
      }
      if ( !AAS_VectorForBSPEpairKey(dest, "origin", destorigin) )
      {
        botimport.Print(PRT_ERROR, "teleporter destination (%s) without origin\n", target);
        goto cont;
      }
      destorigin[2] = destorigin[2] + 24;
      end[0] = destorigin[0];
      end[1] = destorigin[1];
      end[2] = destorigin[2] - 100;
      trace = AAS_TraceClientBBox(destorigin, end, 4, -1);
      if ( trace.startsolid )
      {
        botimport.Print(PRT_ERROR, "teleporter destination (%s) in solid\n", target);
        goto cont;
      }
      VectorCopy(trace.endpos, destorigin);
      area2num = AAS_PointAreaNum(destorigin);
      mins[0] = -8.0;
      mins[1] = -8.0;
      mins[2] = 8.0;
      maxs[0] = 8.0;
      maxs[1] = 8.0;
      maxs[2] = 24.0;
      AAS_PresenceTypeBoundingBox(4, v29, v30);
      /* `origin` is the FIRST argument: it is the operand that stays on the x87 stack
       * (`fld [origin]; fld st(0); fadd [mins]`) and is reused for the maxs add.  With
       * mins/maxs first gcc reloads them and adds st(1) instead. */
      VectorAdd(origin, mins, mins);
      VectorAdd(origin, maxs, maxs);
      VectorSubtract(mins, v30, mins);
      VectorSubtract(maxs, v29, maxs);
      areas = AAS_AASLinkEntity(mins, maxs, -1);
      for ( i = areas; i; i = i->next_area )
      {
        if ( AAS_AreaGrounded(i->areanum) )
        {
          area1num = i->areanum;
          lreach = AAS_AllocReachability();
          if ( !lreach )
            break;
          lreach->reach.areanum = area2num;
          lreach->reach.facenum = 0;
          lreach->reach.edgenum = 0;
          VectorCopy(origin, lreach->reach.start);
          VectorCopy(destorigin, lreach->reach.end);
          lreach->reach.traveltype = 10;
          lreach->reach.traveltime = 50;
          lreach->next = areareachability[area1num];
          areareachability[area1num] = lreach;
          ++reach_teleport;
        }
      }
      AAS_UnlinkFromAreas(areas);
cont:
      ent = ent->next;
      if ( !ent )
        break;
    }
    v0 = v26;
  }
  return ((int (__cdecl *)(bsp_entity_t *))AAS_FreeBSPEntities)(v0);
}

// gladiator.dll: 100160E0..1001696B
// gladi386.so:   000238D0..0002454E
void AAS_Reachability_Elevator()
{
  /* Declaration order of the SCALARS is independent of the by-reference block
   * below: gcc 2.7 fills the address-taken group and the spilled-scalar group
   * separately, each top-down in its own declaration order, so interleaving does
   * not matter and only the order among scalars sets the spill slots.  The order
   * here is the one the reference .so's spill slots record. */
  int modelnum; // eax
  int i;
  int k;
  const char *classname; // eax
  char *model; // eax — AAS_ValueForBSPEpairKey return
  int v10; // ebx
  int area1num; // edi
  int v12; // esi
  int area2num; // ebx
  int l; // ebp
  int p; // ecx
  aas_reachabilitynode_t *v20; // eax
  aas_reachabilitynode_t *lreach; // esi
  float v26; // st7 (was double)
  float height;
  float speed;
  bsp_entity_t *v0; // edi
  bsp_entity_t *ent; // ebp — current entity walk
  /* Contiguous vec3_t arrays: BSPModelMinsMaxs writes 12 bytes to each of
   * mins/maxs/origin, and every other vec3 trio passed by address here has the
   * same requirement. */
  /* Declaration order of the by-reference vec3s is recovered from the reference
   * .so's frame: gcc 2.7 fills the address-taken group top-down in declaration
   * order, and matching each side's per-member REFERENCE-COUNT triples pairs them
   * without needing an instruction-level diff (see regionfp.py).  The first four
   * come out as mins, maxs, origin, angles -- which is Q3's own order in
   * be_aas_reach.c's AAS_Reachability_Elevator, independent corroboration that
   * the frame really does record declaration order. */
  vec3_t mins;          /* was v34/v35/v36 — BSPModelMinsMaxs mins out */
  vec3_t maxs;          /* was v30/v31/v32 — BSPModelMinsMaxs maxs out */
  vec3_t origin;        /* was v53/v54/v55 — BSPModelMinsMaxs origin out */
  float angles[3];         /* [BYREF] — angles to BSPModelMinsMaxs */
  /* `pos1` is a WHOLE vec3, not the single float IDA saw.  Q3 does
   * `VectorCopy(origin, pos1)` and then only ever reads `pos1[2]`, so the
   * decompiler kept the one live component and dropped the array -- but the
   * reference .so allocates all 12 bytes and references them 2 / 1 / 1 (one
   * store each from the VectorCopy, plus the one read of [2]), which is exactly
   * this shape.  Restoring it is what makes our address-taken group 332 bytes
   * like ref's instead of 320. */
  vec3_t pos1;          /* was v75 — top position; only [2] is ever read */
  vec3_t extent;        /* was v64[2]+v65 — Q3's `pos2`, the bottom position */
  vec3_t sumvec;        /* was v37/v38 (+missing v39) — mins+maxs sum for VectorMA */
  vec3_t toporg;        /* was v57/v58/v59 — VectorMA midpoint output (top) */
  vec3_t btmorg;        /* was v61/v62/v63 — VectorMA midpoint output (bottom) */
  vec3_t testpt;        /* was v40/v41/v42 — point for AAS_PointAreaNum */
  vec3_t samplept;      /* was v47/v48/v49 — per-iteration sample point */
  float start[3];         /* [BYREF] */
  float end[3];         /* [BYREF] */
  vec3_t dirvec;        /* was v44/v45/v46 — VectorNormalize input/output */
  float v33;            /* lip value ("lip" epair, defaults to 8.0) */
  /* Candidate-probe offset tables — plain floats, read by indexed loops.  Q3
   * declares them `vec_t xvals[8], yvals[8], xvals_top[8], yvals_top[8];` on one
   * line with `aas_trace_t trace` last, and the reference .so agrees: 128 bytes
   * of tables (four eights, not a ten and three eights) sitting directly above a
   * 36-byte trace at the bottom of the address-taken group.  The `< 9` probe loop
   * guards its ninth iteration with `if (v10 < 8)`, so nothing reads past [7]. */
  float xvals[8];
  float yvals[8];
  float xvals_top[8];
  float yvals_top[8];
  aas_trace_t trace;

  angles[0] = 0;
  angles[1] = 0;
  angles[2] = 0;
  v0 = AAS_ParseBSPEntities();
  for ( ent = v0; ent; ent = ent->next )
  {
    classname = (const char *)AAS_ValueForBSPEpairKey(ent, "classname");
    if ( !classname )
      continue;
    if ( strcmp(classname, "func_plat") )
      continue;
    model = AAS_ValueForBSPEpairKey(ent, "model");
    if ( !model )
    {
      botimport.Print(PRT_ERROR, "func_plat without model\n");
      continue;
    }
    modelnum = atoi(model + 1);
    if ( modelnum <= 0 )
    {
      botimport.Print(PRT_ERROR, "func_plat with invalid model number\n");
      continue;
    }
    AAS_BSPModelMinsMaxsOrigin(modelnum, angles, mins, maxs, origin);
    VectorCopy(origin, pos1);
    VectorCopy(origin, extent);
    v33 = FloatForKey(ent, "lip");
    if ( v33 == 0 )
      v33 = 8.0f;
    height = FloatForKey(ent, "height");
    if ( height == 0 )
      height = maxs[2] - mins[2] - v33;
    speed = FloatForKey(ent, "speed");
    if ( speed == 0 )
      speed = 200.0f;
    extent[2] = extent[2] - height;
    VectorAdd(mins, maxs, sumvec);
    VectorMA(extent, 0.5f, sumvec, toporg);
    toporg[2] = maxs[2] - (pos1[2] - extent[2]) + 2.0f;
    VectorAdd(mins, maxs, sumvec);
    VectorMA(extent, 0.5f, sumvec, btmorg);
    btmorg[2] = maxs[2] + 2.0f;
    for ( i = 0; i < 3; ++i )
    {
      mins[i] -= 1.0f;
      maxs[i] += 1.0f;
    }
    VectorAdd(mins, maxs, sumvec);
    VectorScale(sumvec, 0.5f, sumvec);
    xvals[0] = mins[0];
    xvals[1] = sumvec[0];
    xvals[2] = maxs[0];
    xvals[3] = sumvec[0];
    yvals[0] = sumvec[1];
    yvals[1] = maxs[1];
    yvals[2] = sumvec[1];
    yvals[3] = mins[1];
    xvals[4] = mins[0];
    xvals[5] = maxs[0];
    xvals[6] = maxs[0];
    xvals[7] = mins[0];
    yvals[4] = maxs[1];
    yvals[5] = maxs[1];
    yvals[6] = mins[1];
    yvals[7] = mins[1];
    /* 9 candidate positions around the plat: 8 side probes (v10 = 0..7) plus
     * 1 middle-of-plat probe (v10 == 8). */
    for ( v10 = 0; v10 < 9; ++v10 )
    {
      if ( v10 < 8 )
      {
        testpt[0] = origin[0] + xvals[v10];
        testpt[1] = origin[1] + yvals[v10];
        testpt[2] = toporg[2] + 16.0f;
        area1num = AAS_PointAreaNum(testpt);
        for ( v12 = 0; v12 < 16; ++v12 )
        {
          if ( area1num && (AAS_AreaGrounded(area1num) || AAS_AreaSwim(area1num)) )
            break;
          testpt[2] = testpt[2] + 4.0f;
          area1num = AAS_PointAreaNum(testpt);
        }
        if ( v12 >= 16 )
          continue;
      }
      else
      {
        VectorCopy(btmorg, testpt);
        testpt[2] = testpt[2] + 24.0f;
        area1num = AAS_PointAreaNum(testpt);
        if ( !area1num )
          continue;
        VectorCopy(toporg, testpt);
        testpt[2] = testpt[2] + 24.0f;
      }
        for ( i = 0; i < 3; ++i )
        {
          for ( k = 0; k < 3; ++k )
          {
            mins[k] -= 4.0f;
            maxs[k] += 4.0f;
          }
          xvals_top[0] = mins[0];
          xvals_top[1] = sumvec[0];
          xvals_top[2] = maxs[0];
          xvals_top[3] = sumvec[0];
          yvals_top[0] = sumvec[1];
          yvals_top[1] = maxs[1];
          yvals_top[2] = sumvec[1];
          yvals_top[3] = mins[1];
          xvals_top[4] = mins[0];
          xvals_top[5] = maxs[0];
          xvals_top[6] = maxs[0];
          xvals_top[7] = mins[0];
          yvals_top[4] = maxs[1];
          yvals_top[5] = maxs[1];
          yvals_top[6] = mins[1];
          yvals_top[7] = mins[1];
          for ( k = 0; k < 8; ++k )
          {
            samplept[0] = origin[0] + xvals_top[k];
            samplept[1] = origin[1] + yvals_top[k];
            samplept[2] = btmorg[2] + 16.0f;
            area2num = AAS_PointAreaNum(samplept);
            for ( l = 0; l < 16; ++l )
            {
              if ( area2num && (AAS_AreaGrounded(area2num) || AAS_AreaSwim(area2num)) )
              {
                VectorCopy(btmorg, start);
                start[2] += 32.0f;
                VectorCopy(samplept, end);
                end[2] += 1.0f;
                trace = AAS_TraceClientBBox(start, end, 4, -1);
                if ( trace.fraction >= 1.0f )
                  break;
              }
              samplept[2] = samplept[2] + 4.0f;
              area2num = AAS_PointAreaNum(samplept);
            }
            if ( l >= 16 )
              goto LABEL_53;
            if ( area2num != area1num && AAS_AreaGrounded(area2num) && !AAS_ReachabilityExists(area1num, area2num) )
            {
              VectorSubtract(testpt, toporg, dirvec);
              VectorNormalize(dirvec);
              dirvec[2] = testpt[2];
              dirvec[0] = dirvec[0] * 24.0f + testpt[0];
              dirvec[1] = dirvec[1] * 24.0f + testpt[1];
              for ( p = 0; p < 3; ++p )
              {
                if ( mins[p] + origin[p] > dirvec[p] || maxs[p] + origin[p] < dirvec[p] )
                  break;
              }
              if ( p >= 3 )
                goto LABEL_53;
              v20 = AAS_AllocReachability();
              lreach = v20;
              if ( v20 )
              {
                v20->reach.areanum = area2num;
                v20->reach.facenum = modelnum;
                lreach->reach.edgenum = (int)height;
                VectorCopy(dirvec, lreach->reach.start);
                VectorCopy(samplept, lreach->reach.end);
                lreach->reach.traveltype = 11;
                v26 = height * 100.0f / speed;
                lreach->reach.traveltime = (__int64)v26;
                if ( !(unsigned __int16)(__int64)v26 )
                  lreach->reach.traveltime = 50;
                /* Q3's `n = 9999;` -- "don't go any further to the outside", a
                 * sentinel that ends the enclosing 3-iteration loop.  It really is
                 * an assignment in the original, not a `break`: the reference .so
                 * emits the store.  Q3 keeps it after the list-link statements;
                 * measured either way, the position is byte-neutral here, so this
                 * keeps IDA's. */
                i = 9999;
                lreach->next = areareachability[area1num];
                areareachability[area1num] = lreach;
                ++reach_elevator;
              }
            }
LABEL_53:
            ;
          }
        }
    }
  }
  AAS_FreeBSPEntities(v0);
}

// gladiator.dll: 10016BA0..100171BC
// gladi386.so:   00024550..00024DE6
int __cdecl AAS_Reachability_Grapple(int area1num, int area2num)
{
  int i; // eax
  aas_area_t *area2; // ebp
  aas_area_t *area1; // ecx
  int v4;
  int face2num; // ebp
  aas_face_t *face2; // esi
  float *v; // rax (was __int64) — pointer to vertex (float[3])
  float hordist; // st7 (was double)
  int areanum; // edi
  aas_reachabilitynode_t *lreach; // eax
  aas_reachabilitynode_t *v13; // esi (was int) — alias of lreach (aas_reachability_t *)
  float v36;          /* VectorLength horizontal-distance result */
  float v37;          /* vertical delta (vertex_z - grounded_z) */
  /* Contiguous vec3_t arrays: VectorLength, VectorNormalize and AAS_Trace all
   * read three floats from the pointers passed in.  The order below is the
   * original declaration order, recovered from the reference .so's frame (gcc
   * 2.7 lays the address-taken group out top-down in declaration order): the
   * two trace structs first, then Q3's own vec3 list `areastart, facecenter,
   * start, end, dir, down`.  IDA cannot see this -- MSVC /O2 assigns slots in
   * first-reference order, so the DLL carries no declaration-order signal. */
  bsp_trace_t bsptrace; /* [BYREF] */
  aas_trace_t trace;
  vec3_t areastart;    /* was v27/v28/v29 — origin dropped onto floor */
  vec3_t facecenter;      /* was v33/v34/v35 — face center from AAS_FaceCenter */
  vec3_t start;   /* was v24/v25/v26 — center origin (trace start) */
  vec3_t end;        /* VectorMA output */
  vec3_t dir;       /* was v21/v22/v23 — vertex - origin delta vec */
  vec3_t down = { 0, 0, -1 };

  if ( !AAS_AreaGrounded(area1num) && !AAS_AreaSwim(area1num) )
    return 0;
  if ( (AAS_AreaPresenceType(area1num) & 2) == 0 )
    return 0;
  if ( AAS_AreaSwim(area1num) )
    return 0;
  area1 = &aasworld.areas[area1num];
  area2 = &aasworld.areas[area2num];
  if ( area2->maxs[2] < area1->mins[2] )
    return 0;
  VectorCopy(aasworld.areas[area1num].center, start);
  if ( !AAS_AreaSwim(area1num) )
  {
    if ( !AAS_PointAreaNum(start) )
      Log_Write("area %d center %f %f %f in solid?", area1num, start[0],
                start[1], start[2]);
    VectorCopy(start, end);
    end[2] -= 1000.0f;
    trace = AAS_TraceClientBBox(start, end, 4, -1);
    if ( trace.startsolid )
      return 0;
    VectorCopy(trace.endpos, areastart);
  }
  else
  {
    v4 = sub_10003080((float *)start);   /* swim-area liquid check */
    if ( (v4 & 0x38) == 0 )
      return 0;
  }
  for ( i = 0; i < area2->numfaces; i++ )
  {
    face2num = aasworld.faceindex[area2->firstface + i];
    face2 = &aasworld.faces[abs(face2num)];
    if ( (face2->faceflags & 1) != 0 )
    {
      v = aasworld.vertexes[aasworld.edges[abs(aasworld.edgeindex[face2->firstedge])].v[0]];
      VectorSubtract(v, areastart, dir);
      if ( dir[2] * aasworld.planes[face2->planenum].normal[2]
         + dir[1] * aasworld.planes[face2->planenum].normal[1]
         + dir[0] * aasworld.planes[face2->planenum].normal[0] <= 0.0f )
      {
        AAS_FaceCenter(face2num, facecenter);
        if ( areastart[2] + 64.0f <= facecenter[2] && DotProduct(aasworld.planes[face2->planenum].normal, down) >= 0.0f )
        {
          dir[2] = 0.0f;
          dir[0] = facecenter[0] - areastart[0];
          dir[1] = facecenter[1] - areastart[1];
          v37 = facecenter[2] - areastart[2];
          hordist = VectorLength(dir);
          v36 = hordist;
          if ( hordist != 0.0f && v36 <= 2000.0f && v37 / v36 >= tan(0.2617993877991494) )
          {
            VectorCopy(facecenter, start);
            /* Indexed access: aas_plane_t is 20 BYTES (normal[3] + dist + type), so the
             * decompiler's `+ 20 * planenum` on a float* advanced 80 bytes per plane. */
            VectorMA(facecenter, -500.0f, aasworld.planes[face2->planenum].normal, end);
            bsptrace = AAS_Trace((float*)(start), (float*)(uintptr_t)(0), (float*)(uintptr_t)(0), (float*)(end), 0, 100663299);
            if ( (LOBYTE(bsptrace.surface.flags) & 4) == 0 && bsptrace.fraction * 500.0f < 32.0f )
            {
              VectorSubtract(facecenter, areastart, dir);
              VectorNormalize(dir);
              VectorMA(areastart, 4.0f, dir, start);
              /* All three vmav[] slots are written here. */
              VectorCopy(bsptrace.endpos, end);
              trace = AAS_TraceClientBBox(start, end, 2, -1);
              VectorSubtract(trace.endpos, facecenter, dir);
              if ( VectorLength(dir) <= 24.0f )
              {
                VectorCopy(trace.endpos, start);
                /* All three vmav[] slots are written from trace.endpos. */
                VectorCopy(trace.endpos, end);
                end[2] -= AAS_FallDamageDistance();
                trace = AAS_TraceClientBBox(start, end, 2, -1);
                if ( trace.fraction < 1.0f )
                {
                  areanum = AAS_PointAreaNum(trace.endpos);
                  if ( (aasworld.areasettings[areanum].contents & 6) == 0
                    && areanum != area1num
                    && !AAS_ReachabilityExists(area1num, areanum)
                    && AAS_AreaGrounded(areanum) )
                  {
                    lreach = AAS_AllocReachability();
                    v13 = lreach;
                    if ( !lreach )
                      return 0;
                    lreach->reach.areanum = areanum;
                    lreach->reach.facenum = face2num;
                    lreach->reach.edgenum = 0;
                    lreach->reach.start[0] = areastart[0];
                    lreach->reach.start[1] = areastart[1];
                    v13->reach.start[2] = areastart[2];
                    VectorCopy(bsptrace.endpos, v13->reach.end);
                    v13->reach.traveltype = 14;
                    dir[0] = bsptrace.endpos[0] - lreach->reach.start[0];
                    dir[1] = v13->reach.end[1] - v13->reach.start[1];
                    dir[2] = v13->reach.end[2] - v13->reach.start[2];
                    v13->reach.traveltime = (__int64)(VectorLength(dir) * 0.25 + 500.0);
                    v13->next = areareachability[area1num];
                    areareachability[area1num] = v13;
                    ++reach_grapple;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return 0;
}

// gladiator.dll: 10017350..10017ABC
// gladi386.so:   00024DE8..000251E4
int AAS_SetWeaponJumpAreaFlags()
{
  bsp_entity_t *v0; // ebx — entity list head
  bsp_entity_t *ent; // ebp — current entity
  const char *v2; // eax
  const char *classname; // edi
  int areanum; // eax
  vec3_t mins; // BYREF
  vec3_t maxs; // BYREF
  vec3_t origin; // BYREF

  /* Float literals, not the decompiler's raw ±15.0f bit patterns: as int literals
   * assigned to a float[3] they convert to ~±1.1e9, leaving the bounds effectively
   * unbounded so AAS_BestReachableArea flags the wrong areas. */
  mins[0] = -15.0f;
  mins[1] = -15.0f;
  mins[2] = -15.0f;
  maxs[0] = 15.0f;
  maxs[1] = 15.0f;
  maxs[2] = 15.0f;
  v0 = AAS_ParseBSPEntities();
  ent = v0;
  if ( v0 )
  {
    do
    {
      v2 = (const char *)AAS_ValueForBSPEpairKey(ent, "classname");
      classname = v2;
      if ( v2
        && (!strcmp(v2, "item_armor_body")
            || !strcmp(classname, "item_armor_combat")
            || !strcmp(classname, "item_power_screen")
            || !strcmp(classname, "item_power_shield")
            || !strcmp(classname, "weapon_grenadelauncher")
            || !strcmp(classname, "weapon_rocketlauncher")
            || !strcmp(classname, "weapon_hyperblaster")
            || !strcmp(classname, "weapon_railgun")
            || !strcmp(classname, "weapon_bfg")
            || !strcmp(classname, "weapon_boomer")
            || !strcmp(classname, "weapon_phalanx")
            || !strcmp(classname, "item_quadfire")
            || !strcmp(classname, "weapon_etf_rifle")
            || !strcmp(classname, "weapon_proxlauncher")
            || !strcmp(classname, "weapon_plasmabeam")
            || !strcmp(classname, "weapon_chainfist")
            || !strcmp(classname, "weapon_disintegrator")
            || !strcmp(classname, "item_ir_goggles")
            || !strcmp(classname, "item_double")
            || !strcmp(classname, "item_compass")
            || !strcmp(classname, "item_sphere_vengeance")
            || !strcmp(classname, "item_sphere_hunter")
            || !strcmp(classname, "item_sphere_defender")
            || !strcmp(classname, "item_doppleganger")
            || !strcmp(classname, "dm_tag_token")
            || !strcmp(classname, "dm_tag_token")
            || !strcmp(classname, "item_health_mega")
            || !strcmp(classname, "item_quad")
            || !strcmp(classname, "item_invulnerability"))
        && AAS_VectorForBSPEpairKey(ent, "origin", (float *)origin) )
      {
        if ( !AAS_DropToFloor((float *)origin, (float *)mins, (float *)maxs) )
          botimport.Print(
            1,
            "%s in solid at (%1.1f %1.1f %1.1f)\n",
            classname,
            *(float *)origin,
            origin[1],
            origin[2]);
        areanum = AAS_BestReachableArea(origin, (float *)mins, (float *)maxs, (float *)origin);
        aasworld.areasettings[areanum].areaflags |= 0x2000u;
      }
      ent = ent->next;
    }
    while ( ent );
  }
  return ((int (__cdecl *)(bsp_entity_t *))AAS_FreeBSPEntities)(v0);
}

// gladiator.dll: 10017CA0..100180B1
// gladi386.so:   000251E4..00025707
int __cdecl AAS_Reachability_WeaponJump(int area1num, int area2num)
{
  aas_area_t *area2; // ebx
  aas_area_t *area1; // ecx
  int v4; // eax
  int i; // esi
  int face2num; // rax (only the low 32 bits are used; abs() idiom)
  int n; // ebp
  int v9; // edi
  int v10; // esi
  int v11; // eax
  int reached;
  int reached_face;
  aas_reachabilitynode_t *lreach; // eax
  float zvel; // [esp+28h] [ebp-130h]
  /* Contiguous vec3_t arrays. */
  vec3_t groundedpos;   /* was v17/v18/v19 — origin dropped onto floor */
  vec3_t centerorg;     /* was v20/v21/v22 — area center origin */
  int v23;
  float speed;
  vec3_t predictpos;    /* was v31[2]+v32 — VectorMA output (predicted landing) */
  float hordist;
  vec3_t velocity; /* [BYREF] */
  vec3_t end; /* [BYREF] */
  vec3_t dir; /* [BYREF] */
  vec3_t cmdmove; /* [BYREF] */
  aas_clientmove_t move; /* [BYREF] */
  vec3_t facecenter;    /* was v25/v26/v27 — face-center from AAS_FaceCenter */
  aas_trace_t trace;

  if ( !AAS_AreaGrounded(area1num) || AAS_AreaSwim(area1num) ) return 0;
  if ( !AAS_AreaGrounded(area2num) ) return 0;
  if ( (aasworld.areasettings[area2num].areaflags & 0x2000) == 0 ) return 0;
  area1 = &aasworld.areas[area1num];
  area2 = &aasworld.areas[area2num];
  if ( area2->maxs[2] < (float)area1->mins[2] )
    return 0;
  VectorCopy(aasworld.areas[area1num].center, centerorg);
  if ( !AAS_PointAreaNum(centerorg) )
    Log_Write("area %d center %f %f %f in solid?", area1num, centerorg[0],
              centerorg[1], centerorg[2]);
  VectorCopy(centerorg, end);
  end[2] -= 1000.0f;
  trace = AAS_TraceClientBBox(centerorg, end, 4, -1);
  if ( trace.startsolid )
    return 0;
  VectorCopy(trace.endpos, groundedpos);
  v4 = area2->numfaces;
  i = 0;
  v23 = 0;
  if ( v4 <= 0 )
    return 0;
  while ( 1 )
  {
    face2num = aasworld.faceindex[i + area2->firstface];
    if ( (aasworld.faces[abs(face2num)].faceflags & 4) != 0 )
    {
      AAS_FaceCenter(aasworld.faceindex[i + area2->firstface], facecenter);
      if ( groundedpos[2] + 64.0f <= facecenter[2] )
      {
        n = 0;
        reached_face = 0;
        while ( 1 )
        {
          if ( n )
            zvel = AAS_BFGJumpZVelocity(groundedpos);
          else
            zvel = AAS_RocketJumpZVelocity(groundedpos);
          if ( AAS_HorizontalVelocityForJump(zvel, groundedpos, facecenter, &speed) )
          {
            if ( speed < 270.0f )
            {
              dir[2] = 0;
              dir[0] = facecenter[0] - groundedpos[0];
              dir[1] = facecenter[1] - groundedpos[1];
              hordist = VectorNormalize(dir);
              if ( hordist < facecenter[2] * 1.6 - groundedpos[2] )
              {
                VectorScale(dir, speed, cmdmove);
                velocity[2] = zvel;
                velocity[0] = 0;
                velocity[1] = 0;
                move = AAS_ClientMovementPrediction(-1, groundedpos, 2, 1, velocity, cmdmove, 3, 30, 0.1f, 61, 0);
                if ( move.frames < 30 && (move.stopevent & 0x38) == 0 )
                {
                  reached = 0;
                  for ( v9 = 0, v10 = 0; v10 >= -32; v10 -= 8, v9 += 8 )
                  {
                    VectorMA(move.endpos, (float)v10, dir, predictpos);
                    predictpos[2] += 0.125;
                    if ( AAS_PointAreaNum(predictpos) == area2num )
                    {
                      reached = 1;
                      break;
                    }
                  }
                  if ( reached && v9 <= 32 )
                  {
                    reached_face = 1;
                    break;
                  }
                }
              }
            }
          }
          if ( ++n >= 1 )
            break;
        }
        if ( reached_face )
          break;
      }
    }
    v11 = area2->numfaces;
    v23 = ++i;
    if ( i >= v11 )
      return 0;
  }
  lreach = AAS_AllocReachability();
  if ( !lreach )
    return 0;
  lreach->reach.facenum = 0;
  lreach->reach.areanum = area2num;
  lreach->reach.edgenum = 0;
  VectorCopy(groundedpos, lreach->reach.start);
  VectorCopy(facecenter, lreach->reach.end);
  if ( n )
    lreach->reach.traveltype = 13;
  else
    lreach->reach.traveltype = 12;
  lreach->reach.traveltime = 500;
  lreach->next = areareachability[area1num];
  areareachability[area1num] = lreach;
  ++reach_rocketjump;
  return 1;
}

// gladiator.dll: 100181D0..1001869C
// gladi386.so:   00025708..00025D6C
void __cdecl AAS_Reachability_WalkOffLedge(int areanum)
{
  int i; // ebx
  int j; // eax
  int k; // edi
  int l; // ecx
  int m; // ecx
  int n; // [esp+8h] [ebp-ACh]
  int face1num; // rax (was __int64 — abs32 idiom)
  int face2num; // rax (was __int64 — abs32 idiom)
  int edge1num; // [esp+18h] [ebp-9Ch]
  int edge2num; // rax (was __int64 — abs32 idiom)
  int otherareanum; // ecx
  int gap; // ebx
  int reachareanum; // eax
  int v10; // edi
  int v25; // eax
  int v33; // edi
  int v36; // edx
  int v46; // [esp+24h] [ebp-90h]
  unsigned int v20; // eax
  float v39; // [esp+8h] [ebp-ACh]
  BOOL side; // ecx
  aas_area_t *area; // esi
  aas_area_t *area2; // eax
  aas_face_t *face1; // edx
  aas_face_t *face2; // esi
  aas_face_t *face3; // ebp
  aas_edge_t *edge; // eax
  aas_plane_t *plane;
  float *v29; // esi
  float *v30; // edi
  aas_reachabilitynode_t *lreach; // eax
  aas_reachabilitynode_t *v35; // esi (was int) — alias of lreach
  /* midorigin: the edge midpoint, lifted 8 units off the floor along edgecross.
   * Must be a real vec3_t — it is passed by address to VectorScale / VectorMA /
   * AAS_TraceClientBBox and copied as the reach's `start`. */
  float testend[3]; // [esp+48h] [ebp-6Ch] BYREF
  vec3_t midorigin; // [esp+Ch..14h] [ebp-A8h..-A0h] BYREF — v40/v41/v42 collapsed
  float dir[3]; // [esp+60h] [ebp-54h] BYREF
  float sharededgevec[3]; // [esp+54h] [ebp-60h] BYREF
  aas_trace_t trace; // [esp+6Ch] [ebp-48h] (was int v58[9] + char v59[36] hidden return buffer)
  if ( !AAS_AreaGrounded(areanum) || AAS_AreaSwim(areanum) )
    return;
  area = &aasworld.areas[areanum];
  for ( i = 0; i < area->numfaces; i++ )
  {
    face1num = aasworld.faceindex[area->firstface + i];
    face1 = &aasworld.faces[abs(face1num)];
    if ( (face1->faceflags & 4) == 0 )
      continue;
    for ( k = 0; k < face1->numedges; k++ )
    {
      edge1num = aasworld.edgeindex[face1->firstedge + k];
      for ( j = 0; j < area->numfaces; j++ )
      {
        face2num = aasworld.faceindex[area->firstface + j];
        v10 = abs(face2num);
        face2 = &aasworld.faces[v10];
        if ( (face2->faceflags & 4) != 0 )
          continue;
        for ( l = 0; l < face2->numedges; l++ )
        {
          edge2num = aasworld.edgeindex[l + face2->firstedge];
          if ( abs(edge1num) != abs(edge2num) )
            continue;
          otherareanum = face2->frontarea;
          if ( otherareanum == areanum )
            otherareanum = face2->backarea;
          area2 = &aasworld.areas[otherareanum];
          if ( (aasworld.areasettings[otherareanum].areaflags & 1) != 0 )
          {
            gap = 0;
            for ( n = 0; n < area2->numfaces; n++ )
            {
              v20 = abs(aasworld.faceindex[area2->firstface + n]);
              if ( v20 == v10 )
                continue;
              face3 = &aasworld.faces[v20];
              for ( m = 0; m < face3->numedges; m++ )
              {
                if ( abs(aasworld.edgeindex[face3->firstedge + m]) == abs(edge1num) )
                {
                  v25 = face3->faceflags;
                  if ( (v25 & 1) == 0 )
                  {
                    gap = 1;
                    break;
                  }
                  if ( (v25 & 4) != 0 )
                  {
                    gap = 0;
                    break;
                  }
                  gap = 1;
                  break;
                }
              }
              if ( m < face3->numedges )
                break;
            }
            if ( !gap )
              break;
          }
          edge = &aasworld.edges[abs(edge1num)];
          side = edge1num < 0;
          v29 = (float *)(&aasworld.vertexes[edge->v[side]]);
          v30 = (float *)(&aasworld.vertexes[edge->v[!side]]);
          plane = &aasworld.planes[face1->planenum];
          VectorSubtract(v30, v29, sharededgevec);
          CrossProduct(plane->normal, sharededgevec, dir);
          VectorNormalize(dir);
          VectorAdd(v29, v30, midorigin);
          VectorScale(midorigin, 0.5, midorigin);
          VectorMA(midorigin, 8.0, (float *)dir, midorigin);
          VectorCopy(midorigin, testend);
          testend[2] -= 1000.0f;
          trace = AAS_TraceClientBBox(midorigin, (float *)testend, 4, -1);
          if ( trace.startsolid )
            break;
          reachareanum = AAS_PointAreaNum(trace.endpos);
          v33 = reachareanum;
          if ( reachareanum == areanum || AAS_ReachabilityExists(areanum, reachareanum) || !AAS_AreaGrounded(v33) && !AAS_AreaSwim(v33) )
            break;
          if ( (aasworld.areasettings[v33].contents & 6) != 0 )
            break;
          lreach = AAS_AllocReachability();
          v35 = lreach;
          if ( !lreach )
            break;
          v36 = edge1num;
          lreach->reach.areanum = v33;
          lreach->reach.facenum = 0;
          lreach->reach.edgenum = v36;
          VectorCopy(midorigin, lreach->reach.start);
          VectorCopy(trace.endpos, lreach->reach.end);
          lreach->reach.traveltype = 7;
          /* Q3's polarity (`if (!AAS_AreaSwim(a) && falldelta > threshold) penalise`,
           * be_aas_reach.c:4285): the fall test is a STRICT `<` on the threshold, which
           * is the operator gladi386.so's `and ah,0x45; dec ah; cmp ah,0x40; jb` encodes.
           * IDA's `>=` with the arms swapped is the negation and emits `and ah,0x45; je`. */
          if ( !AAS_AreaSwim(v33) && (v39 = midorigin[2] - trace.endpos[2], v46 = AAS_FallDamageDistance(), (float)v46 < v39) )
            v35->reach.traveltime = 3000;
          else
            v35->reach.traveltime = 100;
          v35->next = areareachability[areanum];
          areareachability[areanum] = v35;
          ++reach_walkoffledge;
        }
      }
    }
  }
}

// gladiator.dll: 100187E0..100188D1
// gladi386.so:   00025D6C..00025EA2
void AAS_StoreReachability()
{
  int i;
  aas_areasettings_t *areasettings;
  aas_reachabilitynode_t *lreach;
  aas_reachability_t *reach;

  if ( aasworld.reachability )
    FreeMemory(aasworld.reachability);
  aasworld.reachability = (aas_reachability_t *)GetClearedMemory(2883584);
  aasworld.reachabilitysize = 1;
  for ( i = 0; i < aasworld.numareas; i++ )
  {
    areasettings = &aasworld.areasettings[i];
    areasettings->firstreachablearea = aasworld.reachabilitysize;
    areasettings->numreachableareas = 0;
    for ( lreach = areareachability[i]; lreach; lreach = lreach->next )
    {
      reach = &aasworld.reachability[areasettings->firstreachablearea +
                                      areasettings->numreachableareas];
      reach->areanum = lreach->reach.areanum;
      reach->facenum = lreach->reach.facenum;
      reach->edgenum = lreach->reach.edgenum;
      VectorCopy(lreach->reach.start, reach->start);
      VectorCopy(lreach->reach.end, reach->end);
      reach->traveltype = lreach->reach.traveltype;
      reach->traveltime = lreach->reach.traveltime;
      areasettings->numreachableareas++;
    }
    aasworld.reachabilitysize += areasettings->numreachableareas;
  }
}

// gladiator.dll: 10018920..10018BB2
// gladi386.so:   00025EA4..00026229
int AAS_ContinueInitReachability(float time)
{
  int todo; // ebx
  libvar_t *v4;
  int i; // esi
  int start_time; // ebp
  int j; // edi

  (void)time; /* caller passes arg; original function ignores it (no ebp frame at 0x10018920) */

  if ( !aasworld.loaded )
    return 0;
  if ( aasworld.numreachabilityareas >= aasworld.numareas )
    return 0;
  /* Through the global, not IDA's `v1`/`v2` aliases: the original re-reads
   * libvar_framereachability for the `->value` load (`mov eax,[GOT]; fld
   * [eax+0x10]`), and the `double v2` copy is what makes our frame 0x14 against
   * real's 0x10. */
  if ( !libvar_framereachability )
  {
    libvar_framereachability = LibVar("framereachability", (char *)"20");
    if ( libvar_framereachability->value <= 0.0f )
      libvar_framereachability->value = 15.0f;
  }
  /* `(int)`, not IDA's `(__int64)`: gladi386.so does the 32-bit `fistp DWORD PTR
   * [esp]` round-to-zero sequence here, and the 64-bit cast costs an 8-byte temp
   * (frame 0x14 vs real's 0x10) plus the QWORD store.  `todo` is an int anyway;
   * MSVC's __ftol leaves the value in eax either way, which is why IDA wrote 64. */
  todo = aasworld.numreachabilityareas + (int)libvar_framereachability->value;
  if ( !libvar_reachabilitydelay )
  {
    v4 = LibVar("reachability_delay", (char *)"100");
    libvar_reachabilitydelay = v4;
    if ( v4->value <= 0.0f )
      v4->value = 200.0f;
  }
  start_time = Sys_MilliSeconds();
  i = aasworld.numreachabilityareas;
  for ( ; i < aasworld.numareas; ++i )
  {
    if ( i >= todo )
      break;
    ++aasworld.numreachabilityareas;
    for ( j = 1; j < aasworld.numareas; ++j )
    {
      if ( i != j
        && !AAS_ReachabilityExists(i, j)
        && !AAS_Reachability_Swim(i, j)
        && !AAS_Reachability_EqualFloorHeight(i, j)
        && !AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge(i, j)
        && !AAS_Reachability_Ladder(i, j) )
      {
        AAS_Reachability_Jump(i, j);
      }
    }
    for ( j = 1; j < aasworld.numareas; ++j )
    {
      if ( i != j && !AAS_ReachabilityExists(i, j) )
      {
        AAS_Reachability_Grapple(i, j);
        AAS_Reachability_WeaponJump(i, j);
      }
    }
    if ( Sys_MilliSeconds() - start_time > (int)libvar_reachabilitydelay->value )
      break;
  }
  if ( aasworld.numreachabilityareas >= aasworld.numareas )
  {
    for ( i = 1; i < aasworld.numareas; ++i )
      AAS_Reachability_WalkOffLedge(i);
    AAS_Reachability_Teleport();
    AAS_Reachability_Elevator();
    AAS_StoreReachability();
    AAS_ShutDownReachabilityHeap();
    FreeMemory(areareachability);
    botimport.Print(PRT_MESSAGE, "calculating clusters...\n");
  }
  else
  {
    if ( aasworld.numreachabilityareas - (int)libvar_framereachability->value <= 1 )
      botimport.Print(PRT_MESSAGE, "calculating reachability...\n");
    if ( aasworld.numreachabilityareas + (int)libvar_framereachability->value >= aasworld.numareas )
    {
      botimport.Print(PRT_MESSAGE, "\r%6d%%%%", 100);
      botimport.Print(PRT_MESSAGE, "\nplease wait while storing reachability...\n");
    }
    else
    {
      botimport.Print(PRT_MESSAGE, "\r%6d%%%%", 100 * aasworld.numreachabilityareas / aasworld.numareas);
    }
  }
  return 1;
}

// gladiator.dll: 10018C70..10018CD7
// gladi386.so:   0002622C..0002639C
void AAS_InitReachability()
{
  if ( aasworld.loaded )
  {
    if ( aasworld.reachabilitysize && !(unsigned int)(int)LibVarGetValue("forcereachability") )
    {
      aasworld.numreachabilityareas = aasworld.numareas;
    }
    else
    {
      aasworld.savefile = 1;
      aasworld.numreachabilityareas = 1;
      AAS_SetupReachabilityHeap();
      areareachability = (aas_reachabilitynode_t **)GetClearedMemory(aasworld.numareas * sizeof(aas_reachabilitynode_t *));
      AAS_SetWeaponJumpAreaFlags();
    }
  }
}
