/*
 * be_aas_reach.h — interface of be_aas_reach.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AAS_REACH_H
#define BOTLIB_BE_AAS_REACH_H

#include "botlib_local.h"

void *AAS_AllocReachability(void);
int __cdecl AAS_AreaCrouch(int areanum);
float __cdecl AAS_AreaGroundFaceArea(int areanum);
int __cdecl AAS_AreaGrounded(int areanum);
int __cdecl AAS_AreaLadder(int areanum);
int __cdecl AAS_AreaLiquid(int areanum);
int __cdecl AAS_AreaReachability(int areanum);
int __cdecl AAS_AreaSwim(int areanum);
float __cdecl AAS_AreaVolume(int areanum);
int AAS_ContinueInitReachability(int time);
float __cdecl AAS_FaceArea(aas_face_t *face);
void __cdecl AAS_FaceCenter(int facenum, vec3_t center);
int AAS_FallDamageDistance();
void AAS_InitReachability();
float __cdecl AAS_MaxJumpDistance(float phys_jumpvel);
float __cdecl AAS_MaxJumpHeight(float phys_jumpvel);
BOOL __cdecl AAS_NearbySolidOrGap(vec3_t start, vec3_t end);
qboolean __cdecl AAS_ReachabilityExists(int area1num, int area2num);
void AAS_Reachability_Elevator();
int __cdecl AAS_Reachability_EqualFloorHeight(int area1num, int area2num);
int __cdecl AAS_Reachability_Grapple(int area1num, int area2num);
int AAS_Reachability_Jump(int area1num, int area2num);
int AAS_Reachability_Ladder(int area1num, int area2num);
int __cdecl AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge(int area1num, int area2num);
int __cdecl AAS_Reachability_Swim(int area1num, int area2num);
int AAS_Reachability_Teleport();
void __cdecl AAS_Reachability_WalkOffLedge(int areanum);
int __cdecl AAS_Reachability_WeaponJump(int area1num, int area2num);
int AAS_SetWeaponJumpAreaFlags();
int AAS_SetupReachabilityHeap();
void AAS_ShutDownReachabilityHeap();
int AAS_StoreReachability();
int __cdecl VectorBetweenVectors(vec3_t v, vec3_t v1, vec3_t v2);
float __cdecl VectorDistance(vec3_t v1, vec3_t v2);
void __cdecl VectorMiddle(vec3_t v1, vec3_t v2, vec3_t middle);
int __cdecl sub_100116D0(void);

#endif /* BOTLIB_BE_AAS_REACH_H */
