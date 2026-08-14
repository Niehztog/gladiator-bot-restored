/*
 * be_aas_sample.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x1001AC00..0x1001C6C0; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * Its own interface is in the matching .h; botlib_local.h, which that header
 * pulls in, carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs, the side-band scheme and the externs for
 * botlib.c's remaining globals).
 */

#include "botlib_port.h"
#include "l_libvar.h"      /* libvar_t: 24-byte botlib cvar (reconstructed) */
#undef VectorNegate
#include "be_ea.h"    /* ea_state_t: 36-byte per-client EA struct (reconstructed) */
#include "q2files.h"       /* Q2 BSP file format (+ the BSP header) */
#include "aasfile.h"       /* AAS file format: aas_lump_t, aas_header_t */
#include "be_aas_def.h"   /* aas_t, aas_area_t etc. (reconstructed from aasworld_* globals) */
#include "l_script.h"      /* token_t, script_t, punctuation_t */
#include "l_precomp.h"     /* source_t, define_t */
#include "l_struct.h"      /* structdef_t */
#include "l_utils.h"       /* bot_fileref_t + the Win32 UnZip declarations */
#include "be_ai_def.h"     /* the bot-AI structures and interfaces */
#include "be_interface.h"   /* botimport / botstate / bot_exports + libvar aliases */
#include "struct_sizes_asserts.h" /* compile-time struct-layout guard */
#include "be_aas_sample.h"
#include "be_aas_bspq2.h"
#include "be_aas_main.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_memory.h"

//----- (1001AC00) --------------------------------------------------------
void AAS_InitAASLinkHeap()
{
  aas_link_t *heap;
  int i;
  int count;

  count = aasworld.linkheapsize;
  heap = aasworld.linkheap;
  if ( !heap )
  {
    count = (int)(intptr_t)LibVarValue("max_aaslinks", (char *)"4096");
    if ( count < 0 )
      count = 0;
    aasworld.linkheapsize = count;
    heap = (aas_link_t *)GetMemory(sizeof(aas_link_t) * count);
    aasworld.linkheap = heap;
  }
  aasworld.linkheap[0].prev_ent = NULL;
  aasworld.linkheap[0].next_ent = &aasworld.linkheap[1];
  for ( i = 1; i < count - 1; ++i )
  {
    aasworld.linkheap[i].prev_ent = &aasworld.linkheap[i - 1];
    aasworld.linkheap[i].next_ent = &aasworld.linkheap[i + 1];
  }
  aasworld.linkheap[count - 1].prev_ent = &aasworld.linkheap[count - 2];
  aasworld.linkheap[count - 1].next_ent = NULL;
  aasworld.freelinks = aasworld.linkheap;
}
//----- (1001AD10) --------------------------------------------------------
void AAS_FreeAASLinkHeap()
{
  if ( aasworld.linkheap )
    FreeMemory(aasworld.linkheap);
  aasworld.linkheap = NULL;
  aasworld.linkheapsize = 0;
}
//----- (1001AD50) --------------------------------------------------------
aas_link_t *AAS_AllocAASLink()
{
  aas_link_t *result;
  aas_link_t *next;

  result = aasworld.freelinks;
  if ( !result )
  {
    botimport.Print(PRT_FATAL, "empty aas link heap\n");
    return NULL;
  }
  next = result->next_ent;
  aasworld.freelinks = next;
  if ( next )
    next->prev_ent = NULL;
  return result;
}
//----- (1001ADA0) --------------------------------------------------------
void __cdecl AAS_DeAllocAASLink(aas_link_t *link)
{
  if ( aasworld.freelinks )
    aasworld.freelinks->prev_ent = link;
  link->prev_ent = NULL;
  link->next_ent = aasworld.freelinks;
  link->prev_area = NULL;
  link->next_area = NULL;
  aasworld.freelinks = link;
}
//----- (1001ADE0) --------------------------------------------------------
void AAS_InitAASLinkedEntities(void)
{
  if ( aasworld.loaded )
  {
    if ( aasworld.arealinkedentities )
      FreeMemory(aasworld.arealinkedentities);
    aasworld.arealinkedentities = (aas_link_t **)GetClearedMemory(sizeof(aas_link_t *) * aasworld.numareas);
  }
}
//----- (1001AE30) --------------------------------------------------------
void AAS_FreeAASLinkedEntities()
{
  if ( aasworld.arealinkedentities )
    FreeMemory(aasworld.arealinkedentities);
  aasworld.arealinkedentities = NULL;
}
//----- (1001AE60) --------------------------------------------------------
int __cdecl AAS_PointAreaNum(vec3_t point)
{
  int nodenum; // eax
  aas_node_t *node; // ecx
  aas_plane_t *plane;
  vec_t dist;

  if ( !aasworld.loaded )
  {
    botimport.Print(PRT_ERROR, "AAS_PointAreaNum: aas not loaded\n");
    return 0;
  }
  nodenum = 1;
  do
  {
    node = &aasworld.nodes[nodenum];
    plane = &aasworld.planes[node->planenum];
    dist = DotProduct(point, plane->normal) - plane->dist;
    if ( dist > 0.0f )
      nodenum = node->children[0];
    else
      nodenum = node->children[1];
  }
  while ( nodenum > 0 );
  if ( !nodenum )
    return 0;
  return -nodenum;
}
//----- (1001AF00) --------------------------------------------------------
// Returns aasworld.areasettings[areanum].cluster
// (struct offset +0xC into the 28-byte aas_areasettings_t).  No
// aasworld.loaded guard (unlike AAS_AreaPresenceType — this variant just
// trusts the bounds-check).  Uses `areanum > 0` (not `>= 0`) so area 0 is
// treated as OOR.  On OOR prints "AAS_AreaCluster: invalid area number\n"
// at level 3 and returns 0.
int __cdecl AAS_AreaCluster(int areanum)
{
  if ( areanum <= 0 || areanum >= aasworld.numareas )
  {
    botimport.Print(PRT_ERROR, "AAS_AreaCluster: invalid area number\n");
    return 0;
  }
  return aasworld.areasettings[areanum].cluster;
}
//----- (1001AF50) --------------------------------------------------------
int __cdecl AAS_AreaPresenceType(int areanum)
{
  if ( !aasworld.loaded )
    return 0;
  if ( areanum <= 0 || areanum >= aasworld.numareas )
  {
    botimport.Print(PRT_ERROR, "AAS_AreaPresenceType: invalid area number\n");
    return 0;
  }
  return aasworld.areasettings[areanum].presencetype;
}
//----- (1001AFA0) --------------------------------------------------------
int __cdecl AAS_PointContents(vec3_t point)
{
  int areanum; // eax

  if ( !aasworld.loaded )
    return 0;
  areanum = AAS_PointAreaNum(point);
  if ( !areanum )
    return 1;
  return aasworld.areasettings[areanum].presencetype;
}
//----- (1001AFF0) --------------------------------------------------------
// Signed support-distance of an AABB along a 3D direction.  Builds a
// per-axis support point from {mins, maxs} keyed on sign(normal[i])
// with a +/-0.001 deadband (.rdata QWORD doubles 0x10058230 = +0.001,
// 0x10058220 = -0.001 — `near` and `far` switch when normal[i] lands
// inside that band, the component is forced to 0), then returns
// dot(support, normalize(normal)).  The `sign_select` flag (arg4)
// toggles which AABB end maps to "positive normal":
//   sign_select != 0 → far  side : normal[i] > 0 picks maxs[i]
//                                  normal[i] < 0 picks mins[i]
//   sign_select == 0 → near side : normal[i] > 0 picks mins[i]
//                                  normal[i] < 0 picks maxs[i]
// VectorNormalize is run on a *local copy* of the normal (see esp+0x10
// memcpy then push+call), so the caller's vector survives unchanged
// even though the assembly does an in-place normalize.  DEAD in
// Gladiator — /INCREMENTAL.
double __cdecl sub_1001AFF0(float *normal, float *mins, float *maxs, int sign_select)
{
  vec3_t support;
  vec3_t normal_local;
  int i;

  if ( sign_select )
  {
    for ( i = 0; i < 3; i++ )
    {
      if ( (float)normal[i] > 0.001 )
        support[i] = maxs[i];
      else if ( (float)normal[i] < -0.001 )
        support[i] = mins[i];
      else
        support[i] = 0.0f;
    }
  }
  else
  {
    for ( i = 0; i < 3; i++ )
    {
      if ( (float)normal[i] > 0.001 )
        support[i] = mins[i];
      else if ( (float)normal[i] < -0.001 )
        support[i] = maxs[i];
      else
        support[i] = 0.0f;
    }
  }
  VectorCopy(normal, normal_local);
  /* The original negates the normal in place (call 0x1000147E = VectorNegate,
   * void/eax — NOT VectorNormalize, which would leave a float on ST0 and force
   * an `fstp st(0)` the original never emits). */
  VectorNegate(normal_local);
  return support[0] * normal_local[0]
       + support[1] * normal_local[1]
       + support[2] * normal_local[2];
}
//----- (1001B130) --------------------------------------------------------
qboolean __cdecl AAS_AreaEntityCollision(int areanum, char *start, vec3_t end, int presencetype, int passent, aas_trace_t *trace)
{
  aas_link_t *link; // esi
  vec3_t boxmins; // [esp+10h] [ebp-60h] BYREF
  vec3_t boxmaxs; // [esp+4h] [ebp-6Ch] BYREF
  bsp_trace_t bsptrace; // [esp+1Ch] [ebp-54h] BYREF
  int collision; // [esp+80h] [ebp+10h]

  AAS_PresenceTypeBoundingBox(presencetype, boxmins, boxmaxs);
  bsptrace.fraction = 1.0;
  collision = 0;
  link = aasworld.arealinkedentities[areanum];
  /* The empty-list guard reaches the shared `return 0` tail via `goto fail`, and
   * the post-loop test is Q3's positive `if (collision)`, so the success block
   * is the warm fall-through and `return 0` the cold tail.  Inverting it moves
   * the success block cold. */
  if ( !link )
    goto fail;
  do
  {
    if ( link->entnum != passent )
    {
      if ( AAS_EntityCollision(link->entnum, start, boxmins, boxmaxs, end, 33619971, &bsptrace) )
        collision = 1;
    }
    link = link->next_ent;
  }
  while ( link );
  if ( collision )
  {
    trace->startsolid = bsptrace.startsolid;
    trace->ent = bsptrace.ent;
    VectorCopy(bsptrace.endpos, trace->endpos);
    trace->area = 0;
    trace->planenum = 0;
    return 1;
  }
fail:
  return 0;
}
//----- (1001B260) --------------------------------------------------------
/* AAS_TraceClientBBox — sweep a presence-typed bbox along a line and return
 * the first AAS-leaf hit (fraction=1.0 if the trace clears the BSP).  The BSP
 * traversal stack is aas_tracestack_t frames walked by one tstack_p, as in Q3.
 *
 * Differences from Q3, all faithful to the original:
 *   - ON_EPSILON is 0.0005 (Q3's pre-bk010221 value).
 *   - No PLANE_X/Y/Z axial shortcut (Q3 added, then disabled, one).
 *   - frac is clamped to [0,1], not Q3's later [0.001,0.999].
 *   - No `if (front == back)` FPE guard.
 *
 * Returns by value, as in Q3; passent = -1 disables the entity collision test.
 */
aas_trace_t __cdecl AAS_TraceClientBBox(vec3_t start, vec3_t end,
                                        int presencetype, int passent)
{
  int side, nodenum, tmpplanenum;
  float front, back, frac;
  /* Function-scoped as in Q3: v1 = trace direction, v2 = traversed segment.
   * The startsolid arms have NO VectorClear(v1), so the later plane-facing test
   * reads a stale v1 — an authentic original bug; Q3 added the clear. */
  vec3_t cur_start, cur_end, cur_mid, v1, v2;
  aas_tracestack_t tracestack[64];
  aas_tracestack_t *tstack_p;
  aas_node_t *aasnode;
  aas_plane_t *plane;
  aas_trace_t trace;

  memset(&trace, 0, sizeof(trace));
  if ( !aasworld.loaded )
    return trace;

  tstack_p = tracestack;
  /* we start with the whole line on the stack */
  VectorCopy(start, tstack_p->start);
  VectorCopy(end, tstack_p->end);
  tstack_p->planenum = 0;
  /* start with node 1 because node zero is a dummy for a solid leaf */
  tstack_p->nodenum = 1;     /* starting at the root of the tree */
  tstack_p++;

  while ( 1 )
  {
    /* pop up the stack */
    tstack_p--;
    /* if the trace stack is empty (ended up with a piece of the
     * line to be traced in an area) */
    if ( tstack_p < tracestack )
    {
      /* nothing was hit */
      trace.startsolid = 0;
      trace.fraction = 1.0;
      /* endpos is the end of the line */
      VectorCopy(end, trace.endpos);
      /* nothing hit */
      trace.ent = 0;
      trace.area = 0;
      trace.planenum = 0;
      return trace;
    }
    /* number of the current node to test the line against */
    nodenum = tstack_p->nodenum;
    /* if it is an area */
    if ( nodenum < 0 )
    {
      /* if can't enter the area because it hasn't got the right presence type */
      if ( !(aasworld.areasettings[-nodenum].presencetype & presencetype) )
      {
        /* if the start point is still the initial start point
         * NOTE: no need for epsilons because the points will be
         * exactly the same when they're both the start point */
        if ( tstack_p->start[0] == start[0]
          && tstack_p->start[1] == start[1]
          && tstack_p->start[2] == start[2] )
        {
          trace.startsolid = 1;
          trace.fraction = 0.0;
        }
        else
        {
          trace.startsolid = 0;
          VectorSubtract(end, start, v1);
          VectorSubtract(tstack_p->start, start, v2);
          trace.fraction = VectorLength(v2) / VectorNormalize(v1);
          VectorMA(tstack_p->start, -0.125, v1, tstack_p->start);
        }
        VectorCopy(tstack_p->start, trace.endpos);
        trace.ent = 0;
        trace.area = -nodenum;
        trace.planenum = tstack_p->planenum;
        /* always take the plane with normal facing towards the trace start */
        plane = &aasworld.planes[trace.planenum];
        if ( v1[0] * plane->normal[0] + v1[1] * plane->normal[1] + v1[2] * plane->normal[2] > 0.0f )
          trace.planenum ^= 1;
        return trace;
      }
      else
      {
        if ( passent >= 0 )
        {
          if ( AAS_AreaEntityCollision(-nodenum, tstack_p->start,
                                       tstack_p->end, presencetype, passent,
                                       &trace) )
          {
            if ( !trace.startsolid )
            {
              VectorSubtract(end, start, v1);
              VectorSubtract(trace.endpos, start, v2);
              trace.fraction = VectorLength(v2) / VectorLength(v1);
            }
            return trace;
          }
        }
      }
      trace.lastarea = -nodenum;
      continue;
    }
    /* if it is a solid leaf */
    if ( !nodenum )
    {
      /* if the start point is still the initial start point
       * NOTE: no need for epsilons because the points will be
       * exactly the same when they're both the start point */
      if ( tstack_p->start[0] == start[0]
        && tstack_p->start[1] == start[1]
        && tstack_p->start[2] == start[2] )
      {
        trace.startsolid = 1;
        trace.fraction = 0.0;
      }
      else
      {
        trace.startsolid = 0;
        VectorSubtract(end, start, v1);
        VectorSubtract(tstack_p->start, start, v2);
        trace.fraction = VectorLength(v2) / VectorNormalize(v1);
        VectorMA(tstack_p->start, -0.125, v1, tstack_p->start);
      }
      VectorCopy(tstack_p->start, trace.endpos);
      trace.ent = 0;
      trace.area = 0;     /* hit solid leaf */
      trace.planenum = tstack_p->planenum;
      /* always take the plane with normal facing towards the trace start */
      plane = &aasworld.planes[trace.planenum];
      if ( v1[0] * plane->normal[0] + v1[1] * plane->normal[1] + v1[2] * plane->normal[2] > 0.0f )
        trace.planenum ^= 1;
      return trace;
    }
    /* the node to test against */
    aasnode = &aasworld.nodes[nodenum];
    /* start point of current line to test against node */
    VectorCopy(tstack_p->start, cur_start);
    /* end point of the current line to test against node */
    VectorCopy(tstack_p->end, cur_end);
    /* the current node plane */
    plane = &aasworld.planes[aasnode->planenum];
    front = DotProduct(cur_start, plane->normal) - plane->dist;
    back = DotProduct(cur_end, plane->normal) - plane->dist;
    /* if the whole to be traced line is totally at the front of this node
     * only go down the tree with the front child */
    if ( front > -0.0005 && back > -0.0005 )
    {
      /* keep the current start and end point on the stack
       * and go down the tree with the front child */
      tstack_p->nodenum = aasnode->children[0];
      tstack_p++;
    }
    /* if the whole to be traced line is totally at the back of this node
     * only go down the tree with the back child */
    else if ( front < 0.0005 && back < 0.0005 )
    {
      /* keep the current start and end point on the stack
       * and go down the tree with the back child */
      tstack_p->nodenum = aasnode->children[1];
      tstack_p++;
    }
    /* go down the tree both at the front and back of the node */
    else
    {
      tmpplanenum = tstack_p->planenum;
      /* calculate the hitpoint with the node (split point of the line)
       * put the crosspoint TRACEPLANE_EPSILON pixels on the near side */
      if ( front < 0 )
        frac = (front + 0.125) / (front - back);
      else
        frac = (front - 0.125) / (front - back);
      if ( frac < 0 )
        frac = 0;
      else if ( frac > 1 )
        frac = 1;
      cur_mid[0] = cur_start[0] + (cur_end[0] - cur_start[0]) * frac;
      cur_mid[1] = cur_start[1] + (cur_end[1] - cur_start[1]) * frac;
      cur_mid[2] = cur_start[2] + (cur_end[2] - cur_start[2]) * frac;

      /* side the front part of the line is on */
      side = front < 0;
      /* first put the end part of the line on the stack (back side) */
      VectorCopy(cur_mid, tstack_p->start);
      /* not necessary to store because still on stack:
       * VectorCopy(cur_end, tstack_p->end); */
      tstack_p->planenum = aasnode->planenum;
      tstack_p->nodenum = aasnode->children[!side];
      tstack_p++;
      /* now put the part near the start of the line on the stack so we will
       * continue with that part first */
      VectorCopy(cur_start, tstack_p->start);
      VectorCopy(cur_mid, tstack_p->end);
      tstack_p->planenum = tmpplanenum;
      tstack_p->nodenum = aasnode->children[side];
      tstack_p++;
    }
  }
}
//----- (1001BA00) --------------------------------------------------------
/* AAS_TraceAreas — recursive subdivision of the line by the BSP tree,
 * collecting all areas the line passes through (up to maxareas).
 *
 * Faithful restoration matching Q3 botlib AAS_TraceAreas (be_aas_sample.c:725).
 * The Gladiator version is simpler: 4 args (no separate `points` output),
 * uses a 64-frame stack, and no TRACEPLANE_EPSILON adjustment in the split.
 *
 * a1 = start (vec3*), a2 = end (vec3*), a3 = areas[] output, a4 = maxareas. */
int __cdecl AAS_TraceAreas(float *start, float *end, int *areas, int maxareas)
{
  int side, nodenum, tmpplanenum;
  int numareas;
  float front, back, frac;
  vec3_t cur_start, cur_end, cur_mid;
  aas_tracestack_t tracestack[64];
  aas_tracestack_t *tstack_p;
  aas_node_t *aasnode;
  aas_plane_t *plane;

  numareas = 0;
  areas[0] = 0;
  if ( !aasworld.loaded )
    return numareas;

  tstack_p = tracestack;
  VectorCopy(start, tstack_p->start);
  VectorCopy(end, tstack_p->end);
  tstack_p->planenum = 0;
  tstack_p->nodenum = 1;     /* root of BSP */
  tstack_p++;

  while ( 1 )
  {
    tstack_p--;
    if ( tstack_p < tracestack )
      return numareas;

    nodenum = tstack_p->nodenum;
    if ( nodenum < 0 )
    {
      areas[numareas] = -nodenum;
      numareas++;
      if ( numareas >= maxareas )
        return numareas;
      continue;
    }
    if ( !nodenum )
      continue;

    aasnode = &aasworld.nodes[nodenum];
    VectorCopy(tstack_p->start, cur_start);
    VectorCopy(tstack_p->end, cur_end);
    plane = &aasworld.planes[aasnode->planenum];
    front = DotProduct(cur_start, plane->normal) - plane->dist;
    back = DotProduct(cur_end, plane->normal) - plane->dist;

    if ( front > 0.0f && back > 0.0f )
    {
      tstack_p->nodenum = aasnode->children[0];
      tstack_p++;
    }
    else if ( front <= 0.0f && back <= 0.0f )
    {
      tstack_p->nodenum = aasnode->children[1];
      tstack_p++;
    }
    else
    {
      tmpplanenum = tstack_p->planenum;
      /* Keep the stripped-epsilon branch skeleton: collapsing it changes MSVC6's
       * x87 compare/divide schedule. */
      if ( front < 0.0f )
        frac = front / (front - back);
      else
        frac = front / (front - back);
      if ( frac < 0.0f )
        frac = 0.0f;
      else if ( frac > 1.0f )
        frac = 1.0f;
      cur_mid[0] = cur_start[0] + (cur_end[0] - cur_start[0]) * frac;
      cur_mid[1] = cur_start[1] + (cur_end[1] - cur_start[1]) * frac;
      cur_mid[2] = cur_start[2] + (cur_end[2] - cur_start[2]) * frac;

      side = front < 0.0f;
      VectorCopy(cur_mid, tstack_p->start);
      tstack_p->planenum = aasnode->planenum;
      tstack_p->nodenum = aasnode->children[!side];
      tstack_p++;

      VectorCopy(cur_start, tstack_p->start);
      VectorCopy(cur_mid, tstack_p->end);
      tstack_p->planenum = tmpplanenum;
      tstack_p->nodenum = aasnode->children[side];
      tstack_p++;
    }
  }
}
//----- (1001BD40) --------------------------------------------------------
// Four args (face, pnormal, point, epsilon) with an inlined CrossProduct +
// DotProduct loop, matching Q3's AAS_InsideFace (be_aas_route.c); the callers'
// `add esp,0x10` cleanup confirms the count.
// confirms the 4-dword stack-arg layout.
// DEAD in Gladiator — only reachable via /INCREMENTAL thunks
// (0x1000119f → 1001BD40) from sub_1001C0B0 and sub_1001C210.
qboolean __cdecl AAS_InsideFace(aas_face_t *face, vec3_t pnormal, vec3_t point, float epsilon)
{
  int i, firstvertex, edgenum;
  vec3_t v0;
  vec3_t edgevec, pointvec, sepnormal;
  aas_edge_t *edge;

  if ( !aasworld.loaded )
    return 0;
  for ( i = 0; i < face->numedges; i++ )
  {
    edgenum = aasworld.edgeindex[face->firstedge + i];
    edge = &aasworld.edges[abs(edgenum)];
    firstvertex = edgenum < 0;
    VectorCopy(aasworld.vertexes[edge->v[firstvertex]], v0);
    VectorSubtract(aasworld.vertexes[edge->v[!firstvertex]], v0, edgevec);
    VectorSubtract(point, v0, pointvec);
    sepnormal[0] = edgevec[1] * pnormal[2] - edgevec[2] * pnormal[1];
    sepnormal[1] = edgevec[2] * pnormal[0] - edgevec[0] * pnormal[2];
    sepnormal[2] = edgevec[0] * pnormal[1] - edgevec[1] * pnormal[0];
    if ( DotProduct(pointvec, sepnormal) < -epsilon )
      return 0;
  }
  return 1;
}
//----- (1001BF00) --------------------------------------------------------
qboolean __cdecl AAS_PointInsideFace(int facenum, vec3_t point, float epsilon)
{
  int i;
  int edgenum;
  int firstvertex;
  vec_t *v1;
  vec_t *v2;
  aas_edge_t *edge;
  aas_plane_t *plane;
  vec3_t edgevec;
  vec3_t pointvec;
  vec3_t sepnormal;
  aas_face_t *face;

  if ( !aasworld.loaded )
    return 0;
  face = &aasworld.faces[facenum];
  plane = &aasworld.planes[face->planenum];
  for ( i = 0; i < face->numedges; i++ )
  {
    edgenum = aasworld.edgeindex[face->firstedge + i];
    edge = &aasworld.edges[abs(edgenum)];
    firstvertex = edgenum < 0;
    v1 = aasworld.vertexes[edge->v[firstvertex]];
    v2 = aasworld.vertexes[edge->v[!firstvertex]];
    VectorSubtract(v2, v1, edgevec);
    VectorSubtract(point, v1, pointvec);
    CrossProduct(edgevec, plane->normal, sepnormal);
    if ( DotProduct(pointvec, sepnormal) < -epsilon )
      return 0;
  }
  return 1;
}
//----- (1001C0B0) --------------------------------------------------------
// Scans an area's face list for the first face whose faceflags byte
// (offset +4) has bit 0x04 set, and that survives a predicate-call
// into AAS_InsideFace with a +Z or -Z unit vector (chosen by the sign
// of the face plane's z-component) and a 0.01f epsilon.  Returns
// the matching face pointer or NULL.  aasworld globals consulted:
// areas (0x1006694c, stride 48 — numfaces at +4, firstface at +8),
// faceindex (0x10066944), faces pool (0x1006693c, stride 24 —
// planenum at +0, faceflags at +4), planes pool (0x10066924,
// stride 20 — normal at +0..+8, dist at +12, signbits at +16).
// DEAD in Gladiator — /INCREMENTAL.
void *__cdecl AAS_AreaGroundFace(int areanum, void *point)
{
  int    i;
  int    facenum;
  aas_face_t *face;
  aas_area_t *area;
  float  plane_z;
  vec3_t up = { 0.0f, 0.0f, 1.0f };
  vec3_t dir;

  if ( !aasworld.loaded )
    return 0;
  area = &aasworld.areas[areanum];
  for ( i = 0; i < area->numfaces; i++ )
  {
    facenum = aasworld.faceindex[area->firstface + i];
    face = &aasworld.faces[abs(facenum)];
    if ( !(face->faceflags & 4) )
      continue;
    plane_z = aasworld.planes[face->planenum].normal[2];
    if ( plane_z < 0.0f )
    {
      dir[0] = -up[0];
      dir[1] = -up[1];
      dir[2] = -up[2];
    }
    else
    {
      VectorCopy(up, dir);
    }
    if ( AAS_InsideFace(face, dir, (float *)point, 0.01f) )
      return face;
  }
  return 0;
}
//----- (1001C1C0) --------------------------------------------------------
/* Copies a face's BSP plane (normal + dist) into the caller's buffers.
 * DEAD in Gladiator — live code walks aasworld.planes/faces directly. */
void __cdecl AAS_FacePlane(int facenum, float *normal, float *dist)
{
  int    plane_idx;
  float *plane;

  plane_idx = aasworld.faces[facenum].planenum;
  plane     = (float *)&aasworld.planes[plane_idx];
  VectorCopy(plane, normal);
  *dist     = plane[3];
}
//----- (1001C210) --------------------------------------------------------
// Sibling of sub_1001C0B0 — same face-scan skeleton, but driven from
// a caller-supplied struct (arg1) instead of a bare areanum.  Guard:
// arg1->_i0 must be 0; areanum is read from arg1->_i18.  For each
// face the test is planenum XOR arg1->_i20 masked with 0xFFFFFFFE
// — i.e. "equal except possibly the low bit" (matches the two
// orientations of a single BSP plane).  On match, calls
// AAS_InsideFace(face, &aasworld.planes[planenum*20], (char*)arg1 + 8,
// 0.01f); a non-zero return promotes the face pointer to the result.
// DEAD in Gladiator — /INCREMENTAL.
void *__cdecl sub_1001C210(int *gate)
{
  int    i;
  int    facenum;
  aas_face_t  *face;
  aas_area_t  *area;

  if ( !aasworld.loaded )
    return 0;
  if ( gate[0] != 0 )
    return 0;
  area = &aasworld.areas[gate[6]];                    /* gate->_i18 */
  for ( i = 0; i < area->numfaces; i++ )
  {
    facenum = aasworld.faceindex[area->firstface + i];
    face = &aasworld.faces[abs(facenum)];
    if ( (face->planenum & ~1) != (gate[8] & ~1) )   /* gate->_i20 */
      continue;
    if ( AAS_InsideFace(face, (float *)&aasworld.planes[face->planenum], (float *)(gate + 2), 0.01f) )
      return face;
  }
  return 0;
}
//----- (1001C2E0) --------------------------------------------------------
int __cdecl sub_1001C2E0(float *a1, float *a2, float *a3)
{
  int   i, sides;
  vec3_t corners[2]; /* [0]=closer to plane normal, [1]=farther */
  float dist1, dist2;

  for ( i = 0; i < 3; ++i )
  {
    if ( a3[i] < 0.0f )
    {
      corners[0][i] = a1[i];
      corners[1][i] = a2[i];
    }
    else
    {
      corners[1][i] = a1[i];
      corners[0][i] = a2[i];
    }
  }
  dist1 = a3[0]*corners[0][0] + a3[1]*corners[0][1] + a3[2]*corners[0][2] - a3[3];
  dist2 = a3[0]*corners[1][0] + a3[1]*corners[1][1] + a3[2]*corners[1][2] - a3[3];
  sides = 0;
  if ( dist1 >= 0.0f )
    sides = 1;
  if ( dist2 < 0.0f )
    sides |= 2;
  return sides;
}
//----- (1001C3F0) --------------------------------------------------------
void __cdecl AAS_UnlinkFromAreas(aas_link_t *areas)
{
  aas_link_t *result; // eax
  aas_link_t *prev;   // ecx — prev_ent in area chain
  aas_link_t *v3;     // esi — saved next_area for iteration
  aas_link_t *next;   // ecx — next_ent in area chain

  result = areas;
  if ( areas )
  {
    do
    {
      v3 = result->next_area;
      prev = result->prev_ent;
      if ( prev )
        prev->next_ent = result->next_ent;
      else
        aasworld.arealinkedentities[result->areanum] = result->next_ent;
      next = result->next_ent;
      if ( next )
        next->prev_ent = result->prev_ent;
      AAS_DeAllocAASLink(result);
      result = v3;
    }
    while ( v3 );
  }
}
//----- (1001C460) --------------------------------------------------------
aas_link_t *__cdecl AAS_AASLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum)
{
  aas_link_t *areas;
  aas_link_t *link;
  aas_link_t *next;
  aas_node_t *aasnode;
  int *lstack_p;
  int nodenum;
  int type;
  int side;
  /* The BSP traversal queue is 256 bytes = 64 int slots in the original frame.
   * Sized any smaller (as the decompiler had it), pushing both children of a
   * node runs off the array into adjacent locals and corrupts the traversal,
   * which leaves nearly every level item with goal_areanum 0. */
  int linkstack[64]; // [esp+10h] [ebp-100h] BYREF — stack-based BSP traversal queue

  if ( !aasworld.loaded )
  {
    botimport.Print(PRT_ERROR, "AAS_LinkEntity: aas not loaded\n");
    return 0;
  }
  areas = 0;
  linkstack[0] = 1;
  lstack_p = &linkstack[1];
  while ( 1 )
  {
    --lstack_p;
    if ( lstack_p < &linkstack[0] )
      break;
    nodenum = *lstack_p;
    if ( nodenum < 0 )
    {
      link = AAS_AllocAASLink();
      if ( !link )
        return areas;
      link->prev_area = NULL;
      link->entnum = entnum;
      link->areanum = -nodenum;
      link->next_area = areas;
      if ( areas )
        areas->prev_area = link;
      link->prev_ent = NULL;
      areas = link;
      link->next_ent = ((aas_link_t **)aasworld.arealinkedentities)[-nodenum];
      next = ((aas_link_t **)aasworld.arealinkedentities)[-nodenum];
      if ( next )
        next->prev_ent = link;
      ((aas_link_t **)aasworld.arealinkedentities)[-nodenum] = link;
      continue;
    }
    if ( !nodenum )
      continue;
    aasnode = &aasworld.nodes[nodenum];
    type = aasworld.planes[aasnode->planenum].type;
    if ( type < 3 )
    {
      /* Axial fast-path of AAS_BoxOnPlaneSide2, inlined: side&1 descends front
       * child[0], side&2 descends back child[1].  The comparison polarity below
       * is the original's and matters behaviourally — inverted, a box straddling
       * the plane classifies as front-only instead of both children, so
       * straddling entities get linked into too few areas. */
      if ( aasworld.planes[aasnode->planenum].dist <= (float)absmins[type] )
        side = 1;
      else if ( aasworld.planes[aasnode->planenum].dist >= (float)absmaxs[type] )
        side = 2;
      else
        side = 3;
    }
    else
    {
      side = sub_1001C2E0(absmins, absmaxs, aasworld.planes[aasnode->planenum].normal);
    }
    if ( (side & 1) != 0 )
      *lstack_p++ = aasnode->children[0];
    if ( (side & 2) != 0 )
      *lstack_p++ = aasnode->children[1];
  }
  return areas;
}
//----- (1001C620) --------------------------------------------------------
/* AAS_LinkEntityClientBBox — adjust the entity bbox by the presence-type
 * bounding box, then link it into the AAS area tree. */
aas_link_t *__cdecl AAS_LinkEntityClientBBox(vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype)
{
  vec3_t mins, maxs; // [esp+Ch]/[esp+0h] [ebp-24h]/[ebp-30h] BYREF
  vec3_t newabsmins, newabsmaxs; // [esp+24h]/[esp+18h] [ebp-Ch]/[ebp-18h] BYREF

  AAS_PresenceTypeBoundingBox(presencetype, mins, maxs);
  VectorSubtract(absmins, maxs, newabsmins);
  VectorSubtract(absmaxs, mins, newabsmaxs);
  return AAS_AASLinkEntity(newabsmins, newabsmaxs, entnum);
}
//----- (1001C6C0) --------------------------------------------------------
char *__cdecl AAS_PlaneFromNum(int planenum)
{
  if ( !aasworld.loaded )
    return 0;
  return &aasworld.planes[planenum];
}
