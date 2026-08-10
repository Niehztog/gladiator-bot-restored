/*
 * botlib.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * Naming: a function is renamed only where a unique string in its body matches
 * Q3 botlib or the implementation is structurally identical; otherwise it keeps
 * its sub_XXXXXXXX address name.  Most 0x10001xxx symbols are 5-byte JMP
 * thunks, named for the function they jump to.
 *
 * See CLAUDE.md and .claude/memory/ for the methodology and the 32->64-bit
 * side-band scheme (BOTLIB_NEED_SIDEBAND, below).
 */

#include "botlib_local.h"
#include "be_ai2_main.h"




float flt_10062984 = 0.0; // weak
float flt_10062988 = 0.0; // weak
float flt_1006298C = 0.0; // weak
float flt_1006319C; // weak
float flt_100631A0; // weak
float flt_100631A8; // weak
int dword_10063388; // weak


#if BOTLIB_NEED_SIDEBAND
#else
#endif
















//----- (100426B0) --------------------------------------------------------
/* AngleVectors — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10042860) --------------------------------------------------------
/* ProjectPointOnPlane — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10042920) --------------------------------------------------------
/* PerpendicularVector — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (100431B0) --------------------------------------------------------
/* ClearBounds — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (100431F0) --------------------------------------------------------
/* AddPointToBounds — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043240) --------------------------------------------------------
/* VectorCompare — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043290) --------------------------------------------------------
/* VectorNormalize — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043380) --------------------------------------------------------
/* VectorMA — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (100433D0) --------------------------------------------------------
/* _DotProduct — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043480) --------------------------------------------------------
/* _VectorCopy — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (100434B0) --------------------------------------------------------
/* CrossProduct — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043500) --------------------------------------------------------
/* VectorLength — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043540) --------------------------------------------------------
float *__cdecl VectorNegate(float *v)
{
  float *result; // eax

  result = v;
  *v = -*v;
  v[1] = -v[1];
  v[2] = -v[2];
  return result;
}

//----- (10043570) --------------------------------------------------------
/* VectorScale — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043640) --------------------------------------------------------
/* COM_FileExtension — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (100436B0) --------------------------------------------------------
/* COM_FileBase — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043790) --------------------------------------------------------
/* COM_DefaultExtension — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043810..0x100439F0) ---------------------------------------------
/* Byte-order subsystem (the BigShort/LittleShort/… dispatchers, the
 * Swap/NoSwap helpers and Swap_Init, with fn-ptr slots dword_100637CC..E0 and
 * `bigendien`) lives in q_shared.c — Mr. Elusive's lcc.mak builds q_shared.obj
 * as its own object; declarations are in game/q_shared.h. */

//----- (10043C40) --------------------------------------------------------
/* Q_strncasecmp — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043CC0) --------------------------------------------------------
/* Q_stricmp — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043CF0) --------------------------------------------------------
/* Com_sprintf — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043EA0) --------------------------------------------------------
/* Info_RemoveKey — pure Q2 q_shared.c helper; body lives in game/q_shared.c — compiled separately to q_shared.o */

//----- (10043FC0) --------------------------------------------------------
/* Info_Validate's body lives in game/q_shared.c.  This declaration exists only
 * so the oracle funcmap parser attributes 0x10043FC0 here rather than letting
 * the marker leak onto the trailing Com_Printf stub. */
qboolean Info_Validate(char *s);

// nfuncs=1767 queued=667 decompiled=667 lumina nreq=0 worse=0 better=0
// Note: MSVC CRT functions that were statically compiled into the DLL have been removed
// (addresses 10044286–10052E44): sub_10044286, fread_locked, fread, remove_file,
// getcwd_locked, sub_10045A05, sub_10049D0E, ld12_to_double, ld12_to_float,
// sub_1004C7A7, sub_1004C802, sub_1004D4BC, sub_1004EFD8, SpawnProcess.
// These are provided by the standard C library via the headers included above.
// 2 decompilation failures (see the #error stubs above)

/* Com_Printf @0x10042410 — a single-byte `ret` in the original, so q_shared.c's
 * Com_sprintf / Info_* diagnostics silently drop.  Keep the empty body: routing
 * to bi_Print would be more chatty than the original.  It is a botlib-side stub
 * because lcc.mak builds q_shared.obj as its own object. */
void Com_Printf(char *msg, ...) { (void)msg; }
