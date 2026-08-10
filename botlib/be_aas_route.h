/*
 * be_aas_route.h — interface of be_aas_route.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AAS_ROUTE_H
#define BOTLIB_BE_AAS_ROUTE_H

/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
void AAS_InitTravelFlagFromType(void); /* sub_10018D00 */
int AAS_FreeRoutingCaches(void);  /* sub_10019550 */
int AAS_FreeAllPortalCache(void); /* sub_100193E0 */
extern int numportalcacheupdates;
extern int numareacacheupdates;

aas_routingcache_t *__cdecl AAS_AllocRoutingCache(int numtraveltimes);
unsigned short __cdecl AAS_AreaTravelTime(int areanum, float *start, float *end);
__int16 __cdecl AAS_AreaTravelTimeToGoalArea(int areanum, int a2, int goalareanum);
void AAS_CalculateAreaTravelTimes(void);
void AAS_CreateReversedReachability(void);
void AAS_FreeAllClusterAreaCache(void);
int AAS_FreeAllPortalCache(void);
void __cdecl AAS_FreeRoutingCache(void *cache);
int AAS_FreeRoutingCaches(void);
aas_routingcache_t *__cdecl AAS_GetAreaRoutingCache(int clusternum, int areanum, int travelflags);
aas_routingcache_t *__cdecl AAS_GetPortalRoutingCache(int clusternum, int areanum, int travelflags);
void AAS_InitClusterAreaCache();
int AAS_InitPortalCache();
void AAS_InitRouting(void);
int AAS_InitRoutingUpdate();
void AAS_InitTravelFlagFromType(void);
int __cdecl AAS_NextAreaReachability(int areanum, int reachnum);
int __cdecl AAS_RandomGoalArea(int areanum, int travelflags, _DWORD *goalareanum, vec3_t goalorigin);
aas_reachability_t __cdecl AAS_ReachabilityFromNum(int num);
int AAS_RoutingInfo();
int __cdecl AAS_TravelFlagForType(int traveltype);
/* Area contents bits and the travel flags they map to.  Both sets are read
 * out of AAS_GetAreaContentsTravelFlags' own disassembly (gladi386.so F526);
 * Q3 botlib has the same function with the same shape but its own values. */
#define AREACONTENTS_WATER   0x0001
#define AREACONTENTS_SLIME   0x0002
#define AREACONTENTS_LAVA    0x0004

#define TFL_AIR              0x8000
#define TFL_WATER           0x10000
#define TFL_LAVA            0x20000
#define TFL_SLIME           0x40000

#ifndef _WIN32
float __cdecl AAS_RoutingTime(void);
void __cdecl F525(aas_routingupdate_t **updateliststart, aas_routingupdate_t **updatelistend, aas_routingupdate_t *update);
int __cdecl AAS_GetAreaContentsTravelFlags(int areanum);
void __cdecl F524(void);
int __cdecl AAS_ClusterAreaNum(int cluster, int areanum);
#endif
void __cdecl AAS_UpdateAreaRoutingCache(aas_routingcache_t *areacache);
void __cdecl AAS_UpdatePortalRoutingCache(aas_routingcache_t *portalcache);
static void sub_10019570(void);

#endif /* BOTLIB_BE_AAS_ROUTE_H */
