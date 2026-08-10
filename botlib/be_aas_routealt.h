/*
 * be_aas_routealt.h — interface of be_aas_routealt.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AAS_ROUTEALT_H
#define BOTLIB_BE_AAS_ROUTEALT_H

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
