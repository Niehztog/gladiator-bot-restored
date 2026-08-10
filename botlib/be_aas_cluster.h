/*
 * be_aas_cluster.h — interface of be_aas_cluster.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AAS_CLUSTER_H
#define BOTLIB_BE_AAS_CLUSTER_H

/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
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
int AAS_RemoveClusterAreas();
int AAS_TestPortals();
int __cdecl AAS_UpdatePortal(int areanum, int clusternum);

#endif /* BOTLIB_BE_AAS_CLUSTER_H */
