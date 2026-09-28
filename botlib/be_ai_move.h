/* be_ai_move.h — interface of be_ai_move.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AI_MOVE_H
#define BOTLIB_BE_AI_MOVE_H

/* Q3's movement types, verbatim from its be_ai_move.h: the `type` of
 * BotMoveInDirection.  BotWalkInDirection fixes the values -- 4 makes it EA_Jump,
 * 2 EA_Crouch, and only 2 without 4 gives the crouch presence -- and BotAttackMove
 * picks among 1, 4 and 2.  MOVE_GRAPPLE, MOVE_ROCKETJUMP and MOVE_BFGJUMP have no
 * user here, as in Q3. */
//movement types
#define MOVE_WALK						1
#define MOVE_CROUCH						2
#define MOVE_JUMP						4
#define MOVE_GRAPPLE					8
#define MOVE_ROCKETJUMP					16
#define MOVE_BFGJUMP					32

/* Q3's move flags and move-result flags, verbatim from the same header except for
 * Gladiator's values.  The move flags are Q3's up to MFL_TELEPORTED: BotEntityInfo
 * derives 2, 16 and 32 from PMF_ON_GROUND and the water-jump and teleport timers, and
 * BotMoveToGoal sets 2, 4 and 8 from AAS_OnGround, AAS_Swimming and AAS_AgainstLadder.
 * The grapple pair sits one bit lower than in Q3, which later put MFL_GRAPPLEPULL at
 * 64: BotTravel_Grapple sets 0x40 together with lastgrappledist = 999999, Q3's
 * MFL_ACTIVEGRAPPLE, and tests 0x80 on entry, Q3's MFL_GRAPPLERESET.  Q3's
 * MFL_GRAPPLEPULL and MFL_WALK are set from its game module, which has no counterpart
 * here.  The move-result flags are Q3's lowest four.  BotTravel_RocketJump sets 8
 * after aiming with EA_View, and the AI nodes then leave the view alone: four skip
 * BotChangeViewAngles, one skips its aiming.  MOVERESULT_MOVEMENTWEAPON and above are
 * left out: Gladiator's bot_moveresult_t has no weapon field, and the rest belong to
 * Q3's func_bob, elevator-top and avoid-spot code. */
//move flags
#define MFL_BARRIERJUMP					1		//bot is performing a barrier jump
#define MFL_ONGROUND					2		//bot is in the ground
#define MFL_SWIMMING					4		//bot is swimming
#define MFL_AGAINSTLADDER				8		//bot is against a ladder
#define MFL_WATERJUMP					16		//bot is waterjumping
#define MFL_TELEPORTED					32		//bot is being teleported
#define MFL_ACTIVEGRAPPLE				64		//bot is using the grapple hook
#define MFL_GRAPPLERESET				128		//bot has reset the grapple
// move result flags
#define MOVERESULT_MOVEMENTVIEW			1		//bot uses view for movement
#define MOVERESULT_SWIMVIEW				2		//bot uses view for swimming
#define MOVERESULT_WAITING				4		//bot is waiting for something
#define MOVERESULT_MOVEMENTVIEWSET		8		//bot has set the view in movement code

/* Declarations for what this TU defines — last, so the types above are in scope. */
bot_moveresult_t __cdecl BotMoveToGoal(bot_movestate_t *movestate, bot_goal_t *goal, int travelflags); /* 0x100343A0: build bot_moveresult_t for current goal */

float __cdecl AngleDiff(float ang1, float ang2);
void __cdecl BotAddToAvoidReach(bot_movestate_t *ms, int number, float avoidtime);
int __cdecl BotCheckBarrierJump(bot_movestate_t *ms, vec3_t dir, float speed);
int __cdecl BotCheckBlocked(bot_movestate_t *ms, float *dir, bot_moveresult_t *moveresult);
void __cdecl BotClearMoveResult(bot_moveresult_t *moveresult);
bot_moveresult_t __cdecl BotFinishTravel_BarrierJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_Elevator(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_Jump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_Walk(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_WalkOffLedge(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_WaterJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_WeaponJump(bot_movestate_t *ms, aas_reachability_t *reach);
float __cdecl BotGapDistance(bot_movestate_t *ms, float *dir);
int __cdecl BotGetReachabilityToGoal(float *origin, int areanum, int entnum, int lastgoalareanum, int lastareanum, int *avoidreach, float *avoidreachtimes, int *avoidreachtries, bot_goal_t *goal, int travelflags);
int __cdecl BotMoveInDirection(bot_movestate_t *movestate, vec3_t dir, float speed, int type);
bot_moveresult_t __cdecl BotMoveInGoalArea(bot_movestate_t *ms, bot_goal_t *goal);
bot_moveresult_t __cdecl BotMoveToGoal(bot_movestate_t *movestate, bot_goal_t *goal, int travelflags);
int __cdecl BotMovementViewTarget(bot_movestate_t *ms, bot_goal_t *goal, int travelflags, float *target);
BOOL __cdecl BotOnMover(vec3_t origin, int entnum, aas_reachability_t* reach);
int __cdecl BotReachabilityArea(int *origin, int client);
int __cdecl BotReachabilityTime(aas_reachability_t* reach);
void __cdecl BotResetAvoidReach(_DWORD *movestate);
void __cdecl BotResetGrapple(bot_movestate_t *ms);
void __cdecl BotResetLastAvoidReach(bot_movestate_t *ms);
void __cdecl BotResetMoveState(void *movestate);
int __cdecl BotSwimInDirection(bot_movestate_t *ms, vec3_t dir, float speed, int type);
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
int __cdecl BotWalkInDirection(bot_movestate_t *ms, vec3_t dir, float speed, int type);
int __cdecl GrappleState(bot_movestate_t *ms, aas_reachability_t *reach);
int __cdecl Intersection(float *p1, float *p2, float *p3, float *p4, float *out);
void __cdecl MoverBottomCenter(aas_reachability_t *reach, vec3_t bottomcenter);
BOOL __cdecl MoverDown(aas_reachability_t* reach);
void *__cdecl sub_10034070(void *out);

#endif /* BOTLIB_BE_AI_MOVE_H */
