/*
 * be_ai_weight.h — interface of be_ai_weight.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AI_WEIGHT_H
#define BOTLIB_BE_AI_WEIGHT_H

/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */

void __cdecl EvolveFuzzySeperator_r(fuzzyseperator_t *fs);
void __cdecl EvolveWeightConfig(int *config);
int __cdecl FindFuzzyWeight(weightconfig_t *wc, const char *name);
void __cdecl FreeFuzzySeperators_r(fuzzyseperator_t *fs);
void              __cdecl FreeWeightConfig2(weightconfig_t *config);
double __cdecl FuzzyWeight(int *facts, weight_t *w);
double __cdecl FuzzyWeightUndecided(int *facts, weight_t *w);
double __cdecl FuzzyWeightUndecided_r(int *inventory, fuzzyseperator_t *fs);
double __cdecl FuzzyWeight_r(int *inventory, fuzzyseperator_t *fs);
int __cdecl InterbreedFuzzySeperator_r(fuzzyseperator_t *fs1, fuzzyseperator_t *fs2);
void __cdecl ScaleWeight(weightconfig_t *config, char *name, float scale);
void __cdecl InterbreedWeightConfigs(weightconfig_t *a, weightconfig_t *b);
fuzzyseperator_t *__cdecl ReadFuzzySeperators_r(source_t *source);
int __cdecl ReadFuzzyWeight(source_t *source, fuzzyseperator_t *fs);
int __cdecl ReadValue(source_t *source, float *value);
weightconfig_t   *__cdecl ReadWeightConfig(char *filename);
void __cdecl ScaleFuzzySeperator_r(fuzzyseperator_t *fs, float scale);
qboolean __cdecl WriteFuzzySeperators_r(FILE *fp, int, int);
qboolean __cdecl WriteFuzzyWeight(FILE *fp, fuzzyseperator_t * fs);
int __cdecl WriteWeightConfig(const char *filename, weightconfig_t *config);

#endif /* BOTLIB_BE_AI_WEIGHT_H */
