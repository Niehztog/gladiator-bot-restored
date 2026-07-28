#ifndef BOTLIB_STATE_H
#define BOTLIB_STATE_H

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
#include "ea_state.h"        /* ea_state_t (BotInput arg type) */
#include "libvar.h"          /* libvar_t   (block-1 handles)   */

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

#endif /* BOTLIB_STATE_H */
