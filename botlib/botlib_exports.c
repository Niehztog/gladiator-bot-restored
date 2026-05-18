/*
 * botlib_exports.c — bot_export_t wrappers.
 *
 * Faithful reconstruction of the original 20 export wrappers
 * (0x100379E0 .. 0x10038460) from the gladiator.dll_ binary.  Each function
 * matches the disassembled control flow:
 *
 *   slot 0  BotVersion            impl=0x100379E0   "BotLib v0.96"
 *   slot 1  BotSetupLibrary       impl=0x10037BB0   sets up library + LibVars
 *   slot 2  BotShutdownLibrary    impl=0x10037CF0   tears down (5 calls + memsets)
 *   slot 3  BotLibraryInit        impl=0x10037D80   tail-jmp AAS_Initialized
 *   slot 4  BotLibVarSet          impl=0x10037DA0   forwards to sub_10038AC0
 *   slot 5  BotDefine             impl=0x10037DD0   PC_AddGlobalDefine + warn
 *   slot 6  BotLoadMap            impl=0x10037E10   BotLibSetup + load + sub_10029C10
 *   slot 7  BotSetupClient        impl=0x10037F00   ValidClient + sub_100085F0 + !!body
 *   slot 8  BotShutdownClient     impl=0x10037F70
 *   slot 9  BotMoveClient         impl=0x10037FE0
 *   slot 10 BotClientSettings     impl=0x10038070
 *   slot 11 BotSettings           impl=0x100380E0
 *   slot 12 BotStartFrame         impl=0x10038150   (defined in botlib.c as Export_BotLibStartFrame)
 *   slot 13 BotUpdateClient       impl=0x10038190
 *   slot 14 BotUpdateEntity       impl=0x10038200
 *   slot 15 BotAddSound           impl=0x10038270
 *   slot 16 BotAddPointLight      impl=0x100382F0
 *   slot 17 BotAI                 impl=0x10038380   (defined in botlib.c as Export_BotLibAI)
 *   slot 18 BotConsoleMessage     impl=0x100383F0   (defined in botlib.c as Export_BotLibConsoleMessage)
 *   slot 19 Test                  impl=0x10038460   return 0
 *
 * All `BotLibSetup` / `ValidClientNumber` / `ValidEntityNumber` calls are
 * delegated to the genuine implementations at 0x100379A0/0x10037900/0x10037950
 * (defined in botlib.c) so the error-print side effects of the originals are
 * preserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "../game/botlib.h"  /* bot_export_t, bot_import_t + prerequisite Q2 types */
#include "gladiator.dll.h"
#include "bot_state.h"

/* -----------------------------------------------------------------------
 * Forward declarations for implementations in botlib.c
 * --------------------------------------------------------------------- */

/* Validation helpers (real ones; they print on failure) */
extern int  BotLibSetup(const char *str);                /* 0x100379A0 */
extern int  ValidClientNumber(int num, const char *str); /* 0x10037900 */
extern int  ValidEntityNumber(int num, const char *str); /* 0x10037950 */

/* Engine import pointer (bi_Print) */
extern int (*bi_Print)(int type, const char *fmt, ...);  /* @ 0x10063FE8 */

/* Globals */
extern int  botlibsetup;     /* @ 0x10064020 */
extern int  maxclients;      /* @ 0x10064028 */
extern int  maxentities;     /* @ 0x10064024 */

/* Slot 1 helpers */
extern int    Swap_Init(void);                            /* 0x100439F0 */
extern int    BotSetupLibrary(void);                      /* 0x10029C90 inner */
extern double LibVarValue(char *name, char *default_str); /* 0x10038A90 */
extern int    BotSetupMoveAI(void);                       /* 0x10037A00 */
extern int    sub_1000EDC0(int maxentities, int maxclients);
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
extern void   sub_10038AC0(char *name, char *value);      /* LibVarSet body */
extern int    PC_AddGlobalDefine(const char *string);

/* Slot 6 helpers */
extern int    BotLoadMap(int mapname, int modelindexes, int modelindex,
                         int soundindexes, int soundindex,
                         int imageindexes, int imageindex);
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
extern int    AAS_UpdateEntity(int entnum, float *state);
extern int    sub_1001CE20(int origin, int ent, int channel, int soundindex,
                           int volume_bits, int attn_bits, float timeofs);
extern int    BotAddPointLight(_DWORD *origin, int ent, int radius_bits,
                               int r_bits, int g_bits, int b_bits,
                               int time_bits, int decay_bits);


/* =========================================================================
 * SLOT 0 — BotVersion: returns "BotLib v0.96".
 * ========================================================================= */
//----- (100379E0) --------------------------------------------------------
char *Export_BotVersion(void)
{
    return "BotLib v0.96";
}

/* =========================================================================
 * SLOT 1 — BotSetupLibrary (0x10037BB0).
 *
 * Disassembled sequence:
 *   if (botlibsetup) { bi_Print(3, "bot library already setup\n"); return 2; }
 *   Log_Open("botlib.log");
 *   bi_Print(1, "------- BotLib Initialization -------\n");
 *   bi_Print(1, "BotLib v0.96\n");
 *   Swap_Init();
 *   botlibsetup = 1;
 *   maxclients  = (int)LibVarValue("maxclients",  "4");
 *   maxentities = (int)LibVarValue("maxentities", "1024");
 *   BotSetupMoveAI();
 *   errno = sub_1000EDC0(maxentities, maxclients);
 *   if (errno) return errno;
 *   errno = BotSetupLibrary();
 *   if (errno) return errno;
 *   EA_Setup();
 *   bi_Print(1, "-------------------------------------\n");
 *   return 0;
 * ========================================================================= */
//----- (10037BB0) --------------------------------------------------------
int Export_BotSetupLibrary(void)
{
    if (botlibsetup) {
        bi_Print(3, "bot library already setup\n");
        return 2;
    }

    Log_Open("botlib.log");
    bi_Print(1, "------- BotLib Initialization -------\n");
    bi_Print(1, "BotLib v0.96\n");

    Swap_Init();
    botlibsetup = 1;

    maxclients  = (int)LibVarValue("maxclients",  "4");
    maxentities = (int)LibVarValue("maxentities", "1024");

    BotSetupMoveAI();

    errno = sub_1000EDC0(maxentities, maxclients);
    if (errno) return errno;

    errno = BotSetupLibrary();
    if (errno) return errno;

    EA_Setup();

    bi_Print(1, "-------------------------------------\n");
    return 0;
}

/* =========================================================================
 * SLOT 2 — BotShutdownLibrary (0x10037CF0).
 *
 * Disassembled sequence:
 *   if (!botlibsetup) {
 *       bi_Print(3, "library not setup\n");
 *       return 1;
 *   }
 *   BotShutdownLibrary();    // 0x10029DA0 — bot-state cleanup
 *   AAS_Shutdown();          // 0x1000EE30
 *   EA_Shutdown();           // 0x10037690
 *   Log_Close();             // 0x10038D60
 *   DumpMemory();            // 0x100391C0 — frees all tracked allocations
 *   memset(&botlibsetup, 0, 80);      // clears 20 dwords incl. botlibsetup,
 *                                     //   maxclients, maxentities, bottime,
 *                                     //   libvar_sv_* pointers
 *   memset(bot_import_table, 0, 40);  // 10 import pointers
 *   memset(bot_export_table, 0, 80);  // 20 export pointers
 *   return 0;
 *
 * Our reconstruction has the bot_import_t fields as named globals and the
 * bot_export_t as a static struct inside GetBotAPI.  The semantically
 * important step for the engine is botlibsetup = 0 — subsequent dispatch
 * calls will then take the "not setup" path.  Other globals are also
 * zeroed for faithfulness.
 * ========================================================================= */
extern void *bi_BotClientCommand, *bi_Trace, *bi_PointContents,
            *bi_GetMemory, *bi_FreeMemory, *bi_DebugLineCreate,
            *bi_DebugLineShow;
extern int dword_10063FE0;  /* imp->BotInput pointer */
extern int dword_1006402C;  /* bottime */
extern struct libvar_s *libvar_sv_friction, *libvar_sv_stopspeed,
                       *libvar_sv_gravity, *libvar_sv_waterfriction,
                       *libvar_sv_watergravity, *libvar_sv_maxvelocity,
                       *libvar_sv_maxwalkvelocity, *libvar_sv_maxcrouchvelocity,
                       *libvar_sv_maxswimvelocity, *libvar_sv_maxaccelerate,
                       *libvar_sv_airaccelerate, *libvar_sv_step,
                       *libvar_sv_maxbarrier, *libvar_sv_maxsteepness,
                       *libvar_sv_jumpvel, *libvar_sv_maxwaterjump;

//----- (10037CF0) --------------------------------------------------------
int Export_BotShutdownLibrary(void)
{
    if (!botlibsetup) {
        bi_Print(3, "library not setup\n");
        return 1;
    }

    BotShutdownLibrary();
    AAS_Shutdown();
    EA_Shutdown();
    Log_Close();
    DumpMemory();

    /* Zero the globals at 0x10064020..0x1006406C (20 dwords).  In the
     * original these are addressed as a contiguous block, but the linker
     * makes no such guarantee in our reconstruction — clear each by name. */
    botlibsetup = 0;
    maxentities = 0;
    maxclients  = 0;
    dword_1006402C = 0;
    libvar_sv_friction         = NULL;
    libvar_sv_stopspeed        = NULL;
    libvar_sv_gravity          = NULL;
    libvar_sv_waterfriction    = NULL;
    libvar_sv_watergravity     = NULL;
    libvar_sv_maxvelocity      = NULL;
    libvar_sv_maxwalkvelocity  = NULL;
    libvar_sv_maxcrouchvelocity= NULL;
    libvar_sv_maxswimvelocity  = NULL;
    libvar_sv_maxaccelerate    = NULL;
    libvar_sv_airaccelerate    = NULL;
    libvar_sv_step             = NULL;
    libvar_sv_maxbarrier       = NULL;
    libvar_sv_maxsteepness     = NULL;
    libvar_sv_jumpvel          = NULL;
    libvar_sv_maxwaterjump     = NULL;

    /* Zero the bot_import_t globals at 0x10063FE0..0x10064007 (10 dwords) */
    dword_10063FE0 = 0;
    bi_BotClientCommand = NULL;
    bi_Print            = NULL;
    bi_Trace            = NULL;
    bi_PointContents    = NULL;
    bi_GetMemory        = NULL;
    bi_FreeMemory       = NULL;
    bi_DebugLineCreate  = NULL;
    /* DebugLineDelete slot — not stored in a named global */
    bi_DebugLineShow    = NULL;

    /* The bot_export_t table is a static inside GetBotAPI; it is not
     * directly addressable here.  The engine holds a pointer to it but
     * after shutdown should not dispatch into it (botlibsetup=0 makes
     * every wrapper return 1).  Skipping the explicit memset is safe. */

    return 0;
}

/* =========================================================================
 * SLOT 3 — BotLibraryInitialized (0x10037D80).
 *   Original: jmp 0x1000DEE0  (AAS_Initialized: return aasworld.initialized)
 * ========================================================================= */
//----- (10037D80) --------------------------------------------------------
int Export_BotLibraryInitialized(void)
{
    return AAS_Initialized();
}

/* =========================================================================
 * SLOT 4 — BotLibVarSet (0x10037DA0).
 *   mov eax,[esp+8]; mov ecx,[esp+4]; push eax; push ecx;
 *   call sub_10038AC0; add esp,8; xor eax,eax; ret
 * Returns 0 unconditionally.  No null-check in the original.
 * ========================================================================= */
//----- (10037DA0) --------------------------------------------------------
int Export_BotLibVarSet(char *var_name, char *value)
{
    sub_10038AC0(var_name, value);
    return 0;
}

/* =========================================================================
 * SLOT 5 — BotDefine (0x10037DD0).
 *   if (!PC_AddGlobalDefine(string))
 *       bi_Print(3, "couldn't add define %s\n", string);
 *   return 0;
 * ========================================================================= */
//----- (10037DD0) --------------------------------------------------------
int Export_BotDefine(char *string)
{
    if (!PC_AddGlobalDefine(string))
        bi_Print(3, "couldn't add define %s\n", string);
    return 0;
}

/* =========================================================================
 * SLOT 6 — BotLoadMap (0x10037E10).
 *
 *   if (!BotLibSetup("BotLoadMap")) return 1;
 *   if (mapname == NULL) {
 *       return BotLoadMap(NULL, modelindexes, modelindex,
 *                         soundindexes, soundindex,
 *                         imageindexes, imageindex);  // unload path
 *   }
 *   bi_Print(1, "------------ Map Loading ------------\n");
 *   errno = BotLoadMap(mapname, ...);
 *   if (errno) return errno;
 *   sub_10029C10();
 *   bi_Print(1, "-------------------------------------\n");
 *   return 0;
 * ========================================================================= */
//----- (10037E10) --------------------------------------------------------
int Export_BotLoadMap(char *mapname, int modelindexes, char **modelindex,
                     int soundindexes, char **soundindex,
                     int imageindexes, char **imageindex)
{
    if (!BotLibSetup("BotLoadMap")) return 1;

    if (!mapname) {
        return BotLoadMap(0, modelindexes, (int)modelindex,
                          soundindexes, (int)soundindex,
                          imageindexes, (int)imageindex);
    }

    bi_Print(1, "------------ Map Loading ------------\n");

    errno = BotLoadMap((int)mapname, modelindexes, (int)modelindex,
                       soundindexes, (int)soundindex,
                       imageindexes, (int)imageindex);
    if (errno) return errno;

    sub_10029C10();
    bi_Print(1, "-------------------------------------\n");
    return 0;
}

/* =========================================================================
 * SLOT 7 — BotSetupClient (0x10037F00).
 *
 *   if (!BotLibSetup("BotSetupClient")) return 0;          (eax==0 fall-through)
 *   if (!ValidClientNumber(client, "BotSetupClient")) return 0;
 *   sub_100085F0();                  // AAS map-sync pre-setup
 *   return BotSetupClient(client, settings) != 0;          (neg/sbb/neg)
 * ========================================================================= */
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

/* =========================================================================
 * SLOT 8 — BotShutdownClient (0x10037F70).
 * ========================================================================= */
//----- (10037F70) --------------------------------------------------------
int Export_BotShutdownClient(int client)
{
    if (!BotLibSetup("BotShutdownClient")) return 1;
    if (!ValidClientNumber(client, "BotShutdownClient")) return 3;
    return BotShutdownClient(client);
}

/* =========================================================================
 * SLOT 9 — BotMoveClient (0x10037FE0).
 * ========================================================================= */
//----- (10037FE0) --------------------------------------------------------
int Export_BotMoveClient(int oldclnum, int newclnum)
{
    if (!BotLibSetup("BotMoveClient")) return 1;
    if (!ValidClientNumber(oldclnum, "BotMoveClient, parm0")) return 3;
    if (!ValidClientNumber(newclnum, "BotMoveClient, parm1")) return 3;
    return BotMoveClient(oldclnum, newclnum);
}

/* =========================================================================
 * SLOT 10 — BotClientSettings (0x10038070).  Inner body at 0x10029920.
 * ========================================================================= */
//----- (10038070) --------------------------------------------------------
int Export_BotClientSettings(int client, void *settings)
{
    if (!BotLibSetup("BotClientSettings")) return 1;
    if (!ValidClientNumber(client, "BotClientSettings")) return 3;
    return BotClientSettings(client, settings);
}

/* =========================================================================
 * SLOT 11 — BotSettings (0x100380E0).  Inner body at 0x100299D0.
 * ========================================================================= */
//----- (100380E0) --------------------------------------------------------
int Export_BotSettings(int client, void *settings)
{
    if (!BotLibSetup("BotSettings")) return 1;
    if (!ValidClientNumber(client, "BotSettings")) return 3;
    return BotSettings(client, settings);
}

/* Slot 12 (Export_BotLibStartFrame), Slot 17 (Export_BotLibAI), Slot 18
 * (Export_BotLibConsoleMessage) are defined in botlib.c.  See addresses
 * 0x10038150 / 0x10038380 / 0x100383F0 there. */

/* =========================================================================
 * SLOT 13 — BotUpdateClient (0x10038190).
 * ========================================================================= */
//----- (10038190) --------------------------------------------------------
int Export_BotUpdateClient(int client, void *buc)
{
    if (!BotLibSetup("BotUpdateClient")) return 1;
    if (!ValidClientNumber(client, "BotUpdateClient")) return 3;
    return BotUpdateClient(client, buc);
}

/* =========================================================================
 * SLOT 14 — BotUpdateEntity (0x10038200).
 *
 *   if (!BotLibSetup("BotUpdateEntity")) return 1;
 *   if (!ValidEntityNumber(ent, "BotUpdateEntity")) return 4;
 *   return AAS_UpdateEntity(ent, bue);
 * ========================================================================= */
//----- (10038200) --------------------------------------------------------
int Export_BotUpdateEntity(int ent, void *bue)
{
    if (!BotLibSetup("BotUpdateEntity")) return 1;
    if (!ValidEntityNumber(ent, "BotUpdateEntity")) return 4;
    return AAS_UpdateEntity(ent, (float *)bue);
}

/* =========================================================================
 * SLOT 15 — BotAddSound (0x10038270).
 *
 *   if (!BotLibSetup("BotUpdateSound")) return 1;
 *   if (!ValidEntityNumber(ent, "BotUpdateSound")) return 4;
 *   return sub_1001CE20(origin, ent, channel, soundindex,
 *                       volume_bits, attn_bits, timeofs);
 *
 * Note: the binary uses "BotUpdateSound" as the context string (verified at
 * file offset 0x5EE28).  The first six args to sub_1001CE20 are pushed as
 * raw 32-bit values (float bits passed through int slots).  The last
 * argument (timeofs) is pushed as a float.
 * ========================================================================= */
//----- (10038270) --------------------------------------------------------
int Export_BotAddSound(int *origin, int ent, int channel, int soundindex,
                       float volume, float attenuation, float timeofs)
{
    if (!BotLibSetup("BotUpdateSound")) return 1;
    if (!ValidEntityNumber(ent, "BotUpdateSound")) return 4;
    return sub_1001CE20((int)origin, ent, channel, soundindex,
                        *(int *)&volume,
                        *(int *)&attenuation,
                        timeofs);
}

/* =========================================================================
 * SLOT 16 — BotAddPointLight (0x100382F0).
 *
 *   if (!BotLibSetup("BotAddPointLight")) return 1;
 *   if (!ValidEntityNumber(ent, "BotAddPointLight")) return 4;
 *   return BotAddPointLight(origin, ent, radius_bits,
 *                           r_bits, g_bits, b_bits,
 *                           time_bits, decay_bits);
 * ========================================================================= */
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

/* =========================================================================
 * SLOT 19 — Test (0x10038460).  Original: xor eax,eax; ret.
 * ========================================================================= */
//----- (10038460) --------------------------------------------------------
int Export_Test(int parm0, char *parm1, float *parm2, float *parm3)
{
    (void)parm0; (void)parm1; (void)parm2; (void)parm3;
    return 0;
}
