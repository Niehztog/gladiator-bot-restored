/*
 * be_aas_optimize.h — interface of be_aas_optimize.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AAS_OPTIMIZE_H
#define BOTLIB_BE_AAS_OPTIMIZE_H

#include "botlib_local.h"

int AAS_KeepEdge(aas_edge_t *edge);
int __cdecl AAS_KeepFace(aas_face_t *face);
void AAS_Optimize(void);
int __cdecl AAS_OptimizeAlloc(optimized_t *optimized);
int __cdecl AAS_OptimizeArea(optimized_t *optimized, int areanum);
int __cdecl AAS_OptimizeEdge(optimized_t *optimized, int edgenum);
int __cdecl AAS_OptimizeFace(optimized_t *optimized, int facenum);
int __cdecl AAS_OptimizeStore(optimized_t *optimized);

#endif /* BOTLIB_BE_AAS_OPTIMIZE_H */
