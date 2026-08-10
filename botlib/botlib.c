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

#include "botlib_local.h"   /* the shared environment + all prototypes; see that file */


__int16 word_1005E498 = 45; // weak

CHAR FileName[] = "UNZIP32.DLL"; // idb
_UNKNOWN unk_10061280; // weak
_UNKNOWN unk_10061298; // weak
char byte_1006294C = '\0'; // idb
#ifdef _WIN32
LPDCL dword_1006296C = NULL; /* locked DCL option block — UnZip windll, Windows-only */
#endif
float flt_10062984 = 0.0; // weak
float flt_10062988 = 0.0; // weak
float flt_1006298C = 0.0; // weak
float flt_1006319C; // weak
float flt_100631A0; // weak
float flt_100631A8; // weak
int dword_10063388; // weak
#ifdef _WIN32
LPUSERFUNCTIONS dword_100639F0; /* locked USERFUNCTIONS callback table — UnZip windll, Windows-only */
#endif
botimport_block_t botimport;   /* block 2 @0x10063FE0 — engine import callbacks */
botstate_block_t  botstate;    /* block 1 @0x10064020 — setup flag + counts + libvars */
bot_export_t      bot_exports; /* block 3 @0x10063F80 — exported API table */
int numbots; /* active-bot count, ++/-- in BotSetupClient/BotShutdownClient, returned by
                NumBots(); name recovered from the tourney-2.5 Linux gladi386.so .dynsym */
bsp_entity_t *dword_10064398; // BSP entity list head (parsed by AAS_ParseBSPEntities)
#if BOTLIB_NEED_SIDEBAND
bot_character_t **botcharacters;
#else
#endif

#if BOTLIB_NEED_SIDEBAND
void **botgoalstate_p0;  /* weightconfig_t* */
void **botgoalstate_p1;  /* iteminfo weight table */
#else
#endif

#if BOTLIB_NEED_SIDEBAND
#else
#endif

#if BOTLIB_NEED_SIDEBAND
bot_weaponstate_t **botweaponstates;
#else
#endif

#if BOTLIB_NEED_SIDEBAND
void **botchatdumps;
#else
#endif

#if BOTLIB_NEED_SIDEBAND
chatmsg_links_t *botchatmsglinks;
#else
#endif

#if BOTLIB_NEED_SIDEBAND
ai_node_fn_t *botainodes;
#else
#endif

#if BOTLIB_NEED_SIDEBAND
bot_waypoint_t **botcheckpoints;
bot_waypoint_t **botpatrolpoints;
bot_waypoint_t **botcurpatrolpoint;
#else
#endif

#if BOTLIB_NEED_SIDEBAND
aas_link_t **aasentity_arealinks;
bsp_link_t **aasentity_bsplinks;
#else
#endif





bot_clientsettings_t *clientsettings; /* per-client {netname[16], skin[128]} = 144 B; name recovered
                                          from the tourney-2.5 Linux gladi386.so .dynsym (unstripped
                                          data symbols) */
libvar_t *libvar_ctf; /* libvar handle */
bot_goal_t ctf_blueflag; /* 0x100643E0 blue flag goal (ai_dmq3.c; was unk_100643E0) */
bot_goal_t ctf_redflag;  /* 0x10064420 red flag goal  (ai_dmq3.c; was unk_10064420) */
libvar_t *libvar_usehook; /* libvar handle */
libvar_t *libvar_runes; /* libvar handle */
libvar_t *libvar_rocketjump; /* libvar handle */


aas_world_t aasworld;

int numplanes;            // 0x100674F0  (was dword_100674F0)
int numvertexes;          // 0x100674F8  (was dword_100674F8)
int numnodes;             // 0x10067500  (was dword_10067500)
int numfaces;             // 0x10067510  (was dword_10067510)
int numedges;             // 0x10067518  (was dword_10067518)
int numareas;             // 0x10067548  (was dword_10067548)


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
