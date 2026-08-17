/* be_interface.h — interface of be_interface.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_INTERFACE_H
#define BOTLIB_BE_INTERFACE_H

/* scriptcrc_t: the (filename, CRC) record this TU's list is made of. */
/* scriptcrc_t — record in the dword_10063F2C list holding a (filename,
 * CRC-hash) pair, so the engine can warn when a script's CRC changes. */
typedef struct scriptcrc_s {
    unsigned __int16     hash;            /* +0   CRC-16 of the file body  */
    char                 name[146];       /* +2   filename (zero-terminated) */
    struct scriptcrc_s  *next;            /* +148 on 32-bit; offset moves to +152 on 64-bit */
} scriptcrc_t;                            /* sizeof = 152 on 32-bit, 160 on 64-bit */

/* botimport / botstate / bot_exports and the libvar handle macros -- this TU defines all of them, and Q3's own be_interface.h declares the same set. */
/* The three contiguous .data/.bss blocks that GetBotAPI (@0x10038480) and
 * Export_BotShutdownLibrary (@0x10037CF0) address as whole aggregates via
 * rep movs / rep stos:
 *
 *   block 3  bot_exports @0x10063F80  20 dwords  bot_export_t (the API table)
 *   block 2  botimport   @0x10063FE0  10 dwords  engine import callbacks
 *   block 1  botstate    @0x10064020  20 dwords  setup flag + counts + libvars
 *
 * Keeping them as aggregates (rather than the decompiler's scattered dword_/bi_
 * symbols) is what makes those bulk copies/clears compile back to rep movs /
 * rep stos, and is 64-bit-correct since they are sizeof-based.  Call sites use
 * botimport.* / botstate.* members directly.
 *
 * ../game/botlib.h has no include guard and is NOT re-included here — both
 * includers pull it in first. */

/* ---- block 2: engine import callbacks (botimport, @0x10063FE0) -----------
 * 10 function pointers; note Print's int return.
 *
 * `Trace` returns bsp_trace_t BY VALUE, which is what `gladq2_src/bl_main.c`'s
 * BotLibImport_Trace is declared as.  On 32-bit that is not a choice between
 * two ABIs -- it IS the retbuf ABI, just spelled in C: both compilers pass a
 * hidden buffer pointer as the first argument and return it in eax.  Measured
 * (probe_cl.sh, 2026-08-16): MSVC6 /O2 compiles `return f(a..)` from the
 * by-value declaration and `return *f(&local, a..)` from an explicit-retbuf
 * one to BYTE-IDENTICAL code, and that code is gladiator.dll's AAS_Trace at
 * 0x10003010 instruction for instruction.  gcc 2.7.2.3 is NOT indifferent: it
 * forwards the caller's own retbuf for the by-value form (74 B, = F663) and
 * materialises a local plus a `rep movs` for the explicit one (+7 insns).
 * So by-value is the spelling that reproduces both originals.
 *
 * 64-bit is the exception and needs the explicit form: aarch64/x86-64 pass the
 * struct-return buffer in a hidden register (x8 / rax), not as a visible
 * argument, so the game side hands us one explicitly instead --
 * `game/bl_main.c` has the matching 64-bit BotLibImport_Trace.  Do not
 * "simplify" the two branches together. */
typedef struct botimport_block_s {
    void  (__cdecl *BotInput)(int, ea_state_t *);
    int   (__cdecl *BotClientCommand)(int client, char *str, ...);
    int   (*Print)(_DWORD, const char *, ...);
#if defined(__x86_64__) || defined(__aarch64__)
    bsp_trace_t *(__cdecl *Trace)(bsp_trace_t *retbuf, vec3_t start, vec3_t mins,
                                  vec3_t maxs, vec3_t end, int passent, int contentmask);
#else
    bsp_trace_t (__cdecl *Trace)(vec3_t start, vec3_t mins, vec3_t maxs,
                                 vec3_t end, int passent, int contentmask);
#endif
    int   (__cdecl *PointContents)(float *point);
    void *(__cdecl *GetMemory)(int);
    void  (__cdecl *FreeMemory)(void *);
    int   (*DebugLineCreate)(void);
    int   (__cdecl *DebugLineDelete)(int line);
    int   (__cdecl *DebugLineShow)(int, float *, float *, int);
} botimport_block_t;

/* ---- block 1: interface state (botstate, @0x10064020) --------------------
 * setup flag + entity/client counts + bottime + the 16 movement libvar handles, in
 * original memory order (4 ints then 16 pointers = 20 dwords). */
typedef struct botstate_block_s {
    int       setup;        /* botlibsetup    @0x10064020 */
    int       num_entities; /* maxentities    @0x10064024 */
    int       num_clients;  /* maxclients     @0x10064028 */
    int       bottime;      /* dword_1006402C @0x1006402C */
    libvar_t *libvars[16];  /* sv_friction .. sv_maxwaterjump @0x10064030.. */
} botstate_block_t;

extern botimport_block_t botimport;
extern botstate_block_t  botlibglobals;
extern bot_export_t      botexport;  /* block 3 @0x10063F80 */

/* ---- libvar handle aliases (for brevity at call sites) ------------------ */
#define libvar_sv_friction          botlibglobals.libvars[0]
#define libvar_sv_stopspeed         botlibglobals.libvars[1]
#define libvar_sv_gravity           botlibglobals.libvars[2]
#define libvar_sv_waterfriction     botlibglobals.libvars[3]
#define libvar_sv_watergravity      botlibglobals.libvars[4]
#define libvar_sv_maxvelocity       botlibglobals.libvars[5]
#define libvar_sv_maxwalkvelocity   botlibglobals.libvars[6]
#define libvar_sv_maxcrouchvelocity botlibglobals.libvars[7]
#define libvar_sv_maxswimvelocity   botlibglobals.libvars[8]
#define libvar_sv_maxaccelerate     botlibglobals.libvars[9]
#define libvar_sv_airaccelerate     botlibglobals.libvars[10]
#define libvar_sv_step              botlibglobals.libvars[11]
#define libvar_sv_maxbarrier        botlibglobals.libvars[12]
#define libvar_sv_maxsteepness      botlibglobals.libvars[13]
#define libvar_sv_jumpvel           botlibglobals.libvars[14]
#define libvar_sv_maxwaterjump      botlibglobals.libvars[15]



/* Declarations for what this TU defines — last, so the types above are in scope. */
extern int filecrcs[]; /* CRC16 weapon table (92 entries × 8 bytes) — defined in botlib_structdefs.c */
/* NOT a variable and has no original name to recover: 0x1005E958 is exactly
 * filecrcs + 736, i.e. the one-past-the-end address MSVC folded into the scan loop's
 * bound as a link-time constant, and IDA had to invent a symbol for it.  The DLL holds
 * only zero fill there, and the Linux .so — whose .dynsym kept every real global — has
 * no symbol at the corresponding offset either.  Do not chase a name for it. */
/* `logfile` and the seven Log_* functions live in their own TU: botlib/l_log.c
 * (l_log.obj, DLL 0x10038BE0..0x10038F0F -- see .claude/memory/tu_partition.md). */
/* `libvarlist` and the thirteen LibVar* functions live in their own TU:
 * botlib/l_libvar.c (DLL 0x10038750..0x10038BDF). */
extern struct scriptcrc_s *dumpcrcs;
/* The three interface blocks are typed aggregates so that GetBotAPI's import copy and
 * Export_BotShutdownLibrary's clears compile back to the original rep movs / rep stos.
 * Storage is defined here; call sites use the botimport.* / botstate.* members. */
extern botimport_block_t botimport;
extern botstate_block_t botlibglobals;
extern bot_export_t botexport;

qboolean __cdecl BotLibSetup(const char *str);
void BotSetupMoveAI();
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
void __cdecl sub_100376B0(char *String1, unsigned __int16);
int __cdecl sub_100377E0(char *String1, unsigned __int16);
int __cdecl sub_10037820(char *name, const unsigned char *buf, int len);
int __cdecl sub_10037850(char *String1, const unsigned char *, int);
void __cdecl sub_10037880(void);

#endif /* BOTLIB_BE_INTERFACE_H */
