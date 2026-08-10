/*
 * be_aas_cluster.h — interface of be_aas_cluster.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AAS_CLUSTER_H
#define BOTLIB_BE_AAS_CLUSTER_H

#include "botlib_local.h"

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
