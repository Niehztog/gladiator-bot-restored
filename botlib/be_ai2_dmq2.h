/*
 * be_ai2_dmq2.h — interface of be_ai2_dmq2.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AI2_DMQ2_H
#define BOTLIB_BE_AI2_DMQ2_H

/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
int __cdecl FindClientByName(char *name);  /* 1-arg roster substring search (sub_100268D0); was incorrectly 3-arg */
/* G_SetMovedir's four direction constants, in the original declaration order;
 * all four are defined in botlib_structdefs.c.  IDA rendered the two MOVEDIR_*
 * vec3s as per-dword scalars (flt_1005C578/dword_1005C57C/dword_1005C580 and
 * flt_1005C590/dword_1005C594/dword_1005C598) because VectorCopy expands to
 * three separate moves and MSVC6 kept only the first in the FPU — a decompiler
 * artifact, not the original shape.  The array form is proven twice over: the
 * 1999 Linux gladi386.so's symbol table sizes each of the four at 12 bytes
 * (`readelf -s`: "12 OBJECT GLOBAL … MOVEDIR_UP"), and BotSetMovedir stays
 * byte-identical (125 B) under the MSVC6 oracle with float[3] + VectorCopy. */
extern float VEC_UP[3];      /* 0x1005C56C {0,-1, 0} — defined in botlib_structdefs.c */
extern float MOVEDIR_UP[3];  /* 0x1005C578 {0, 0, 1} — defined in botlib_structdefs.c */
extern float VEC_DOWN[3];    /* 0x1005C584 {0,-2, 0} — defined in botlib_structdefs.c */
extern float MOVEDIR_DOWN[3]; /* 0x1005C590 {0, 0,-1} — defined in botlib_structdefs.c */
extern bot_clientsettings_t *clientsettings;
extern libvar_t *libvar_ctf;
/* CTF flag goals.  BotGetLevelItemGoal fills 48 bytes of each 56-byte
 * bot_goal_t slot; `areanum` doubles as the "flag found" flag (0 = not yet). */
extern bot_goal_t ctf_blueflag;
extern bot_goal_t ctf_redflag;
extern libvar_t *libvar_usehook;
extern libvar_t *libvar_ch;
extern libvar_t *libvar_teamplay;
extern libvar_t *libvar_ra;
extern libvar_t *libvar_runes;
extern int dword_1006446C;
extern libvar_t *libvar_dmflags;
extern libvar_t *libvar_nochat;
extern libvar_t *libvar_rocketjump;
extern libvar_t *libvar_fastchat;
extern libvar_t *libvar_assimilation;
extern int dword_10064484;
extern libvar_t *libvar_teamplay_shell;
extern int dword_1006448C;
extern int dword_10064490;
extern int dword_10064494;
extern int dword_10064498;
extern int dword_1006449C;

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
float __cdecl BotChatTime(bot_state_t *bs);
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
void BotShutdownDeathmatchAI(void);
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
