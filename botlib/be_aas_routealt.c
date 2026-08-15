/*
 * be_aas_routealt.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x1001A650..0x1001AB80; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * Its own interface is in the matching .h; botlib_local.h, which that header
 * pulls in, carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs, the side-band scheme and the externs for
 * botlib.c's remaining globals).
 *
 * Every function below carries a two-line address annotation: its extent in the
 * 1999 Windows `gladiator.dll` (PE32) and in the 1999 Linux `gladi386.so`
 * (ELF32, glibc build), each start..end with the end exclusive.  `absent` means
 * the Linux image has no counterpart.  CLAUDE.md, "Function address
 * annotations", records how both ranges are derived.
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
#include "be_aas_routealt.h"
#include "be_aas_bspq2.h"
#include "be_aas_main.h"
#include "be_aas_reach.h"
#include "be_aas_route.h"
#include "be_aas_sample.h"
#include "be_interface.h"
#include "l_log.h"
#include "l_memory.h"

int numclusterareas;     // 0x10066730 area count — stays int (was dword_10066730)
midrangearea_t *midrangeareas; // 0x10066740 (was dword_10066740)
int *clusterareas;             // 0x10066744 (was dword_10066744)

// gladiator.dll: 1001A650..1001A6E8
// gladi386.so:   000282E0..00028392
int __cdecl AAS_AltRoutingFloodCluster_r(int areanum)
{
  int i;
  int otherareanum;
  aas_area_t *area;
  aas_face_t *face;

  clusterareas[numclusterareas] = areanum;
  numclusterareas++;
  midrangeareas[areanum].valid = 0;
  area = &aasworld.areas[areanum];
  for ( i = 0; i < area->numfaces; i++ )
  {
    face = &aasworld.faces[abs(aasworld.faceindex[area->firstface + i])];
    if ( face->frontarea == areanum )
      otherareanum = face->backarea;
    else
      otherareanum = face->frontarea;
    if ( !otherareanum )
      continue;
    if ( !midrangeareas[otherareanum].valid )
      continue;
    AAS_AltRoutingFloodCluster_r(otherareanum);
  }
  /* Falls through with the loop's final area->numfaces load still in eax; the
   * return value is unused. */
}

// gladiator.dll: 1001A720..1001AA98
// gladi386.so:   00028394..000289B4
/* AAS_AlternativeRouteGoals — find clusters of "midrange" route-portal areas
 * lying within a 1.5x detour budget of the direct start->goal route, and emit
 * one alternative-route goal per cluster.
 *
 *   1) Resolve start/goal to areas, and take the baseline travel time between
 *      them.
 *   2) Mark each area whose contents carry AREACONTENTS_ROUTEPORTAL (0x20) and
 *      which has reachabilities, if travel(start->a) and travel(a->goal) are
 *      both within 1.5x the baseline, recording both times.
 *   3) Flood-fill each marked area's connected cluster via face neighbours,
 *      take the cluster centroid, pick the member area whose center is closest
 *      to it, and emit one aas_altroutegoal_t for it — origin, areanum, the
 *      two travel times, and (start+goal) - baseline as the extra travel.
 *      Stop at maxaltroutegoals.
 *
 * Quirk preserved: the per-iteration Log_Write prints the match counter BEFORE
 * it is incremented, so the printed number lags by one.
 *
 * Do NOT cache aasworld.areasettings/areas or midrangeareas/clusterareas in
 * function-lifetime locals: the original re-derives each from its global at
 * every use, which is what MSVC6's aliasing model produces once the accesses
 * go through the plain indexed form Q3 itself uses.
 *
 * DEAD in Gladiator — no .text caller.  Q3's version adds a `type`
 * discriminator and takes explicit start/goal areanums.
 */
int __cdecl AAS_AlternativeRouteGoals(
    vec3_t start, vec3_t goal, int travelflags,
    aas_altroutegoal_t *altroutegoals, int maxaltroutegoals)
{
  int   startareanum;
  int   goalareanum;
  short baseline_travel;
  short travel_to_start;
  short travel_to_goal;
  int   areanum;
  int   ebp_area;
  int   i;
  int   nummidrangeareas;
  int   numaltroutegoals;
  int   best_area;
  vec3_t centroid;
  vec3_t diff;
  float best_dist;
  double threshold;
  float fcount;

  startareanum = AAS_PointAreaNum(start);
  if ( !startareanum )
    return 0;
  goalareanum = AAS_PointAreaNum(goal);
  if ( !goalareanum )
    return 0;

  baseline_travel = AAS_AreaTravelTimeToGoalArea(
      startareanum, goalareanum, travelflags);

  /* Zero candidate flag table: 8 bytes per area. */
  memset((void *)midrangeareas, 0, aasworld.numareas * sizeof(midrangearea_t));
  nummidrangeareas = 0;
  numaltroutegoals = 0;

  /* Phase 1: mark candidate route-portal areas within 1.5x baseline.  No explicit
   * numareas>1 wrapper: the for-loop entry guard already emits the single
   * `cmp numareas,1; jle` the original has (an outer `if` duplicated it). */
  for ( areanum = 1; areanum < aasworld.numareas; areanum++ )
  {
    if ( (aasworld.areasettings[areanum].contents & 0x20)
      && AAS_AreaReachability(areanum) )
    {
      travel_to_start = AAS_AreaTravelTimeToGoalArea(
          startareanum, areanum, travelflags);
      if ( travel_to_start )
      {
        threshold = (float)(unsigned short)baseline_travel * 1.5;
        if ( (float)(unsigned short)travel_to_start <= threshold )
        {
          travel_to_goal = AAS_AreaTravelTimeToGoalArea(
              areanum, goalareanum, travelflags);
          if ( travel_to_goal
            && (float)(unsigned short)travel_to_goal <= threshold )
          {
            midrangeareas[areanum].valid     = 1;
            midrangeareas[areanum].starttime = travel_to_start;
            midrangeareas[areanum].goaltime  = travel_to_goal;
            /* Log_Write("%d midrange area %d", count_pre_inc, areanum) */
            Log_Write("%d midrange area %d", nummidrangeareas, areanum);
            nummidrangeareas++;
          }
        }
      }
    }
  }

  /* Phase 2: flood-fill each remaining candidate cluster and emit
   * one alt-route goal per cluster (centroid → nearest member). */
  for ( ebp_area = 1; ebp_area < aasworld.numareas; ebp_area++ )
  {
    if ( !midrangeareas[ebp_area].valid )
      continue;

    numclusterareas = 0;
    AAS_AltRoutingFloodCluster_r(ebp_area);          /* fills clusterareas[0..N-1] */

    VectorClear(centroid);
    for ( i = 0; i < numclusterareas; i++ )
    {
      centroid[0] += aasworld.areas[clusterareas[i]].center[0];
      centroid[1] += aasworld.areas[clusterareas[i]].center[1];
      centroid[2] += aasworld.areas[clusterareas[i]].center[2];
    }
    fcount = (float)(1.0 / (float)numclusterareas);
    VectorScale(centroid, fcount, centroid);

    best_dist = 999999.0f;            /* 0x497423F0 */
    best_area = 0;
    for ( i = 0; i < numclusterareas; i++ )
    {
      float d;
      VectorSubtract(centroid, aasworld.areas[clusterareas[i]].center, diff);
      d = VectorLength(diff);
      if ( d < best_dist )
      {
        best_dist = d;
        best_area = clusterareas[i];
      }
    }

    VectorCopy(aasworld.areas[best_area].center, altroutegoals[numaltroutegoals].origin);
    altroutegoals[numaltroutegoals].areanum         = best_area;
    altroutegoals[numaltroutegoals].travel_to_start = midrangeareas[best_area].starttime;
    altroutegoals[numaltroutegoals].travel_to_goal  = midrangeareas[best_area].goaltime;
    altroutegoals[numaltroutegoals].extra_travel_time =
        midrangeareas[best_area].starttime
      + midrangeareas[best_area].goaltime
      - baseline_travel;
    numaltroutegoals++;
    if ( numaltroutegoals >= maxaltroutegoals )
      break;
  }

  botimport.Print(PRT_MESSAGE, "%d alternative route goals\n", numaltroutegoals);
  return numaltroutegoals;
}
// gladiator.dll: 1001AB80..1001ABDA
// gladi386.so:   000289B4..00028A33
void sub_1001AB80()
{
  if ( midrangeareas )
    FreeMemory(midrangeareas);
  midrangeareas = GetMemory(sizeof(midrangearea_t) * aasworld.numareas);
  if ( clusterareas )
    FreeMemory(clusterareas);
  clusterareas = GetMemory(sizeof(int) * aasworld.numareas);
}
