/*
 * botlib_exports.c — the 20 bot_export_t wrappers (0x100379E0 .. 0x10038460).
 *
 *   slot 0  BotVersion            0x100379E0
 *   slot 1  BotSetupLibrary       0x10037BB0
 *   slot 2  BotShutdownLibrary    0x10037CF0
 *   slot 3  BotLibraryInit        0x10037D80
 *   slot 4  BotLibVarSet          0x10037DA0
 *   slot 5  BotDefine             0x10037DD0
 *   slot 6  BotLoadMap            0x10037E10
 *   slot 7  BotSetupClient        0x10037F00
 *   slot 8  BotShutdownClient     0x10037F70
 *   slot 9  BotMoveClient         0x10037FE0
 *   slot 10 BotClientSettings     0x10038070
 *   slot 11 BotSettings           0x100380E0
 *   slot 12 BotStartFrame         0x10038150   (botlib.c: Export_BotLibStartFrame)
 *   slot 13 BotUpdateClient       0x10038190
 *   slot 14 BotUpdateEntity       0x10038200
 *   slot 15 BotAddSound           0x10038270
 *   slot 16 BotAddPointLight      0x100382F0
 *   slot 17 BotAI                 0x10038380   (botlib.c: Export_BotLibAI)
 *   slot 18 BotConsoleMessage     0x100383F0   (botlib.c: Export_BotLibConsoleMessage)
 *   slot 19 Test                  0x10038460
 *
 * BotLibSetup / ValidClientNumber / ValidEntityNumber are the real
 * implementations from botlib.c, so their error prints still happen.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

/* q_shared.h first: game/botlib.h uses Q2 types without including it.  Then
 * drop q_shared.h's 2-arg VectorNegate macro so botlib's 1-arg function stays
 * callable (same pattern as botlib.c). */
#include "../game/q_shared.h"
#undef VectorNegate
#include "../game/botlib.h"  /* bot_export_t, bot_import_t */
#include "gladiator.dll.h"
#include "bot_state.h"

/* ---- Declarations for the implementations in botlib.c ------------------- */

#include "botlib_state.h"    /* botimport / botstate / bot_exports blocks */

/* Validation helpers (real ones; they print on failure) */
extern int  BotLibSetup(const char *str);                /* 0x100379A0 */
extern int  ValidClientNumber(int num, const char *str); /* 0x10037900 */
extern int  ValidEntityNumber(int num, const char *str); /* 0x10037950 */

/* Slot 1 helpers */
extern void   Swap_Init(void);                            /* 0x100439F0 — game/q_shared.c, included at end of botlib.c */
extern int    BotSetupLibrary(void);                      /* 0x10029C90 inner */
extern float  LibVarValue(char *name, char *default_str); /* 0x10038A90 */
extern int    BotSetupMoveAI(void);                       /* 0x10037A00 */
extern int    sub_1000EDC0(int numentities, int numclients);
extern int    EA_Setup(void);                             /* 0x10037660 */
extern void   Log_Open(char *filename);                   /* 0x10038BE0 */

/* Slot 2 helpers */
extern int    BotShutdownLibrary(void);                   /* 0x10029DA0 inner */
extern int    AAS_Shutdown(void);                         /* 0x1000EE30 */
extern int    EA_Shutdown(void);                          /* 0x10037690 */
extern FILE  *Log_Close(void);                            /* 0x10038D60 */
extern int    DumpMemory(void);                           /* 0x100391C0 */

/* Slot 3-5 */
extern int    AAS_Initialized(void);
extern void   LibVarSet(char *name, char *value);      /* LibVarSet body */
extern int    PC_AddGlobalDefine(const char *string);

/* Slot 6 helpers */
extern int    BotLoadMap(char *mapname, int modelindexes, char **modelindex,
                         int soundindexes, char **soundindex,
                         int imageindexes, char **imageindex);
extern int    sub_10029C10(void);                         /* post-load init */

/* Slot 7 helpers */
extern int    BotSetupClient(int client, const void *settings);
extern int    sub_100085F0(void);                         /* AAS map-sync pre-setup */

/* Slot 8-11, 13-18 inner bodies */
extern int    BotShutdownClient(int client);
extern int    BotMoveClient(int oldclnum, int newclnum);
extern int    BotClientSettings(int client, const void *settings);  /* 0x10029920 */
extern int    BotSettings(int client, const void *settings);        /* 0x100299D0 */
extern int    BotUpdateClient(int client, const void *buc);
extern int    AAS_UpdateEntity(int entnum, bot_updateentity_t *state);
extern int    sub_1001CE20(intptr_t origin, int ent, int channel, int soundindex,
                           int volume_bits, int attn_bits, float timeofs);
extern int    BotAddPointLight(_DWORD *origin, int ent, int radius_bits,
                               int r_bits, int g_bits, int b_bits,
                               int time_bits, int decay_bits);


/* ---- SLOT 0 — BotVersion ------------------------------------------------ */
//----- (100379E0) --------------------------------------------------------
char *Export_BotVersion(void)
{
    return "BotLib v0.96";
}

/* ---- SLOT 1 — BotSetupLibrary (0x10037BB0) ------------------------------ */
//----- (10037BB0) --------------------------------------------------------
int Export_BotSetupLibrary(void)
{
    if (botstate.setup) {
        botimport.Print(3, "bot library already setup\n");
        return 2;
    }

    Log_Open("botlib.log");
    botimport.Print(1, "------- BotLib Initialization -------\n");
    botimport.Print(1, "BotLib v0.96\n");

    Swap_Init();
    botstate.setup = 1;

    botstate.num_clients  = (int)LibVarValue("maxclients",  "4");
    botstate.num_entities = (int)LibVarValue("maxentities", "1024");

    BotSetupMoveAI();

    errno = sub_1000EDC0(botstate.num_entities, botstate.num_clients);
    if (errno) return errno;

    errno = BotSetupLibrary();
    if (errno) return errno;

    EA_Setup();

    botimport.Print(1, "-------------------------------------\n");
    return 0;
}

/* ---- SLOT 2 — BotShutdownLibrary (0x10037CF0) --------------------------- */
//----- (10037CF0) --------------------------------------------------------
int Export_BotShutdownLibrary(void)
{
    if (!botstate.setup) {
        botimport.Print(3, "library not setup\n");
        return 1;
    }

    BotShutdownLibrary();
    AAS_Shutdown();
    EA_Shutdown();
    Log_Close();
    DumpMemory();

    /* Three rep stos (20 / 10 / 20 dwords) over the contiguous interface
     * blocks, then the scalar botlibsetup = 0.  sizeof keeps the clears
     * 64-bit-correct; see botlib_state.h for the block layout. */
    memset(&botstate,    0, sizeof(botstate));     /* block 1 @0x10064020, 20 dwords */
    memset(&botimport,   0, sizeof(botimport));    /* block 2 @0x10063FE0, 10 dwords */
    memset(&bot_exports, 0, sizeof(bot_exports));  /* block 3 @0x10063F80, 20 dwords */
    botstate.setup = 0;

    return 0;
}

/* ---- SLOT 3 — BotLibraryInitialized (0x10037D80); tail-jmp AAS_Initialized */
//----- (10037D80) --------------------------------------------------------
int Export_BotLibraryInitialized(void)
{
    return AAS_Initialized();
}

/* ---- SLOT 4 — BotLibVarSet (0x10037DA0).  Returns 0 unconditionally; the
 * original has no null-check. --------------------------------------------- */
//----- (10037DA0) --------------------------------------------------------
int Export_BotLibVarSet(char *var_name, char *value)
{
    LibVarSet(var_name, value);
    return 0;
}

/* ---- SLOT 5 — BotDefine (0x10037DD0) ----------------------------------- */
//----- (10037DD0) --------------------------------------------------------
int Export_BotDefine(char *string)
{
    if (!PC_AddGlobalDefine(string))
        botimport.Print(3, "couldn't add define %s\n", string);
    return 0;
}

/* ---- SLOT 6 — BotLoadMap (0x10037E10).  A NULL mapname is the unload
 * path. ------------------------------------------------------------------- */
//----- (10037E10) --------------------------------------------------------
int Export_BotLoadMap(char *mapname, int modelindexes, char **modelindex,
                     int soundindexes, char **soundindex,
                     int imageindexes, char **imageindex)
{
    if (!BotLibSetup("BotLoadMap")) return 1;

    if (!mapname) {
        return BotLoadMap(0, modelindexes, modelindex,
                          soundindexes, soundindex,
                          imageindexes, imageindex);
    }

    botimport.Print(1, "------------ Map Loading ------------\n");

    errno = BotLoadMap(mapname, modelindexes, modelindex,
                       soundindexes, soundindex,
                       imageindexes, imageindex);
    if (errno) return errno;

    sub_10029C10();
    botimport.Print(1, "-------------------------------------\n");
    return 0;
}

/* ---- SLOT 7 — BotSetupClient (0x10037F00) ------------------------------ */
//----- (10037F00) --------------------------------------------------------
int Export_BotSetupClient(int client, void *settings)
{
    int r;
    if (!BotLibSetup("BotSetupClient")) return 0;
    if (!ValidClientNumber(client, "BotSetupClient")) return 0;
    sub_100085F0();
    r = BotSetupClient(client, settings);
    return r != 0;
}

/* ---- SLOT 8 — BotShutdownClient (0x10037F70) --------------------------- */
//----- (10037F70) --------------------------------------------------------
int Export_BotShutdownClient(int client)
{
    if (!BotLibSetup("BotShutdownClient")) return 1;
    if (!ValidClientNumber(client, "BotShutdownClient")) return 3;
    return BotShutdownClient(client);
}

/* ---- SLOT 9 — BotMoveClient (0x10037FE0) ------------------------------- */
//----- (10037FE0) --------------------------------------------------------
int Export_BotMoveClient(int oldclnum, int newclnum)
{
    if (!BotLibSetup("BotMoveClient")) return 1;
    if (!ValidClientNumber(oldclnum, "BotMoveClient, parm0")) return 3;
    if (!ValidClientNumber(newclnum, "BotMoveClient, parm1")) return 3;
    return BotMoveClient(oldclnum, newclnum);
}

/* ---- SLOT 10 — BotClientSettings (0x10038070); body at 0x10029920 ------ */
//----- (10038070) --------------------------------------------------------
int Export_BotClientSettings(int client, void *settings)
{
    if (!BotLibSetup("BotClientSettings")) return 1;
    if (!ValidClientNumber(client, "BotClientSettings")) return 3;
    return BotClientSettings(client, settings);
}

/* ---- SLOT 11 — BotSettings (0x100380E0); body at 0x100299D0 ------------ */
//----- (100380E0) --------------------------------------------------------
int Export_BotSettings(int client, void *settings)
{
    if (!BotLibSetup("BotSettings")) return 1;
    if (!ValidClientNumber(client, "BotSettings")) return 3;
    return BotSettings(client, settings);
}

/* Slots 12, 17 and 18 live in botlib.c (0x10038150 / 0x10038380 /
 * 0x100383F0). */

/* ---- SLOT 13 — BotUpdateClient (0x10038190) ---------------------------- */
//----- (10038190) --------------------------------------------------------
int Export_BotUpdateClient(int client, void *buc)
{
    if (!BotLibSetup("BotUpdateClient")) return 1;
    if (!ValidClientNumber(client, "BotUpdateClient")) return 3;
    return BotUpdateClient(client, buc);
}

/* ---- SLOT 14 — BotUpdateEntity (0x10038200) ---------------------------- */
//----- (10038200) --------------------------------------------------------
int Export_BotUpdateEntity(int ent, void *bue)
{
    if (!BotLibSetup("BotUpdateEntity")) return 1;
    if (!ValidEntityNumber(ent, "BotUpdateEntity")) return 4;
    return AAS_UpdateEntity(ent, (bot_updateentity_t *)bue);
}

/* ---- SLOT 15 — BotAddSound (0x10038270).  The context string really is
 * "BotUpdateSound".  The first six args reach sub_1001CE20 as raw 32-bit
 * values (float bits through int slots); only timeofs is a real float. ---- */
//----- (10038270) --------------------------------------------------------
int Export_BotAddSound(int *origin, int ent, int channel, int soundindex,
                       float volume, float attenuation, float timeofs)
{
    if (!BotLibSetup("BotUpdateSound")) return 1;
    if (!ValidEntityNumber(ent, "BotUpdateSound")) return 4;
    return sub_1001CE20((intptr_t)origin, ent, channel, soundindex,
                        *(int *)&volume,
                        *(int *)&attenuation,
                        timeofs);
}

/* ---- SLOT 16 — BotAddPointLight (0x100382F0); floats passed as bits ---- */
//----- (100382F0) --------------------------------------------------------
int Export_BotAddPointLight(int *origin, int ent, float radius,
                            float r, float g, float b,
                            float time, float decay)
{
    if (!BotLibSetup("BotAddPointLight")) return 1;
    if (!ValidEntityNumber(ent, "BotAddPointLight")) return 4;
    return BotAddPointLight((_DWORD *)origin, ent,
                            *(int *)&radius,
                            *(int *)&r, *(int *)&g, *(int *)&b,
                            *(int *)&time, *(int *)&decay);
}

/* ---- SLOT 19 — Test (0x10038460); returns 0 ---------------------------- */
//----- (10038460) --------------------------------------------------------
int Export_Test(int parm0, char *parm1, float *parm2, float *parm3)
{
    (void)parm0; (void)parm1; (void)parm2; (void)parm3;
    return 0;
}
