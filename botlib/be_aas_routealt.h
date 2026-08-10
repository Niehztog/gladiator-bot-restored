/*
 * be_aas_routealt.h — interface of be_aas_routealt.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AAS_ROUTEALT_H
#define BOTLIB_BE_AAS_ROUTEALT_H

#include "botlib_local.h"

/* Output entry for AAS_AlternativeRouteGoals (24 bytes).  Declared ahead of
 * the docblock so the ref-funcmap generator attributes 0x1001A720 to the
 * function rather than this type. */
typedef struct aas_altroutegoal_s {
  vec3_t          origin;
  int             areanum;
  unsigned short  travel_to_start;
  unsigned short  travel_to_goal;
  unsigned short  extra_travel_time;
  unsigned short  pad;
} aas_altroutegoal_t;

int __cdecl AAS_AltRoutingFloodCluster_r(int areanum);
int __cdecl AAS_AlternativeRouteGoals(
    vec3_t start, vec3_t goal, int travelflags,
    aas_altroutegoal_t *altroutegoals, int maxaltroutegoals);
int sub_1001AB80();

#endif /* BOTLIB_BE_AAS_ROUTEALT_H */
