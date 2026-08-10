/*
 * be_ai2_dmq2.h — interface of be_ai2_dmq2.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AI2_DMQ2_H
#define BOTLIB_BE_AI2_DMQ2_H

#include "botlib_local.h"

void __cdecl BotAIBlocked(bot_state_t *bs, bot_moveresult_t *moveresult, int activate);
int __cdecl BotAddressedToBot(bot_state_t *bs, bot_match_t *match);
float __cdecl BotAggression(bot_state_t *bs);
void BotAimAtEnemy(bot_state_t *bs);
bot_moveresult_t __cdecl BotAttackMove(bot_state_t *bs, int a3);
void __cdecl BotBattleUseItems(bot_state_t *bs);
int __cdecl BotCTFCarryingFlag(bot_state_t *bs);
void __cdecl BotCTFRetreatGoals(bot_state_t *bs);
void __cdecl BotCTFSeekGoals(bot_state_t *bs);
int __cdecl BotCTFTeam(bot_state_t *bs);
BOOL BotCanAndWantsToRocketJump(bot_state_t *bs);
double __cdecl BotChatTime(bot_state_t *bs);
int __cdecl BotChat_Death(int *bs);
int __cdecl BotChat_EndLevel(bot_state_t *bs);
BOOL __cdecl BotChat_EnterGame(bot_state_t *bs);
int __cdecl BotChat_ExitGame(bot_state_t *bs);
BOOL __cdecl BotChat_Kill(int *bs);
int __cdecl BotChat_Random(bot_state_t *bs);
int __cdecl BotChat_StartLevel(bot_state_t *bs);
void BotCheckAttack(bot_state_t *bs);
void __cdecl BotCheckConsoleMessages(bot_state_t *bs);
bot_waypoint_t *__cdecl BotCreateWayPoint(const char *name, vec3_t origin, int areanum);
int BotDeathmatchAI(bot_state_t *bs, float a2);
_DWORD *__cdecl BotEntityInfo(bot_state_t *bs, _DWORD *info);
int *__cdecl BotEntityToActivate(int a1);
int __cdecl BotFindEnemy(bot_state_t *bs);
bot_waypoint_t *__cdecl BotFindWayPoint(bot_waypoint_t *waypoints, char *name);
void            __cdecl BotFreeWaypoints(bot_waypoint_t *wp);
int __cdecl BotGPSToPosition(char *buf, float *position);
BOOL __cdecl BotGetItemTeamGoal(char *goalname, bot_goal_t *goal);
int __cdecl BotGetMessageTeamGoal(bot_state_t *bs, char *goalname, bot_goal_t *goal);
int __cdecl BotGetPatrolWaypoints(bot_state_t *bs, bot_match_t *match);
float __cdecl BotGetTime(bot_match_t *match);
BOOL __cdecl BotIntermission(bot_state_t *bs);
BOOL __cdecl BotIsDead(bot_state_t *bs);
BOOL __cdecl BotIsObserver(bot_state_t *bs);
int __cdecl BotMatchMessage(bot_state_t *bs, char *message);
int __cdecl BotNumTeamMates(bot_state_t *bs);
float *__cdecl BotRoamGoal(bot_state_t *bs, float *goal);
BOOL __cdecl BotSameTeam(bot_state_t *bs, int entnum);
int __cdecl BotSetMovedir(float *angles, float *movedir);
void BotSetupDeathmatchAI();
int __cdecl BotUpdateBattleInventory(bot_state_t *bs, int enemy);
void __cdecl BotUpdateInventory(bot_state_t *bs);
BOOL __cdecl BotValidChatPosition(bot_state_t *bs);
BOOL __cdecl BotWantsToChase(int *bs);
int __cdecl BotWantsToHelp(bot_state_t *bs);
BOOL __cdecl BotWantsToRetreat(int *bs);
char *__cdecl EasyClientName(int client, char *buf);
BOOL __cdecl EntityIsShooting(intptr_t a1);
int __cdecl FindClientByName(char *name);
BOOL TeamPlayIsOn();
char *__cdecl stristr(char *str, char *charset);
char *__cdecl sub_10020FE0(bot_state_t *bs, bot_weaponstate_t *ws);
int __cdecl sub_100214E0(bot_state_t *p);
void __cdecl sub_100215E0(bot_state_t *bs);
BOOL __cdecl sub_10021710(int *a1);
void __cdecl sub_10025070(void);
void __cdecl sub_100262C0(_DWORD *a1, bot_goal_t *a2);
float *__cdecl sub_100289A0(bot_state_t *bs, float a2);
int __cdecl sub_10028A40(bot_state_t *bs, float a2);

#endif /* BOTLIB_BE_AI2_DMQ2_H */
