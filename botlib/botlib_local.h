/*
 * botlib_local.h — the shared compilation environment of the reconstructed
 * Gladiator botlib: the #include set, the CRT/POSIX shims, the Win32 import
 * and UnZip-windll declarations, the forward typedefs, and the prototypes of
 * every function the library defines.
 *
 * This block was botlib.c's own prologue (its lines 14..814) up to the point
 * where botlib.c started being split back into Mr. Elusive's original
 * translation units (.claude/memory/tu_partition.md).  It is lifted here
 * VERBATIM and included first by botlib.c and by every split TU, so all of
 * them compile in one identical macro/typedef/prototype environment and no
 * declaration can drift or go missing between them.
 *
 * TRANSITIONAL.  The 1999 sources had per-TU headers (be_aas_*.h, l_*.h etc.),
 * not one shared local header; replacing this with that structure is a later
 * step, once the .c split is finished and each TU's true interface is known.
 *
 * It carries no definitions -- only declarations, typedefs, macros and static
 * inline CRT shims -- so including it in N translation units costs nothing.
 */
#ifndef BOTLIB_LOCAL_H
#define BOTLIB_LOCAL_H

#include <math.h>
#include <stdarg.h>
#include <stddef.h>    /* offsetof, size_t */
#include <stdint.h>    /* intptr_t */
#include <stdio.h>     /* file I/O, sprintf, sscanf */
#include <string.h>    /* string and memory ops */
#include <stdlib.h>    /* malloc, atoi, rand, abs */
#include <ctype.h>     /* toupper */
#include <errno.h>     /* errno */
#include <time.h>      /* time(), ctime() */
#include <unistd.h>    /* access(), chdir(), getcwd() */

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

/* signed high-word/dword accessors */
#define SHIDWORD(x)  (*((int *)&(x) + 1))

/* Stubs for the statically-linked MSVC CRT helpers the bot code still calls. */
static size_t fread_locked(void *buf, size_t sz, size_t n, FILE *f) { return fread(buf, sz, n, f); }
#ifdef _WIN32
/* Windows-only winbspc/zip subsystem (sub_1000E140/sub_1000E430, sub_10041FF0);
 * the Linux botlib has neither a process-spawn nor a chdir-for-bspc path. */
static int    remove_file(const char *path) { return remove(path); }
static int    getcwd_locked(char *buf, int size) { return getcwd(buf, size) ? 0 : -1; }
static intptr_t SpawnProcess(int mode, char *file, char *args, char *cmd, char *addargs) { (void)mode; (void)file; (void)args; (void)cmd; (void)addargs; return -1; }
/* _access is provided by MinGW's <io.h>; _chdir by <direct.h> — use POSIX wrappers */
static int    _chdir(const char *path) { return chdir(path); }
#endif /* _WIN32 */

#ifndef _WIN32
/* POSIX equivalents for the Windows CRT functions used below. */
#include <strings.h>    /* strcasecmp */
#define _strcmpi   strcasecmp
#define _access    access
typedef int __time32_t;   /* Windows 32-bit time type; int is 32-bit on all targets */
#endif

/* Win32 imports for the UnZip/ZIP32 windll path.  Declared by hand rather than
 * via <windows.h>, whose typedefs collide with our local ones; both Windows
 * toolchains resolve them to the same kernel32 imports as the original.  The
 * Linux build gates the whole unzip/zip subsystem out. */
#ifdef _WIN32
__declspec(dllimport) void * __stdcall GlobalAlloc(unsigned int uFlags, unsigned int dwBytes);
__declspec(dllimport) void * __stdcall GlobalLock(void *hMem);
__declspec(dllimport) void * __stdcall GlobalFree(void *hMem);
__declspec(dllimport) int    __stdcall GlobalUnlock(void *hMem);
__declspec(dllimport) unsigned int __stdcall SearchPathA(const char *p, const char *f, const char *x, unsigned int n, char *b, char **lp);
__declspec(dllimport) void * __stdcall LoadLibraryA(const char *f);
__declspec(dllimport) void * __stdcall GetProcAddress(void *h, const char *name);
__declspec(dllimport) int    __stdcall FreeLibrary(void *h);
__declspec(dllimport) char * __stdcall lstrcpyA(char *d, const char *s);
__declspec(dllimport) int    __stdcall lstrlenA(const char *s);
#endif

/* ------------------------------------------------------------------------
 * Info-ZIP UnZip windll SDK structures (windll/structs.h) — the option block
 * (DCL, 0x44 B) and callback table (USERFUNCTIONS, 0x28 B) that sub_10041240
 * passes to UNZIP32.DLL's "windll_unzip" to extract the .aas from aasN.zip.
 *
 * The shipped unzip32.dll is UnZip 5.33, so the layouts follow the 5.32 SDK
 * headers vendored at reference/unzip532/structs.h (5.51's 0x2C USERFUNCTIONS
 * adds a 6th callback and is NOT the right shape here).
 *
 * Every pointer-bearing field is a 4-byte `int` slot, as in the 32-bit
 * original, so both struct sizes stay 0x44 / 0x28 on 64-bit too; callback
 * addresses are cast to (intptr_t) on assignment.  Windows-only — the Linux
 * botlib has no unzip support and loads .aas files directly. */
#ifdef _WIN32
typedef struct {
  int   ExtractOnlyNewer;   /* +0x00  TRUE => "update" without overwriting   */
  int   SpaceToUnderscore;  /* +0x04  TRUE => convert spaces to underscores  */
  int   PromptToOverwrite;  /* +0x08  TRUE => prompt before overwriting      */
  int   fQuiet;             /* +0x0C  0=all msgs, 1=fewer, 2=none            */
  int   ncflag;             /* +0x10  write to stdout if TRUE                */
  int   ntflag;             /* +0x14  test archive                          */
  int   nvflag;             /* +0x18  verbose listing                       */
  int   nUflag;             /* +0x1C  5.32 windll header name (5.5x renamed this slot nfflag); zero-filled, so immaterial */
  int   nzflag;             /* +0x20  display archive comment               */
  int   ndflag;             /* +0x24  (sub)dir recreation control            */
  int   noflag;             /* +0x28  always overwrite if TRUE              */
  int   naflag;             /* +0x2C  do end-of-line translation            */
  int   nZIflag;            /* +0x30  return ZipInfo if TRUE                 */
  int   C_flag;             /* +0x34  case-insensitive match if TRUE        */
  int   fPrivilege;         /* +0x38  1=restore ACLs, 2=use privileges       */
  int   lpszZipFN;          /* +0x3C  LPSTR — archive file name (4-byte slot) */
  int   lpszExtractDir;     /* +0x40  LPSTR — extract dir    (4-byte slot)   */
} DCL, *LPDCL;               /* sizeof == 0x44 */

typedef struct {
  int            print;                  /* +0x00 DLLPRNT*    (4-byte fn-ptr slot) */
  int            sound;                  /* +0x04 DLLSND*                          */
  int            replace;                /* +0x08 DLLREPLACE*                      */
  int            password;               /* +0x0C DLLPASSWORD*                     */
  int            SendApplicationMessage; /* +0x10 DLLMESSAGE*                      */
  unsigned short cchComment;             /* +0x14 WORD comment length; +0x16 pad   */
  unsigned int   TotalSizeComp;          /* +0x18 (statistics — unused here)       */
  unsigned int   TotalSize;              /* +0x1C                                  */
  int            CompFactor;             /* +0x20 (present in 5.32; unused here)    */
  unsigned int   NumMembers;             /* +0x24                                  */
} USERFUNCTIONS, *LPUSERFUNCTIONS;        /* sizeof == 0x28 */
#endif /* _WIN32 — UnZip windll DCL/USERFUNCTIONS */

/* AI node function pointer type (used for BotAINode side-band table) */
typedef int (*ai_node_fn_t)(struct bot_state_s *bs);

/* ------------------------------------------------------------------------
 * Side-band gate.
 *
 * Every "pointer slot" in bot_state_t / bot_chatstate_t / aas_entity_t is a
 * 4-byte int field holding a pointer bit-pattern.  On 64-bit those slots are
 * too narrow, so each is mirrored into a parallel heap array (the side-band)
 * reached through helper macros (BotCharacter, BotAINode, BotWS, …).
 *
 * On 32-bit the macros just reinterpret the inline int slot and the side-band
 * tables compile out entirely, reproducing the original memory image exactly.
 * Every gate in botlib.c follows this one predicate. */
#if defined(__x86_64__) || defined(__aarch64__)
#define BOTLIB_NEED_SIDEBAND 1
#else
#define BOTLIB_NEED_SIDEBAND 0
#endif

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
int BotSetupChatAI();
int AAS_ContinueInitReachability(int a1); // caller passes arg but function body ignores it (no ebp frame)
int AAS_FreeRoutingCaches(void);  /* sub_10019550 */
int AAS_FreeAllPortalCache(void); /* sub_100193E0 */
libvar_t *__cdecl LibVar(char *var_name, char *value);            /* register/lookup libvar */
float     __cdecl LibVarValue(char *var_name, char *value);    /* register, return value */
int __cdecl AAS_BoxOnPlaneSide2(vec3_t absmins, vec3_t absmaxs, float *p);  /* Q3 canonical name */
void *AAS_AllocReachability(void);  /* sub_10010FF0 — pop AAS-link from free chain */
int __cdecl WriteFloat(FILE *fp, float value);
int __cdecl sub_10007150(intptr_t start, intptr_t end, intptr_t endpos, _DWORD *red, _DWORD *green, _DWORD *blue);
void __cdecl LibVarSet(char *var_name, char *value);  /* body at ~30304 */
int __cdecl sub_10003080(vec3_t point);
void __cdecl AnglesToAxis(const vec3_t angles, float axis[3][3]);  // 0x100034D0; was sub_100034D0 (originally also mislabeled sub_100423B0)
int __cdecl sub_10006100(int *a1, int a2, float *a3);
int __cdecl RecursiveLightPoint(int nodenum, float *start, float *end, float *lightspot, int *pointcolor);
void CalcSurfaceExtents();
int Q2_SwapBSPFile(void);
int AAS_DumpBSPData();
void *__cdecl sub_10007C40(FILE *Stream, int Offset, size_t ElementSize, int a4, char *ArgList);
int sub_100085F0();
int __cdecl AAS_DropToFloor(vec3_t origin, vec3_t mins, vec3_t maxs);  // 5-param: matches call sites
_DWORD sub_10010FF0();
int __cdecl AAS_AreaSwim(int areanum); /* AAS_AreaSwim impl */
int __cdecl AAS_AreaGrounded(int areanum); /* AAS_AreaGrounded impl */
int __cdecl FindClientByName(char *name);  /* 1-arg roster substring search (sub_100268D0); was incorrectly 3-arg */
const char *__cdecl StringContains(const char *str1, const char *str2, int casesensitive);  /* 0x1002ACF0 — substring search */
void __cdecl BotReplaceSynonyms(char *string, unsigned long int context);
void __cdecl BotReplaceWeightedSynonyms(const char *string, int context);
bot_randomlist_t *__cdecl BotLoadRandomStrings(char * filename);
char *__cdecl RandomString(const char *name);
void __cdecl BotFreeMatchPieces(bot_matchpiece_t *matchpieces);
bot_matchpiece_t *__cdecl BotLoadMatchPieces(source_t *source, const char *endtoken);
void __cdecl BotFreeMatchTemplates(bot_matchtemplate_t *mt);
bot_matchtemplate_t *__cdecl BotLoadMatchTemplates(char * matchfile);
BOOL __cdecl StringsMatch(bot_matchpiece_t *pieces, bot_match_t *match);
int  __cdecl BotFindMatch(char *str, bot_match_t *match, int context);
char *__cdecl BotMatchVariable(bot_match_t *match, int variable, char *buf);
bot_stringlist_t *__cdecl BotCheckChatMessageIntegrety(const char *message, bot_stringlist_t *stringlist);
void __cdecl BotCheckReplyChatIntegrety(bot_replychat_t *replychat);
void __cdecl BotCheckInitialChatIntegrety(struct chatlist_s *chat);
int __cdecl BotLoadChatMessage(source_t *source, char *chatmessagestring);
void __cdecl BotFreeReplyChat(bot_replychat_t *replychat);
bot_replychat_t *__cdecl BotLoadReplyChat(char *filename);
void *__cdecl BotLoadInitialChat(char *chatfile, char *chatname);
int __cdecl BotFreeChatFile(bot_chatstate_t *chatstate);
int __cdecl BotFreeChatState(bot_chatstate_t *cs);
int __cdecl BotLoadChatFile(bot_chatstate_t *chatstate, char *chatfile, char *chatname);
void __cdecl BotConstructChatMessage(bot_chatstate_t *cs, const char *message, int mcontext, bot_chatvar_t *vars, int vcontext);
char *__cdecl BotChooseInitialChatMessage(chatlist_t *cs, char *type);
void __cdecl BotInitialChat(bot_chatstate_t *cs, char *type, ...);
int __cdecl BotReplyChat(bot_chatstate_t *cs, const char *message);
unsigned int __cdecl BotChatLength(bot_chatstate_t *chatstate);
void __cdecl BotEnterChat(bot_chatstate_t *chatstate, int clientto, int sendto);
void BotShutdownChatAI();
int __cdecl FindFuzzyWeight(weightconfig_t *wc, const char *name);
double __cdecl FuzzyWeight_r(int *inventory, fuzzyseperator_t *fs);
double __cdecl FuzzyWeightUndecided_r(int *inventory, fuzzyseperator_t *fs);
double __cdecl FuzzyWeight(int *facts, weight_t *w);
double __cdecl FuzzyWeightUndecided(int *facts, weight_t *w);
void __cdecl EvolveFuzzySeperator_r(fuzzyseperator_t *fs);
void __cdecl ScaleFuzzySeperator_r(fuzzyseperator_t *fs, float scale);
int __cdecl InterbreedFuzzySeperator_r(fuzzyseperator_t *fs1, fuzzyseperator_t *fs2);
void __cdecl EA_Move(int client, vec3_t dir, float speed); /* EA_Move impl */
void __cdecl EA_View(int client, vec3_t viewangles); /* EA_View impl */
char     *__cdecl LibVarString(char *var_name, char *value);   /* returns libvar->string */
int __cdecl FreeMemory(void *ptr);  /* dummy `int` return; see definition */
int __cdecl PC_ReadSourceToken(source_t *source, token_t *token); /* l_precomp.c: reads one token from source, handling pushed-back tokens */
bot_stringlist_t *__cdecl BotFindStringInList(bot_stringlist_t *list, const char *string);
int __cdecl PC_Directive_line(source_t * source); /* #line handler */
int __cdecl WriteIndent(FILE *fp, int indent);
int __cdecl WriteStructWithIndent(FILE *fp, structdef_t * def, int structure, int indent);
int __cdecl WriteStructure(FILE *fp, int def, int structure);
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
#ifdef _WIN32
extern LPDCL dword_1006296C;
#endif
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
#ifdef _WIN32
extern LPUSERFUNCTIONS dword_100639F0;
#endif
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



/* Field-table spelling for the structdef_t descriptors recovered from .data.
 * Shared by be_aas_sound / be_ai_goal / be_ai_weap, which own one table each.
 * A structdef is { int size; char **fields; }; each field entry is 7 (char *)
 * slots: name, byte offset, type flags, array count, 0, default float-bits, 0. */
/* Cast integer constant to char * for mixed pointer/int field table slots */
#define P(x) ((char *)(uintptr_t)(x))

/* One field table entry (7 slots): name, offset, flags, arrcount, 0, default, 0 */
#define FE(name, off, flags, arr, def) \
    (name), P(off), P(flags), P(arr), P(0), P(def), P(0)

/* Null terminator entry */
#define FE_END \
    P(0),  P(0),   P(0),   P(0),  P(0),  P(0),  P(0)

#endif /* BOTLIB_LOCAL_H */
