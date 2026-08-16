/* be_ai2_main.h — interface of be_ai2_main.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AI2_MAIN_H
#define BOTLIB_BE_AI2_MAIN_H

/* Declarations for what this TU defines — last, so the types above are in scope. */
/* These declarations are load-bearing: without one, C's
 * default argument promotion passes Export_BotAIFrame's `float` as a `double`
 * and its caller's codegen changes (caught as Export_BotLibAI OUR+1 by the
 * MSVC6 oracle). */
int  Export_BotAIFrame(int a1, float a2);          /* be_ai2_main.c   0x10029320 */
extern int numbots;
extern bsp_entity_t *entities;
extern int gametype;
extern bot_state_t *botstates;
extern bot_character_t **botcharacters;
extern void **botgoalstate_p0;
extern void **botgoalstate_p1;
extern bot_weaponstate_t **botweaponstates;
extern void **botchatdumps;
extern chatmsg_links_t *botchatmsglinks;
extern ai_node_fn_t *botainodes;
extern bot_waypoint_t **botcheckpoints;
extern bot_waypoint_t **botpatrolpoints;
extern bot_waypoint_t **botcurpatrolpoint;
extern float regularupdate_time;
/* The side-band macros above index botstates, which is DEFINED in
 * be_ai2_main.c.  Declared here at the end, after bot_state_t exists --
 * a macro expands at its use site, so this need only precede the caller. */
extern bot_state_t *botstates;

float __cdecl AngleDifference(float ang1, float ang2);
float BotChangeViewAngle(float angle, float ideal_angle, float speed);
int __cdecl BotChangeViewAngles(bot_state_t *bs, float thinktime);
int __cdecl BotClientSettings(int a1, const void *a2);
int __cdecl BotConsoleMessage(int, int, char *Source);
int __cdecl BotMoveClient(int a1, int a2);
int __cdecl BotResetState(bot_state_t *bs);
int __cdecl BotSettings(int a1, const void *a2);
int __cdecl BotSetupClient(int, char *Source);
int BotSetupLibrary();
int __cdecl BotShutdownClient(int a1);
void BotShutdownLibrary(void);
int __cdecl BotUpdateClient(int a1, const void *a2);
int __cdecl ClientFromName(const char *name);
char *__cdecl ClientName(int client);
char *__cdecl ClientSkin(int client);
int Export_BotAIFrame(int a1, float a2);
int NumBots();
void sub_100292E0();
void sub_100293A0(bot_state_t *bs);
int sub_10029C10();

#endif /* BOTLIB_BE_AI2_MAIN_H */
