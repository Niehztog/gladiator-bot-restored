/*
 * be_aas_move.h — interface of be_aas_move.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AAS_MOVE_H
#define BOTLIB_BE_AAS_MOVE_H

int __cdecl AAS_AgainstLadder(vec3_t origin);
void __cdecl AAS_ApplyFriction(vec3_t vel, float friction, float stopspeed, float frametime);
double __cdecl AAS_BFGJumpZVelocity(vec3_t origin);
aas_clientmove_t __cdecl AAS_ClientMovementPrediction(int entnum, float * origin, int presencetype, int onground, float * velocity, float * cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int visualize);
int __cdecl AAS_HorizontalVelocityForJump(float zvel, vec3_t start, vec3_t end, float * velocity);
void __cdecl AAS_JumpReachRunStart(aas_reachability_t* reach, intptr_t runstart);
BOOL __cdecl AAS_OnGround(vec3_t origin, int presencetype, int passent);
float __cdecl AAS_RocketJumpZVelocity(vec3_t origin);
BOOL __cdecl AAS_Swimming(vec3_t origin);
void AAS_TestMovementPrediction(int entnum, vec3_t origin, vec3_t dir);
double __cdecl AAS_WeaponJumpZVelocity(vec3_t origin, float radiusdamage);
int __cdecl sub_1000F130(vec3_t origin);

#endif /* BOTLIB_BE_AAS_MOVE_H */
