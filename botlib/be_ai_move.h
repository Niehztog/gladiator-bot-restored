/*
 * be_ai_move.h — interface of be_ai_move.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AI_MOVE_H
#define BOTLIB_BE_AI_MOVE_H

/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
bot_moveresult_t *__cdecl BotMoveToGoal(bot_moveresult_t *a1, bot_movestate_t *movestate, bot_goal_t *goal, int travelflags); /* 0x100343A0: build bot_moveresult_t for current goal */
extern int dword_1006295C;
extern libvar_t *libvar_laserhook;

double __cdecl AngleDiff(float ang1, float ang2);
void __cdecl BotAddToAvoidReach(bot_movestate_t *ms, int number, float avoidtime);
int __cdecl BotCheckBarrierJump(bot_movestate_t *ms, float *dir, float speed);
int __cdecl BotCheckBlocked(bot_movestate_t *ms, float *dir, bot_moveresult_t *moveresult);
bot_moveresult_t *__cdecl BotClearMoveResult(bot_moveresult_t *moveresult);
bot_moveresult_t __cdecl BotFinishTravel_BarrierJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_Elevator(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_Jump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_Walk(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_WalkOffLedge(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_WaterJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_WeaponJump(bot_movestate_t *ms, aas_reachability_t *reach);
float __cdecl BotGapDistance(bot_movestate_t *ms, float *dir);
int __cdecl BotGetReachabilityToGoal(float *origin, int areanum, int entnum, int lastgoalareanum, int lastareanum, int *avoidreach, float *avoidreachtimes, int *avoidreachtries, bot_goal_t *goal, int travelflags);
int __cdecl BotMoveInDirection(bot_movestate_t *movestate, float *dir, float speed, int type);
bot_moveresult_t __cdecl BotMoveInGoalArea(bot_movestate_t *ms, bot_goal_t *goal);
bot_moveresult_t *__cdecl BotMoveToGoal(bot_moveresult_t *a1, bot_movestate_t *movestate, bot_goal_t *goal, int travelflags);
int __cdecl BotMovementViewTarget(bot_movestate_t *ms, bot_goal_t *goal, int travelflags, float *target);
BOOL __cdecl BotOnMover(float *origin, int entnum, aas_reachability_t* reach);
int __cdecl BotReachabilityArea(int *origin, int client);
int __cdecl BotReachabilityTime(aas_reachability_t* reach);
void __cdecl BotResetAvoidReach(_DWORD *movestate);
void __cdecl BotResetGrapple(bot_movestate_t *ms);
void __cdecl BotResetLastAvoidReach(intptr_t movestate);
void __cdecl BotResetMoveState(void *movestate);
int __cdecl BotSwimInDirection(bot_movestate_t *ms, float *dir, float speed, int type);
bot_moveresult_t __cdecl BotTravel_BarrierJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Crouch(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Elevator(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Grapple(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Jump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Ladder(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_RocketJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Swim(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Teleport(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Walk(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_WalkOffLedge(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_WaterJump(bot_movestate_t *ms, aas_reachability_t *reach);
BOOL __cdecl BotValidTravel(float *a1, int a2, aas_reachability_t *a3, int a4);
int __cdecl BotWalkInDirection(bot_movestate_t *ms, float *dir, float speed, int type);
int __cdecl GrappleState(bot_movestate_t *ms, aas_reachability_t *reach);
int __cdecl Intersection(float *p1, float *p2, float *p3, float *p4, float *out);
void __cdecl MoverBottomCenter(aas_reachability_t *reach, vec3_t bottomcenter);
BOOL __cdecl MoverDown(aas_reachability_t* reach);
void *__cdecl sub_10034070(void *out);

#endif /* BOTLIB_BE_AI_MOVE_H */
