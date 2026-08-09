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
char word_1005F588[] = { '\"', '\0' }; // idb

CHAR aWindllUnzip[] = "windll_unzip"; // idb
CHAR FileName[] = "UNZIP32.DLL"; // idb
_UNKNOWN unk_10061280; // weak
_UNKNOWN unk_10061298; // weak
char byte_1006294C = '\0'; // idb
libvar_t *libvar_framereachability; /* cached LibVar handle (was libvar_framereachability) */
libvar_t *libvar_reachabilitydelay; /* cached LibVar handle (was libvar_reachabilitydelay) */
int dword_1006295C = 0; // weak
libvar_t *libvar_laserhook; /* libvar handle */
HGLOBAL dword_10062968 = NULL; // idb
#ifdef _WIN32
LPDCL dword_1006296C = NULL; /* locked DCL option block — UnZip windll, Windows-only */
#endif
HGLOBAL dword_10062970 = NULL; // idb
HGLOBAL hMem = NULL; // idb
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
int (__stdcall *windll_unzip)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD); // weak
HMODULE hLibModule; // idb
define_t *globaldefines;
botimport_block_t botimport;   /* block 2 @0x10063FE0 — engine import callbacks */
botstate_block_t  botstate;    /* block 1 @0x10064020 — setup flag + counts + libvars */
bot_export_t      bot_exports; /* block 3 @0x10063F80 — exported API table */
ea_state_t *ea_controls; /* per-client EA state array, sized 36 * maxclients */
weaponconfig_t *weaponconfig; /* current weapon config (was dword_10064080) */
levelitem_t *freelevelitems; // 0x10064344 free-list head    (be_ai_goal.c; was dword_10064344)
int numlevelitems;           // 0x10064354 active item count  (be_ai_goal.c; was dword_10064354)
levelitem_t *levelitemheap;  // 0x10064358 pool base          (be_ai_goal.c; was dword_10064358)
itemconfig_t *itemconfig; /* current item config (was dword_1006435C) */
levelitem_t *levelitems;     // 0x10064360 active-list head   (be_ai_goal.c; was dword_10064360)
bot_consolemessage_t *freeconsolemessages; // 0x10064364 free-list head (be_ai_chat.c; was dword_10064364)
bot_consolemessage_t *consolemessageheap; // pool base (initial bulk allocation)
bot_matchtemplate_t *matchtemplates; // weak
bot_randomlist_t *randomstrings; // weak
bot_replychat_t *replychats; // weak
bot_synonymlist_t *synonyms; /* synonyms head, set by BotLoadSynonyms */
int numbots; /* active-bot count, ++/-- in BotSetupClient/BotShutdownClient, returned by
                NumBots(); name recovered from the tourney-2.5 Linux gladi386.so .dynsym */
bsp_entity_t *dword_10064398; // BSP entity list head (parsed by AAS_ParseBSPEntities)
int dword_1006439C; // weak
bot_state_t *botstates; // base array of maxclients bot states
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





float flt_100643A4; // weak
bot_clientsettings_t *clientsettings; /* per-client {netname[16], skin[128]} = 144 B; name recovered
                                          from the tourney-2.5 Linux gladi386.so .dynsym (unstripped
                                          data symbols) */
libvar_t *libvar_ctf; /* libvar handle */
bot_goal_t ctf_blueflag; /* 0x100643E0 blue flag goal (ai_dmq3.c; was unk_100643E0) */
bot_goal_t ctf_redflag;  /* 0x10064420 red flag goal  (ai_dmq3.c; was unk_10064420) */
libvar_t *libvar_usehook; /* libvar handle */
libvar_t *libvar_ch; /* libvar handle */
libvar_t *libvar_teamplay; /* libvar handle */
libvar_t *libvar_ra; /* libvar handle */
libvar_t *libvar_runes; /* libvar handle */
int dword_1006446C; // weak
libvar_t *libvar_dmflags; /* libvar handle */
libvar_t *libvar_nochat; /* libvar handle */
libvar_t *libvar_rocketjump; /* libvar handle */
libvar_t *libvar_fastchat; /* libvar handle */
libvar_t *libvar_assimilation; /* libvar handle */
int dword_10064484; // weak
libvar_t *libvar_teamplay_shell; /* libvar handle */
int dword_1006448C; // weak
int dword_10064490; // weak
int dword_10064494; // weak
int dword_10064498; // weak
int dword_1006449C; // weak
int numnodeswitches;     // 0x100644A0 (game ai_dmnet.c; was dword_100644A0)
char nodeswitch[7344];   // 0x10064A80 nodeswitch[MAX_NODESWITCHES+1=51][144] (ai_dmnet.c; was byte_10064A80)

int numclusterareas;     // 0x10066730 area count — stays int (was dword_10066730)
midrangearea_t *midrangeareas; // 0x10066740 (was dword_10066740)
int *clusterareas;             // 0x10066744 (was dword_10066744)
int numportalcacheupdates; // weak
int numareacacheupdates; // weak
aas_reachabilitynode_t **areareachability;   /* per-area linked-list-head array */
int reach_ladder; // weak
int reach_elevator; // weak
intptr_t reachabilityheap; // pool base
int reach_jump; // weak
int reach_grapple; // weak
int reach_waterjump; // weak
int reach_teleport; // weak
int reach_barrier; // weak
int reach_swim; // weak
int reach_equalfloor; // weak
intptr_t nextreachability; // free-list head
int reach_walkoffledge; // weak
int reach_rocketjump; // weak
int reach_step; // weak
int reach_walk; // weak

aas_world_t aasworld;

int numdebuglines;          // 0x10066B14 (was dword_10066B14)
int debuglinevisible[256];  // 0x10066CC0 (was dword_10066CC0)
int debuglines[256];        // 0x100670C0 (was dword_100670C0)
int dword_100674C0; // weak — "BSP loaded" guard flag (no l_bsp_q2.c cognate; left unnamed)
int nummodels;            // 0x100674C4  (was dword_100674C4)
dmodel_t *dmodels;        // 0x100674C8  (was dword_100674C8)
int visdatasize;          // 0x100674CC  (was dword_100674CC)
char *dvisdata;           // 0x100674D0  (was dword_100674D0)
dvis_t *dvis;             // 0x100674D4  dvis_t* alias of dvisdata (was dword_100674D4)
int lightdatasize;        // 0x100674D8  (was dword_100674D8)
char *dlightdata;         // 0x100674DC  (was dword_100674DC)
int entdatasize;          // 0x100674E0  (was dword_100674E0)
unsigned char *dentdata;  // 0x100674E4  (was dword_100674E4; was int in 32-bit binary)
int numleafs;             // 0x100674E8  (was dword_100674E8)
dleaf_t *dleafs;          // 0x100674EC  (was dword_100674EC)
int numplanes;            // 0x100674F0  (was dword_100674F0)
dplane_t *dplanes;        // 0x100674F4  (was dword_100674F4)
int numvertexes;          // 0x100674F8  (was dword_100674F8)
dvertex_t *dvertexes;     // 0x100674FC  (was dword_100674FC)
int numnodes;             // 0x10067500  (was dword_10067500)
dnode_t *dnodes;          // 0x10067504  (was dword_10067504)
int numtexinfo;           // 0x10067508  (was dword_10067508)
texinfo_t *texinfo;       // 0x1006750C  (was dword_1006750C)
int numfaces;             // 0x10067510  (was dword_10067510)
dface_t *dfaces;          // 0x10067514  (was dword_10067514)
int numedges;             // 0x10067518  (was dword_10067518)
dedge_t *dedges;          // 0x1006751C  (was dword_1006751C)
int numleaffaces;         // 0x10067520  (was dword_10067520)
unsigned short *dleaffaces; // 0x10067524  (was dword_10067524)
int numleafbrushes;       // 0x10067528  (was dword_10067528)
unsigned short *dleafbrushes; // 0x1006752C  (was dword_1006752C)
int numsurfedges;         // 0x10067530  (was dword_10067530)
int *dsurfedges;          // 0x10067534  (was dword_10067534)
int numbrushes;           // 0x10067538  (was dword_10067538)
dbrush_t *dbrushes;       // 0x1006753C  (was dword_1006753C)
int numbrushsides;        // 0x10067540  (was dword_10067540)
dbrushside_t *dbrushsides; // 0x10067544  (was dword_10067544)
int numareas;             // 0x10067548  (was dword_10067548)
darea_t *dareas;          // 0x1006754C  (was dword_1006754C)
int numareaportals;       // 0x10067550  (was dword_10067550)
dareaportal_t *dareaportals; // 0x10067554  (was dword_10067554)
char *dword_10067558; // per-face {short texturemins[2]; short extents[2]} table,
char *dword_1006755C; // pointer
char *dword_10067560; // pointer
char byte_10067564[8192]; // weak
int dword_10069564; // weak
int dword_10069568; // weak
float flt_1006956C; // weak
float flt_10069570; // weak
float flt_10069574; // weak
bsp_link_t  *dword_10069578; // bsp_linkheap (pool base)
int          dword_1006957C; // bsp_linkheapsize (count)
bsp_link_t  *dword_10069580; // bsp_freelinks (head of free list)
bsp_link_t **dword_10069584; // bsp_leaflinks (per-leaf list-heads array)


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
