/*
 * be_aas_bspq2.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10003010..0x100085F0.
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
#include "be_aas_bspq2.h"
#include "be_aas_entity.h"
#include "be_aas_light.h"
#include "be_aas_main.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_utils.h"

/* Type + offset asserts live in be_aas_bspq2.h, the single instance here. */
bspworld_t bspworld;

// gladiator.dll: 10003010..10003056
// gladi386.so:   0000A3F8..0000A442
/* Returns bsp_trace_t BY VALUE; MSVC lowers that to a hidden caller-allocated
 * return buffer.
 *
 * THE ENGINE TRACE, on both platforms.  It used to be `#ifdef _WIN32`-split,
 * with this body on Windows and a direct `AAS_TraceBSPModel(0, ...)` body on
 * Linux, on the reading that "the two 1999 images have genuinely different
 * bodies here".  That reading was WRONG, and the correction is worth
 * understanding because it was invisible to both audits (they mask call
 * displacements):
 *
 *   - gladi386.so has BOTH bodies.  F663 (0xa3f8, 74 B) is this one -- it
 *     loads a GOT slot that `.rel.got` names `botimport` and calls
 *     `[edi+0xc]` -- and ALL 30 of the .so's trace call sites go to it.
 *   - F680 (0xd0cc, 106 B) is the direct-AAS_TraceBSPModel body, and it is
 *     called by NOBODY.  We already had it, as sub_10005640 below, which the
 *     DLL likewise keeps uncalled at 0x10005640.
 *
 * So the Linux build was routing every bot trace into the world BSP model
 * alone, where the 1999 build went through the engine and therefore saw
 * entities and movers.  That is a behavioural divergence, not only a byte
 * one.  (x87cmp/soannotate/contentsweep, 2026-08-16.) */
bsp_trace_t __cdecl AAS_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask)
{
#if defined(__x86_64__) || defined(__aarch64__)
  /* 64-bit only: the retbuf is a hidden register there, so the game side
   * hands us one explicitly.  See botimport_block_t's Trace member. */
  bsp_trace_t bsptrace;

  return *botimport.Trace(&bsptrace, start, mins, maxs, end, passent, contentmask);
#else
  return botimport.Trace(start, mins, maxs, end, passent, contentmask);
#endif
}
/* BSP-leaf link heap.  Gladiator carries TWO link families and this is the
 * BSP-leaf one; the AAS-area trio at 0x1001AC00/0x1001AD50/0x1001ADA0 already
 * owns the Q3 cognate names (AAS_InitAASLinkHeap etc.).  They are distinguished
 * at every site: "max_bsplinks" vs "max_aaslinks", "empty bsp link heap" vs
 * "empty aas link heap", link->prev_leaf vs link->prev_area.  A verbatim
 * cognate name is impossible, so this trio stays unnamed — do NOT resolve it by
 * renaming the 0x1001ACxx family. */
// gladiator.dll: 10003080..1000308F
// gladi386.so:   0000A444..0000A466
// thin wrapper forwarding to bi_PointContents
int __cdecl sub_10003080(vec3_t point)
{
  return botimport.PointContents(point);
}
// gladiator.dll: 100030A0..10003162
// gladi386.so:   0000A468..0000A722
void sub_100030A0()
{
  int i;
  int count = bspworld.dword_1006957C;

  if ( !bspworld.dword_10069578 )
  {
    count = (intptr_t)LibVarValue("max_bsplinks", (char *)"4096");
    if ( count < 0 )
      count = 0;
    bspworld.dword_1006957C = count;
    bspworld.dword_10069578 = (bsp_link_t *)GetMemory(sizeof(bsp_link_t) * count);
  }
  /* Doubly-linked free list via next_ent / prev_ent slots. */
  bspworld.dword_10069578[0].prev_ent = NULL;
  bspworld.dword_10069578[0].next_ent = &bspworld.dword_10069578[1];
  for ( i = 1; i < count - 1; ++i )
  {
    bspworld.dword_10069578[i].prev_ent = &bspworld.dword_10069578[i - 1];
    bspworld.dword_10069578[i].next_ent = &bspworld.dword_10069578[i + 1];
  }
  bspworld.dword_10069578[count - 1].prev_ent = &bspworld.dword_10069578[count - 2];
  bspworld.dword_10069578[count - 1].next_ent = NULL;
  bspworld.dword_10069580 = bspworld.dword_10069578;
}
// gladiator.dll: 100031B0..100031DA
// gladi386.so:   0000A724..0000A76B
// Walks the bsp_link freelist headed at dword_10069580 and prints
// "%d free bsp links, %s\n".  DEAD in Gladiator.
void __cdecl sub_100031B0(char *name)
{
  bsp_link_t *node;
  int count;

  count = 0;
  for ( node = bspworld.dword_10069580; node; node = node->next_ent )
    ++count;
  botimport.Print(PRT_MESSAGE, "%d free bsp links, %s\n", count, name);
}
// gladiator.dll: 100031F0..10003221
// gladi386.so:   0000A76C..0000A7BA
bsp_link_t *sub_100031F0()
{
  bsp_link_t *result;
  bsp_link_t *next;

  result = bspworld.dword_10069580;
  if ( !result )
  {
    botimport.Print(PRT_FATAL, "empty bsp link heap\n");
    return NULL;
  }
  next = result->next_ent;
  bspworld.dword_10069580 = next;
  if ( next )
    next->prev_ent = NULL;
  return result;
}
// gladiator.dll: 10003240..1000326B
// gladi386.so:   0000A3F8..0000A442
void __cdecl sub_10003240(bsp_link_t *a1)
{
  if ( bspworld.dword_10069580 )
    bspworld.dword_10069580->prev_ent = a1;
  a1->prev_ent = NULL;
  a1->next_ent = bspworld.dword_10069580;
  a1->prev_leaf = NULL;
  a1->next_leaf = NULL;
  bspworld.dword_10069580 = a1;
}
// gladiator.dll: 10003280..100032B6
// gladi386.so:   0000A80C..0000A85C
void sub_10003280()  /* InitBSPLinkedEntities */
{
  if ( bspworld.dword_100674C0 )
  {
    if ( bspworld.dword_10069584 )
      FreeMemory(bspworld.dword_10069584);
    bspworld.dword_10069584 = (bsp_link_t **)GetClearedMemory(sizeof(bsp_link_t *) * bspworld.numleafs);
  }
}
// gladiator.dll: 100032D0..10003333
// gladi386.so:   0000A85C..0000A8F3
void sub_100032D0()
{
  if ( bspworld.dword_100674C0 )
  {
    if ( bspworld.dword_1006755C )
      FreeMemory(bspworld.dword_1006755C);
    bspworld.dword_1006755C = GetClearedMemory(4 * bspworld.numareaportals);
    if ( bspworld.dword_10067560 )
      FreeMemory(bspworld.dword_10067560);
    bspworld.dword_10067560 = GetClearedMemory(bspworld.numareas * bspworld.numareas * 4);
  }
}
// gladiator.dll: 10003360..100033E5
// gladi386.so:   0000A8F4..0000A990
/* Q3 engine cognate CM_PointLeafnum_r (qcommon/cm_test.c).  Q3 hardcodes the
 * world model's root; this takes an explicit modelnum and starts from
 * dmodels[modelnum].headnode, so inline sub-models work too. */
int __cdecl CM_PointLeafnum(const vec3_t point, int modelnum)
{
  vec_t d;
  dplane_t *plane;
  int node; // ecx

  if ( !bspworld.dword_100674C0 )
    return 0;
  node = bspworld.dmodels[modelnum].headnode;
  if ( node >= 0 )
  {
    do
    {
      dnode_t *dn = &bspworld.dnodes[node];
      int planenum = dn->planenum;
      plane = &bspworld.dplanes[planenum];
      /* `point` FIRST, not `plane->normal`: with the loop-INVARIANT operand
       * leading, gcc 2.7 emits `fld point[i]` as its own insn and hoists all
       * three out of the descent loop.  MSVC6 canonicalises either spelling.
       * NB not the AAS_PointAreaNum dot-product conflict — that one is about
       * term STAGING order. */
      d = DotProduct(point, plane->normal) - plane->dist;
      if ( d > 0 )
        node = dn->children[0];
      else
        node = dn->children[1];
    }
    while ( node >= 0 );
  }
  return -1 - node;
}
// gladiator.dll: 10003420..10003450
// gladi386.so:   0000A990..0000AA3F
dleaf_t *__cdecl sub_10003420(const vec3_t point, int modelnum)
{
  if ( !bspworld.dword_100674C0 )
    return 0;
  return &bspworld.dleafs[CM_PointLeafnum(point, modelnum)];
}
// gladiator.dll: 10003460..100034AF
// gladi386.so:   0000AA40..0000AAB1
/* `RotatePoint(vec3_t point, float m[3][3])`, declared in Q3's bspc/l_math.h --
 * Mr Elusive's own name, not an invention.
 *
 * This USED to be two functions: `sub_10003460` here, written with scalar
 * x/y/z temporaries, and a second `#ifndef _WIN32`-gated `RotatePoint` at the
 * end of the file carrying the Q3 spelling.  They are one function.  The
 * duplicate was created because the ELF funcmap had F668 (0xa7bc, 80 B) paired
 * with sub_10003460 -- a pairing its own override row already flagged WEAK for
 * "19 real insns vs 33 ours" -- when F668 is actually a linked-list prepend on
 * `bspworld` (+0x20c0), i.e. sub_10003240.  The real RotatePoint is F673
 * (0xaa40, 113 B), which sits between F672=sub_10003420 and F674=AnglesToAxis
 * exactly as 0x10003460 sits between 0x10003420 and 0x100034D0 in the DLL.
 *
 * The `vec3_t tvec` + DotProduct spelling is the one that matters: it is
 * byte-identical to F673 on the ELF, where the scalar form is not.  MSVC6 /O2
 * compiles the two spellings BYTE-IDENTICALLY (probe_cl.sh, 32 insns each), so
 * there is no two-oracle conflict here -- the PE keeps its MATCH either way.
 * (2026-08-16.) */
void __cdecl RotatePoint(vec3_t point, float matrix[3][3])
{
  vec3_t tvec;

  VectorCopy(point, tvec);
  point[0] = DotProduct(matrix[0], tvec);
  point[1] = DotProduct(matrix[1], tvec);
  point[2] = DotProduct(matrix[2], tvec);
} //end of the function RotatePoint
// gladiator.dll: 100034D0..10003616
// gladi386.so:   0000AAB4..0000AC61
/* Build a 3x3 row-major rotation matrix from angles[PITCH,YAW,ROLL]:
 * output = roll_m * pitch_m * yaw_m.
 *
 *   yaw_m   = [[ cy, sy,0],[-sy,cy,0],[0,0,1]]   ; around Z
 *   pitch_m = [[ cp, 0,-sp],[0, 1,0],[sp,0,cp]]  ; around Y
 *   roll_m  = [[ 1, 0, 0],[0,cr,sr],[0,-sr,cr]]  ; around X
 *
 * Degrees->radians uses the unfolded `angle * M_PI * 2 / 360` idiom with no
 * grouping parens: `angle` is a runtime value inside the left-associative parse
 * tree, so gcc cannot constant-fold M_PI*2 and keeps PI resident on the x87
 * stack for the whole function. */
void __cdecl AnglesToAxis(const vec3_t angles, float axis[3][3])
{
  /* Only THREE matrix buffers: `m` holds the pitch matrix for the first concat,
     then is rebuilt in place as the roll matrix for the second. */
  float angle;
  float sp, cp;
  float sy, cy;
  float sr, cr;
  struct {
    float tmp[3][3];
    float m[3][3];
    float yaw[3][3];
  } mats;

  /* Row-major, constant cells interleaved in place rather than seeded up front. */
  angle = angles[1] * M_PI*2 / 360;
  sy = (float)sin(angle);
  cy = (float)cos(angle);
  mats.yaw[0][0] = cy;
  mats.yaw[0][1] = sy;
  mats.yaw[0][2] = 0;
  mats.yaw[1][0] = -sy;
  mats.yaw[1][1] = cy;
  mats.yaw[1][2] = 0;
  mats.yaw[2][0] = 0;
  mats.yaw[2][1] = 0;
  mats.yaw[2][2] = 1;

  /* pitch matrix (around Y), same row-major/interleaved shape as yaw. */
  angle = angles[0] * M_PI*2 / 360;
  sp = (float)sin(angle);
  cp = (float)cos(angle);
  mats.m[0][0] = cp;
  mats.m[0][1] = 0;
  mats.m[0][2] = -sp;
  mats.m[1][0] = 0;
  mats.m[1][1] = 1;
  mats.m[1][2] = 0;
  mats.m[2][0] = sp;
  mats.m[2][1] = 0;
  mats.m[2][2] = cp;

  /* tmp = pitch_m * yaw_m.  The roll angle and matrix are computed only AFTER
     this call, so the roll matrix can reuse the now-dead pitch buffer. */
  R_ConcatRotations(mats.m, mats.yaw, mats.tmp);

  /* roll matrix (rotation around X), rebuilt in `m`, same row-major/interleaved
     shape as yaw and pitch. */
  angle = angles[2] * M_PI*2 / 360;
  sr = (float)sin(angle);
  cr = (float)cos(angle);
  mats.m[0][0] = 1;
  mats.m[0][1] = 0;
  mats.m[0][2] = 0;
  mats.m[1][0] = 0;
  mats.m[1][1] = cr;
  mats.m[1][2] = sr;
  mats.m[2][0] = 0;
  mats.m[2][1] = -sr;
  mats.m[2][2] = cr;

  /* output = roll_m * tmp */
  R_ConcatRotations(mats.m, mats.tmp, axis);
}
// gladiator.dll: 10003680..10003AC7
// gladi386.so:   0000AC64..0000B34F
qboolean __cdecl AAS_EntityCollision(int entnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int contentmask, bsp_trace_t *trace)
{
  int v7; // edi
  int v12; // edx
  int v15; // ecx
  float v16; // st7
  float v17; // st6
  int v18; // edi
  int v19; // edx
  float v20; // st6
  int v24; // edx
  float v25; // st6
  int v31; // esi
  float v35; // st7
  vec3_t clipend; // [esp+14h] [ebp-D8h] BYREF — temporary intersection point
  vec3_t v44; /* was v44,v44[1],v44[2]: vec3_t local bbox min2 */
  vec3_t v41; /* was v41,v41[1],v41[2]: vec3_t local bbox min1 */
  vec3_t v40; // [esp+8h] [ebp-E4h] BYREF
  bsp_trace_t modeltrace; // [esp+38h] [ebp-B4h] BYREF
  bsp_entdata_t entdata; // [esp+8Ch] [ebp-60h] BYREF
  float v39; // [esp+10h] [ebp-DCh]

  if ( !bspworld.dword_100674C0 )
    return 0;
  AAS_EntityBSPData(entnum, &entdata);
  if ( entdata.solid != 2 && entdata.solid != 3 )
    return 0;
  if ( boxmaxs )
  {
    VectorSubtract(entdata.absmins, boxmaxs, v41);
  }
  else
  {
    VectorCopy(entdata.absmins, v41);
  }
  if ( boxmins )
  {
    VectorSubtract(entdata.absmaxs, boxmins, v44);
  }
  else
  {
    VectorCopy(entdata.absmaxs, v44);
  }
  for (v7 = 0; v7 < 3; v7++)
  {
      if (start[v7] < v41[v7] && end[v7] < v41[v7]) break;
      if (start[v7] > v44[v7] && end[v7] > v44[v7]) break;
  }
  if ( v7 != 3 )
    return 0;
  if ( entdata.solid == 2 )
  {
    /* 0.5 is a DOUBLE (fadd QWORD 0.5) and both operands are indexed off `start`,
     * reusing the offsets CSE'd from the first bounds loop.  Keep the pure-index
     * form and keep 0.5, not 0.5f. */
    for ( v12 = 0; v12 < 3; ++v12 )
    {
      if ( start[v12] <= v41[v12] + 0.5 )
        break;
      if ( start[v12] >= v44[v12] - 0.5 )
        break;
    }
    /* Relational, not equality: `cmp edx,3; jl`.  `== 3` emits `jne` and drops the
     * skip-jump.  The FIRST bounds loop above does use `!= 3`. */
    if ( v12 >= 3 )
    {
      trace->ent = entnum;
      trace->startsolid = 1;
      trace->allsolid = 1;
      trace->fraction = 0.0f;
      trace->contents = 0;
      trace->sidenum = -1;
      /* Keep the memset (an inlined 20-byte one): five scalar `planeints[i] = 0`
       * stores would fold the base into the addressing mode instead. */
      memset(&trace->plane, 0, sizeof(trace->plane));
      VectorCopy(start, trace->endpos);
      return 1;
    }
  }
  v15 = 0;
  VectorSubtract(end, start, v40);
  while ( 1 )
  {
    if ( v40[v15] > 0.0f )
      v16 = v41[v15];
    else
      v16 = v44[v15];
    v17 = start[v15] - v16;
    /* ONE `v15 + 1`, and the advance is `v15 = v18` — not `++v15`, which would
     * carry v15 and v15+1 as two parallel induction variables. */
    v18 = v15 + 1;
    v19 = v18;
    v39 = v17 / (v17 - (end[v15] - v16));
    if ( v15 > 1 )
      v19 = 0;
    v20 = v39 * v40[v19] + start[v19];
    clipend[v19] = v20;
    if ( v41[v19] < v20 && v20 < v44[v19] )
    {
      v24 = v19 + 1;
      if ( v24 > 2 )
        v24 = 0;
      v25 = v39 * v40[v24] + start[v24];
      clipend[v24] = v25;
      if ( v41[v24] < v25 && v25 < v44[v24] )
        break;
    }
    v15 = v18;
    if ( v15 >= 3 )
      goto LABEL_40;
  }
  clipend[v15] = v16;
LABEL_40:
  if ( v15 == 3 || v39 >= trace->fraction )
    return 0;
  if ( entdata.solid == 2 )
  {
    trace->fraction = v39;
    trace->sidenum = -1;
    trace->startsolid = 0;
    trace->allsolid = 0;
    trace->ent = entnum;
    if ( boxmins && boxmaxs )
    {
      if ( v40[v15] > 0.0f )
        trace->exp_dist = boxmaxs[v15];
      else
        trace->exp_dist = -boxmins[v15];
    }
    /* Plain indexed form: cl.exe strength-reduces it to ONE induction pointer plus
     * a base difference.  A pre-biased pointer walk costs a `lea eax,[edx-0x4]`. */
    for ( v31 = 0; v31 < 3; ++v31 )
      trace->endpos[v31] = v39 * v40[v31] + start[v31];
    trace->plane.normal[(v15 + 1) % 3] = 0.0f;
    trace->plane.normal[(v15 + 2) % 3] = 0.0f;
    if ( v40[v15] > 0.0f )
      trace->plane.normal[v15] = -1.0f;
    else
      trace->plane.normal[v15] = 1.0f;
    v35 = trace->endpos[v15];
    if ( v40[v15] > 0.0f )
      v35 = -v35;
    trace->plane.type = (byte)v15;
    trace->plane.dist = v35 - trace->exp_dist;
    return 1;
  }
  if ( entdata.solid == 3 )
  {
    modeltrace = AAS_TraceBSPModel(entdata.modelnum, entdata.origin, entdata.angles, start, boxmins, boxmaxs, end, 0, contentmask);
    if ( modeltrace.fraction < trace->fraction )
    {
      memcpy(trace, &modeltrace, sizeof(modeltrace));
      return 1;
    }
  }
  return 0;
}
// gladiator.dll: 10003BF0..10003C69
// gladi386.so:   0000B350..0000B3CE
int __cdecl sub_10003BF0(int leafnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int passent, int contentmask, bsp_trace_t *trace)
{
  bsp_link_t *i; // esi
  int v10; // [esp+0h] [ebp-4h]

  if ( !bspworld.dword_100674C0 )
    return 0;
  v10 = 0;
  for ( i = bspworld.dword_10069584[leafnum]; i; i = i->next_ent )
  {
    if ( i->entnum != passent )
    {
      if ( AAS_EntityCollision(i->entnum, start, boxmins, boxmaxs, end, contentmask, trace) )
        v10 = 1;
    }
  }
  return v10;
}
// gladiator.dll: 10003C90..100041BC
// gladi386.so:   0000B3D0..0000BA06
/* Q3 engine cognate CM_TraceThroughBrush (qcommon/cm_trace.c): iterate a brush's
 * sides, expand each plane by the trace box, track the enter/leave fraction.  No
 * capsule/sphere collision — Q3 added that later. */
int __cdecl CM_TraceThroughBrush(
        dbrush_t *a1,
        float *a2,
        float *a3,
        float *a4,
        float *a5,
        float *a6,
        float *a7,
        float *a8,
        _DWORD *a9,
        float *a10,
        float *a11)
{
  int v11; // edi
  float *v12; // edx
  float *v13; // eax
  dbrush_t *v14; // ebx
  float *v15; // ecx
  /* v16: BSP plane pointer — an `int` would truncate dplanes on 64-bit. */
  dplane_t *v16;
  int v17; // ecx
  float v18; // st7
  float v19; // st7
  float v20; // st6
  float v25; // st5
  float v29; // st7
  float v30; // st7
  float v31; // st7
  float v35; // [esp+10h] [ebp-7Ch]
  int v36; // [esp+14h] [ebp-78h]
  float v37; // [esp+14h] [ebp-78h]
  float v38; // [esp+18h] [ebp-74h]
  int v39; // [esp+1Ch] [ebp-70h]
  vec3_t endp; // [esp+44h] [ebp-48h] — clipped end point
  int v42; // [esp+24h] [ebp-68h]
  float v41; // [esp+20h] [ebp-6Ch]
  int v40; // [esp+20h] [ebp-6Ch]
  float v43; // [esp+28h] [ebp-64h]
  vec3_t dir; // [esp+50h] [ebp-3Ch] BYREF — clipped-distance vec (VectorLength input)
  vec3_t vec; // [esp+5Ch] [ebp-30h] BYREF — line vec (VectorLength input)
  vec3_t normal; // [esp+38h] [ebp-54h] BYREF — plane normal (RotatePoint input/output)
  vec3_t startp; // [esp+2Ch] [ebp-60h] — clipped start point
  float v59[3][3]; // [esp+68h] [ebp-24h] BYREF

  if ( *a3 == 0.0f && a3[1] == 0.0f && a3[2] == 0.0f )
  {
    v39 = 0;
  }
  else
  {
    v39 = 1;
    AnglesToAxis(a3, v59);
  }
  v12 = a2;
  if ( *a2 != 0.0f || a2[1] != 0.0f || (v40 = 0, a2[2] != 0.0f) )
    v40 = 1;
  /* One shared zero, hoisted here with the other zero initialisers. */
  v11 = 0;
  v13 = a4;
  v14 = a1;
  v42 = 0;
  v36 = 0;
  VectorCopy(a4, startp);
  v15 = a7;
  VectorCopy(a7, endp);
  if ( a1->numsides > 0 )
  {
    while ( 1 )
    {
      v16 = &bspworld.dplanes[bspworld.dbrushsides[v11 + v14->firstside].planenum];
      if ( v39 )
      {
        VectorCopy(v16->normal, normal);
        RotatePoint(normal, v59);
        v12 = a2;
        v17 = 4;
      }
      else
      {
        v17 = v16->type;
        VectorCopy(v16->normal, normal);
      }
      if ( v40 )
      {
        if ( v17 < 3 )
        {
          if ( normal[v17] > 0.0f )
            v18 = v12[v17] + v16->dist;
          else
            v18 = v16->dist - v12[v17];
          v38 = v18;
        }
        else
        {
          v38 = normal[2] * v12[2] + normal[1] * v12[1] + normal[0] * *v12 + v16->dist;
        }
      }
      else
      {
        v38 = v16->dist;
      }
      if ( v17 < 3 )
      {
        if ( a5 )
        {
          if ( a6 )
          {
            /* Positive guard: the `normal>0` arm is the warm fall-through and the
             * `<=0` arm the cold jump target.  Inverting it swaps the blocks. */
            if ( normal[v17] > 0.0f )
            {
              v12 = a2;
              v19 = -a5[v17];
            }
            else
            {
              v19 = a6[v17];
              v12 = a2;
            }
            goto LABEL_30;
          }
          v12 = a2;
        }
        v19 = 0.0;
LABEL_30:
        v20 = v19 + v38;
        /* As above: the un-negated `normal>0` arm is the fall-through. */
        if ( normal[v17] > 0.0f )
        {
          v35 = startp[v17] - v20;
          v25 = endp[v17];
        }
        else
        {
          v35 = -startp[v17] - v20;
          v25 = -endp[v17];
        }
      }
      else
      {
        if ( a5 && a6 )
        {
          /* Per component, select a5 or a6 by the sign of normal[i].  Test
           * positively (the a5 arm is inline) and use two assignment STATEMENTS,
           * not a ternary — a float-valued ternary routes the copy through the
           * x87 stack where the original uses integer movs. */
          {
            int _k;
            for (_k = 0; _k < 3; _k++)
            {
              if ( normal[_k] > 0.0f )
                vec[_k] = a5[_k];
              else
                vec[_k] = a6[_k];
            }
          }
          dir[0] = -normal[0];
          dir[1] = -normal[1];
          dir[2] = -normal[2];
          v11 = v36;
          v14 = a1;
          v12 = a2;
          v19 = vec[2] * dir[2] + vec[1] * dir[1] + vec[0] * dir[0];
        }
        else
        {
          v19 = 0.0;
        }
        v20 = v19 + v38;
        v35 = normal[2] * startp[2] + normal[1] * startp[1] + normal[0] * startp[0] - v20;
        v25 = normal[2] * endp[2] + normal[1] * endp[1] + normal[0] * endp[0];
      }
      v37 = v25 - v20;
      if ( v35 > -0.005 && v37 > -0.005 )
        return 0;
      if ( v35 >= 0.005 || v37 >= 0.005 )
      {
        if ( v35 > -0.005 )
        {
          v43 = v19;
          v42 = v11 + v14->firstside;
        }
        if ( v35 > 0.005 )
        {
          v29 = v35 / (v35 - v37);
          startp[0] = (endp[0] - startp[0]) * v29 + startp[0];
          startp[1] = (endp[1] - startp[1]) * v29 + startp[1];
          startp[2] = (endp[2] - startp[2]) * v29 + startp[2];
        }
        else if ( v37 > 0.005 )
        {
          v30 = v35 / (v35 - v37);
          endp[0] = (endp[0] - startp[0]) * v30 + startp[0];
          endp[1] = (endp[1] - startp[1]) * v30 + startp[1];
          endp[2] = (endp[2] - startp[2]) * v30 + startp[2];
        }
      }
      v36 = ++v11;
      if ( v11 >= v14->numsides )
      {
        v15 = a7;
        v13 = a4;
        break;
      }
    }
  }
  VectorSubtract(v15, v13, vec);
  VectorSubtract(startp, v13, dir);
  v41 = VectorLength(dir);
  v31 = v41 / VectorLength(vec);
  if ( v31 < *a8 )
  {
    *a8 = v31;
    *a9 = v42;
    *a10 = v43;
    /* ONE grouped vec3 copy.  [1] and [2] are copied with integer movs, which a
     * float-typed local would prevent. */
    VectorCopy(startp, a11);
    return 1;
  }
  return 0;
}
// gladiator.dll: 10004310..1000448D
// gladi386.so:   0000BA08..0000BBBA
/* Q3 engine cognate CM_TraceThroughLeaf (qcommon/cm_trace.c).  No patch/curve
 * surfaces in Q2, so Q3's second leafsurfaces loop is absent. */
int __cdecl CM_TraceThroughLeaf(int leafnum, vec3_t origin, vec3_t angles, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int contentmask, bsp_trace_t *trace)
{
  int v9; // ebp
  /* v11: BSP leaf pointer for leaf `a1`. */
  dleaf_t *v11;
  dbrush_t *v13; // edi
  int v16; // ecx
  /* v17: BSP plane pointer for the hit brush side. */
  dplane_t *v17;
  float v20; // [esp+10h] [ebp-10h] BYREF — expanded plane dist filled by CM_TraceThroughBrush
  vec3_t endpos; // [esp+14h] [ebp-Ch] BYREF — endpoint filled by CM_TraceThroughBrush via a11
  int sidenum; // [esp+20h] [ebp+0h]
  dbrush_t *v24; // [esp+24h] [ebp+4h]

  v11 = &bspworld.dleafs[leafnum];
  v9 = 0;
  v24 = 0;
  /* Compare against the loop counter v9 (=0), not a literal 0: that is what
   * yields `cmp WORD,bp; jbe` instead of `je`.  Same skip-when-empty result. */
  if ( v11->numleafbrushes <= (unsigned int)v9 )
    goto fail;
  do
  {
    int brushnum = bspworld.dleafbrushes[v9 + v11->firstleafbrush];
    v13 = &bspworld.dbrushes[brushnum];
    if ( (v13->contents & contentmask) != 0
      && CM_TraceThroughBrush(v13, origin, angles, start, boxmins, boxmaxs, end, &trace->fraction, (_DWORD *)&sidenum, &v20, endpos) )
    {
      v24 = v13;
    }
    ++v9;
  }
  while ( v9 < v11->numleafbrushes );
  /* `if (v24)` rather than `if (!v24) goto fail`, so the fill code is the warm
   * fall-through and the shared return-0 stays a cold tail block. */
  if ( v24 )
  {
  if ( endpos[0] == start[0] && endpos[1] == start[1] && endpos[2] == start[2] )
  {
    trace->allsolid   = 1;
    trace->startsolid = 1;
    trace->fraction   = 0;
  }
  else
  {
    trace->allsolid   = 0;
    trace->startsolid = 0;
  }
  VectorCopy(endpos, trace->endpos);
  v16 = sidenum;
  trace->sidenum = v16;
  v17 = &bspworld.dplanes[bspworld.dbrushsides[v16].planenum];
  VectorCopy(v17->normal, trace->plane.normal);
  trace->plane.dist      = v17->dist;
  trace->plane.type      = v17->type;
  trace->exp_dist = v20;
  trace->contents = v24->contents;
  return 1;
  }
fail:
  /* Both early-out guards share this ONE return-0 epilogue; a `return 0;` at
   * each guard would emit two copies. */
  return 0;
}

typedef struct bsp_model_tracestack_s {
  vec3_t start;
  vec3_t end;
  int nodenum;
  int planenum;
  float planedist;
  int next;   /* raw 32-bit pointer in the original DLL; offset-encoded on 64-bit */
} bsp_model_tracestack_t;

typedef struct bsp_model_plane_sidecache_s {
  int sideflags[4];
  float offsets[2];
} bsp_model_plane_sidecache_t;

_Static_assert(sizeof(bsp_model_tracestack_t) == 40, "bsp_model_tracestack_t size");
_Static_assert(sizeof(bsp_model_plane_sidecache_t) == 24, "bsp_model_plane_sidecache_t size");
// gladiator.dll: 100044F0..100052B4
// gladi386.so:   0000BBBC..0000D0C9
/* Sweep a box from start to end through BSP model `modelnum` at
 * `modelorigin`/`angles` — Gladiator's own Q2-era BSP-model collision (Q3
 * delegates BSP traces to the engine).
 *
 * Two original stack aggregates: a 40-byte trace-stack frame
 * {start,end,nodenum,planenum,planedist,next} and a 24-byte
 * {int sideflags[4], float offsets[2]} side-cache block.  Returns bsp_trace_t by
 * value through MSVC's hidden-retbuf ABI. */
bsp_trace_t __cdecl AAS_TraceBSPModel(
        int modelnum,
        const vec3_t modelorigin,
        vec3_t angles,
        vec3_t start,
        vec3_t boxmins,
        vec3_t boxmaxs,
        vec3_t end,
        int passent,
        int contentmask)
{
  int v12; // eax
  float v13; // st7
  /* v14: the selected BSP model — an `int` truncates dmodels on 64-bit. */
  dmodel_t *v14;
  float v15; // st7
  bsp_model_tracestack_t *v16; // eax
  int v17; // edx
  /* v19, v56, v100: pointers into trace_stack.  The link slots stay 4 bytes wide,
   * but on 64-bit they hold encoded byte offsets — see TR_ENC/TR_DEC below. */
  bsp_model_tracestack_t *v19;
  bsp_model_tracestack_t *v24; // ebx
  bsp_model_tracestack_t *v25; // esi
  int *v27; // ebp
  int v28; // eax
  int v29; // esi
  dleaf_t *v30; // eax
  dnode_t *v31; // edx
  int v37; // eax
  /* v38: the BSP plane for `v37`; same truncation class as v14 above. */
  dplane_t *v38;
  int v39; // ecx
  float v40; // st7
  float v41; // st6
  float v42; // st7
  float v44; // st6
  float v45; // st5
  float v46; // st4
  int v47; // eax
  float v48; // st6
  BOOL v49; // eax
  BOOL v51; // eax
  int v53; // ecx
  int v54; // eax
  /* side_a/side_b: the 0/1 "which side of the plane" selectors, plain ints with no
   * FPU traffic.  Float locals here cost a real fld/fstp per read.  v109's genuine
   * float distance value is a separate thing. */
  int side_a;
  int side_b;
  bsp_model_tracestack_t *v56; // eax — one temp shared by all three disjoint fresh-node sites
  int *v57; // esi  — points at a single 4-byte link slot (an encoded offset on 64-bit);
       // shared by all three disjoint fresh-node sites
  int v59; // edx
  float v60; // st7
  bsp_model_tracestack_t *v64; // esi
  bsp_model_tracestack_t *v70; // edx
  int v82; // edx
  float v83; // st7
  int v89; // eax
  int v90; // ecx
  int v96; // eax
  int v97; // ecx
  float v98; // st7
  bsp_model_tracestack_t *v100; // eax — AArch64: trace-stack frame pointer (see v19)
  int v101; // ecx
  int v103; // edx
  vec3_t v106; // [esp+10h] [ebp-1548h] BYREF — current piece start
  float v109; // [esp+1Ch] [ebp-153Ch]
  float v110; // [esp+20h] [ebp-1538h]
  vec3_t v111; // [esp+24h] [ebp-1534h] BYREF — current piece end
  float v114[3]; /* was v114,v115,v116: vec3_t plane normal */
  /* v115 subsumed into v114[1] */
  /* v116 subsumed into v114[2] */
  int v117; // [esp+3Ch] [ebp-151Ch]
  float v118; // [esp+40h] [ebp-1518h]
  int v119; // [esp+44h] [ebp-1514h]
  dnode_t *v120; // [esp+48h] [ebp-1510h]
  int v121; // [esp+4Ch] [ebp-150Ch]
  int *v50; // ecx
  int *v122; // [esp+50h] [ebp-1508h]
  vec3_t v123; // [esp+54h] [ebp-1504h] BYREF — split point A
  int v126; // [esp+60h] [ebp-14F8h]
  float v127; // [esp+64h] [ebp-14F4h]
  vec3_t v128; // [esp+68h] [ebp-14F0h] BYREF — overall trace delta
  /* The 24-byte side-cache block.  These MUST stay two separate scalar floats and
   * the indexed reads must stay byte-offset expressions: an array forces MSVC to
   * keep the pair addressable as a unit, while two scalars let it coalesce them.
   * The `*(&v133 + v121)` walks are the original's addressing — do not "fix". */
  int plane_sideflags[4];
  float v133;
  float v134;
  BOOL v135; // [esp+8Ch] [ebp-14CCh]
  vec3_t v136; // [esp+90h] [ebp-14C8h] BYREF — model origin + dmodel origin
  vec3_t v139; // [esp+9Ch] [ebp-14BCh] BYREF — split point B
  int v142; // [esp+A8h] [ebp-14B0h]
  int v144; // [esp+B0h] [ebp-14A8h]
  float v145[2]; // [esp+B4h] [ebp-14A4h]
  float v147[2]; // [esp+C0h] [ebp-1498h]
  float v149[3]; // [esp+D4h] [ebp-1484h] BYREF
  float v148[3]; // [esp+C8h] [ebp-1490h] BYREF
  bsp_trace_t trace; // [esp+E0h] [ebp-1478h] BYREF
  float v151[3][3]; // [esp+134h] [ebp-1424h] BYREF
  /* 128 contiguous 40-byte trace frames; v153 aliases trace_stack[0].next. */
  bsp_model_tracestack_t trace_stack[128];   // [esp+158h] [ebp-13A0h] BYREF
  /* The trace-stack lists keep their next-pointer in 4-byte int slots.  On 32-bit
   * a pointer fits and TR_ENC/TR_DEC are the identity; on 64-bit each link becomes
   * a byte offset into trace_stack, stored as offset+1 so 0 stays NULL.
   * MSVC6 does not define __SIZEOF_POINTER__, hence the `!defined` clause. */
#if !defined(__SIZEOF_POINTER__) || __SIZEOF_POINTER__ == 4
  #define TR_ENC(p) ((int)(intptr_t)(p))
  #define TR_DEC(i) ((bsp_model_tracestack_t *)(intptr_t)(i))
#else
  #define TR_ENC(p) ((p) ? (int)(((intptr_t)(p) - (intptr_t)trace_stack) + 1) : 0)
  #define TR_DEC(i) ((i) ? (bsp_model_tracestack_t *)((intptr_t)trace_stack + ((unsigned int)(i) - 1u)) : (bsp_model_tracestack_t *)0)
#endif

  memset(&trace, 0, sizeof(trace));
  VectorCopy(end, trace.endpos);
  trace.allsolid = 0;
  trace.startsolid = 0;
  trace.fraction = 1.0f;
  if ( !bspworld.dword_100674C0 )
    return trace;
  VectorSubtract(end, start, v128);
  if ( v128[0] > v128[1] )
    v12 = v128[0] > v128[2] ? 0 : 2;
  else
    v12 = v128[1] > v128[2] ? 1 : 2;
  v13 = v128[v12];
  v126 = v12;
  /* Explicit if/else, not `v135 = v13 > 0`: gcc272 emits one byte less and 4 fewer
   * instruction diffs, and MSVC6 folds all three spellings identically.  Do not
   * "simplify" back to the assignment. */
  if ( v13 > 0 )
    v135 = 1;
  else
    v135 = 0;
  if ( angles[0] == 0 && angles[1] == 0 && angles[2] == 0 )
  {
    v144 = 0;
  }
  else
  {
    v144 = 1;
    AnglesToAxis(angles, v151);
  }
  v14 = &bspworld.dmodels[modelnum];
  v136[0] = v15 = v14->origin[0] + modelorigin[0];
  v136[1] = v14->origin[1] + modelorigin[1];
  v136[2] = v14->origin[2] + modelorigin[2];
  if ( v15 != 0 || v136[1] != 0 || (v142 = 0, v136[2] != 0) )
    v142 = 1;
  v16 = trace_stack;
  v17 = 127;
  do
  {
    v16->next = TR_ENC(v16 + 1);   /* AArch64: encode offset into trace_stack, not raw ptr */
    ++v16;
    --v17;
  }
  while ( v17 );
  trace_stack[127].next = 0;
  /* A dead-in-practice NULL check on &trace_stack[0] that the 1998 compiler still
   * emitted.  v16 holds the fresh base address, matching the original's fresh `lea`
   * rather than the loop's advanced pointer.  Do not fold into v24's definition. */
  v16 = trace_stack;
  if ( !v16 )
    goto LABEL_125;
  v19 = TR_DEC(trace_stack[0].next);
  VectorCopy(start, trace_stack[0].start);
  VectorCopy(end, trace_stack[0].end);
  trace_stack[0].nodenum = v14->headnode;
  trace_stack[0].planenum = 0;
  trace_stack[0].planedist = 0.0f;
  trace_stack[0].next = 0;
  v24 = trace_stack;
  while ( 1 )
  {
    /* ONE merged node loop with the BOX path as the warm fall-through.  The box arm
     * must be the `if` body and the no-box arm the `else`: that is what places the
     * whole no-box arm cold, each exit jumping back to the trace-stack pop. */
      while ( 1 )
      {
        while ( 1 )
        {
          v25 = v24;
          if ( !v24 )
            return trace;
          v119 = v25->planenum;
          v27 = &v24->next;
          v24 = TR_DEC(v24->next);
          if ( v119 < 0 )
            return trace;
          v28 = v25->nodenum;
          if ( v28 >= 0 )
            break;
          v29 = -1 - v28;
          v30 = &bspworld.dleafs[v29];
          if ( v30->numleafbrushes && (contentmask & v30->contents) != 0 )
            CM_TraceThroughLeaf(v29, v136, angles, start, boxmins, boxmaxs, end, contentmask, &trace);
          if ( bspworld.dword_10069584[v29] )
            sub_10003BF0(v29, start, boxmins, boxmaxs, end, passent, contentmask, &trace);
        }
          VectorCopy(v25->start, v106);
          v31 = &bspworld.dnodes[v28];
          VectorCopy(v25->end, v111);
          v127 = v25->planedist;
          *v27 = TR_ENC(v19);   /* AArch64: link slot must be offset-encoded, not raw ptr */
          v37 = v31->planenum;
          v120 = v31;
          v19 = v25;
          v38 = &bspworld.dplanes[v37];
          if ( v144 )
          {
            VectorCopy(v38->normal, v114);
            RotatePoint(v114, v151);
            v31 = v120;
            v39 = 4;
          }
          else
          {
            v117 = v38->type;
            VectorCopy(v38->normal, v114);
            v39 = v117;
          }
          if ( v142 )
            v40 = v39 < 3 ? v136[v39] + v38->dist : DotProduct(v114, v136) + v38->dist;
          else
            v40 = v38->dist;
        if ( boxmins && boxmaxs )
        {
        if ( v39 < 3 )
        {
          v41 = v106[v39] - v40;
          v121 = 1;
          v109 = v41;
          v42 = v111[v39] - v40;
          if ( v128[v39] >= 0 )
            v121 = 0;
          v133 = -boxmins[v39];
          v134 = -boxmaxs[v39];
        }
        else
        {
          v121 = 1;
          v109 = DotProduct(v114, v106) - v40;
          v42 = DotProduct(v114, v111) - v40;
          if ( DotProduct(v114, v128) >= 0 )
            v121 = 0;
          /* 3-component loop selecting boxmaxs[i] or boxmins[i] by the sign of the
           * plane normal v114[i], into the BSP extents v148[i] / v149[i]. */
          {
            int _j;
            for (_j = 0; _j < 3; _j++) {
              if (v114[_j] > 0.0f) {
                v148[_j] = boxmins[_j];
                v149[_j] = boxmaxs[_j];
              } else {
                v148[_j] = boxmaxs[_j];
                v149[_j] = boxmins[_j];
              }
            }
          }
          v44 = -v114[0];
          v45 = -v114[1];
          v46 = -v114[2];
          v31 = v120;
          v133 = v148[2] * v46 + v148[1] * v45 + v148[0] * v44;
          v134 = v149[2] * v46 + v149[1] * v45 + v149[0] * v44;
        }
        v122 = &plane_sideflags[1];
        v47 = 0;
        do
        {
          v48 = v109 - *(float *)((char *)&v133 + v47);
          *(float *)((char *)v145 + v47) = v48;
          v110 = v42 - *(float *)((char *)&v133 + v47);
          *(float *)((char *)v147 + v47) = v110;
          v49 = v48 > -0.005 && v110 > -0.005;
          v50 = v122;
          *(v122 - 1) = v49;
          v51 = v48 < 0.005 && v110 < 0.005;
          *v50 = v51;
          v47 += 4;
          v122 = v50 + 2;
        }
        while ( v47 < 8 );
        v53 = v121;
        if ( !plane_sideflags[v121] && !plane_sideflags[v121 + 2] )
          break;
        v19 = TR_DEC(*v27);
        VectorCopy(v106, v25->start);
        VectorCopy(v111, v25->end);
        v25->planenum = v119;
        v25->planedist = v127;
        v25->nodenum = v31->children[v53];
        v54 = plane_sideflags[v53];
        *v27 = TR_ENC(v24);
        v24 = v25;
        if ( !v54 )
          break;
          if ( !plane_sideflags[v53 + 2] )
            break;
        }
        else
        {
          if ( v39 < 3 )
          {
            v109 = v106[v39] - v40;
            v83 = v111[v39] - v40;
          }
          else
          {
            v109 = DotProduct(v114, v106) - v40;
            v83 = DotProduct(v114, v111) - v40;
          }
          if ( v109 <= -0.005 || v83 <= -0.005 )
          {
            if ( v109 >= 0.005 || v83 >= 0.005 )
            {
              side_a = 1;
              v98 = v109 / (v109 - v83);
              v123[0] = (v111[0] - v106[0]) * v98 + v106[0];
              v123[1] = (v111[1] - v106[1]) * v98 + v106[1];
              v123[2] = (v111[2] - v106[2]) * v98 + v106[2];
              if ( v109 >= 0 )
                side_a = 0;
              VectorCopy(v123, v25->start);
              VectorCopy(v111, v25->end);
              v100 = TR_DEC(*v27);
              v25->planenum = v31->planenum;
              v25->planedist = 0.0f;
              v101 = v31->children[side_a == 0];
              *v27 = TR_ENC(v24);
              v25->nodenum = v101;
              if ( !v100 )
                goto LABEL_125;
              v19 = TR_DEC(v100->next);
              VectorCopy(v106, v100->start);
              VectorCopy(v123, v100->end);
              v100->planenum = v119;
              v100->planedist = 0.0f;
              v103 = v31->children[side_a];
              v24 = v100;
              v100->nodenum = v103;
              v100->next = TR_ENC(v25);
            }
            else
            {
              v19 = TR_DEC(*v27);
              VectorCopy(v106, v25->start);
              VectorCopy(v111, v25->end);
              v96 = v119;
              v25->planenum = v96;
              v25->planedist = 0.0f;
              v97 = v31->children[1];
              *v27 = TR_ENC(v24);
              v25->nodenum = v97;
              v24 = v25;
            }
          }
          else
          {
            v19 = TR_DEC(*v27);
            VectorCopy(v106, v25->start);
            VectorCopy(v111, v25->end);
            v89 = v119;
            v25->planenum = v89;
            v25->planedist = 0.0f;
            v90 = v31->children[0];
            *v27 = TR_ENC(v24);
            v25->nodenum = v90;
            v24 = v25;
          }
        }
        }
      side_b = (v53 == 0);
      if ( plane_sideflags[side_b] || plane_sideflags[side_b + 2] )
      {
        if ( !v19 )
          break;
        v56 = v19;
        v57 = &v19->next;
        VectorCopy(v106, v56->start);
        VectorCopy(v111, v56->end);
        v19 = TR_DEC(v19->next);
        v56->planenum = v119;
        v56->planedist = v127;
        v59 = v31->children[side_b];
        *v57 = TR_ENC(v24);
        v56->nodenum = v59;
        v24 = v56;
        if ( !plane_sideflags[side_b] || !plane_sideflags[side_b + 2] )
          goto LABEL_71;
      }
      else
      {
LABEL_71:
        if ( plane_sideflags[2 * side_b] || plane_sideflags[2 * side_b + 1] )
        {
          v118 = -1.0f;
        }
        else
        {
          v118 = v145[side_b] / (v145[side_b] - v147[side_b]);
          v123[0] = (v111[0] - v106[0]) * v118 + v106[0];
          v123[1] = (v111[1] - v106[1]) * v118 + v106[1];
          v123[2] = (v111[2] - v106[2]) * v118 + v106[2];
        }
        if ( plane_sideflags[2 * v53] || plane_sideflags[2 * v53 + 1] )
        {
          v60 = -1.0f;
        }
        else
        {
          v60 = v145[v53] / (v145[v53] - v147[v53]);
          v139[0] = (v111[0] - v106[0]) * v60 + v106[0];
          v139[1] = (v111[1] - v106[1]) * v60 + v106[1];
          v139[2] = (v111[2] - v106[2]) * v60 + v106[2];
        }
        if ( v118 >= 0 || v60 >= 0 )
        {
          if ( plane_sideflags[v53] || plane_sideflags[v53 + 2] )
            goto LABEL_103;
          if ( v118 < 0 )
          {
            if ( v60 < 0 )
              goto LABEL_103;
            if ( !v19 )
              break;
            v56 = v19;
            v57 = &v19->next;
            VectorCopy(v139, v56->start);
            VectorCopy(v111, v56->end);
            v19 = TR_DEC(v19->next);
            v56->planenum = v120->planenum;
            v56->planedist = *(&v133 + v121);
            v64 = 0;
            v56->nodenum = v120->children[v121];
            v70 = v24;
            if ( !v24 )
            {
LABEL_101:
              v24 = v56;
              goto LABEL_102;
            }
            while ( v56->start[v126] < v70->start[v126] != v135 )
            {
              v64 = v70;
              v70 = TR_DEC(v70->next);
              if ( !v70 )
                goto LABEL_99;
            }
          }
          else
          {
            if ( !v19 )
              break;
            v56 = v19;
            v57 = &v19->next;
            v64 = 0;
            VectorCopy(v123, v56->start);
            VectorCopy(v111, v56->end);
            v19 = TR_DEC(v19->next);
            v56->planenum = v120->planenum;
            v56->planedist = *(&v133 + side_b);
            v70 = v24;
            v56->nodenum = v120->children[v121];
            if ( !v24 )
              goto LABEL_101;
            while ( v56->start[v126] < v70->start[v126] != v135 )
            {
              v64 = v70;
              v70 = TR_DEC(v70->next);
              if ( !v70 )
                goto LABEL_99;
            }
          }
          *v57 = TR_ENC(v70);
          if ( v64 )
            v64->next = TR_ENC(v56);
          else
            v24 = v56;
          if ( v70 )
            goto LABEL_103;
LABEL_99:
          if ( !v64 )
            goto LABEL_101;
          v64->next = TR_ENC(v56);
LABEL_102:
          *v57 = 0;
LABEL_103:
          if ( !plane_sideflags[side_b] && !plane_sideflags[side_b + 2] )
          {
            if ( v60 >= 0 )
            {
              if ( !v19 )
                break;
              v56 = v19;
              v57 = &v19->next;
              VectorCopy(v106, v56->start);
              v19 = TR_DEC(v19->next);
              VectorCopy(v139, v56->end);
              goto LABEL_111;
            }
            if ( v118 >= 0 )
            {
              if ( !v19 )
                break;
              v56 = v19;
              v57 = &v19->next;
              VectorCopy(v106, v56->start);
              v19 = TR_DEC(v19->next);
              VectorCopy(v123, v56->end);
LABEL_111:
              v56->planenum = v119;
              v56->planedist = v127;
              v82 = v120->children[side_b];
              *v57 = TR_ENC(v24);
              v56->nodenum = v82;
              v24 = v56;
            }
          }
      }
    }
  }
LABEL_125:
  botimport.Print(PRT_ERROR, "AAS_TraceBSPModel: out of trace lines\n");
  return trace;
}
// gladiator.dll: 10005640..100056AC
// gladi386.so:   0000D0CC..0000D136
// Thin wrapper around AAS_TraceBSPModel supplying two zero vec3 locals as the
// entity-origin / angles slots, i.e. "AAS_Trace, but straight into world model
// 0 with no engine call".  DEAD in Gladiator: no caller in EITHER image.  The
// DLL keeps it through its /INCREMENTAL thunk; the .so exports it as F680,
// which is why it must NOT be `static` -- both compilers would strip a static
// function with no callers, and F680's 106 bytes are exactly this body.
//
// IDA rendered the hidden struct-return buffer as an explicit leading
// `void *out` parameter and the tail as `*out = AAS_TraceBSPModel(...)`.  That
// is the sret artifact, not the source: gcc will not forward a caller's buffer
// through a pointer PARAMETER (it cannot prove it does not alias the args), so
// the cast form costs an 84-byte temp plus a `rep movs` -- frame 0x60 against
// real's 0xc, OUR+4.  Returning bsp_trace_t BY VALUE, exactly like AAS_Trace,
// is what forwards it.  (2026-08-16; the F680/F663 mix-up is written up on
// AAS_Trace's own banner above.)
bsp_trace_t __cdecl sub_10005640(vec3_t start, vec3_t boxmins, vec3_t boxmaxs,
                                 vec3_t end, int passent, int contentmask)
{
  vec3_t zero_vec;

  zero_vec[0] = 0.0f;
  zero_vec[1] = 0.0f;
  zero_vec[2] = 0.0f;
  return AAS_TraceBSPModel(0, zero_vec, zero_vec,
                           start, boxmins, boxmaxs, end, passent, contentmask);
}
// gladiator.dll: 100056D0..10005767
// gladi386.so:   0000D138..0000D1F6
int __cdecl sub_100056D0(dbrush_t *a1, float *a2)
{
  int v3; // ebx
  /* v5: same BSP-plane-pointer-truncation bug class as sub_100044F0:v14/v38.
   * Original `int v5` truncated the selected dplane_t * on aarch64. */
  dplane_t *v5;
  int v6; // edx
  float v8; // st7

  for ( v3 = 0; v3 < a1->numsides; ++v3 )
  {
    v5 = &bspworld.dplanes[bspworld.dbrushsides[a1->firstside + v3].planenum];
    v6 = v5->type;
    if ( v6 < 3 )
    {
      /* axial plane shortcut: signbits of normal[v6] determine sign of a2[v6].
       * Original asm uses fcomps result (C0|C3 = "<= 0") to decide negation. */
      if ( v5->normal[v6] > 0.0f )
        v8 = a2[v6];
      else
        v8 = -a2[v6];
    }
    else
    {
      v8 = v5->normal[0] * *a2 + v5->normal[1] * a2[1] + v5->normal[2] * a2[2];
    }
    if ( v8 - v5->dist > 0.005 )
      return 0;
  }
  return 1;
}
// gladiator.dll: 100057A0..10005983
// gladi386.so:   0000D1F8..0000D5AC
int __cdecl sub_100057A0(float *a1, int a2, float *a3, float *a4)
{
  int v4; // edi
  int v6; // ebp
  /* v7: BSP leaf pointer for the containing leaf. */
  dleaf_t *v7;
  bsp_link_t *i; // ebp
  int v9; // [esp+10h] [ebp-7Ch]
  dbrush_t *v10; // [esp+14h] [ebp-78h]
  float v12[3]; // [esp+24h] [ebp-68h] BYREF
  float v11[3]; // [esp+18h] [ebp-74h] BYREF
  float v13[3][3]; // [esp+30h] [ebp-5Ch] BYREF
  /* one bsp_entdata_t local */
  bsp_entdata_t entdata; // [esp+54h] [ebp-38h] BYREF

  if ( !bspworld.dword_100674C0 )
    return 0;
  v9 = 0;
  VectorSubtract(a1, a3, v11);
  v12[0] = -*a4;
  v12[1] = -a4[1];
  v12[2] = -a4[2];
  AnglesToAxis(v12, v13);
  RotatePoint(v11, v13);
  v6 = CM_PointLeafnum(v11, a2);
  v7 = &bspworld.dleafs[v6];
  for ( v4 = 0; v4 < v7->numleafbrushes; ++v4 )
  {
    int brushnum;

    brushnum = bspworld.dleafbrushes[v4 + v7->firstleafbrush];
    v10 = &bspworld.dbrushes[brushnum];
    if ( sub_100056D0(v10, v11) )
      return v10->contents;
  }
  for ( i = bspworld.dword_10069584[v6]; i; i = i->next_ent )
  {
    AAS_EntityBSPData(i->entnum, &entdata);
    if ( *a1 > entdata.absmins[0]
      && *a1 < entdata.absmaxs[0]
      && a1[1] > entdata.absmins[1]
      && a1[1] < entdata.absmaxs[1]
      && a1[2] > entdata.absmins[2]
      && a1[2] < entdata.absmaxs[2] )
    {
      if ( entdata.solid == 2 )
      {
        v9 |= 0x2000000u;
      }
      else if ( entdata.solid == 3 )
      {
        v9 |= sub_100057A0(a1, entdata.modelnum, entdata.origin, entdata.angles);
      }
    }
  }
  return v9;
}
// gladiator.dll: 10005A10..10005A45
// gladi386.so:   0000D5AC..0000D5EE
/* Point-only flavour of sub_100057A0: the caller's vec3 as origin with
 * zero mins/maxs, i.e. a degenerate point box.  DEAD in Gladiator. */
int __cdecl sub_10005A10(float *origin)
{
  vec3_t zero_vec;
  zero_vec[0] = 0.0f;
  zero_vec[1] = 0.0f;
  zero_vec[2] = 0.0f;
  return sub_100057A0(origin, 0, zero_vec, zero_vec);
}
// gladiator.dll: 10005A60..10005B00
// gladi386.so:   0000D5F0..0000D6E1
void __cdecl AAS_DecompressVis(int cluster, int visType)
{
  int c;
  int row;
  char *in;
  char *out;

  if ( cluster == bspworld.dword_10069564 )
    return;
  in = bspworld.dvisdata + bspworld.dvis->bitofs[cluster][visType];
  row = (bspworld.dvis->numclusters + 7) >> 3;
  out = bspworld.byte_10067564;
  do
  {
    if ( *in )
    {
      *out++ = *in++;
      continue;
    }
    c = (unsigned __int8)in[1];
    if ( !c )
    {
      AAS_Error("AAS_DecompressVis: 0 repeat");
      return;
    }
    in += 2;
    while ( c )
    {
      *out++ = 0;
      --c;
    }
  }
  while ( out - bspworld.byte_10067564 < row );
  bspworld.dword_10069564 = cluster;
}
// gladiator.dll: 10005B30..10005C1C
// gladi386.so:   0000D6E4..0000D8EB
BOOL __cdecl AAS_InPVS(float *a1, float *a2, int a3)
{
  int v4; // edi
  dleaf_t *v5; // eax
  __int16 v6; // cx
  dleaf_t *v7; // esi

  if ( !bspworld.dword_100674C0 )
    return 1;
  if ( !bspworld.visdatasize )
    return 1;
  if ( bspworld.flt_1006956C == *a1 && bspworld.flt_10069570 == a1[1] && bspworld.flt_10069574 == a1[2] )
  {
    v4 = bspworld.dword_10069564;
  }
  else
  {
    v5 = sub_10003420(a1, 0);
    v6 = v5->cluster;
    if ( v6 == -1 )
      return 0;
    v4 = v6;
    bspworld.flt_1006956C = *a1;
    bspworld.flt_10069570 = a1[1];
    bspworld.flt_10069574 = a1[2];
    bspworld.dword_10069568 = v5->area;
  }
  v7 = sub_10003420(a2, 0);
  if ( v7->cluster == -1 )
    return 0;
  AAS_DecompressVis(v4, a3);
  v6 = v7->cluster;
  return ((unsigned __int8)bspworld.byte_10067564[v6 >> 3] & (unsigned __int8)(1 << (v6 & 7))) != 0;
}
// gladiator.dll: 10005C60..10005C75
// gladi386.so:   0000D8EC..0000D90F
qboolean __cdecl AAS_inPVS(vec3_t p1, vec3_t p2)
{
  return AAS_InPVS(p1, p2, 0);
}
// gladiator.dll: 10005C90..10005CA5
// gladi386.so:   0000D910..0000D933
BOOL __cdecl sub_10005C90(float *a1, float *a2)
{
  return AAS_InPVS(a1, a2, 1);
}
// gladiator.dll: 10005CC0..10005CD5
// gladi386.so:   0000D934..0000D95D
/* Double-indirect lookup into the cluster-routing pointer table.  DEAD in
 * Gladiator — the live accessors index aasworld.clusterareacache directly. */
int __cdecl sub_10005CC0(int a, int b)
{
  return ((int **)bspworld.dword_10067560)[a][b];
}
// gladiator.dll: 10005CF0..10005E10
// gladi386.so:   0000D960..0000DAF5
/* Reach-graph propagation over the cluster-routing matrix (dword_10067560, an
 * int[N] row-pointer table followed by N int[N] rows) and the 1-D flag row
 * dword_1006755C.  Three phases:
 *
 *   A: flag_row[row_index] = value;
 *   B: for each area i, clear matrix[i][*], set the diagonal, and for every
 *      areaportal of i whose flag is set, mark matrix[i][link] and
 *      matrix[link][i];
 *   C: the nested walk below.
 *
 * Phase C carries a faithful Mr. Elusive bug: its innermost `while (numareas >
 * 0)` tests a value the loop never changes, so it spins forever walking j off
 * the end of the matrix.  The byte-match needs the unbounded shape.
 *
 * Do NOT cache the globals in locals: the original re-reads numareas / dareas /
 * dareaportals / both matrix pointers at each use.
 *
 * DEAD in Gladiator. */
void __cdecl sub_10005CF0(int row_index, int value)
{
  int i, j, k, col;
  darea_t *area;
  dareaportal_t *portal;

  /* Phase A */
  ((int *)bspworld.dword_1006755C)[row_index] = value;

  /* Phase B */
  for (i = 0; i < bspworld.numareas; i++) {
    for (j = 0; j < bspworld.numareas; j++)
      ((int **)bspworld.dword_10067560)[i][j] = 0;
    ((int **)bspworld.dword_10067560)[i][i] = 1;

    area = &bspworld.dareas[i];
    for (k = 0; k < area->numareaportals; k++) {
      col = area->firstareaportal + k;
      if (((int *)bspworld.dword_1006755C)[col] != 0) {
        portal = &bspworld.dareaportals[col];
        ((int **)bspworld.dword_10067560)[i][portal->otherarea] = 1;
        ((int **)bspworld.dword_10067560)[portal->otherarea][i] = 1;
      }
    }
  }

  /* Phase C — the faithful Mr. Elusive bug (see banner); DEAD code,
   * never executed. */
  for (i = 0; i < bspworld.numareas; i++) {
    for (j = 0; j < bspworld.numareas; j++) {
      while (bspworld.numareas > 0) {
        if (((int **)bspworld.dword_10067560)[i][j] != 0 && ((int **)bspworld.dword_10067560)[j][0] != 0) {
          ((int **)bspworld.dword_10067560)[i][0] = 1;
          ((int **)bspworld.dword_10067560)[0][i] = 1;
        }
        j++;
      }
    }
  }
}
// gladiator.dll: 10005E60..1000601A
// gladi386.so:   0000DAF8..0000DDA6
/* Rotated AABB of a Q2 BSP inline model.  Q3's cognate delegates the rotation to
 * a botimport callback; this does it locally.  Each Q2 dmodel_t entry is 48
 * bytes: mins[3], maxs[3], origin[3], headnode, firstface, numfaces. */
void __cdecl AAS_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin)
{
  vec3_t axis[3];            /* rotation matrix from angles                 */
  vec3_t corner;
  vec3_t local_mins, local_maxs;
  vec3_t bb_mins, bb_maxs;   /* accumulator for the rotated bbox */
  int    i;

  if ( !bspworld.dword_100674C0 )
    return;
  if ( modelnum < 0 || modelnum >= bspworld.nummodels )
  {
    botimport.Print(PRT_FATAL, "AAS_BSPModelMinsMaxs: modelnum %d out of range [0-%d]", modelnum, bspworld.nummodels);
    if ( mins )   { VectorClear(mins); }
    if ( maxs )   { VectorClear(maxs); }
    if ( origin ) { VectorClear(origin); }
    return;
  }

  VectorCopy(bspworld.dmodels[modelnum].mins, local_mins);
  VectorCopy(bspworld.dmodels[modelnum].maxs, local_maxs);

  AnglesToAxis(angles, axis);   /* build 3x3 row-major rotation matrix */
  ClearBounds(bb_mins, bb_maxs);

  /* Rotate all 8 AABB corners through `axis`.  `corner` must stay an array:
   * RotatePoint reads it as a vec3_t. */
  for ( i = 0; i < 8; ++i )
  {
    /* Keep `(i < 4)` with mins in the THEN arm: the inverted
     * `(i >= 4) ? maxs : mins` flips the branch polarity. */
    corner[0] = (i < 4)          ? local_mins[0] : local_maxs[0];
    corner[1] = (i & 1)          ? local_mins[1] : local_maxs[1];
    corner[2] = (i < 2 || i > 6) ? local_mins[2] : local_maxs[2];
    RotatePoint(corner, (float *)axis);
    AddPointToBounds(corner, bb_mins, bb_maxs);
  }

  if ( mins ) { VectorCopy(bb_mins, mins); }
  if ( maxs ) { VectorCopy(bb_maxs, maxs); }
  if ( origin )
  {
    VectorCopy(bspworld.dmodels[modelnum].origin, origin);
  }
}
// gladiator.dll: 10006090..100060DA
// gladi386.so:   0000DDA8..0000DE3D
void __cdecl AAS_UnlinkFromBSPLeaves(bsp_link_t *leaves)
{
  bsp_link_t *result; // eax
  bsp_link_t *prev;   // ecx — prev_ent in leaf chain
  bsp_link_t *v3;     // esi — saved next_leaf
  bsp_link_t *next;   // ecx — next_ent in leaf chain

  result = leaves;
  if ( leaves )
  {
    do
    {
      v3 = result->next_leaf;
      prev = result->prev_ent;
      if ( prev )
        prev->next_ent = result->next_ent;
      else
        bspworld.dword_10069584[result->leafnum] = result->next_ent;
      next = result->next_ent;
      if ( next )
        next->prev_ent = result->prev_ent;
      sub_10003240(result);
      result = v3;
    }
    while ( v3 );
  }
}
// gladiator.dll: 10006100..100061C5
// gladi386.so:   0000DE40..0000DF48
/* Classify an AABB against a BSP splitting plane, returning 1/2/3 for
 * front/back/spanning (Q3 be_aas_sample.c). */
int __cdecl AAS_BoxOnPlaneSide2(vec3_t absmins, vec3_t absmaxs, float *p)
{
  int    i, sides;
  vec3_t corners[2];
  float  dist1, dist2;

  for ( i = 0; i < 3; ++i )
  {
    if ( p[i] < 0.0f )
    {
      corners[0][i] = absmins[i];
      corners[1][i] = absmaxs[i];
    }
    else
    {
      corners[1][i] = absmins[i];
      corners[0][i] = absmaxs[i];
    }
  }
  dist1 = DotProduct(p, corners[0]) - p[3];
  dist2 = DotProduct(p, corners[1]) - p[3];
  sides = 0;
  if ( dist1 >= 0.0f )
    sides = 1;
  if ( dist2 < 0.0f )
    sides |= 2;
  return sides;
}
// gladiator.dll: 10006210..1000636D
// gladi386.so:   0000DF48..0000E26F
bsp_link_t *__cdecl AAS_BSPLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum, int modelnum)
{
  bsp_link_t *link; // edi
  int *v6; // ebx
  int nodenum; // eax
  int leafnum; // esi
  bsp_link_t *newlink; // eax
  bsp_link_t *v10; // ecx
  dnode_t *v11; // esi
  dplane_t *plane; // ecx
  int v13; // edx
  int v14; // eax
  int v15[64]; // [esp+10h] [ebp-100h] BYREF — stack-based BSP traversal queue

  if ( !bspworld.dword_100674C0 )
    return 0;
  link = 0;
  v15[0] = bspworld.dmodels[modelnum].headnode;
  v6 = &v15[1];
  /* while(1)+break, NOT `while (--v6 >= &v15[0])`: v6 starts at &v15[1], so MSVC6
   * can prove that guard holds first time and rotates to a bottom test.  The
   * infinite-loop form keeps the test at the top — do not "simplify". */
  while ( 1 )
  {
    if ( --v6 < &v15[0] )
      break;
    nodenum = *v6;
    if ( nodenum < 0 )
    {
      leafnum = -1 - nodenum;
      newlink = sub_100031F0();
      if ( !newlink )
        return link;
      newlink->entnum    = entnum;
      newlink->leafnum   = leafnum;
      newlink->prev_leaf = 0;
      newlink->next_leaf = link;
      if ( link )
        link->prev_leaf = newlink;
      link = newlink;
      newlink->prev_ent  = 0;
      newlink->next_ent  = 0;
      newlink->next_ent  = bspworld.dword_10069584[leafnum];
      v10 = bspworld.dword_10069584[leafnum];
      if ( v10 )
        v10->prev_ent = newlink;
      bspworld.dword_10069584[leafnum] = newlink;
    }
    else
    {
      v11 = &bspworld.dnodes[nodenum];
      plane = &bspworld.dplanes[v11->planenum];
      v13 = plane->type;
      if ( v13 < 3 )
      {
        if ( plane->dist <= (float)absmins[v13] )
          v14 = 1;
        else if ( plane->dist >= (float)absmaxs[v13] )
          v14 = 2;
        else
          v14 = 3;
      }
      else
      {
        v14 = AAS_BoxOnPlaneSide2(absmins, absmaxs, (float *)plane);
      }
      if ( (v14 & 1) != 0 )
        *v6++ = v11->children[0];
      if ( (v14 & 2) != 0 )
        *v6++ = v11->children[1];
    }
  }
  return link;
}
// gladiator.dll: 100063D0..10006588
// gladi386.so:   0000E270..0000E563
/* AAS_EntitiesInBox(mins, maxs, list, maxcount): write up to `maxcount` entnums
 * of the world entities overlapping [mins,maxs] into list[] and return the count.
 *
 * AAS_BSPLinkEntity builds a transient bsp_link_t chain, one node per leaf the
 * box touches.  Each such leaf's entity-link chain is walked, duplicates skipped
 * by a linear scan, and each candidate's bbox overlap-tested.  SOLID_BBOX and
 * SOLID_TRIGGER are added directly; a SOLID_BSP movable brush is re-linked by
 * its own modelnum and added only if one of ITS leaves carries an AAS area
 * marker.  Every chain is released with AAS_UnlinkFromBSPLeaves, NULL included.
 *
 * DEAD in Gladiator. */
int __cdecl sub_100063D0(vec3_t mins, vec3_t maxs, int *list, int maxcount)
{
  bsp_link_t *linkhead;
  bsp_link_t *link;
  bsp_link_t *ent_link;
  bsp_link_t *brush_links;
  bsp_link_t *brush_iter;
  int         count;
  int         j;
  int         solid;
  bsp_entdata_t entdata;      /* AAS_EntityBSPData destination — 56 B */

  count = 0;
  linkhead = AAS_BSPLinkEntity(mins, maxs, 0, 0);
  /* NO early `if (!linkhead) return 0;` — the original's test is this loop's entry
   * guard and lands on the shared exit, so the NULL path still calls
   * AAS_UnlinkFromBSPLeaves(NULL), which tolerates it. */
  for (link = linkhead; link && count < maxcount; link = link->next_leaf) {
    ent_link = bspworld.dword_10069584[link->leafnum];
    if (!ent_link)
      continue;

    while (ent_link && count < maxcount) {
      /* Dedupe scan over the results so far.  Re-read `ent_link->entnum` at each of
       * the four use sites — the original never caches it in a local. */
      for (j = 0; j < count; j++) {
        if (list[j] == ent_link->entnum)
          break;
      }

      if (j == count) {
        AAS_EntityBSPData(ent_link->entnum, &entdata);

        /* All six bounds are the correct ones.  The two textually identical
         * `fld [esp+0x3c]` in the original read DIFFERENT fields: the first runs
         * while esp is still 8 lower, so it is absmins[0], the second absmins[2]. */
        if (entdata.absmins[0] <= maxs[0]
         && entdata.absmaxs[0] >= mins[0]
         && entdata.absmins[1] <= maxs[1]
         && entdata.absmaxs[1] >= mins[1]
         && entdata.absmins[2] <= maxs[2]
         && entdata.absmaxs[2] >= mins[2]) {
          solid = entdata.solid;
          if (solid == 1 || solid == 2) {
            list[count++] = ent_link->entnum;
          } else if (solid == 3) {
            /* 4th arg is entdata.modelnum, not entnum. */
            brush_links = AAS_BSPLinkEntity(mins, maxs, 0, entdata.modelnum);
            /* No `if (brush_links)` guard either — the NULL case unlinks NULL.  The
             * hit is reported by BREAKing to the shared post-loop re-check. */
            for (brush_iter = brush_links; brush_iter; brush_iter = brush_iter->next_leaf) {
              /* Through a `dleaf_t *`, not a subscript: the original folds the
               * scaled index and the lump base into ONE address (`add eax,[ecx+0x2c]`)
               * where the subscript form keeps two registers and a SIB. */
              dleaf_t *leaf = &bspworld.dleafs[brush_iter->leafnum];
              if (leaf->numleafbrushes)
                break;
            }
            if (brush_iter) {
              list[count++] = ent_link->entnum;
            }
            AAS_UnlinkFromBSPLeaves(brush_links);
          }
        }
      }
      ent_link = ent_link->next_ent;
    }
  }

  AAS_UnlinkFromBSPLeaves(linkhead);
  return count;
}
// gladiator.dll: 10006600..10006702
// gladi386.so:   0000E564..0000E653
// Set/update a BSP epair in an entity's epair list — the writing counterpart of
// AAS_ValueForBSPEpairKey.  On a key-hit frees the old value and replaces it; on
// a miss prepends a freshly-cleared 12-byte epair.  The original inlines the
// strcmp/strlen/memcpy as repe scas / rep movs; written here as the equivalent
// strdup-style idiom MSVC inlined from <string.h>.
// Allocator thunks: 0x10001479 -> GetClearedMemory, 0x10001AB4 -> GetMemory,
// 0x1000180C -> FreeMemory.
// DEAD in Gladiator; preserved by /INCREMENTAL.
void __cdecl sub_10006600(bsp_epair_t **head, char *key, char *value)
{
  bsp_epair_t *ep;
  int          klen;
  int          vlen1;
  int          vlen2;

  for ( ep = *head; ep; ep = ep->next )
  {
    if ( !strcmp(ep->key, key) )
    {
      FreeMemory(ep->value);
      vlen1 = strlen(value) + 1;
      ep->value = (char *)GetMemory(vlen1);
      strcpy(ep->value, value);
      return;
    }
  }
  ep = (bsp_epair_t *)GetClearedMemory(12);
  ep->next = *head;
  *head    = ep;
  klen = strlen(key) + 1;
  ep->key = (char *)GetMemory(klen);
  strcpy(ep->key, key);
  vlen2 = strlen(value) + 1;
  ep->value = (char *)GetMemory(vlen2);
  strcpy(ep->value, value);
}
// gladiator.dll: 10006760..100067BD
// gladi386.so:   0000E654..0000E699
char *__cdecl AAS_ValueForBSPEpairKey(bsp_entity_t *ent, const char *key)
{
  bsp_epair_t *ep;

  for ( ep = ent->epairs; ep; ep = ep->next )
  {
    if ( !strcmp(ep->key, key) )
      return ep->value;
  }
  return 0;
}
// gladiator.dll: 100067E0..1000686E
// gladi386.so:   0000E69C..0000E76A
int __cdecl AAS_VectorForBSPEpairKey(bsp_entity_t *ent, const char *key, vec3_t v)
{
  char *value; // eax
  double v1; // [esp+0h] [ebp-18h] BYREF
  double v2; // [esp+8h] [ebp-10h] BYREF
  double v3; // [esp+10h] [ebp-8h] BYREF

  value = AAS_ValueForBSPEpairKey(ent, key);
  if ( !value )
    return (int)(intptr_t)value;
  v3 = 0.0;
  v2 = 0.0;
  v1 = 0.0;
  sscanf(value, "%lf %lf %lf", &v1, &v2, &v3);
  *v = v1;
  v[1] = v2;
  v[2] = v3;
  return 1;
}
// gladiator.dll: 100068A0..100068C5
// gladi386.so:   0000E76C..0000E7D9
float __cdecl FloatForKey(bsp_entity_t *ent, const char *key)
{
  const char *value; // eax

  value = (const char *)AAS_ValueForBSPEpairKey(ent, key);
  if ( !value )
    return 0.0f;
  return atof(value);
}
// gladiator.dll: 100068E0..10006901
// gladi386.so:   0000E7DC..0000E83A
int __cdecl AAS_IntForBSPEpairKey(bsp_entity_t *ent, const char *key)
{
  const char *value; // eax

  value = (const char *)AAS_ValueForBSPEpairKey(ent, key);
  if ( !value )
    return (int)value;
  return atoi(value);
}
// gladiator.dll: 10006920..1000697A
// gladi386.so:   0000E83C..0000E8B2
void __cdecl AAS_FreeBSPEntities(bsp_entity_t *a1)
{
  bsp_entity_t *v1; // ebx
  bsp_epair_t  *epair; // esi
  bsp_entity_t *v3; // ebp
  bsp_epair_t  *nextepair; // edi

  for ( v1 = a1; v1; v1 = v3 )
  {
    v3 = v1->next;
    for ( epair = v1->epairs; epair; epair = nextepair )
    {
      nextepair = epair->next;
      if ( epair->key )
        FreeMemory(epair->key);
      if ( epair->value )
        FreeMemory(epair->value);
      FreeMemory(epair);
    }
    FreeMemory(v1);
  }
}
// gladiator.dll: 100069A0..10006C59
// gladi386.so:   0000E8B4..0000EC80
bsp_entity_t *AAS_ParseBSPEntities(void)
{
  script_t *script; // ebp
  token_t token;
  bsp_entity_t *ent; // [esp+10h]
  bsp_epair_t *epair; // ebx
  bsp_entity_t *entities; // edi

  script = LoadScriptMemory(bspworld.dentdata, bspworld.entdatasize, "entdata");
  SetScriptFlags(script, 12);
  entities = 0;
  while ( PS_ReadToken(script, &token) )
  {
    if ( strcmp(token.string, "{") )
    {
      ScriptError(script, "invalid %s\n", token.string);
      AAS_FreeBSPEntities(entities);
      FreeScript(script);
      return 0;
    }
    ent = (bsp_entity_t *)GetClearedMemory(sizeof(bsp_entity_t));
    ent->next = entities;
    entities = ent;
    while ( PS_ReadToken(script, &token) )
    {
      if ( !strcmp(token.string, "}") )
        break;
      epair = (bsp_epair_t *)GetClearedMemory(sizeof(bsp_epair_t));
      epair->next = ent->epairs;
      ent->epairs = epair;
      if ( token.type != 1 )
      {
        ScriptError(script, "invalid %s\n", token.string);
        AAS_FreeBSPEntities(entities);
        FreeScript(script);
        return 0;
      }
      StripDoubleQuotes(token.string);
      epair->key = (char *)GetMemory(strlen(token.string) + 1);
      strcpy(epair->key, token.string);
      if ( !PS_ExpectTokenType(script, 1, 0, &token) )
      {
        AAS_FreeBSPEntities(entities);
        FreeScript(script);
        return 0;
      }
      StripDoubleQuotes(token.string);
      epair->value = (char *)GetMemory(strlen(token.string) + 1);
      strcpy(epair->value, token.string);
    }
    if ( strcmp(token.string, "}") )
    {
      ScriptError(script, "missing }\n");
      AAS_FreeBSPEntities(entities);
      FreeScript(script);
      return 0;
    }
  }
  FreeScript(script);
  return entities;
}
// 10001C30: thunk -> 0x1003F5C0 = PS_ExpectTokenType (script-level expect)
// gladiator.dll: 10006D10..1000706B
// gladi386.so:   0000EC80..0000F0A1
/* Quake 1's `WinQuake/gl_rlight.c` RecursiveLightPoint (the GL variant, which
 * also exports the impact point), with Q1 `world.c`'s SV_RecursiveHullCheck head
 * grafted on, plus an axial fast path Q1 never had.  Algorithm is Q1, data
 * layout is Q2.
 *
 * Deliberate deviations from Q1 — do NOT "restore" them:
 *   - returns 0/1 rather than -1/0..255, and drops Q1's redundant second
 *     `if ((back<0)==side) return -1;`
 *   - drops Q1's `surf->flags & SURF_DRAWTILED` skip
 *   - `mid[i] = (end[i]-start[i])*frac + start[i]`, not Q1's
 *     `start[i] + (end[i]-start[i])*frac`
 *   - Q1's single-channel `r += *lightmap * scale` becomes a 3-channel RGB read.
 * Reads the per-face {texturemins[2], extents[2]} table CalcSurfaceExtents
 * builds. */
int __cdecl RecursiveLightPoint(int nodenum, float *start, float *end, float *lightspot, int *pointcolor)
{
  dnode_t *v6; // esi
  int v7; // edx
  dplane_t *v8; // eax
  float v10; // st7
  float v11; // st6
  int v14; // eax
  int v15; // ebx
  __int16 *v16; // edi
  texinfo_t *v17; // esi
  int v18; // ebp
  int v19; // eax
  int v20; // ecx
  int v21; // eax
  int v22; // ecx
  int v24; // esi
  int v25; // edx
  int v29; // ebx
  int v30; // ebp
  int v31; // ecx
  qboolean side;
  int v32; // edx
  unsigned __int8 *v33; // ecx
  int v35; // eax
  int v39; // [esp+10h] [ebp-14h]
  dnode_t *v40; // [esp+14h] [ebp-10h]
  vec3_t mid; // [esp+18h] [ebp-Ch] BYREF — intersection on splitting plane, passed to recursive RecursiveLightPoint
  int i; // [esp+30h] [ebp+Ch]
  dface_t *v45; // [esp+2Ch] [ebp+8h]

  if ( nodenum < 0 )
    return 0;
  v6 = &bspworld.dnodes[nodenum];
  v40 = v6;
  v8 = &bspworld.dplanes[v6->planenum];
  v7 = v8->type;
  if ( v7 < 3 )
  {
    v10 = start[v7] - v8->dist;
    v11 = end[v7] - v8->dist;
  }
  else
  {
    v10 = *start * v8->normal[0] + start[1] * v8->normal[1] + start[2] * v8->normal[2] - v8->dist;
    v11 = *end * v8->normal[0] + end[1] * v8->normal[1] + end[2] * v8->normal[2] - v8->dist;
  }
  side = v10 < 0.0f;
  if ( side == (v11 < 0.0f) )
    return RecursiveLightPoint(v6->children[side], start, end, lightspot, pointcolor);
  mid[0] = (*end - *start) * (v10 / (v10 - v11)) + *start;
  mid[1] = (end[1] - start[1]) * (v10 / (v10 - v11)) + start[1];
  mid[2] = (end[2] - start[2]) * (v10 / (v10 - v11)) + start[2];
  if ( RecursiveLightPoint(v6->children[side], start, mid, lightspot, pointcolor) )
    return 1;
  v14 = v6->firstface;
  v39 = 0;
  v15 = v6->numfaces;
  v45 = &bspworld.dfaces[v14];
  v16 = (__int16 *)(bspworld.dword_10067558 + 8 * v14);
  for ( ; v39 < v15; ++v39, ++v45, v16 += 4 )
  {
    v17 = &bspworld.texinfo[v45->texinfo];
    v18 = (int)(mid[2] * v17->vecs[0][2] + mid[1] * v17->vecs[0][1] + mid[0] * v17->vecs[0][0] + v17->vecs[0][3]);
    v19 = (int)(mid[2] * v17->vecs[1][2] + mid[1] * v17->vecs[1][1] + mid[0] * v17->vecs[1][0] + v17->vecs[1][3]);
    v20 = *v16;
    if ( v18 < v20 )
      continue;
    if ( v19 < v16[1] )
      continue;
    v21 = v19 - v16[1];
    v22 = v18 - v20;
    if ( v22 > v16[2] )
      continue;
    if ( v21 <= v16[3] )
      goto sample_lightmap;
  }
  v6 = v40;
  return RecursiveLightPoint(v6->children[!side], mid, end, lightspot, pointcolor);
sample_lightmap:
  v25 = v45->lightofs;
  if ( v25 < 0 )
  {
    *pointcolor = 0;
    pointcolor[1] = 0;
    pointcolor[2] = 0;
    VectorCopy(mid, lightspot);
    return 1;
  }
  v24 = 0;
  v30 = 0;
  v29 = (v16[2] >> 4) + 1;
  v31 = v25 + 2 * ((v21 >> 4) + v29 * (v22 >> 4)) + (v21 >> 4) + v29 * (v22 >> 4);
  v32 = 0;
  v33 = (unsigned __int8 *)(bspworld.dlightdata + v31);
  for ( i = 0; i < 4; ++i )
  {
    if ( v45->styles[i] == 0xFF )
      break;
    v32 += 264 * *v33;
    v24 += 264 * v33[1];
    v30 += 264 * v33[2];
    v35 = ((v16[2] >> 4) + 1) * ((v16[3] >> 4) + 1);
    v33 += 2 * v35 + v35;
  }
  *pointcolor = v32 >> 8;
  pointcolor[1] = v24 >> 8;
  pointcolor[2] = v30 >> 8;
  VectorCopy(mid, lightspot);
  return 1;
}
// gladiator.dll: 10007150..100071BC
// gladi386.so:   0000F0A4..0000F125
/* Static-light helper for AAS_BSPTraceLight: traces model 0 via
 * RecursiveLightPoint, returning endpos plus the surface RGB.  Q3 stubs the whole
 * BSPTraceLight feature, so there is no cognate name. */
int __cdecl sub_10007150(intptr_t start, intptr_t end, intptr_t endpos, _DWORD *red, _DWORD *green, _DWORD *blue)
{
  /* int[3], not float[3]: RecursiveLightPoint writes the RGB samples as ints and
   * the original copies them out with plain movs.  Typed float, the copy becomes a
   * float->int conversion that truncates the denormal-looking samples to 0. */
  int v7[3]; // [esp+0h] [ebp-Ch] BYREF

  if ( !bspworld.dword_100674C0 )
    return 0;
  if ( !bspworld.dlightdata || !RecursiveLightPoint(bspworld.dmodels[0].headnode, (float *)start, (float *)end, (float *)endpos, v7) )
    return 0;
  *red = v7[0];
  *green = v7[1];
  *blue = v7[2];
  return 1;
}
// gladiator.dll: 100071E0..100073D3
// gladi386.so:   0000F128..0000F4CF
/* Quake 1 `WinQuake/model.c` CalcSurfaceExtents, walking the BSP file lumps
 * instead of a loaded model_t and writing an 8-byte {short texturemins[2]; short
 * extents[2]} record per face into the dword_10067558 side table.  Consumed by
 * RecursiveLightPoint.
 *
 * Deliberate deviations from Q1 — do NOT restore:
 *   - mins init is 99999.0f, not id's 999999 (maxs is -99999.0f as in Q1)
 *   - Q1's TEX_SPECIAL bad-extents Sys_Error check is absent
 *   - the `val` dot product is written vecs-first and out of index order.  This
 *     one is unobservable: MSVC6 /O2 canonicalises a 3-term FP sum to (2,1,0)
 *     whatever the source order. */
void CalcSurfaceExtents()
{
  int v2; // eax
  dedge_t *v3; // edi
  dvertex_t *v4; // ebx
  int *v5; // ebp
  int v6; // ecx
  int v7; // eax
  int j; // edx
  float v10; // st7
  int v11; // edi
  int k; // esi
  int v13; // eax
  int v15; // eax
  int result; // eax
  qboolean v17; // cc
  char *v16; // ecx
  dface_t *face; // eax
  int i; // [esp+1Ch] [ebp-34h]
  int v19; // [esp+20h] [ebp-30h]
  int v20; // [esp+24h] [ebp-2Ch]
  texinfo_t *v22; // [esp+2Ch] [ebp-24h]
  int v23[2]; // [esp+30h] [ebp-20h]
  int v24[6]; // [esp+38h] [ebp-18h]

  if ( bspworld.dword_10067558 )
    FreeMemory(bspworld.dword_10067558);
  bspworld.dword_10067558 = GetClearedMemory(8 * bspworld.numfaces);
  result = bspworld.numfaces;
  i = 0;
  if ( bspworld.numfaces > 0 )
  {
    v19 = 4;
    /* ONE loop index: the ×20 byte offset the original also spills is MSVC's
     * strength-reduced temp for `dfaces[i]`, not a second source variable.
     * Do NOT convert this to a counted `for` and drop `result`: the ELF gains
     * one insn_diff, and MSVC6 then re-permutes the frame (`v19` moves from
     * [esp+0x14] to [esp+0x10]) and reorders an fld/fmul pair, costing 14 bytes
     * on the DLL.  Measured 2026-08-17. */
    for ( i = 0; ; )
    {
      /* SEVENTH irreconcilable gcc272-vs-MSVC6 conflict.  The DLL emits four
       * separate `mov DWORD PTR [esp+N],0x47c34f80` integer-immediate stores with
       * the `movsx …[+0xa]` face read WEDGED between the third and fourth
       * (0x10007248..0x1000726c) -- exactly the form below.  gladi386.so instead
       * materialises the constant once on the x87 stack and stores it twice
       * (`fld; fst; fstp`), which is Q1's chained `mins[0] = mins[1] = 99999;`.
       * Writing the chain gains ~4 ELF insn_diffs and costs the DLL 7 bytes.
       * The DLL is the canonical target: keep the four stores and the interleave. */
      *(float *)&v23[1] = 99999.0f;
      *(float *)&v23[0] = 99999.0f;
      *(float *)&v24[1] = -99999.0f;
      face = &bspworld.dfaces[i];
      v2 = face->texinfo;
      *(float *)&v24[0] = -99999.0f;
      v22 = &bspworld.texinfo[v2];
      /* ONE read, sign-extended once, hoisted ABOVE the guard.  Reading the field
       * twice makes the test 16-bit with a separate sign-extend; folding the
       * assignment INTO the condition also regresses. */
      v20 = face->numedges;
      if ( v20 > 0 )
      {
        /* Initialiser order is NOT a lever here — the residual is register
         * pressure, not a source shape. */
        v3 = bspworld.dedges;
        v4 = bspworld.dvertexes;
        v5 = &bspworld.dsurfedges[face->firstedge];
        do
        {
          v6 = *v5;
          if ( v6 >= 0 )
            v7 = v3[v6].v[0];
          else
            v7 = v3[-v6].v[1];
          for ( j = 0; j < 2; ++j )
          {
            v10 = v22->vecs[j][2] * v4[v7].point[2]
                + v22->vecs[j][0] * v4[v7].point[0]
                + v22->vecs[j][1] * v4[v7].point[1]
                + v22->vecs[j][3];
            if ( v10 < *(float *)&v23[j] )
              *(float *)&v23[j] = v10;
            if ( v10 > *(float *)&v24[j] )
              *(float *)&v24[j] = v10;
          }
          ++v5;
          --v20;
        }
        while ( v20 );
      }
      v11 = v19;
      for ( k = 0; k < 2; )
      {
        v13 = (int)floor(*(float *)&v23[k] * 0.0625f);
        v24[k + 2] = v13;
        v15 = (int)ceil(*(float *)&v24[k] * 0.0625f);
        v16 = bspworld.dword_10067558;
        v24[k + 4] = v15;
        ++k;
        v11 += 2;
        *(_WORD *)(v11 + v16 - 6) = v24[k + 1] * 16;
        *(_WORD *)(v11 + bspworld.dword_10067558 - 2) = (v24[k + 3] - v24[k + 1]) * 16;
      }
      result = i + 1;
      v17 = ++i < bspworld.numfaces;
      v19 += 8;
      if ( !v17 )
        break;
    }
  }
  { (void)(result); return; }
}
// gladiator.dll: 10007460..1000786F
// gladi386.so:   0000F4D0..0000FC69
/* No parameters: the `a1` the decompiler shows is a phantom __fastcall arg — the
 * prologue's `push ecx` only reserves a local slot, and the sole caller sets up
 * no ecx at all.  Named from its cognate Q2_SwapBSPFile in bspc/l_bsp_q2.c,
 * minus bspc's `todisk` reverse direction.
 *
 * void, like bspc's: gladi386.so's epilogue is a bare `pop/pop/pop/pop; add esp,8;
 * ret` with nothing written to eax, and IDA's `result = bspworld.nummodels` inside
 * the models loop is what lets gcc hoist `nummodels` out of the loop guard -- the
 * original re-reads it from memory on every iteration, as aliasing requires. */
void Q2_SwapBSPFile(void)
{
  /* ONE counter for all four lump loops.  IDA lists i/j/k/m at the same
   * [esp+10h] because MSVC6 coalesced them; gcc 2.7 does NOT coalesce, so every
   * extra name is another frame slot -- the original gets by on TWO (frame 0x8:
   * this counter plus the vis cluster count).  Declared FIRST because gcc 2.7
   * lays the frame out in REVERSE declaration order and the original keeps the
   * counter in the HIGHER of its two slots. */
  int i;
  int v1; // ebp
  int v2; // ebx
  texinfo_t *v3; // esi
  float *v4; // edi
  int v10; // ebp
  int v11; // edi
  int v16; // ebx
  dplane_t *v17; // esi
  float *v18; // edi
  int v22; // ebp
  dnode_t *v23; // esi
  int v30; // ebx
  int v37; // ebp
  dleaf_t *v38; // esi
  int v45; // ebx
  int v69; // ebx
  dmodel_t *v70; // esi
  int v76; // edi

  v1 = 0;
  for ( i = 0; i < bspworld.numtexinfo; ++i )
  {
    v2 = 8;
    v3 = &bspworld.texinfo[i];
    v4 = &v3->vecs[0][0];
    do
    {
      *v4 = LittleFloat(*v4);
      ++v4;
      --v2;
    }
    while ( v2 );
    v3->flags = LittleLong(v3->flags);
    v3->value = LittleLong(v3->value);
    v3->nexttexinfo = LittleLong(v3->nexttexinfo);
    v1 += 76;
  }
  v10 = 0;
  v11 = 0;
  if ( bspworld.dvis )
  {
    bspworld.dvis->numclusters = LittleLong(bspworld.dvis->numclusters);
    v11 = bspworld.dvis->numclusters;
  }
  /* bspc's plain `for (i = 0; i < j; i++)` (l_bsp_q2.c), NOT IDA's `while (1)` with
   * a mid-loop break and a trailing reload of `dvis`: the write-only `char *v9` was
   * IDA's name for that reload, and the break shape stops gcc's -funroll-loops from
   * unrolling this loop 4x with runtime preconditioning the way the original does. */
  for ( i = 0; i < v11; i++ )
  {
    bspworld.dvis->bitofs[i][0] = LittleLong(bspworld.dvis->bitofs[i][0]);
    bspworld.dvis->bitofs[i][1] = LittleLong(bspworld.dvis->bitofs[i][1]);
  }
  for ( i = 0; i < bspworld.numplanes; ++i )
  {
    v16 = 3;
    v17 = &bspworld.dplanes[i];
    v18 = v17->normal;
    do
    {
      *v18 = LittleFloat(*v18);
      ++v18;
      --v16;
    }
    while ( v16 );
    v17->dist = LittleFloat(v17->dist);
    v17->type = LittleLong(v17->type);
    v10 += 20;
  }
  v22 = 0;
  for ( i = 0; i < bspworld.numnodes; ++i )
  {
    v23 = &bspworld.dnodes[i];
    v23->planenum = LittleLong(v23->planenum);
    v23->children[0] = LittleLong(v23->children[0]);
    v23->children[1] = LittleLong(v23->children[1]);
    for ( v30 = 0; v30 < 3; ++v30 )
    {
      v23->mins[v30] = LittleShort(v23->mins[v30]);
      v23->maxs[v30] = LittleShort(v23->maxs[v30]);
    }
    v23->firstface = LittleShort(v23->firstface);
    v23->numfaces = LittleShort(v23->numfaces);
    v22 += 28;
  }
  v37 = 0;
  for ( i = 0; i < bspworld.numleafs; ++i )
  {
    v38 = &bspworld.dleafs[i];
    v38->contents = LittleLong(v38->contents);
    v38->cluster = LittleShort(v38->cluster);
    v38->area = LittleShort(v38->area);
    for ( v45 = 0; v45 < 3; ++v45 )
    {
      v38->mins[v45] = LittleShort(v38->mins[v45]);
      v38->maxs[v45] = LittleShort(v38->maxs[v45]);
    }
    v38->firstleafface = LittleShort(v38->firstleafface);
    v38->numleaffaces = LittleShort(v38->numleaffaces);
    v38->firstleafbrush = LittleShort(v38->firstleafbrush);
    v38->numleafbrushes = LittleShort(v38->numleafbrushes);
    v37 += 28;
  }
  for ( i = 0; i < bspworld.numleafbrushes; ++i )
  {
    bspworld.dleafbrushes[i] = LittleShort(bspworld.dleafbrushes[i]);
  }
  for ( i = 0; i < bspworld.numbrushsides; ++i )
  {
    /* The LittleShort round-trip must stay (identity on little-endian): without it
     * an uninitialised temp lands on every brush's planenum at map-load time and
     * intermittently drives CM_TraceThroughBrush into an out-of-range plane. */
    bspworld.dbrushsides[i].planenum = LittleShort(bspworld.dbrushsides[i].planenum);
    bspworld.dbrushsides[i].texinfo  = LittleShort(bspworld.dbrushsides[i].texinfo);
  }
  for ( i = 0; i < bspworld.numbrushes; ++i )
  {
    bspworld.dbrushes[i].firstside = LittleLong(bspworld.dbrushes[i].firstside);
    bspworld.dbrushes[i].numsides = LittleLong(bspworld.dbrushes[i].numsides);
    bspworld.dbrushes[i].contents = LittleLong(bspworld.dbrushes[i].contents);
  }
  /* The ONE sanctioned byte-view left on a BSP lump.  A typed `&dmodels[i]`
   * yields the same base+offset induction pair but swaps the SIB roles, costing
   * this function its byte-match for a pure encoding tie.
   * This loop keeps IDA's guard-plus-do/while shape while the three above are
   * counted `for`s: the DLL emits `xor ebp,ebp; test eax,eax; jle …; xor ebx,ebx`,
   * i.e. `v69 = 0` INSIDE the guard, which a counted `for` cannot express without
   * an outer `if` that costs MSVC6 two extra instructions.  Measured 2026-08-17:
   * counted-for OUR+0/21b, counted-for-in-an-if OUR+2/234b, this form MATCH. */
  i = 0;
  if ( bspworld.nummodels > 0 )
  {
    v69 = 0;
    do
    {
      v70 = (dmodel_t *)((char *)bspworld.dmodels + v69);
      v70->firstface = LittleLong(v70->firstface);
      v70->numfaces = LittleLong(v70->numfaces);
      v70->headnode = LittleLong(v70->headnode);
      for ( v76 = 0; v76 < 3; ++v76 )
      {
        v70->mins[v76] = LittleFloat(v70->mins[v76]);
        v70->maxs[v76] = LittleFloat(v70->maxs[v76]);
        v70->origin[v76] = LittleFloat(v70->origin[v76]);
      }
      ++i;
      v69 += 48;
    }
    while ( i < bspworld.nummodels );
  }
}
// 1000775A: Q2_SwapBSPFile brushsides loop — the LittleShort round-trip.
//            See note inside the function.
// gladiator.dll: 10007980..10007BAD
// gladi386.so:   0000FC6C..0001000F
void AAS_DumpBSPData()
{
  bspworld.nummodels = 0;
  if ( bspworld.dmodels )
    FreeMemory(bspworld.dmodels);
  bspworld.dmodels = 0;
  bspworld.visdatasize = 0;
  if ( bspworld.dvisdata )
    FreeMemory(bspworld.dvisdata);
  bspworld.dvisdata = 0;
  bspworld.dvis = 0;
  bspworld.lightdatasize = 0;
  if ( bspworld.dlightdata )
    FreeMemory(bspworld.dlightdata);
  bspworld.dlightdata = 0;
  bspworld.entdatasize = 0;
  if ( bspworld.dentdata )
    FreeMemory(bspworld.dentdata);
  bspworld.dentdata = 0;
  bspworld.numleafs = 0;
  if ( bspworld.dleafs )
    FreeMemory(bspworld.dleafs);
  bspworld.dleafs = 0;
  bspworld.numplanes = 0;
  if ( bspworld.dplanes )
    FreeMemory(bspworld.dplanes);
  bspworld.dplanes = 0;
  bspworld.numvertexes = 0;
  if ( bspworld.dvertexes )
    FreeMemory(bspworld.dvertexes);
  bspworld.dvertexes = 0;
  bspworld.numnodes = 0;
  if ( bspworld.dnodes )
    FreeMemory(bspworld.dnodes);
  bspworld.dnodes = 0;
  bspworld.numtexinfo = 0;
  if ( bspworld.texinfo )
    FreeMemory(bspworld.texinfo);
  bspworld.texinfo = 0;
  bspworld.numfaces = 0;
  if ( bspworld.dfaces )
    FreeMemory(bspworld.dfaces);
  bspworld.dfaces = 0;
  bspworld.numedges = 0;
  if ( bspworld.dedges )
    FreeMemory(bspworld.dedges);
  bspworld.dedges = 0;
  bspworld.numleaffaces = 0;
  if ( bspworld.dleaffaces )
    FreeMemory(bspworld.dleaffaces);
  bspworld.dleaffaces = 0;
  bspworld.numleafbrushes = 0;
  if ( bspworld.dleafbrushes )
    FreeMemory(bspworld.dleafbrushes);
  bspworld.dleafbrushes = 0;
  bspworld.numsurfedges = 0;
  if ( bspworld.dsurfedges )
    FreeMemory(bspworld.dsurfedges);
  bspworld.dsurfedges = 0;
  bspworld.numbrushes = 0;
  if ( bspworld.dbrushes )
    FreeMemory(bspworld.dbrushes);
  bspworld.dbrushes = 0;
  bspworld.numbrushsides = 0;
  if ( bspworld.dbrushsides )
    FreeMemory(bspworld.dbrushsides);
  bspworld.dbrushsides = 0;
  bspworld.numareas = 0;
  if ( bspworld.dareas )
    FreeMemory(bspworld.dareas);
  bspworld.dareas = 0;
  bspworld.numareaportals = 0;
  if ( bspworld.dareaportals )
    FreeMemory(bspworld.dareaportals);
  bspworld.dareaportals = 0;
  bspworld.dword_100674C0 = 0;
}
// gladiator.dll: 10007C40..10007CFD
// gladi386.so:   00010010..000100EE
void *__cdecl sub_10007C40(FILE *Stream, int Offset, size_t ElementSize, int a4, char *ArgList)
{
  void *v6; // esi

  if ( (int)ElementSize % a4 )
  {
    AAS_Error("odd %s bsp lump size\n", ArgList);
    AAS_DumpBSPData();
    fclose(Stream);
    return 0;
  }
  if ( fseek(Stream, Offset, 0) )
  {
    AAS_Error("can't seek to bsp lump %s\n", ArgList);
    AAS_DumpBSPData();
    fclose(Stream);
    return 0;
  }
  v6 = GetClearedMemory(ElementSize);
  if ( fread(v6, ElementSize, 1u, Stream) != 1 )
  {
    AAS_Error("can't read bsp lump %s\n", ArgList);
    FreeMemory(v6);
    AAS_DumpBSPData();
    fclose(Stream);
    return 0;
  }
  return v6;
}
// gladiator.dll: 10007D30..10008422
// gladi386.so:   000100F0..000111BF
int AAS_LoadBSPFile(char *FileName, int Offset, int Length)
{
  FILE *v3; // eax
  FILE *v4; // esi
  int v6; // eax
  int    ofs;
  size_t length;
  int v7; // eax
  dBspHeader_t bsp_h; /* Q2 BSP header: ident+version+19 lumps = 0xA0 bytes */

  AAS_DumpBSPData();
  v3 = fopen(FileName, "rb");
  v4 = v3;
  if ( !v3 )
  {
    AAS_Error("can't open bsp file %s\n", FileName);
    return BLERR_CANNOTOPENBSPFILE;
  }
  if ( fseek(v3, Offset, 0) )
  {
    AAS_Error("can't seek to bsp file %s\n");
    fclose(v4);
    return BLERR_CANNOTSEEKTOBSPFILE;
  }
  if ( fread(&bsp_h, 0xA0u, 1u, v4) != 1 )
  {
    AAS_Error("can't read header of bsp file %s\n", FileName);
    fclose(v4);
    return BLERR_CANNOTREADBSPHEADER;
  }
  v6 = bsp_h.ident = LittleLong(bsp_h.ident);
  if ( v6 != 1347633737 )
  {
    AAS_Error("%s is not an BSP file\n", FileName);
    fclose(v4);
    return BLERR_WRONGBSPFILEID;
  }
  v7 = bsp_h.version = LittleLong(bsp_h.version);
  if ( v7 != 38 )
  {
    AAS_Error("bsp file %s is version %i, not %i\n", FileName, v7, 38);
    fclose(v4);
    return BLERR_WRONGBSPFILEVERSION;
  }
  ofs = Offset + LittleLong(bsp_h.lumps[0].fileofs);
  length = LittleLong(bsp_h.lumps[0].filelen);
  bspworld.dentdata = sub_10007C40(v4, ofs, length, 1, "entity");
  if ( !bspworld.dentdata )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.entdatasize = length;
  ofs = Offset + LittleLong(bsp_h.lumps[1].fileofs);
  length = LittleLong(bsp_h.lumps[1].filelen);
  bspworld.dplanes = sub_10007C40(v4, ofs, length, 20, "planes");
  if ( !bspworld.dplanes )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numplanes = length / 0x14;
  ofs = Offset + LittleLong(bsp_h.lumps[2].fileofs);
  length = LittleLong(bsp_h.lumps[2].filelen);
  bspworld.dvertexes = sub_10007C40(v4, ofs, length, 12, "vertexes");
  if ( !bspworld.dvertexes )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numvertexes = length / 0xC;
  ofs = Offset + LittleLong(bsp_h.lumps[3].fileofs);
  length = LittleLong(bsp_h.lumps[3].filelen);
  if ( length )
  {
    bspworld.dvisdata = sub_10007C40(v4, ofs, length, 1, "visibility");
    if ( !bspworld.dvisdata )
      return BLERR_CANNOTREADBSPLUMP;
  }
  else
  {
    bspworld.dvisdata = 0;
    botimport.Print(PRT_MESSAGE, "WARNGING: bsp has no visibility data\n");
  }
  bspworld.visdatasize = length;
  bspworld.dvis = (dvis_t *)bspworld.dvisdata;
  ofs = Offset + LittleLong(bsp_h.lumps[4].fileofs);
  length = LittleLong(bsp_h.lumps[4].filelen);
  bspworld.dnodes = sub_10007C40(v4, ofs, length, 28, "nodes");
  if ( !bspworld.dnodes )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numnodes = length / 0x1C;
  ofs = Offset + LittleLong(bsp_h.lumps[5].fileofs);
  length = LittleLong(bsp_h.lumps[5].filelen);
  bspworld.texinfo = sub_10007C40(v4, ofs, length, 76, "texinfo");
  if ( !bspworld.texinfo )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numtexinfo = length / 0x4C;
  ofs = Offset + LittleLong(bsp_h.lumps[6].fileofs);
  length = LittleLong(bsp_h.lumps[6].filelen);
  bspworld.dfaces = sub_10007C40(v4, ofs, length, 20, "faces");
  if ( !bspworld.dfaces )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numfaces = length / 0x14;
  ofs = Offset + LittleLong(bsp_h.lumps[7].fileofs);
  length = LittleLong(bsp_h.lumps[7].filelen);
  if ( length )
  {
    bspworld.dlightdata = sub_10007C40(v4, ofs, length, 1, "lightning");
    if ( !bspworld.dlightdata )
      return BLERR_CANNOTREADBSPLUMP;
  }
  else
  {
    bspworld.dlightdata = 0;
    botimport.Print(PRT_MESSAGE, "WARNING: bsp has no light data\n");
  }
  bspworld.lightdatasize = length;
  ofs = Offset + LittleLong(bsp_h.lumps[8].fileofs);
  length = LittleLong(bsp_h.lumps[8].filelen);
  bspworld.dleafs = sub_10007C40(v4, ofs, length, 28, "leafs");
  if ( !bspworld.dleafs )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numleafs = length / 0x1C;
  ofs = Offset + LittleLong(bsp_h.lumps[9].fileofs);
  length = LittleLong(bsp_h.lumps[9].filelen);
  bspworld.dleaffaces = sub_10007C40(v4, ofs, length, 2, "leaf faces");
  if ( !bspworld.dleaffaces )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numleaffaces = length >> 1;
  ofs = Offset + LittleLong(bsp_h.lumps[10].fileofs);
  length = LittleLong(bsp_h.lumps[10].filelen);
  bspworld.dleafbrushes = sub_10007C40(v4, ofs, length, 2, "leaf brushes");
  if ( !bspworld.dleafbrushes )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numleafbrushes = length >> 1;
  ofs = Offset + LittleLong(bsp_h.lumps[11].fileofs);
  length = LittleLong(bsp_h.lumps[11].filelen);
  bspworld.dedges = sub_10007C40(v4, ofs, length, 4, "edges");
  if ( !bspworld.dedges )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numedges = length >> 2;
  ofs = Offset + LittleLong(bsp_h.lumps[12].fileofs);
  length = LittleLong(bsp_h.lumps[12].filelen);
  bspworld.dsurfedges = sub_10007C40(v4, ofs, length, 4, "surfedges");
  if ( !bspworld.dsurfedges )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numsurfedges = length >> 2;
  ofs = Offset + LittleLong(bsp_h.lumps[13].fileofs);
  length = LittleLong(bsp_h.lumps[13].filelen);
  bspworld.dmodels = sub_10007C40(v4, ofs, length, 48, "models");
  if ( !bspworld.dmodels )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.nummodels = length / 0x30;
  ofs = Offset + LittleLong(bsp_h.lumps[14].fileofs);
  length = LittleLong(bsp_h.lumps[14].filelen);
  bspworld.dbrushes = sub_10007C40(v4, ofs, length, 12, "brushes");
  if ( !bspworld.dbrushes )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numbrushes = length / 0xC;
  ofs = Offset + LittleLong(bsp_h.lumps[15].fileofs);
  length = LittleLong(bsp_h.lumps[15].filelen);
  bspworld.dbrushsides = sub_10007C40(v4, ofs, length, 4, "brush sides");
  if ( !bspworld.dbrushsides )
    return BLERR_CANNOTREADBSPLUMP;
  bspworld.numbrushsides = length >> 2;
  Q2_SwapBSPFile();
  bspworld.dword_100674C0 = 1;
  fclose(v4);
  CalcSurfaceExtents();
  sub_100030A0();
  sub_10003280();
  sub_100032D0();
  return BLERR_NOERROR;
}
// gladiator.dll: 100085F0..1000860B
// gladi386.so:   000111C0..000111F1
int sub_100085F0()
{
  return sub_10037850(aasworld.mapname, bspworld.dentdata, bspworld.entdatasize);
}

/* ------------------------------------------------------------------------
 * Present in gladi386.so, ABSENT from gladiator.dll.
 *
 * The .so is the Aug 2 1999 build, the DLL Jul 18, so the Linux image carries
 * functions the Windows one does not.  Gated rather than added unconditionally,
 * because the DLL is the canonical byte-match target and code it does not
 * contain must not appear in it.  Drop the gate for any that later turns up.
 * ------------------------------------------------------------------------ */
#ifndef _WIN32

#endif /* !_WIN32 -- gladi386.so-only */
