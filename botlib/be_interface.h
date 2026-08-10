/*
 * be_interface.h — interface of be_interface.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_INTERFACE_H
#define BOTLIB_BE_INTERFACE_H

#include "botlib_local.h"

qboolean __cdecl BotLibSetup(const char *str);
int BotSetupMoveAI();
int Export_BotAddPointLight(int *origin, int ent, float radius,
                             float r, float g, float b, float time, float decay);
int Export_BotAddSound(int *origin, int ent, int channel, int soundindex,
                       float volume, float attenuation, float timeofs);
int Export_BotClientSettings(int client, void *settings);
int Export_BotDefine(char *string);
int Export_BotLibAI(int a1, float a2);
int __cdecl Export_BotLibConsoleMessage(int client, int a2, char *message);
int __cdecl Export_BotLibStartFrame(float time);
int Export_BotLibVarSet(char *var_name, char *value);
int Export_BotLibraryInitialized(void);
int Export_BotLoadMap(char *mapname, int modelindexes, char **modelindex,
                      int soundindexes, char **soundindex,
                      int imageindexes, char **imageindex);
int Export_BotMoveClient(int oldclnum, int newclnum);
int Export_BotSettings(int client, void *settings);
int Export_BotSetupClient(int client, void *settings);
int Export_BotSetupLibrary(void);
int Export_BotShutdownClient(int client);
int Export_BotShutdownLibrary(void);
int Export_BotUpdateClient(int client, void *buc);
int Export_BotUpdateEntity(int ent, void *bue);
char *Export_BotVersion(void);
int Export_Test(int parm0, char *parm1, float *parm2, float *parm3);
bot_export_t *GetBotAPI(bot_import_t *import);
int Sys_MilliSeconds();
qboolean __cdecl ValidClientNumber(int num, const char *str);
qboolean __cdecl ValidEntityNumber(int num, const char *str);
void __cdecl sub_100376B0(char *String1, __int16);
int __cdecl sub_100377E0(char *String1, __int16);
int __cdecl sub_10037820(char *name, const unsigned char *buf, int len);
int __cdecl sub_10037850(char *String1, const unsigned char *, int);
void __cdecl sub_10037880(void);

#endif /* BOTLIB_BE_INTERFACE_H */
