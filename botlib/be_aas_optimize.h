/*
 * be_aas_optimize.h — interface of be_aas_optimize.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AAS_OPTIMIZE_H
#define BOTLIB_BE_AAS_OPTIMIZE_H

int AAS_KeepEdge(aas_edge_t *edge);
int __cdecl AAS_KeepFace(aas_face_t *face);
void AAS_Optimize(void);
int __cdecl AAS_OptimizeAlloc(optimized_t *optimized);
int __cdecl AAS_OptimizeArea(optimized_t *optimized, int areanum);
int __cdecl AAS_OptimizeEdge(optimized_t *optimized, int edgenum);
int __cdecl AAS_OptimizeFace(optimized_t *optimized, int facenum);
int __cdecl AAS_OptimizeStore(optimized_t *optimized);

#endif /* BOTLIB_BE_AAS_OPTIMIZE_H */
