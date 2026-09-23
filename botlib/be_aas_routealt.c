/*
 * be_aas_routealt.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x1001A650..0x1001AB80.
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
/* Find clusters of "midrange" route-portal areas lying within a 1.5x detour budget of
 * the direct start->goal route, and emit one alternative-route goal per cluster.
 *
 *   1) Resolve start/goal to areas, and take the baseline travel time between them.
 *   2) Mark each area whose contents carry AREACONTENTS_ROUTEPORTAL (0x20) and which has
 *      reachabilities, if travel(start->a) and travel(a->goal) are both within 1.5x the
 *      baseline, recording both times.
 *   3) Flood-fill each marked area's connected cluster via face neighbours, take the
 *      cluster centroid, pick the member area whose center is closest to it, and emit one
 *      aas_altroutegoal_t for it — origin, areanum, the two travel times, and
 *      (start+goal) - baseline as the extra travel.  Stop at maxaltroutegoals.
 *
 * Faithful quirk: the per-iteration Log_Write prints the match counter BEFORE it is
 * incremented, so the printed number lags by one.
 *
 * Do NOT cache aasworld.areasettings/areas or midrangeareas/clusterareas in
 * function-lifetime locals: the original re-derives each from its global at every use,
 * which is what MSVC6's aliasing model produces once the accesses go through the plain
 * indexed form Q3 itself uses.
 *
 * DEAD in Gladiator.  Q3's version adds a `type` discriminator and takes explicit
 * start/goal areanums.
 */
int __cdecl AAS_AlternativeRouteGoals(
    vec3_t start, vec3_t goal, int travelflags,
    aas_altroutegoal_t *altroutegoals, int maxaltroutegoals)
{
  int i, j, startareanum, goalareanum, bestareanum;
  int numaltroutegoals, nummidrangeareas;
  unsigned short starttime, goaltime, goaltraveltime;
  float dist, bestdist;
  vec3_t mid, dir;

  startareanum = AAS_PointAreaNum(start);
  if ( !startareanum )
    return 0;
  goalareanum = AAS_PointAreaNum(goal);
  if ( !goalareanum )
    return 0;
  //travel time towards the goal area
  goaltraveltime = AAS_AreaTravelTimeToGoalArea(startareanum, goalareanum, travelflags);
  //clear the midrange areas
  memset(midrangeareas, 0, aasworld.numareas * sizeof(midrangearea_t));
  numaltroutegoals = 0;
  //
  nummidrangeareas = 0;
  //
  for ( i = 1; i < aasworld.numareas; i++ )
  {
    //
    if ( !(aasworld.areasettings[i].contents & 0x20) )
      continue;
    //if the area has no reachabilities
    if ( !AAS_AreaReachability(i) )
      continue;
    //tavel time from the area to the start area
    starttime = AAS_AreaTravelTimeToGoalArea(startareanum, i, travelflags);
    if ( !starttime )
      continue;
    //if the travel time from the start to the area is greater than the shortest goal travel time
    if ( starttime > 1.5 * goaltraveltime )
      continue;
    //travel time from the area to the goal area
    goaltime = AAS_AreaTravelTimeToGoalArea(i, goalareanum, travelflags);
    if ( !goaltime )
      continue;
    //if the travel time from the area to the goal is greater than the shortest goal travel time
    if ( goaltime > 1.5 * goaltraveltime )
      continue;
    //this is a mid range area
    midrangeareas[i].valid = 1;
    midrangeareas[i].starttime = starttime;
    midrangeareas[i].goaltime = goaltime;
    Log_Write("%d midrange area %d", nummidrangeareas, i);
    nummidrangeareas++;
  }
  //
  for ( i = 1; i < aasworld.numareas; i++ )
  {
    if ( !midrangeareas[i].valid )
      continue;
    //get the areas in one cluster
    numclusterareas = 0;
    AAS_AltRoutingFloodCluster_r(i);
    //now we've got a cluster with areas through which an alternative route could go
    //get the 'center' of the cluster
    VectorClear(mid);
    for ( j = 0; j < numclusterareas; j++ )
    {
      VectorAdd(mid, aasworld.areas[clusterareas[j]].center, mid);
    }
    VectorScale(mid, 1.0 / numclusterareas, mid);
    //get the area closest to the center of the cluster
    bestdist = 999999;
    bestareanum = 0;
    for ( j = 0; j < numclusterareas; j++ )
    {
      VectorSubtract(mid, aasworld.areas[clusterareas[j]].center, dir);
      dist = VectorLength(dir);
      if ( dist < bestdist )
      {
        bestdist = dist;
        bestareanum = clusterareas[j];
      }
    }
    //now we've got an area for an alternative route
    VectorCopy(aasworld.areas[bestareanum].center, altroutegoals[numaltroutegoals].origin);
    altroutegoals[numaltroutegoals].areanum = bestareanum;
    altroutegoals[numaltroutegoals].travel_to_start = midrangeareas[bestareanum].starttime;
    altroutegoals[numaltroutegoals].travel_to_goal = midrangeareas[bestareanum].goaltime;
    altroutegoals[numaltroutegoals].extra_travel_time =
          (midrangeareas[bestareanum].starttime + midrangeareas[bestareanum].goaltime) -
                goaltraveltime;
    numaltroutegoals++;
    //don't return more than the maximum alternative route goals
    if ( numaltroutegoals >= maxaltroutegoals )
      break;
  }
  botimport.Print(PRT_MESSAGE, "%d alternative route goals\n", numaltroutegoals);
  return numaltroutegoals;
}

// gladiator.dll: 1001AB80..1001ABDA
// gladi386.so:   000289B4..00028A33
/* Q3 be_aas_routealt.c's AAS_InitAlternativeRouting (its ENABLE_ALTROUTING body),
 * called last in the map-load path exactly where Q3's AAS_LoadMap calls it. */
void AAS_InitAlternativeRouting()
{
  if ( midrangeareas )
    FreeMemory(midrangeareas);
  midrangeareas = GetMemory(sizeof(midrangearea_t) * aasworld.numareas);
  if ( clusterareas )
    FreeMemory(clusterareas);
  clusterareas = GetMemory(sizeof(int) * aasworld.numareas);
}
