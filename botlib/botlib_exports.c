/*
 * botlib_exports.c — Proper function definitions for all bot_export_t slots.
 *
 * ROOT CAUSE OF ORIGINAL CRASH (fixed here):
 *   In the IDA decompilation, Export_BotXxx symbols were placed in .bss as
 *   `_UNKNOWN Export_BotXxx; // weak` data variables.  GetBotAPI() stored the
 *   .bss DATA address in the export table instead of a code address.  The game
 *   then called that address, jumped into .bss, and crashed (0xC0000005).
 *
 * HOW THE ORIGINAL DLL IS STRUCTURED (confirmed from gladiator.dll_):
 *   GetBotAPI at 0x10038480 fills 20 slots with thunk addresses (0x10001xxx).
 *   Each thunk is a 5-byte `jmp` that forwards to the real implementation:
 *
 *   slot 0  BotVersion          thunk=0x10001A3C  impl=0x100379E0  returns "BotLib v0.96"
 *   slot 1  BotSetupLibrary     thunk=0x100017F8  impl=0x10037BB0  calls BotSetupLibrary()
 *   slot 2  BotShutdownLibrary  thunk=0x1000178A  impl=0x10037CF0  calls BotShutdownLibrary()
 *   slot 3  BotLibraryInit      thunk=0x100019E2  impl=0x10037D80  calls AAS_Initialized()
 *   slot 4  BotLibVarSet        thunk=0x1000157D  impl=0x10037DA0  calls sub_10038AC0()
 *   slot 5  BotDefine           thunk=0x10001046  impl=0x10037DD0  calls PC_AddGlobalDefine()
 *   slot 6  BotLoadMap          thunk=0x10001717  impl=0x10037E10  calls BotLoadMap()
 *   slot 7  BotSetupClient      thunk=0x10001640  impl=0x10037F00  calls BotSetupClient()
 *   slot 8  BotShutdownClient   thunk=0x10001618  impl=0x10037F70  calls BotShutdownClient()
 *   slot 9  BotMoveClient       thunk=0x10001A00  impl=0x10037FE0  calls BotMoveClient()
 *   slot 10 BotClientSettings   thunk=0x1000191F  impl=0x10038070  calls BotClientSettings() at 0x10029920 (144-byte name/skin copy)
 *   slot 11 BotSettings         thunk=0x10001A19  impl=0x100380E0  calls BotSettings()         at 0x100299D0 (432-byte personality copy)
 *   slot 12 BotStartFrame       thunk=0x10001BD1  impl=0x10038150  (= Export_BotLibStartFrame)
 *   slot 13 BotUpdateClient     thunk=0x10001CA3  impl=0x10038190  calls BotUpdateClient()
 *   slot 14 BotUpdateEntity     thunk=0x10001BC2  impl=0x10038200  calls AAS_UpdateEntity()
 *   slot 15 BotAddSound         thunk=0x1000194C  impl=0x10038270  calls sub_1001CE20()
 *   slot 16 BotAddPointLight    thunk=0x1000190B  impl=0x100382F0  calls BotAddPointLight()
 *   slot 17 BotAI               thunk=0x100013A2  impl=0x10038380  calls Export_BotAIFrame()
 *   slot 18 BotConsoleMessage   thunk=0x1000145B  impl=0x100383F0  (= Export_BotLibConsoleMessage)
 *   slot 19 Test                thunk=0x10001109  impl=0x10038460  returns 0
 *
 * Each real implementation validates setup/client before calling the body.
 * The body functions are in gladiator_deobfuscated.c under their proper names.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "gladiator.dll.h"
#include "bot_state.h"

/* -----------------------------------------------------------------------
 * Forward declarations for implementations in gladiator_deobfuscated.c
 * Name mapping confirmed from original gladiator.dll_ thunk table analysis.
 * --------------------------------------------------------------------- */

/* Byte-swap init — called at 0x10037BF5 before BotSetupLibrary body */
extern int  Swap_Init(void);
/* Slot 1  impl — initialises the bot library (time/srand/config loading).
 * Allocates dword_100643A0/dword_100643A8 from maxclients, so this MUST
 * be called AFTER maxclients/maxentities are read.  Original wrapper at
 * 0x10037C6C (after sub_1000EDC0). */
extern int  BotSetupLibrary(void);
/* Original wrapper reads two LibVars (used as the float-via-st0 input):
 *   maxclients  = (int)LibVarValue("maxclients",  "4");
 *   maxentities = (int)LibVarValue("maxentities", "1024");
 * The IDA decompilation shows LibVarValue returning float (st0); the
 * wrapper rounds via _ftol at 0x100442E8.  We match the same effect. */
extern double LibVarValue(char *name, int default_str);
extern int  maxclients;    /* int at 0x10064028 */
extern int  maxentities;   /* int at 0x10064024 */
extern int  BotSetupMoveAI(void);           /* physics/movement LibVars — 0x10037A00 */
extern int  sub_1000EDC0(int a1, int a2);  /* entity setup — 0x1000EDC0; a1=maxentities, a2=maxclients */
extern int  EA_Setup(void);                 /* ea_controls = GetClearedMemory(36 * maxclients) — 0x10037660 */
extern int  (*bi_Print)(int type, const char *fmt, ...);  /* engine print fn at 0x10063FE8 */
extern void Log_Open(char *FileName);       /* 0x10038BE0: opens botlib.log if "log" libvar != 0 */
/* Slot 2  impl — shuts down the bot library, frees all resources */
extern int  BotShutdownLibrary(void);
/* Slot 4  impl — sets a LibVar from a string key/value pair */
extern void sub_10038AC0(char *name, char *value);  /* a2 = char* per body: strlen((char*)a2) */
/* Slot 6  impl — loads BSP + AAS data for a map */
extern int  BotLoadMap(int mapname, int modelindexes, int modelindex,
                       int soundindexes, int soundindex,
                       int imageindexes, int imageindex);
/* Slot 7  impl — per-bot initialisation (loads character, weights, etc.) */
extern int  BotSetupClient(int a1, const void *a2);
/* Slot 8  impl — per-bot teardown */
extern int  BotShutdownClient(int a1);
/* Slot 9  impl — moves bot state from one client slot to another */
extern int  BotMoveClient(int a1, int a2);
/* Slot 10 impl — updates client name/skin settings */
extern int  BotClientSettings(int a1, const void *a2);  /* slot 10: 144-byte name/skin copy at 0x10029920 */
extern int  BotSettings(int a1, const void *a2);        /* slot 11: 432-byte personality copy at 0x100299D0 (with bot-active check) */
extern int  BotConsoleMessage(int a1, int a2, char *Source); /* slot 18: queue console msg for bot */
/* Slot 13 impl — per-frame client state update (position, velocity, health…) */
extern int  BotUpdateClient(int a1, const void *a2);
/* Slot 14 impl — per-frame entity state update (already correctly named) */
extern int  AAS_UpdateEntity(int entnum, float *state);
/* Slot 15 impl — sound event handler (validates index, updates sound info) */
extern int  sub_1001CE20(int a1, int a2, int a3, int a4, int a5, int a6, float a7);
/* Slot 16 impl — dynamic point light event handler */
extern int  BotAddPointLight(_DWORD *a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
/* Slot 17 impl — already named Export_BotAIFrame in decompiled file */
extern int  Export_BotAIFrame(int a1, int a2);
/* Slot 18 impl — already named Export_BotLibConsoleMessage */
extern int  Export_BotLibConsoleMessage(int client, int a2, char *message);

/* Other globals used by the wrappers */
extern int  botlibsetup;         /* non-zero once BotSetupLibrary() has run */
extern int  AAS_Initialized(); /* returns aasworld_initialized (= library ready) */
extern char *LibVar(char *name, int defval); /* LibVar get/create */

/* -----------------------------------------------------------------------
 * Validation helpers (inline wrappers matching the original pattern:
 *   if (!BotLibIsSetup("Name")) return 1;
 *   if (!ValidClientNumber(client,"Name")) return 3;
 * --------------------------------------------------------------------- */
static int botlib_is_setup(const char *ctx)
{
    (void)ctx;
    if (!botlibsetup) return 0;
    return 1;
}

static int valid_client(int client, const char *ctx)
{
    extern int maxclients;
    (void)ctx;
    if (client < 0 || client >= maxclients) return 0;
    return 1;
}


/* =========================================================================
 * SLOT 0 — BotVersion: returns library version string.
 * Original impl at 0x100379E0: `return "BotLib v0.96";`
 * ========================================================================= */
char *Export_BotVersion(void)
{
    return "BotLib v0.96";
}

/* =========================================================================
 * SLOT 1 — BotSetupLibrary: initialise the bot library.
 *
 * Faithful reconstruction of the original wrapper at 0x10037BB0.  Sequence
 * (matching the original disassembly):
 *
 *   if (botlibsetup) { bi_Print("bot library already setup"); return 2; }
 *   Log_Open(libvar("log"), "botlib.log");      // optional file logging
 *   bi_Print("------- BotLib Initialization -------");
 *   bi_Print("BotLib v0.96");
 *   Swap_Init();                                  // 0x100439F0
 *   botlibsetup = 1;                              // 0x10037C04
 *   maxclients  = (int)LibVarValue("maxclients",  "4");
 *   maxentities = (int)LibVarValue("maxentities", "1024");
 *   BotSetupMoveAI();                             // 0x10037A00
 *   if ((errno = sub_1000EDC0(maxentities, maxclients))) return errno;
 *   if ((errno = BotSetupLibrary())) return errno;
 *   EA_Setup();                                   // 0x10037660
 *   bi_Print("-------------------------------------");
 *   return 0;
 *
 * The previous implementation called BotSetupLibrary BEFORE reading
 * maxclients, so its internal `GetClearedMemory(4560 * maxclients)` allocated
 * 0 bytes; we patched that with a Free+realloc hack.  The faithful order
 * eliminates that hack: BotSetupLibrary now sees the real maxclients.
 * ========================================================================= */
int Export_BotSetupLibrary(void)
{
    int errno_val;

    if (botlibsetup) {
        bi_Print(3, "bot library already setup\n");
        return 2;  /* BLERR_LIBRARYALREADYSETUP */
    }

    /* Open botlib.log (no-op unless cvar `log` is non-zero) — 0x10037BD6 */
    Log_Open("botlib.log");

    /* Banner output (matches original at 0x10037BDB/BE2/BEF) */
    bi_Print(1, "------- BotLib Initialization -------\n");
    bi_Print(1, "BotLib v0.96\n");

    Swap_Init();                  /* 0x100439F0: little/big-endian fp init */
    botlibsetup = 1;              /* 0x10037C04 */

    maxclients  = (int)LibVarValue("maxclients",  (int)"4");
    maxentities = (int)LibVarValue("maxentities", (int)"1024");

    BotSetupMoveAI();             /* 0x10037A00 */

    errno_val = sub_1000EDC0(maxentities, maxclients);  /* 0x1000EDC0 */
    if (errno_val) return errno_val;

    errno_val = BotSetupLibrary();  /* 0x10029C90: configs + bot-state alloc */
    if (errno_val) return errno_val;

    EA_Setup();                   /* 0x10037660: ea_controls = GetClearedMemory(36 * maxclients) */

    bi_Print(1, "-------------------------------------\n");
    return 0;
}

/* =========================================================================
 * SLOT 2 — BotShutdownLibrary: shut down the bot library.
 * Wrapper at 0x10037CF0 checks botlibsetup then calls BotShutdownLibrary().
 * ========================================================================= */
int Export_BotShutdownLibrary(void)
{
    if (!botlibsetup) return 1;
    return BotShutdownLibrary();
}

/* =========================================================================
 * SLOT 3 — BotLibraryInitialized: returns non-zero if library is ready.
 * Wrapper at 0x10037D80 calls AAS_Initialized() which returns aasworld_initialized.
 * ========================================================================= */
int Export_BotLibraryInitialized(void)
{
    return AAS_Initialized();
}

/* =========================================================================
 * SLOT 4 — BotLibVarSet: set a library variable from a string value.
 * Wrapper at 0x10037DA0 calls sub_10038AC0(a1, a2).
 * ========================================================================= */
int Export_BotLibVarSet(char *var_name, char *value)
{
    if (!var_name || !value) return 0;
    /* a2 MUST be a char* — the body does strlen/strcpy on it.
       Do NOT atoi(); pass the string pointer directly. */
    sub_10038AC0(var_name, value);
    return 1;
}

/* =========================================================================
 * SLOT 5 — BotDefine: add a preprocessor #define for script parsing.
 * Wrapper at 0x10037DD0 calls PC_AddGlobalDefine via thunk 0x10001B77 → 0x1003B4A0.
 * On failure prints "couldn't add define %s" via PRT_ERROR (=3).  Always
 * returns 0 to caller (xor eax,eax; ret).
 * Faithful transcription of the binary disassembly. */
extern int PC_AddGlobalDefine(const char *string);
int Export_BotDefine(char *string)
{
    if (!PC_AddGlobalDefine(string))
        bi_Print(3, "couldn't add define %s\n", string);
    return 0;
}

/* =========================================================================
 * SLOT 6 — BotLoadMap: load BSP + AAS data for a new map.
 *
 * Faithful transcription of the wrapper at 0x10037E10.  Two branches:
 *
 *   mapname == NULL : just calls BotLoadMap (which forwards to sub_1000DCC0,
 *                     the unload path) and returns its result.  No banners,
 *                     no sub_10029C10.
 *
 *   mapname != NULL : 1. bi_Print(1, "------------ Map Loading ------------\n")
 *                     2. result = BotLoadMap(mapname, ...)
 *                        - stored into errno (call _errno; *errno = eax)
 *                     3. if (errno != 0) return errno
 *                     4. sub_10029C10()  (BotResetState/BotInitLevelItems/…)
 *                     5. bi_Print(1, "-------------------------------------\n")
 *                     6. return 0
 * ========================================================================= */
extern int sub_10029C10(void);
int Export_BotLoadMap(char *mapname, int modelindexes, char **modelindex,
                      int soundindexes, char **soundindex,
                      int imageindexes, char **imageindex)
{
    int result;
    if (!botlib_is_setup("BotLoadMap")) return 1;
    if (!mapname) {
        return BotLoadMap(0, modelindexes, (int)modelindex,
                          soundindexes, (int)soundindex,
                          imageindexes, (int)imageindex);
    }
    bi_Print(1, "------------ Map Loading ------------\n");
    result = BotLoadMap((int)mapname, modelindexes, (int)modelindex,
                        soundindexes, (int)soundindex,
                        imageindexes, (int)imageindex);
    if (result) return result;
    sub_10029C10();
    bi_Print(1, "-------------------------------------\n");
    return 0;
}

/* =========================================================================
 * SLOT 7 — BotSetupClient: per-bot initialisation.
 * Wrapper at 0x10037F00 validates setup + client, then calls BotSetupClient().
 * ========================================================================= */
int Export_BotSetupClient(int client, void *settings)
{
    if (!botlib_is_setup("BotSetupClient")) return 1;
    return BotSetupClient(client, settings);
}

/* =========================================================================
 * SLOT 8 — BotShutdownClient: per-bot teardown.
 * Wrapper at 0x10037F70 validates then calls BotShutdownClient().
 * ========================================================================= */
int Export_BotShutdownClient(int client)
{
    if (!botlib_is_setup("BotShutdownClient")) return 1;
    if (!valid_client(client, "BotShutdownClient")) return 3;
    return BotShutdownClient(client);
}

/* =========================================================================
 * SLOT 9 — BotMoveClient: reassign bot to a new client slot.
 * Wrapper at 0x10037FE0 validates both slots, then calls BotMoveClient().
 * ========================================================================= */
int Export_BotMoveClient(int oldclnum, int newclnum)
{
    if (!botlib_is_setup("BotMoveClient")) return 1;
    if (!valid_client(oldclnum, "BotMoveClient, parm0")) return 3;
    if (!valid_client(newclnum, "BotMoveClient, parm1")) return 3;
    return BotMoveClient(oldclnum, newclnum);
}

/* =========================================================================
 * SLOT 10 — BotClientSettings: update client name/skin settings.
 * Wrapper at 0x10038070 validates, then calls BotClientSettings().
 * ========================================================================= */
int Export_BotClientSettings(int client, void *settings)
{
    if (!botlib_is_setup("BotClientSettings")) return 1;
    if (!valid_client(client, "BotClientSettings")) return 3;
    return BotClientSettings(client, settings);
}

/* =========================================================================
 * SLOT 11 — BotSettings: update bot personality/skill settings.
 * Wrapper at 0x100380E0 validates, then calls BotSettings() (sub_100299D0).
 * This is a DIFFERENT body from BotClientSettings (slot 10): it copies 432
 * bytes of bot-personality data into bot_states[client] + 0x4D8, but only
 * for clients with bot_states[client][0] != 0 (i.e. real bots).  For
 * non-bot clients it warns "tried to update settings of inactive client".
 * The deobfuscator originally swapped slots 10/11 names — restored here. */
int Export_BotSettings(int client, void *settings)
{
    if (!botlib_is_setup("BotSettings")) return 1;
    if (!valid_client(client, "BotSettings")) return 3;
    return BotSettings(client, settings);
}

/* =========================================================================
 * SLOT 13 — BotUpdateClient: per-frame client state update.
 * Wrapper at 0x10038190 validates, then calls BotUpdateClient().
 * NOTE: We renamed sub_10029880 to BotUpdateClient — this is the correct body.
 * ========================================================================= */
int Export_BotUpdateClient(int client, void *buc)
{
    if (!botlib_is_setup("BotUpdateClient")) return 1;
    if (!valid_client(client, "BotUpdateClient")) return 3;
    return BotUpdateClient(client, buc);
}

/* =========================================================================
 * SLOT 14 — BotUpdateEntity: per-frame entity state update.
 * Wrapper at 0x10038200 validates entity, then calls AAS_UpdateEntity().
 * AAS_UpdateEntity (0x1000A920) was already correctly identified.
 * ========================================================================= */
int Export_BotUpdateEntity(int ent, void *bue)
{
    /* AAS_UpdateEntity takes (int entnum, float *state) */
    return AAS_UpdateEntity(ent, (float *)bue);
}

/* =========================================================================
 * SLOT 15 — BotAddSound: feed a sound event to the bot's awareness system.
 * Wrapper at 0x10038270 validates entity, then calls sub_1001CE20().
 * sub_1001CE20 (ex-sub_1001CE20, ex-AAS_CheckSoundIndex) is the real body.
 * ========================================================================= */
int Export_BotAddSound(int *origin, int ent, int channel, int soundindex,
                       float volume, float attenuation, float timeofs)
{
    if (!botlib_is_setup("BotUpdateSound")) return 1;
    return sub_1001CE20((int)origin, ent, channel, soundindex,
                          (int)(*(int*)&volume),
                          (int)(*(int*)&attenuation),
                          timeofs);
}

/* =========================================================================
 * SLOT 16 — BotAddPointLight: feed a dynamic light event to the bot system.
 * Wrapper at 0x100382F0 validates entity, then calls BotAddPointLight().
 * BotAddPointLight (ex-sub_1000D550) tracks BSP lights for bot awareness.
 * ========================================================================= */
int Export_BotAddPointLight(int *origin, int ent, float radius,
                             float r, float g, float b,
                             float time, float decay)
{
    if (!botlib_is_setup("BotAddPointLight")) return 1;
    return BotAddPointLight((_DWORD *)origin, ent,
                            (int)(*(int*)&radius),
                            (int)(*(int*)&r), (int)(*(int*)&g), (int)(*(int*)&b),
                            (int)(*(int*)&time), (int)(*(int*)&decay));
}

/* =========================================================================
 * SLOT 19 — Test: debug/test function.
 * Impl at 0x10038460: `return 0;`
 * ========================================================================= */
int Export_Test(int parm0, char *parm1, float *parm2, float *parm3)
{
    (void)parm0; (void)parm1; (void)parm2; (void)parm3;
    return 0;
}
