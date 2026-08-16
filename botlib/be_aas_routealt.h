/* be_aas_routealt.h — interface of be_aas_routealt.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AAS_ROUTEALT_H
#define BOTLIB_BE_AAS_ROUTEALT_H

/* midrangearea_t: this TU's scratch record. */
typedef struct midrangearea_s {
  int            valid;
  unsigned short starttime;
  unsigned short goaltime;
} midrangearea_t;

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



/* Declarations for what this TU defines — last, so the types above are in scope. */
/* be_aas_routealt.c globals (AAS_AlternativeRouteGoals /
 * AAS_AltRoutingFloodCluster_r).  midrangeareas (8 B/area) and clusterareas
 * (int* area-index list) are heap pointers held in 4-byte .data dwords; typed
 * pointers here, indexed as arrays, so no side-band is needed. */
extern int numclusterareas;
extern midrangearea_t *midrangeareas;
extern int *clusterareas;

int __cdecl AAS_AltRoutingFloodCluster_r(int areanum);
int __cdecl AAS_AlternativeRouteGoals(
    vec3_t start, vec3_t goal, int travelflags,
    aas_altroutegoal_t *altroutegoals, int maxaltroutegoals);
void sub_1001AB80();

#endif /* BOTLIB_BE_AAS_ROUTEALT_H */
