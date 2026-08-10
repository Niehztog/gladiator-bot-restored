/*
 * be_ai2_main.h — interface of be_ai2_main.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AI2_MAIN_H
#define BOTLIB_BE_AI2_MAIN_H

#include "botlib_local.h"

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
int BotShutdownLibrary();
int __cdecl BotUpdateClient(int a1, const void *a2);
int __cdecl ClientFromName(const char *name);
char *__cdecl ClientName(int client);
char *__cdecl ClientSkin(int client);
int Export_BotAIFrame(int a1, float a2);
int NumBots();
void sub_10028E80(void);  
void sub_100292E0();
static void sub_100293A0(bot_state_t *bs);
int sub_10029C10();

#endif /* BOTLIB_BE_AI2_MAIN_H */
