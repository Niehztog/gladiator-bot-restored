/*
 * be_interface.h — interface of be_interface.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_INTERFACE_H
#define BOTLIB_BE_INTERFACE_H

/* scriptcrc_t: the (filename, CRC) record this TU's list is made of. */
/* scriptcrc_t — record in the dword_10063F2C list holding a (filename,
 * CRC-hash) pair, so the engine can warn when a script's CRC changes. */
typedef struct scriptcrc_s {
    __int16              hash;            /* +0   CRC-16 of the file body  */
    char                 name[146];       /* +2   filename (zero-terminated) */
    struct scriptcrc_s  *next;            /* +148 on 32-bit; offset moves to +152 on 64-bit */
} scriptcrc_t;                            /* sizeof = 152 on 32-bit, 160 on 64-bit */

/* botimport / botstate / bot_exports and the libvar handle macros -- this TU defines all of them, and Q3's own be_interface.h declares the same set. Was botlib_state.h until 2026-08-10. */
/* botlib_state.h — the three contiguous .data/.bss blocks that GetBotAPI
 * (@0x10038480) and Export_BotShutdownLibrary (@0x10037CF0) address as whole
 * aggregates via rep movs / rep stos:
 *
 *   block 3  bot_exports @0x10063F80  20 dwords  bot_export_t (the API table)
 *   block 2  botimport   @0x10063FE0  10 dwords  engine import callbacks
 *   block 1  botstate    @0x10064020  20 dwords  setup flag + counts + libvars
 *
 * Keeping them as aggregates (rather than the decompiler's scattered
 * dword_/bi_ symbols) is what makes those bulk copies/clears compile back to
 * rep movs / rep stos, and is 64-bit-correct since they are sizeof-based.
 * Call sites use botimport.* / botstate.* members directly.
 *
 * Shared by botlib.c (which defines the storage) and botlib_exports.c.
 * ../game/botlib.h has no include guard and is NOT re-included here — both
 * includers pull it in first. */

/* ---- block 2: engine import callbacks (botimport, @0x10063FE0) -----------
 * 10 function pointers; note Trace's explicit-retbuf ABI and Print's int
 * return. */
typedef struct botimport_block_s {
    void  (__cdecl *BotInput)(int, ea_state_t *);
    int   (__cdecl *BotClientCommand)(int client, char *str, ...);
    int   (*Print)(_DWORD, const char *, ...);
    bsp_trace_t *(__cdecl *Trace)(bsp_trace_t *retbuf, vec3_t start, vec3_t mins,
                                  vec3_t maxs, vec3_t end, int passent, int contentmask);
    int   (__cdecl *PointContents)(float *point);
    void *(__cdecl *GetMemory)(int);
    void  (__cdecl *FreeMemory)(void *);
    int   (*DebugLineCreate)(void);
    int   (__cdecl *DebugLineDelete)(int line);
    int   (__cdecl *DebugLineShow)(int, float *, float *, int);
} botimport_block_t;

/* ---- block 1: interface state (botstate, @0x10064020) --------------------
 * setup flag + entity/client counts + bottime + the 16 movement libvar
 * handles, in original memory order (4 ints then 16 pointers = 20 dwords). */
typedef struct botstate_block_s {
    int       setup;        /* botlibsetup    @0x10064020 */
    int       num_entities; /* maxentities    @0x10064024 */
    int       num_clients;  /* maxclients     @0x10064028 */
    int       bottime;      /* dword_1006402C @0x1006402C */
    libvar_t *libvars[16];  /* sv_friction .. sv_maxwaterjump @0x10064030.. */
} botstate_block_t;

extern botimport_block_t botimport;
extern botstate_block_t  botstate;
extern bot_export_t      bot_exports;  /* block 3 @0x10063F80 */

/* ---- libvar handle aliases (for brevity at call sites) ------------------ */
#define libvar_sv_friction          botstate.libvars[0]
#define libvar_sv_stopspeed         botstate.libvars[1]
#define libvar_sv_gravity           botstate.libvars[2]
#define libvar_sv_waterfriction     botstate.libvars[3]
#define libvar_sv_watergravity      botstate.libvars[4]
#define libvar_sv_maxvelocity       botstate.libvars[5]
#define libvar_sv_maxwalkvelocity   botstate.libvars[6]
#define libvar_sv_maxcrouchvelocity botstate.libvars[7]
#define libvar_sv_maxswimvelocity   botstate.libvars[8]
#define libvar_sv_maxaccelerate     botstate.libvars[9]
#define libvar_sv_airaccelerate     botstate.libvars[10]
#define libvar_sv_step              botstate.libvars[11]
#define libvar_sv_maxbarrier        botstate.libvars[12]
#define libvar_sv_maxsteepness      botstate.libvars[13]
#define libvar_sv_jumpvel           botstate.libvars[14]
#define libvar_sv_maxwaterjump      botstate.libvars[15]

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
