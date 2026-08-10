/*
 * be_ai_weight.h — interface of be_ai_weight.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AI_WEIGHT_H
#define BOTLIB_BE_AI_WEIGHT_H

#include "botlib_local.h"

void __cdecl FreeFuzzySeperators_r(fuzzyseperator_t *fs);
void              __cdecl FreeWeightConfig2(weightconfig_t *config);
fuzzyseperator_t *__cdecl ReadFuzzySeperators_r(source_t *source);
int __cdecl ReadFuzzyWeight(source_t *source, fuzzyseperator_t *fs);
int __cdecl ReadValue(source_t *source, float *value);
weightconfig_t   *__cdecl ReadWeightConfig(char *filename);
qboolean __cdecl WriteFuzzySeperators_r(FILE *fp, int, int);
qboolean __cdecl WriteFuzzyWeight(FILE *fp, fuzzyseperator_t * fs);

#endif /* BOTLIB_BE_AI_WEIGHT_H */
