/*
 * qshared_shim.c — compile game/q_shared.c into the botlib build so its
 * pure Q2 helpers (vector math, COM_* file utilities, Info_*, string,
 * etc.) live exactly once in the codebase.
 *
 * The original 1999 gladiator.dll was statically linked with q_shared.c,
 * so the reconstructions of those helpers that previously lived inside
 * botlib/botlib.c were *re-implementations* of the same source.  This
 * shim brings the canonical game/q_shared.c into the botlib link
 * instead.
 *
 * Seven symbols overlap with botlib's augmented swap-dispatch
 * infrastructure (botlib's LittleShort/etc. route through the
 * dword_100637CC..E0 table set up by botlib's own Swap_Init).  We
 * rename q_shared.c's versions out of the way here so the linker keeps
 * botlib's authoritative ones.  Without touching game source.
 */
#define C_ONLY  /* force id386=0 so q_shared.c's MSVC __asm blocks compile out */

#define Swap_Init       Swap_Init_qshared_unused
#define LittleShort     LittleShort_qshared_unused
#define LittleLong      LittleLong_qshared_unused
#define LittleFloat     LittleFloat_qshared_unused
#define BigShort        BigShort_qshared_unused
#define BigLong         BigLong_qshared_unused
#define BigFloat        BigFloat_qshared_unused

#include "../game/q_shared.c"

/* q_shared.c's Com_sprintf / Info_RemoveKey / Info_SetValueForKey error
 * paths call Com_Printf for diagnostics.  In game.dll that resolves to
 * the engine print wrapper.  Inside botlib we route it to bi_Print
 * (PRT_WARNING, level 2). */
#include <stdio.h>
#include <stdarg.h>
extern int (*bi_Print)(int level, const char *fmt, ...);
void Com_Printf(char *fmt, ...)
{
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (bi_Print) bi_Print(2, "%s", buf);
}
