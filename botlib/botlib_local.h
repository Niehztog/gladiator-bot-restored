/*
 * botlib_local.h — the declarations shared by every botlib translation unit:
 * the project headers, the forward typedefs, and the internal structures that
 * more than one TU needs.  Q3 botlib's be_aas_def.h plays the same role.
 *
 * The build-environment scaffolding that has no 1999 counterpart moved to
 * botlib_port.h, included first below.  ONE port-only thing unavoidably
 * remains here: the 64-bit side-band accessor macros (BotCharacter, BotWS,
 * BotChatMsgLinks, AAS_EntAreaLink, ...).  Each wraps one specific structure
 * and has to sit next to it, so they cannot be lifted out without splitting
 * the structures they belong to.  They are all gated on BOTLIB_NEED_SIDEBAND,
 * which botlib_port.h defines.
 *
 * Function prototypes live in the per-TU headers (be_aas_reach.h, l_script.h,
 * ...), not here.
 */
#ifndef BOTLIB_LOCAL_H
#define BOTLIB_LOCAL_H

#include "botlib_port.h"


/* q_shared.h first, so its qboolean/vec3_t/cplane_t/… are the canonical types.
 * It sets Q_SHARED_H, which makes botlib.h skip its own Q2-type stubs. */
#include "../game/q_shared.h"
/* q_shared.h's VectorNegate is a 2-arg macro, but botlib has a 1-arg in-place
 * VectorNegate(v) function at 0x10043540.  Drop the macro so it stays callable. */
#undef VectorNegate
#include "../game/botlib.h"  /* bot_export_t, bot_import_t + prerequisite Q2 types */
#include "gladiator.dll.h"
#include "ea_state.h"    /* ea_state_t: 36-byte per-client EA struct (reconstructed) */
#include "aas_world.h"   /* aas_t, aas_area_t etc. (reconstructed from aasworld_* globals) */

#include "bot_state.h"   /* bot_state_t, BOT_* offset constants (reconstructed) */
#include "chat_state.h"  /* bot_match_t, bot_matchpiece_t etc. */
#include "libvar.h"      /* libvar_t: 24-byte botlib cvar (reconstructed) */
#include "botlib_state.h"   /* botimport / botstate / bot_exports + libvar aliases */
#include "botlib_structs.h" /* config, parser, weight, goal and item structs */
#include "struct_sizes_asserts.h" /* compile-time struct-layout guard */
#include "q2files.h"




/* Forward typedefs for the prototypes below; layouts come later. */
typedef struct bot_character_s    bot_character_t;   /* layout defined later; needed by Characteristic_* */
typedef struct bot_weaponstate_s  bot_weaponstate_t;
struct chatlist_s;
typedef struct chatlist_s         chatlist_t;
/* BotInitialChat records up to 10 (string,len) pairs for BotConstructChatMessage
 * to substitute as %v0..%v9.  The original entry is 8 bytes; widened on 64-bit
 * so the pointer survives. */
typedef struct bot_chatvar_s { char *str; int len; } bot_chatvar_t;

/* Export functions — defined in botlib_exports.c. */

/* Forward declarations for the functions defined below. */
int __cdecl PC_DollarEvaluate(source_t *source, int *intvalue, double *floatvalue, int integer); /* l_precomp.c: evaluates #if expression tokens */
int __cdecl PC_ReadLine(source_t *source, token_t *token);                       /* 2-param line reader */
int __cdecl sub_10041BA0(char *a1, char *Source, char *a3, bot_fileref_t *a4); /* search basePath+subdir+paks for file */
void AAS_InitTravelFlagFromType(void); /* sub_10018D00 */
bot_moveresult_t *__cdecl BotMoveToGoal(bot_moveresult_t *a1, bot_movestate_t *movestate, bot_goal_t *goal, int travelflags); /* 0x100343A0: build bot_moveresult_t for current goal */
int AAS_ContinueInitReachability(int a1); // caller passes arg but function body ignores it (no ebp frame)
int AAS_FreeRoutingCaches(void);  /* sub_10019550 */
int AAS_FreeAllPortalCache(void); /* sub_100193E0 */
libvar_t *__cdecl LibVar(char *var_name, char *value);            /* register/lookup libvar */
float     __cdecl LibVarValue(char *var_name, char *value);    /* register, return value */
int __cdecl AAS_BoxOnPlaneSide2(vec3_t absmins, vec3_t absmaxs, float *p);  /* Q3 canonical name */
void *AAS_AllocReachability(void);  /* sub_10010FF0 — pop AAS-link from free chain */
void __cdecl LibVarSet(char *var_name, char *value);  /* body at ~30304 */
void __cdecl AnglesToAxis(const vec3_t angles, float axis[3][3]);  // 0x100034D0; was sub_100034D0 (originally also mislabeled sub_100423B0)
int __cdecl sub_10006100(int *a1, int a2, float *a3);
int __cdecl AAS_DropToFloor(vec3_t origin, vec3_t mins, vec3_t maxs);  // 5-param: matches call sites
_DWORD sub_10010FF0();
int __cdecl AAS_AreaSwim(int areanum); /* AAS_AreaSwim impl */
int __cdecl AAS_AreaGrounded(int areanum); /* AAS_AreaGrounded impl */
int __cdecl FindClientByName(char *name);  /* 1-arg roster substring search (sub_100268D0); was incorrectly 3-arg */
const char *__cdecl StringContains(const char *str1, const char *str2, int casesensitive);  /* 0x1002ACF0 — substring search */
void __cdecl EA_Move(int client, vec3_t dir, float speed); /* EA_Move impl */
void __cdecl EA_View(int client, vec3_t viewangles); /* EA_View impl */
char     *__cdecl LibVarString(char *var_name, char *value);   /* returns libvar->string */
int __cdecl FreeMemory(void *ptr);  /* dummy `int` return; see definition */
int __cdecl PC_ReadSourceToken(source_t *source, token_t *token); /* l_precomp.c: reads one token from source, handling pushed-back tokens */
int __cdecl PC_Directive_line(source_t * source); /* #line handler */
BOOL __cdecl sub_10041240(int a1, const char *a2, int a3);  /* stub: no ZIP support */
/* The Vector / COM_ / Q_ / byte-order / Info_ helpers live in q_shared.c and
 * are prototyped by q_shared.h above; only botlib.c-local ones are declared
 * here. */
float *__cdecl VectorNegate(float *v); /* botlib.c-local; q_shared.c lacks this */

/* Callees whose definition used to precede every caller inside the monolithic
 * botlib.c, so no prototype was ever written for them.  Once the callers moved
 * to their own TUs the declarations became load-bearing: without one, C's
 * default argument promotion passes Export_BotAIFrame's `float` as a `double`
 * and its caller's codegen changes (caught as Export_BotLibAI OUR+1 by the
 * MSVC6 oracle). */
int  Export_BotAIFrame(int a1, float a2);          /* be_ai2_main.c   0x10029320 */
void AAS_InitClustering(void);                     /* be_aas_cluster.c 0x100096E0 */
int  AAS_LoadBSPFile(char *FileName, int Offset, int Length); /* be_aas_bspq2.c 0x10007D30 */



/* ---------------------------------------------------------------------
 * The library's global data: the 64-bit side-band scheme, the structures it
 * needs, and an `extern` for every object botlib.c defines.  This was
 * botlib.c's own declaration block before the TU split; the DEFINITIONS stay
 * in botlib.c, only their declarations live here so all TUs agree.
 * ------------------------------------------------------------------- */

//-------------------------------------------------------------------------
// Data declarations

extern structdef_t soundinfo_struct; /* sound info structdef — defined in botlib_structdefs.c */
/* G_SetMovedir's four direction constants, in the original declaration order;
 * all four are defined in botlib_structdefs.c.  IDA rendered the two MOVEDIR_*
 * vec3s as per-dword scalars (flt_1005C578/dword_1005C57C/dword_1005C580 and
 * flt_1005C590/dword_1005C594/dword_1005C598) because VectorCopy expands to
 * three separate moves and MSVC6 kept only the first in the FPU — a decompiler
 * artifact, not the original shape.  The array form is proven twice over: the
 * 1999 Linux gladi386.so's symbol table sizes each of the four at 12 bytes
 * (`readelf -s`: "12 OBJECT GLOBAL … MOVEDIR_UP"), and BotSetMovedir stays
 * byte-identical (125 B) under the MSVC6 oracle with float[3] + VectorCopy. */
extern float VEC_UP[3];      /* 0x1005C56C {0,-1, 0} — defined in botlib_structdefs.c */
extern float MOVEDIR_UP[3];  /* 0x1005C578 {0, 0, 1} — defined in botlib_structdefs.c */
extern float VEC_DOWN[3];    /* 0x1005C584 {0,-2, 0} — defined in botlib_structdefs.c */
extern float MOVEDIR_DOWN[3]; /* 0x1005C590 {0, 0,-1} — defined in botlib_structdefs.c */
extern structdef_t iteminfo_struct; /* item/entity structdef — defined in botlib_structdefs.c */
extern structdef_t weaponinfo_struct; /* weapon config structdef — defined in botlib_structdefs.c */
extern structdef_t projectileinfo_struct; /* projectile config structdef — defined in botlib_structdefs.c */
extern __int16 word_1005E498;
extern int filecrcs[]; /* CRC16 weapon table (92 entries × 8 bytes) — defined in botlib_structdefs.c */
/* NOT a variable and has no original name to recover: 0x1005E958 is exactly
 * filecrcs + 736, i.e. the one-past-the-end address MSVC folded into the scan
 * loop's bound as a link-time constant, and IDA had to invent a symbol for the
 * referenced address.  Confirmed: the DLL holds only zero fill there, and the
 * Linux .so — whose .dynsym kept every real global — has no symbol at the
 * corresponding end-of-filecrcs offset either.  Do not chase a name for it. */
extern int unk_1005E958;
/* `crctable` and the five CRC_* functions live in their own TU: botlib/l_crc.c
 * (l_crc.obj, DLL 0x100385B0..0x1003874F -- see .claude/memory/tu_partition.md). */
extern char word_1005F588[];

extern char unk_10060418[72]; /* 72-byte blob; &[3]="You are not allowed to..." — botlib_structdefs.c */
extern CHAR aWindllUnzip[];
extern CHAR FileName[];
extern _UNKNOWN unk_10061280;
extern _UNKNOWN unk_10061298;
extern char byte_1006294C;
extern libvar_t *libvar_framereachability;
extern libvar_t *libvar_reachabilitydelay;
extern int dword_1006295C;
extern libvar_t *libvar_laserhook;
extern HGLOBAL dword_10062968;
extern HGLOBAL dword_10062970;
extern HGLOBAL hMem;
extern float flt_10062984;
extern float flt_10062988;
extern float flt_1006298C;
extern float flt_1006319C;
extern float flt_100631A0;
extern float flt_100631A8;
extern float velocity[3]; /* vec3 {0,0,0} zero vector — defined in botlib_structdefs.c */
extern int dword_10063388;
/* The byte-order fn-ptr slots (0x100637CC..E0) and `bigendien` (0x10063884)
 * live in game/q_shared.c. */
extern int (__stdcall *windll_unzip)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD);
extern HMODULE hLibModule;
extern define_t *globaldefines;
/* `logfile` and the seven Log_* functions live in their own TU: botlib/l_log.c
 * (l_log.obj, DLL 0x10038BE0..0x10038F0F -- see .claude/memory/tu_partition.md). */
/* `libvarlist` and the thirteen LibVar* functions live in their own TU:
 * botlib/l_libvar.c (l_libvar.obj, DLL 0x10038750..0x10038BDF -- see
 * .claude/memory/tu_partition.md). */
extern struct scriptcrc_s *dword_10063F2C;
/* The three interface blocks are typed aggregates in botlib_state.h so that
 * GetBotAPI's import copy and Export_BotShutdownLibrary's clears compile back
 * to the original rep movs / rep stos.  Storage is defined here; call sites use
 * the botimport.* / botstate.* members directly. */
extern botimport_block_t botimport;
extern botstate_block_t botstate;
extern bot_export_t bot_exports;
extern ea_state_t *ea_controls;
extern weaponconfig_t *weaponconfig;
extern levelitem_t *freelevelitems;
extern int numlevelitems;
extern levelitem_t *levelitemheap;
extern itemconfig_t *itemconfig;
extern levelitem_t *levelitems;
extern bot_consolemessage_t *freeconsolemessages;
extern bot_consolemessage_t *consolemessageheap;
extern bot_matchtemplate_t *matchtemplates;
extern bot_randomlist_t *randomstrings;
extern bot_replychat_t *replychats;
extern bot_synonymlist_t *synonyms;
extern int numbots;
extern bsp_entity_t *dword_10064398;
extern int dword_1006439C;
/* bot_character (botlib-private):
 *   { int numcharacteristics; bot_characteristic_t pairs[N]; char strings[]; }
 * Each pair is an int[2]: a byte-sized type at +0 (padded) and the value at +4.
 * The byte field stays explicit so MSVC6 emits the same byte accesses, while
 * intptr_t keeps the value slot pointer-sized for the 64-bit port. */
typedef struct bot_characteristic_s {
    unsigned char type; /* 0=unset, 1=int, 2=float, 3=string */
    intptr_t      value; /* int, float (via *(float*)&value), or char * */
} bot_characteristic_t;
typedef struct bot_character_s {
    int numcharacteristics;
} bot_character_t;
#define BC_PAIRS(bc) ((bot_characteristic_t *)((char *)(bc) + \
    ((sizeof(bot_character_t) + sizeof(intptr_t) - 1) & ~(sizeof(intptr_t) - 1))))
extern bot_state_t *botstates;
/* Side-band tables — see the BOTLIB_NEED_SIDEBAND gate above.  Each macro must
 * be an lvalue of the same type in both branches: backed by a parallel array on
 * 64-bit, a direct cast of the inline int slot on 32-bit. */
#if BOTLIB_NEED_SIDEBAND
extern bot_character_t **botcharacters;
#define BotCharacter(bs) (botcharacters[(bs) - botstates])
#else
/* bs->character: the int slot at +1672 holds a bot_character_t pointer. */
#define BotCharacter(bs) (*(bot_character_t **)&(bs)->character)
#endif

/* Side-band pointer slots in bs->goalstate: itemweightconfig (weightconfig_t*,
 * set by BotLoadItemWeights, freed via FreeWeightConfig2) and itemweightindex
 * (ItemWeightIndex's weight×itemconfig table, freed via FreeMemory). */
#if BOTLIB_NEED_SIDEBAND
extern void **botgoalstate_p0;
extern void **botgoalstate_p1;
#define BotGoalP0(bs) (botgoalstate_p0[(bs) - botstates])
#define BotGoalP1(bs) (botgoalstate_p1[(bs) - botstates])
#else
#define BotGoalP0(bs) (*(void **)&(bs)->goalstate.itemweightconfig)
#define BotGoalP1(bs) (*(void **)&(bs)->goalstate.itemweightindex)
#endif

/* Goalstate-handle accessors.  BotLoadItemWeights / BotFreeItemWeights take a
 * `goalstate*` handle (= &bs->goalstate) directly, as the original call sites
 * did.  On 64-bit these recover bs from the goalstate offset and route through
 * the side-band arrays. */
#if BOTLIB_NEED_SIDEBAND
#define _GoalHandleBs(h) ((bot_state_t *)((char *)(h) - offsetof(bot_state_t, goalstate)))
#define BotGoalHandleP0(h) (botgoalstate_p0[_GoalHandleBs(h) - botstates])
#define BotGoalHandleP1(h) (botgoalstate_p1[_GoalHandleBs(h) - botstates])
#else
#define BotGoalHandleP0(h) (*(void **)&(h)->itemweightconfig)
#define BotGoalHandleP1(h) (*(void **)&(h)->itemweightindex)
#endif

/* bs->weaponweights is a flattened inline `int[7]`, five slots of which hold
 * pointers.  Mirrored into this typed struct (one per bot) on 64-bit; the field
 * offsets match the original inline layout (+0 … +24). */
typedef struct bot_weaponstate_s {
    int               client;       /* +0   = bs->client copy */
    int              *inventory;    /* +4   pointer into bs->inventory (item-count array; cast as int* for FuzzyWeight) */
    weightconfig_t   *weightconfig; /* +8   = loaded weights */
    int              *itemweights;  /* +12  = WeaponWeightIndex mapping table */
    char             *modelname;    /* +16  = AAS_ModelFromIndex(bs->snapshot.gunindex) */
    int               weaponindex;  /* +20  = chosen weapon index */
    float             nextthink;    /* +24  = AAS_Time gate */
} bot_weaponstate_t;
#if BOTLIB_NEED_SIDEBAND
extern bot_weaponstate_t **botweaponstates;
#define BotWS(bs) (botweaponstates[(bs) - botstates])
#else
/* On 32-bit the weaponstate IS the inline bs->weaponweights[7], so it is never
 * heap-allocated — the alloc/free paths are #if-guarded out at the call sites. */
#define BotWS(bs) ((bot_weaponstate_t *)&(bs)->weaponweights[0])
#endif

/* Side-band for the BotLoadInitialChat result at chatstate+184 (chatstate[46]).
 * Indexed by client number, recovered from the chatstate pointer's distance
 * from the bot_state_t base array (chatstate sits at +3980). */
#if BOTLIB_NEED_SIDEBAND
extern void **botchatdumps;
#define BotChatDumpSlot(cs_ptr) \
    (botchatdumps[((char *)(cs_ptr) - (char *)botstates \
                   - offsetof(bot_state_t, chatstate)) / sizeof(bot_state_t)])
#else
/* 32-bit: the pointer lives inline at chatstate +184 (_slot_46). */
#define BotChatDumpSlot(cs_ptr) (*(chatlist_t **)&(cs_ptr)->_slot_46)
#endif

/* Side-band for the per-client console-message FIFO held inline in chatstate
 * slots [43..45] (firstmessage, lastmessage, count). */
typedef struct chatmsg_links_s {
    bot_consolemessage_t *first;   /* chatstate[43] @ +172 */
    bot_consolemessage_t *last;    /* chatstate[44] @ +176 */
    int                   count;   /* chatstate[45] @ +180 */
} chatmsg_links_t;
#if BOTLIB_NEED_SIDEBAND
extern chatmsg_links_t *botchatmsglinks;
#define BotChatMsgLinks(client) (botchatmsglinks[(client)])
#define BotChatMsgLinksCS(cs) \
    (botchatmsglinks[((bot_state_t *)((char*)(cs) - offsetof(bot_state_t, chatstate))) - botstates])
#else
/* 32-bit: slots [43..45] (+172/+176/+180) are exactly chatmsg_links_t's 12
 * bytes, so aliasing them as the struct yields an lvalue. */
#define BotChatMsgLinks(client) \
    (*(chatmsg_links_t *)&botstates[(client)].chatstate._slot_43)
#define BotChatMsgLinksCS(cs) \
    (*(chatmsg_links_t *)&(cs)->_slot_43)
#endif

/* Side-band for the AI node function pointer behind BotAINode(bs). */
#if BOTLIB_NEED_SIDEBAND
extern ai_node_fn_t *botainodes;
#define BotAINode(bs) (botainodes[(bs) - botstates])
#else
/* 32-bit: bs->ainode (int @ +1676) IS the function pointer slot. */
#define BotAINode(bs) (*(ai_node_fn_t *)&(bs)->ainode)
#endif

/* Side-band for the three waypoint head slots at bot_state_t +4544
 * (checkpoints), +4548 (patrolpoints) and +4552 (curpatrolpoint), indexed by
 * (bs - botstates).  Allocated in BotSetupLibrary, freed in
 * BotShutdownLibrary. */
#if BOTLIB_NEED_SIDEBAND
extern bot_waypoint_t **botcheckpoints;
extern bot_waypoint_t **botpatrolpoints;
extern bot_waypoint_t **botcurpatrolpoint;
#define BotCheckpoints(bs)    (botcheckpoints[(bs) - botstates])
#define BotPatrolpoints(bs)   (botpatrolpoints[(bs) - botstates])
#define BotCurPatrolPoint(bs) (botcurpatrolpoint[(bs) - botstates])
#else
/* 32-bit: pointers live inline in bs->checkpoints/patrolpoints/curpatrolpoint. */
#define BotCheckpoints(bs)    (*(bot_waypoint_t **)&(bs)->checkpoints)
#define BotPatrolpoints(bs)   (*(bot_waypoint_t **)&(bs)->patrolpoints)
#define BotCurPatrolPoint(bs) (*(bot_waypoint_t **)&(bs)->curpatrolpoint)
#endif

/* Side-band for aas_entity_t's two link-list heads at +124 (areas) and +128
 * (BSP leaves), keyed by entity index.  Allocated with aasworld.entities in
 * sub_1000EDC0. */
#if BOTLIB_NEED_SIDEBAND
extern aas_link_t **aasentity_arealinks;
extern bsp_link_t **aasentity_bsplinks;
#define AAS_EntAreaLink(entnum) (aasentity_arealinks[(entnum)])
#define AAS_EntBspLink(entnum)  (aasentity_bsplinks[(entnum)])
#else
/* 32-bit: the link-list heads are real inline pointer members at +124
 * (area chain) and +128 (BSP-leaf chain) of the 132-byte aas_entity_t. */
#define AAS_EntAreaLink(entnum) (aasworld.entities[(entnum)].areas)
#define AAS_EntBspLink(entnum)  (aasworld.entities[(entnum)].leaves)
#endif


/* Initial-chat dump structures.  BotLoadInitialChat packs them into one
 * contiguous heap buffer:
 *   chatlist_t   @ +0        -> types
 *   chattype_t   @ +0..+43   (name[32], numlines, firstline, next)
 *   chatline_t   @ +0..+11   (string, ltime, next)
 * followed by the inline chat strings.  The 64-bit side-band path instead
 * allocates one node per chat-type / chat-line, string inline behind it. */
typedef struct chatline_s {
    char              *string;       /* +0  pointer to inline string buffer */
    float              ltime;        /* +4  last-time gate (AAS_Time) */
    struct chatline_s *next;         /* +8  next chat-line in this type */
#if BOTLIB_NEED_SIDEBAND
    char               buf[1];       /* +12 inline string follows (allocated) */
#endif
} chatline_t;

typedef struct chattype_s {
    char               name[32];     /* +0  type tag */
    int                numlines;     /* +32 number of chat-lines */
    chatline_t        *firstline;    /* +36 head of chat-line list */
    struct chattype_s *next;         /* +40 next chat-type */
} chattype_t;

typedef struct chatlist_s {
    chattype_t        *types;        /* head of chat-type list */
} chatlist_t;

extern float flt_100643A4;
extern bot_clientsettings_t *clientsettings;
extern libvar_t *libvar_ctf;
/* CTF flag goals.  BotGetLevelItemGoal fills 48 bytes of each 56-byte
 * bot_goal_t slot; `areanum` doubles as the "flag found" flag (0 = not yet). */
extern bot_goal_t ctf_blueflag;
extern bot_goal_t ctf_redflag;
extern libvar_t *libvar_usehook;
extern libvar_t *libvar_ch;
extern libvar_t *libvar_teamplay;
extern libvar_t *libvar_ra;
extern libvar_t *libvar_runes;
extern int dword_1006446C;
extern libvar_t *libvar_dmflags;
extern libvar_t *libvar_nochat;
extern libvar_t *libvar_rocketjump;
extern libvar_t *libvar_fastchat;
extern libvar_t *libvar_assimilation;
extern int dword_10064484;
extern libvar_t *libvar_teamplay_shell;
extern int dword_1006448C;
extern int dword_10064490;
extern int dword_10064494;
extern int dword_10064498;
extern int dword_1006449C;
extern int numnodeswitches;
extern char nodeswitch[7344];
/* midrangearea_t — Q3's name for this 8-byte scratch record
 * (be_aas_routealt.c); `midrangeareas` points at an array of them. */
typedef struct midrangearea_s {
  int            valid;
  unsigned short starttime;
  unsigned short goaltime;
} midrangearea_t;

/* be_aas_routealt.c globals (AAS_AlternativeRouteGoals /
 * AAS_AltRoutingFloodCluster_r).  midrangeareas (8 B/area) and clusterareas
 * (int* area-index list) are heap pointers held in 4-byte .data dwords; typed
 * pointers here, indexed as arrays, so no side-band is needed. */
extern int numclusterareas;
extern midrangearea_t *midrangeareas;
extern int *clusterareas;
extern int numportalcacheupdates;
extern int numareacacheupdates;
extern aas_reachabilitynode_t **areareachability;
extern int reach_ladder;
extern int reach_elevator;
extern intptr_t reachabilityheap;
extern int reach_jump;
extern int reach_grapple;
extern int reach_waterjump;
extern int reach_teleport;
extern int reach_barrier;
extern int reach_swim;
extern int reach_equalfloor;
extern intptr_t nextreachability;
extern int reach_walkoffledge;
extern int reach_rocketjump;
extern int reach_step;
extern int reach_walk;
/* aas_world_t — the single 676-byte AAS state struct occupying
 * 0x100667E0..0x10066A84, which AAS_Shutdown zeros wholesale.  It must stay one
 * aggregate: as separate globals the memset would run off into whatever the
 * linker placed next.  Field offsets are asserted in gladiator.dll.h. */

extern aas_world_t aasworld;

/* be_aas_debug.c debug-line state (MAX_DEBUGLINES=256): debuglines[] holds the
 * DebugLineCreate handles passed to DebugLineShow, debuglinevisible[] the 0/1
 * shown flag. */
extern int numdebuglines;
extern int debuglinevisible[256];
extern int debuglines[256];
extern int dword_100674C0;
/* ---------------------------------------------------------------------------
 * Q2 BSP lump globals @ VA 0x100674C4..0x10067554.  Names and declaration order
 * match Mr. Elusive's own Q2 BSP loader bspc/l_bsp_q2.c 1:1.  Lumps with a real
 * record type use their typed q2files.h pointer and are indexed as arrays; only
 * the genuinely untyped blobs (dvisdata, dlightdata, dentdata) stay byte
 * pointers.  The one remaining byte-view is the dmodels walk in Q2_SwapBSPFile.
 * ------------------------------------------------------------------------- */
extern int nummodels;
extern dmodel_t *dmodels;
extern int visdatasize;
extern char *dvisdata;
extern dvis_t *dvis;
extern int lightdatasize;
extern char *dlightdata;
extern int entdatasize;
extern unsigned char *dentdata;
extern int numleafs;
extern dleaf_t *dleafs;
extern int numplanes;
extern dplane_t *dplanes;
extern int numvertexes;
extern dvertex_t *dvertexes;
extern int numnodes;
extern dnode_t *dnodes;
extern int numtexinfo;
extern texinfo_t *texinfo;
extern int numfaces;
extern dface_t *dfaces;
extern int numedges;
extern dedge_t *dedges;
extern int numleaffaces;
extern unsigned short *dleaffaces;
extern int numleafbrushes;
extern unsigned short *dleafbrushes;
extern int numsurfedges;
extern int *dsurfedges;
extern int numbrushes;
extern dbrush_t *dbrushes;
extern int numbrushsides;
extern dbrushside_t *dbrushsides;
extern int numareas;
extern darea_t *dareas;
extern int numareaportals;
extern dareaportal_t *dareaportals;
/* 0x10067558..0x10067560 — three Gladiator-specific AAS precompute pointers
 * after the standard Q2 lumps; no cognate in l_bsp_q2.c, so left unnamed. */
extern char *dword_10067558;
                      // 8*numfaces, built by CalcSurfaceExtents (Q1 model.c cognate),
                      // read by RecursiveLightPoint.  NOT a PVS table.
extern char *dword_1006755C;
extern char *dword_10067560;
extern char byte_10067564[8192];
extern int dword_10069564;
extern int dword_10069568;
extern float flt_1006956C;
extern float flt_10069570;
extern float flt_10069574;
extern bsp_link_t *dword_10069578;
extern int dword_1006957C;
extern bsp_link_t *dword_10069580;
extern bsp_link_t **dword_10069584;


/* The side-band macros above index botstates, which is DEFINED in
 * be_ai2_main.c.  Declared here at the end, after bot_state_t exists --
 * a macro expands at its use site, so this need only precede the caller. */
extern bot_state_t *botstates;

#endif /* BOTLIB_LOCAL_H */
