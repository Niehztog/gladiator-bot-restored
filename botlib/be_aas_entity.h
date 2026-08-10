/*
 * be_aas_entity.h — interface of be_aas_entity.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AAS_ENTITY_H
#define BOTLIB_BE_AAS_ENTITY_H

#include "botlib_local.h"

int __cdecl AAS_BestReachableArea(int * origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin);
int __cdecl AAS_BestReachableEntityArea(int entnum);
int __cdecl AAS_BestReachableLinkArea(aas_link_t *areas);
int __cdecl AAS_DropToFloor(vec3_t origin, vec3_t mins, vec3_t maxs);
int __cdecl AAS_EntityBSPData(int entnum, bsp_entdata_t *entdata);
aas_entityinfo_t __cdecl AAS_EntityInfo(int entnum);
int __cdecl AAS_EntityModelNum(int entnum);
int __cdecl AAS_EntityModelindex(int entnum);
void __cdecl AAS_EntityOrigin(int entnum, vec3_t origin);
int __cdecl AAS_EntityRenderFX(int entnum);
void __cdecl AAS_EntitySize(int entnum, vec3_t mins, vec3_t maxs);
void AAS_InvalidateEntities();
int __cdecl AAS_NextBSPEntity(int ent);
int __cdecl AAS_OriginOfMoverWithModelNum(int modelnum, vec3_t origin);
void AAS_ResetEntityLinks();
int __cdecl AAS_UpdateEntity(int entnum, bot_updateentity_t *state);
int __cdecl BotEntityVisible(int, float *, float *, float, int);
int InFieldOfVision(float *, float, float *);
int __cdecl sub_1000B1F0(float *ref, int target);
int __cdecl sub_1000BAA0(int, float *, float *, float, int, int *);

#endif /* BOTLIB_BE_AAS_ENTITY_H */
