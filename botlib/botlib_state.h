#ifndef BOTLIB_STATE_H
#define BOTLIB_STATE_H

/* =========================================================================
 * botlib_state.h — the three contiguous .data/.bss blocks that the original
 * Gladiator botlib interface TU laid out as single aggregates, and which
 * GetBotAPI (@0x10038480) and Export_BotShutdownLibrary (@0x10037CF0)
 * address as whole blocks via rep movs / rep stos:
 *
 *   block 3  bot_exports @0x10063F80  20 dwords  bot_export_t (the API table)
 *   block 2  botimport   @0x10063FE0  10 dwords  engine import callbacks
 *   block 1  botstate    @0x10064020  20 dwords  setup flag + counts + libvars
 *
 * IDA decompiled each field as a separate dword_/bi_ symbol; in our single
 * botlib.c TU those scatter across .bss, so GetBotAPI's bulk import copy and
 * Export_BotShutdownLibrary's bulk clears compiled to dozens of individual
 * mov stores instead of the original rep movs / rep stos.  Restoring them as
 * the original aggregates reproduces the bulk copy/clear AND is 64-bit-correct
 * (the copy/clear is sizeof-based, so the 8-byte pointers on aarch64 are
 * handled automatically — no #if split needed).  The #define field aliases
 * keep every existing IDA-era call site (bi_Print, maxclients, …) unchanged;
 * struct members are lvalues, so both reads and writes work verbatim.
 *
 * Shared by botlib.c (defines the storage) and botlib_exports.c.
 * ========================================================================= */

/* NB: ../game/botlib.h (bot_export_t) has no include guard, so it is NOT
 * re-included here — both includers (botlib.c, botlib_exports.c) include it
 * before this header.  ea_state.h / libvar.h are guarded, so re-including
 * them here is safe and keeps this header self-sufficient for those types. */
#include "ea_state.h"        /* ea_state_t (BotInput arg type) */
#include "libvar.h"          /* libvar_t   (block-1 handles)   */

/* ---- block 2: engine import callbacks (botimport, @0x10063FE0) -----------
 * 10 function pointers.  Field types match the original per-call-site tuning
 * (notably Trace's explicit-retbuf ABI and Print's int return); the layout is
 * 10 pointers either way.  DebugLineDelete (slot 9) was previously missing —
 * its slot is restored here so the block is the full 10 dwords the import
 * copy/clear span. */
typedef struct botimport_block_s {
    void  (__cdecl *BotInput)(int, ea_state_t *);
    int   (__cdecl *BotClientCommand)(int client, char *str, ...);
    int   (*Print)(_DWORD, const char *, ...);
    void *(__cdecl *Trace)(void *retbuf, float *start, float *mins, float *maxs,
                           float *end, int passent, int contentmask);
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

/* ---- field aliases: keep IDA-era call sites byte-for-byte unchanged ------ */
#define dword_10063FE0       botimport.BotInput
#define bi_BotClientCommand  botimport.BotClientCommand
#define bi_Print             botimport.Print
#define bi_Trace             botimport.Trace
#define bi_PointContents     botimport.PointContents
#define bi_GetMemory         botimport.GetMemory
#define bi_FreeMemory        botimport.FreeMemory
#define bi_DebugLineCreate   botimport.DebugLineCreate
#define bi_DebugLineShow     botimport.DebugLineShow

#define botlibsetup          botstate.setup
#define maxentities          botstate.num_entities
#define maxclients           botstate.num_clients
#define dword_1006402C       botstate.bottime
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

#endif /* BOTLIB_STATE_H */
