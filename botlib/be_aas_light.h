/*
 * be_aas_light.h — interface of be_aas_light.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AAS_LIGHT_H
#define BOTLIB_BE_AAS_LIGHT_H

int __cdecl AAS_BSPTraceLight(intptr_t start, intptr_t end, intptr_t endpos, int *red, int *green, int *blue);
int __cdecl AAS_PointLight(float *origin, int *red, int *green, int *blue);
int __cdecl BotAddPointLight(vec3_t origin, int ent, float radius, float r, float g, float b, float time, float decay);
bsp_pointlight_t *sub_1000D450();
void __cdecl sub_1000D4A0(bsp_pointlight_t *a1);
void __cdecl sub_1000D4E0(float a1);

#endif /* BOTLIB_BE_AAS_LIGHT_H */
