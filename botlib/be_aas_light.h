/* be_aas_light.h — interface of be_aas_light.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AAS_LIGHT_H
#define BOTLIB_BE_AAS_LIGHT_H

int __cdecl AAS_BSPTraceLight(intptr_t start, intptr_t end, intptr_t endpos, int *red, int *green, int *blue);
int __cdecl AAS_PointLight(float *origin, int *red, int *green, int *blue);
int __cdecl BotAddPointLight(vec3_t origin, int ent, float radius, float r, float g, float b, float time, float decay);
bsp_pointlight_t *sub_1000D450();
void __cdecl sub_1000D4A0(bsp_pointlight_t *a1);
void __cdecl sub_1000D4E0(float a1);

#endif /* BOTLIB_BE_AAS_LIGHT_H */
