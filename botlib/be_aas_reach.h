/*
 * be_aas_reach.h — interface of be_aas_reach.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AAS_REACH_H
#define BOTLIB_BE_AAS_REACH_H

/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
int AAS_ContinueInitReachability(int a1); // caller passes arg but function body ignores it (no ebp frame)
void *AAS_AllocReachability(void);  /* sub_10010FF0 — pop AAS-link from free chain */
int __cdecl AAS_AreaSwim(int areanum); /* AAS_AreaSwim impl */
int __cdecl AAS_AreaGrounded(int areanum); /* AAS_AreaGrounded impl */
extern libvar_t *libvar_framereachability;
extern libvar_t *libvar_reachabilitydelay;
extern aas_reachabilitynode_t **areareachability;
extern int reach_ladder;
extern int reach_elevator;
extern intptr_t reachabilityheap;
extern int reach_jump;
extern int reach_grapple;
extern int reach_waterjump;
extern int reach_teleport;
extern int reach_barrier;
extern int reach_swim;
extern int reach_equalfloor;
extern intptr_t nextreachability;
extern int reach_walkoffledge;
extern int reach_rocketjump;
extern int reach_step;
extern int reach_walk;



/* Declared but never defined -- a dead declaration from the decompilation. */

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
