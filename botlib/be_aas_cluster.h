/* be_aas_cluster.h — interface of be_aas_cluster.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AAS_CLUSTER_H
#define BOTLIB_BE_AAS_CLUSTER_H

/* Declarations for what this TU defines — last, so the types above are in scope. */
void AAS_InitClustering(void);                     /* be_aas_cluster.c 0x100096E0 */

int __cdecl AAS_CheckAreaForPossiblePortals(int areanum);
qboolean __cdecl AAS_ConnectedAreas(_DWORD *areanums, int numareas);
void __cdecl AAS_ConnectedAreas_r(int *areanums, int numareas, int *connectedareas, int curarea);
void AAS_CreatePortals();
int AAS_FindClusters();
int AAS_FindPossiblePortals();
int __cdecl AAS_FloodAreas_r(int *areanum, int cluster, int done);
int __cdecl AAS_FloodClusterAreas_r(int areanum, int clusternum);
int __cdecl AAS_FloodClusterReachabilities(int clusternum);
void AAS_InitClustering();
void __cdecl AAS_NumberClusterPortals(int clusternum);
void AAS_RemoveAllPortals();
void AAS_RemoveClusterAreas(void);
int AAS_TestPortals();
int __cdecl AAS_UpdatePortal(int areanum, int clusternum);

#endif /* BOTLIB_BE_AAS_CLUSTER_H */
