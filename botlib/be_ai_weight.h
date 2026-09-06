/* be_ai_weight.h — interface of be_ai_weight.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AI_WEIGHT_H
#define BOTLIB_BE_AI_WEIGHT_H

/* Declarations for what this TU defines — last, so the types above are in scope. */

void __cdecl EvolveFuzzySeperator_r(fuzzyseperator_t *fs);
void __cdecl EvolveWeightConfig(int *config);
int __cdecl FindFuzzyWeight(weightconfig_t *wc, const char *name);
void __cdecl FreeFuzzySeperators_r(fuzzyseperator_t *fs);
void              __cdecl FreeWeightConfig2(weightconfig_t *config);
float __cdecl FuzzyWeight(int *facts, weight_t *w);
float __cdecl FuzzyWeightUndecided(int *facts, weight_t *w);
float __cdecl FuzzyWeightUndecided_r(int *inventory, fuzzyseperator_t *fs);
float __cdecl FuzzyWeight_r(int *inventory, fuzzyseperator_t *fs);
void __cdecl InterbreedFuzzySeperator_r(fuzzyseperator_t *fs1, fuzzyseperator_t *fs2);
void __cdecl ScaleWeight(weightconfig_t *config, char *name, float scale);
void __cdecl InterbreedWeightConfigs(weightconfig_t *a, weightconfig_t *b);
fuzzyseperator_t *__cdecl ReadFuzzySeperators_r(source_t *source);
int __cdecl ReadFuzzyWeight(source_t *source, fuzzyseperator_t *fs);
int __cdecl ReadValue(source_t *source, float *value);
weightconfig_t   *__cdecl ReadWeightConfig(char *filename);
void __cdecl ScaleFuzzySeperator_r(fuzzyseperator_t *fs, float scale);
qboolean __cdecl WriteFuzzySeperators_r(FILE *fp, fuzzyseperator_t *fs, int indent);
qboolean __cdecl WriteFuzzyWeight(FILE *fp, fuzzyseperator_t * fs);
qboolean __cdecl WriteWeightConfig(const char *filename, weightconfig_t *config);

#endif /* BOTLIB_BE_AI_WEIGHT_H */
