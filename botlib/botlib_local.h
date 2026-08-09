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
char *Export_BotVersion(void);
int Export_BotSetupLibrary(void);
int Export_BotShutdownLibrary(void);
int Export_BotLibraryInitialized(void);
int Export_BotLibVarSet(char *var_name, char *value);
int Export_BotDefine(char *string);
int Export_BotLoadMap(char *mapname, int modelindexes, char **modelindex,
                      int soundindexes, char **soundindex,
                      int imageindexes, char **imageindex);
int Export_BotSetupClient(int client, void *settings);
int Export_BotShutdownClient(int client);
int Export_BotMoveClient(int oldclnum, int newclnum);
int Export_BotClientSettings(int client, void *settings);
int Export_BotSettings(int client, void *settings);
int Export_BotUpdateClient(int client, void *buc);
int Export_BotUpdateEntity(int ent, void *bue);
int Export_BotAddSound(int *origin, int ent, int channel, int soundindex,
                       float volume, float attenuation, float timeofs);
int Export_BotAddPointLight(int *origin, int ent, float radius,
                             float r, float g, float b, float time, float decay);
int Export_Test(int parm0, char *parm1, float *parm2, float *parm3);

/* Forward declarations for the functions defined below. */
int __cdecl AAS_ContinueInit(int time);
int __cdecl PC_DollarEvaluate(source_t *source, int *intvalue, double *floatvalue, int integer); /* l_precomp.c: evaluates #if expression tokens */
int __cdecl PC_ReadLine(source_t *source, token_t *token);                       /* 2-param line reader */
int __cdecl sub_10041BA0(char *a1, char *Source, char *a3, bot_fileref_t *a4); /* search basePath+subdir+paks for file */
void sub_10028E80(void);  
void BotCheckAttack(bot_state_t *bs);
void AAS_InitTravelFlagFromType(void); /* sub_10018D00 */
int __cdecl AAS_AreaLadder(int areanum);
bot_moveresult_t *__cdecl BotMoveToGoal(bot_moveresult_t *a1, bot_movestate_t *movestate, bot_goal_t *goal, int travelflags); /* 0x100343A0: build bot_moveresult_t for current goal */
void AAS_InitReachability();
itemconfig_t *LoadItemConfig(char *Source);
void AAS_Optimize(void);
int BotSetupChatAI();
int sub_1001D260();
float __cdecl Characteristic_Float(bot_character_t * character, int index);
int __cdecl BotReachabilityTime(aas_reachability_t* reach);
void PrintUsedMemorySize(void);
int __cdecl AAS_HorizontalVelocityForJump(float zvel, vec3_t start, vec3_t end, float * velocity);
int __cdecl AAS_UpdatePortal(int areanum, int clusternum);
aas_clientmove_t __cdecl AAS_ClientMovementPrediction(int entnum, float * origin, int presencetype, int onground, float * velocity, float * cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int visualize);
int __cdecl PC_UnreadSourceToken(source_t *source, const void *token);
int __cdecl sub_1001C760(char *Source);
BOOL BotCanAndWantsToRocketJump(bot_state_t *bs);
void __cdecl PC_FreeToken(token_t *token);
int AAS_ContinueInitReachability(int a1); // caller passes arg but function body ignores it (no ebp frame)
void BotInitLevelItems(void);
int __cdecl BotLibLoadMap(char *Source);
int AAS_FreeRoutingCaches(void);  /* sub_10019550 */
int AAS_FreeAllPortalCache(void); /* sub_100193E0 */
libvar_t *__cdecl LibVar(char *var_name, char *value);            /* register/lookup libvar */
float     __cdecl LibVarValue(char *var_name, char *value);    /* register, return value */
int __cdecl AAS_BoxOnPlaneSide2(vec3_t absmins, vec3_t absmaxs, float *p);  /* Q3 canonical name */
void sub_1001D290(void);  
void *AAS_AllocReachability(void);  /* sub_10010FF0 — pop AAS-link from free chain */
int __cdecl Characteristic_Integer(bot_character_t * character, int index);
int __cdecl AAS_StartFrame(float time);
int __cdecl BotEntityVisible(int, float *, float *, float, int);
weaponconfig_t *LoadWeaponConfig(char *filename);
void BotAimAtEnemy(bot_state_t *bs);
int __cdecl AAS_BSPTraceLight(intptr_t start, intptr_t end, intptr_t endpos, int *red, int *green, int *blue);
int InFieldOfVision(float *, float, float *);
int __cdecl WriteFloat(FILE *fp, float value);
void AAS_InitAASLinkHeap();
int __cdecl sub_10007150(intptr_t start, intptr_t end, intptr_t endpos, _DWORD *red, _DWORD *green, _DWORD *blue);
void __cdecl LibVarSet(char *var_name, char *value);  /* body at ~30304 */
float __cdecl VectorDistance(vec3_t v1, vec3_t v2);
int __cdecl AIEnter_Seek_ActivateEntity(bot_state_t *bs);
bsp_link_t *sub_100031F0(void);
int __cdecl AAS_BestReachableArea(int * origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin);
int AAS_TestPortals();
void __cdecl EA_DropItem(int client, char *it);
bsp_trace_t __cdecl AAS_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask);
void __cdecl sub_10003240(bsp_link_t *a1);
int __cdecl sub_10003080(vec3_t point);
void sub_10003280();
void sub_100032D0();
int __cdecl CM_PointLeafnum(const vec3_t point, int modelnum);
dleaf_t *__cdecl sub_10003420(const vec3_t point, int modelnum);
void __cdecl sub_10003460(vec3_t v, float m[3][3]);
void __cdecl AnglesToAxis(const vec3_t angles, float axis[3][3]);  // 0x100034D0; was sub_100034D0 (originally also mislabeled sub_100423B0)
qboolean __cdecl AAS_EntityCollision(int entnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int contentmask, bsp_trace_t *trace);
int __cdecl sub_10003BF0(int leafnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int passent, int contentmask, bsp_trace_t *trace);
int __cdecl CM_TraceThroughBrush(dbrush_t *a1, float *a2, float *a3, float *a4, float *a5, float *a6, float *a7, float *a8, _DWORD *a9, float *a10, float *a11);
int __cdecl CM_TraceThroughLeaf(int leafnum, vec3_t origin, vec3_t angles, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int contentmask, bsp_trace_t *trace);
bsp_trace_t __cdecl AAS_TraceBSPModel(int modelnum, const vec3_t modelorigin, vec3_t angles, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int passent, int contentmask);
int __cdecl sub_100056D0(dbrush_t *a1, float *a2);
int __cdecl sub_100057A0(float *a1, int a2, float *a3, float *a4);
void __cdecl AAS_DecompressVis(int a1, int a2);
BOOL __cdecl AAS_InPVS(float *a1, float *a2, int a3);
qboolean __cdecl AAS_inPVS(vec3_t p1, vec3_t p2);
BOOL __cdecl sub_10005C90(float *a1, float *a2);
void __cdecl AAS_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin);
bsp_link_t *__cdecl AAS_UnlinkFromBSPLeaves(bsp_link_t *leaves);
int __cdecl sub_10006100(int *a1, int a2, float *a3);
bsp_link_t *__cdecl AAS_BSPLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum, int modelnum);
char *__cdecl AAS_ValueForBSPEpairKey(bsp_entity_t *ent, const char *key);
int __cdecl AAS_VectorForBSPEpairKey(bsp_entity_t *ent, const char *key, vec3_t v);
float __cdecl FloatForKey(bsp_entity_t *ent, const char *key);
int __cdecl AAS_IntForBSPEpairKey(bsp_entity_t *ent, const char *key);
void __cdecl AAS_FreeBSPEntities(bsp_entity_t *a1);
bsp_entity_t *AAS_ParseBSPEntities(void);
int __cdecl RecursiveLightPoint(int nodenum, float *start, float *end, float *lightspot, int *pointcolor);
void CalcSurfaceExtents();
int Q2_SwapBSPFile(void);
int AAS_DumpBSPData();
void *__cdecl sub_10007C40(FILE *Stream, int Offset, size_t ElementSize, int a4, char *ArgList);
int sub_100085F0();
int AAS_RemoveClusterAreas();
int __cdecl AAS_FloodClusterAreas_r(int areanum, int clusternum);
int __cdecl AAS_FloodClusterReachabilities(int clusternum);
void __cdecl AAS_NumberClusterPortals(int clusternum);
int AAS_FindClusters();
void AAS_CreatePortals();
void __cdecl AAS_ConnectedAreas_r(int *areanums, int numareas, int *connectedareas, int curarea);
qboolean __cdecl AAS_ConnectedAreas(_DWORD *areanums, int numareas);
int __cdecl AAS_FloodAreas_r(int *areanum, int cluster, int done);
int __cdecl AAS_CheckAreaForPossiblePortals(int areanum);
int AAS_FindPossiblePortals();
void AAS_RemoveAllPortals();
int AAS_ClearShownDebugLines();
int __cdecl AAS_DebugLine(vec3_t start, vec3_t end, int color);
int __cdecl AAS_DrawPermanentCross(vec3_t origin, float size, int color);
void __cdecl AAS_ShowArea(int areanum, int groundfacesonly);
void __cdecl AAS_DrawCross(vec3_t origin, float size, int color);
void __cdecl AAS_PrintTravelType(int traveltype);
void __cdecl AAS_DrawArrow(vec3_t start, vec3_t end, int linecolor, int arrowcolor);
void __cdecl AAS_ShowReachability(aas_reachability_t *reach);
void __cdecl AAS_ShowReachableAreas(int areanum);
int __cdecl AAS_UpdateEntity(int entnum, bot_updateentity_t *state);
aas_entityinfo_t __cdecl AAS_EntityInfo(int entnum);
int __cdecl AAS_EntityModelindex(int entnum);
int __cdecl AAS_EntityRenderFX(int entnum);
int __cdecl AAS_EntityModelNum(int entnum);
int __cdecl AAS_OriginOfMoverWithModelNum(int modelnum, vec3_t origin);
int __cdecl AAS_EntityBSPData(int entnum, bsp_entdata_t *entdata);
int __cdecl AAS_DropToFloor(vec3_t origin, vec3_t mins, vec3_t maxs);  // 5-param: matches call sites
void AAS_ResetEntityLinks();
void AAS_InvalidateEntities();
int __cdecl AAS_BestReachableLinkArea(aas_link_t *areas);
int __cdecl sub_1000BAA0(int, float *, float *, float, int, int *);
int __cdecl AAS_NextBSPEntity(int ent);
void AAS_SwapAASData();
void *AAS_DumpAASData();
void *__cdecl AAS_LoadAASLump(FILE *Stream, int Offset, size_t ElementCount);
int __cdecl AAS_LoadAASFile(char *FileName, int Offset, int Length);
int __cdecl AAS_WriteAASLump(FILE *fp, int *h, int lumpnum, void *data, size_t length);
qboolean __cdecl AAS_WriteAASFile(char *filename);
bsp_pointlight_t *sub_1000D450();
bsp_pointlight_t *__cdecl sub_1000D4A0(bsp_pointlight_t *a1);
void __cdecl sub_1000D4E0(float a1);
int __cdecl BotAddPointLight(vec3_t origin, int ent, float radius, float r, float g, float b, float time, float decay);
int __cdecl AAS_PointLight(float *origin, int *red, int *green, int *blue);
int AAS_Error(char *Format, ...);
char *__cdecl AAS_StringFromIndex(const char *indexname, indexlist_t *list, int index);
int __cdecl AAS_IndexFromString(const char * indexname, indexlist_t * list, char *String2);
char *__cdecl AAS_ModelFromIndex(int index);
int __cdecl IndexFromModel(char *String2);
char *__cdecl AAS_ImageFromIndex(int index);
indexlist_t *__cdecl sub_1000DA80(int numindexes, char **names);
void __cdecl sub_1000DB40(indexlist_t *list, int numindexes, char **names);
int __cdecl sub_1000DBD0(indexlist_t *list);
indexlist_t *__cdecl sub_1000DC20(int a1, char **a2, int a3, char **a4, int a5, char **a6);
void __cdecl sub_1000DCC0(int a1, char **a2, int a3, char **a4, int a5, char **a6);
int __cdecl AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
int AAS_Initialized();
float AAS_Time();
intptr_t __cdecl sub_1000E140(char *Source);
int __cdecl sub_1000E430(char *Source);
int __cdecl BotLoadMap(char *Source, int, char **, int, char **, int, char **);
int __cdecl sub_1000EDC0(int a1, int a2);
int AAS_Shutdown();
BOOL __cdecl AAS_OnGround(vec3_t origin, int presencetype, int passent);
BOOL __cdecl AAS_Swimming(vec3_t origin);
void __cdecl AAS_JumpReachRunStart(aas_reachability_t* reach, intptr_t runstart);
int __cdecl AAS_AgainstLadder(vec3_t origin);
double __cdecl AAS_WeaponJumpZVelocity(vec3_t origin, float radiusdamage);
float __cdecl AAS_RocketJumpZVelocity(vec3_t origin);
double __cdecl AAS_BFGJumpZVelocity(vec3_t origin);
void __cdecl AAS_ApplyFriction(vec3_t vel, float friction, float stopspeed, float frametime);
int AAS_KeepEdge(aas_edge_t *edge);
int __cdecl AAS_OptimizeEdge(optimized_t *optimized, int edgenum);
int __cdecl AAS_KeepFace(aas_face_t *face);
int __cdecl AAS_OptimizeFace(optimized_t *optimized, int facenum);
int __cdecl AAS_OptimizeArea(optimized_t *optimized, int areanum);
int __cdecl AAS_OptimizeAlloc(optimized_t *optimized);
int __cdecl AAS_OptimizeStore(optimized_t *optimized);
int AAS_SetupReachabilityHeap();
void AAS_ShutDownReachabilityHeap();
_DWORD sub_10010FF0();
int __cdecl AAS_AreaReachability(int areanum);
float __cdecl AAS_FaceArea(aas_face_t *face);
float __cdecl AAS_AreaVolume(int areanum);
float __cdecl AAS_AreaGroundFaceArea(int areanum);
void __cdecl AAS_FaceCenter(int facenum, vec3_t center);
int AAS_FallDamageDistance();
float __cdecl AAS_MaxJumpHeight(float phys_jumpvel);
float __cdecl AAS_MaxJumpDistance(float phys_jumpvel);
int __cdecl AAS_AreaCrouch(int areanum);
int __cdecl AAS_AreaSwim(int areanum); /* AAS_AreaSwim impl */
int __cdecl AAS_AreaGrounded(int areanum); /* AAS_AreaGrounded impl */
qboolean __cdecl AAS_ReachabilityExists(int area1num, int area2num);
BOOL __cdecl AAS_NearbySolidOrGap(vec3_t start, vec3_t end);
int __cdecl AAS_Reachability_Swim(int area1num, int area2num);
int __cdecl AAS_Reachability_EqualFloorHeight(int area1num, int area2num);
int __cdecl AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge(int area1num, int area2num);
int __cdecl VectorBetweenVectors(vec3_t v, vec3_t v1, vec3_t v2);
void __cdecl VectorMiddle(vec3_t v1, vec3_t v2, vec3_t middle);
int AAS_Reachability_Teleport();
void AAS_Reachability_Elevator();
int __cdecl AAS_Reachability_Grapple(int area1num, int area2num);
int AAS_SetWeaponJumpAreaFlags();
int __cdecl AAS_Reachability_WeaponJump(int area1num, int area2num);
void __cdecl AAS_Reachability_WalkOffLedge(int areanum);
int AAS_StoreReachability();
int __cdecl AAS_TravelFlagForType(int traveltype);
void AAS_CreateReversedReachability(void);
unsigned short __cdecl AAS_AreaTravelTime(int areanum, float *start, float *end);
void AAS_CalculateAreaTravelTimes(void);
aas_routingcache_t *__cdecl AAS_AllocRoutingCache(int numtraveltimes);
void __cdecl AAS_FreeRoutingCache(void *cache);
void AAS_FreeAllClusterAreaCache(void);
void AAS_InitClusterAreaCache();
int AAS_InitPortalCache();
int AAS_InitRoutingUpdate();
void AAS_InitRouting(void);
void __cdecl AAS_UpdateAreaRoutingCache(aas_routingcache_t *areacache);
aas_routingcache_t *__cdecl AAS_GetAreaRoutingCache(int clusternum, int areanum, int travelflags);
void __cdecl AAS_UpdatePortalRoutingCache(aas_routingcache_t *portalcache);
aas_routingcache_t *__cdecl AAS_GetPortalRoutingCache(int clusternum, int areanum, int travelflags);
__int16 __cdecl AAS_AreaTravelTimeToGoalArea(int areanum, int a2, int goalareanum);
aas_reachability_t __cdecl AAS_ReachabilityFromNum(int num);
int __cdecl AAS_NextAreaReachability(int areanum, int reachnum);
int __cdecl AAS_RandomGoalArea(int areanum, int travelflags, _DWORD *goalareanum, vec3_t goalorigin);
aas_trace_t __cdecl AAS_TraceClientBBox(vec3_t start, vec3_t end, int presencetype, int passent);
int AAS_RoutingInfo();
int __cdecl AAS_AltRoutingFloodCluster_r(int areanum);
int sub_1001AB80();
void AAS_FreeAASLinkHeap();
aas_link_t *AAS_AllocAASLink(void);
aas_link_t *__cdecl AAS_DeAllocAASLink(aas_link_t *link);
int AAS_InitAASLinkedEntities();
void AAS_FreeAASLinkedEntities();
int __cdecl AAS_PointAreaNum(vec3_t point);
int __cdecl AAS_AreaPresenceType(int areanum);
int __cdecl AAS_PointContents(vec3_t point);
qboolean __cdecl AAS_AreaEntityCollision(int areanum, char *start, vec3_t end, int presencetype, int passent, aas_trace_t *trace);
int __cdecl AAS_TraceAreas(float *start, float *end, int *areas, int maxareas);
qboolean __cdecl AAS_InsideFace(aas_face_t *face, vec3_t pnormal, vec3_t point, float epsilon);
qboolean __cdecl AAS_PointInsideFace(int facenum, vec3_t point, float epsilon);
int __cdecl sub_1001C2E0(float *a1, float *a2, float *a3);
aas_link_t *__cdecl AAS_UnlinkFromAreas(aas_link_t *areas);
aas_link_t *__cdecl AAS_AASLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum);
aas_link_t *__cdecl AAS_LinkEntityClientBBox(vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype);
char *__cdecl AAS_PlaneFromNum(int planenum);
aas_soundpool_t *sub_1001CBE0();
aas_soundpool_t *sub_1001CC10(aas_soundpool_t *a1);
void sub_1001CC50(aas_soundpool_t *a1);
aas_soundpool_t *sub_1001CCC0(aas_soundpool_t *a1);
void sub_1001CD10(aas_soundpool_t *a1);
aas_soundpool_t *sub_1001CD80(aas_soundpool_t *a1);
void __cdecl sub_1001CDD0(int a1, int a2);
int __cdecl sub_1001CE20(float *, int, int, int, int, int, float);
void __cdecl sub_1001CFA0(float a1);
void sub_1001D140();
void BotResetNodeSwitches();
int __cdecl BotDumpNodeSwitches(bot_state_t *bs);
int __cdecl BotRecordNodeSwitch(bot_state_t *bs, const char *node, const char *str);
float *__cdecl BotLongTermGoal(bot_state_t *bs, int tfl, int retreat);
void __cdecl AIEnter_Intermission(bot_state_t *bs);
int __cdecl AINode_Intermission(bot_state_t *bs);
int __cdecl AIEnter_Observer(bot_state_t *bs);
int __cdecl AINode_Observer(bot_state_t *bs);
int __cdecl AIEnter_Stand(bot_state_t *bs);
int __cdecl AINode_Stand(bot_state_t *bs);
void __cdecl AIEnter_Respawn(bot_state_t *bs);
int __cdecl AINode_Respawn(bot_state_t *bs);
int __cdecl AINode_Seek_ActivateEntity(bot_state_t *bs);
int __cdecl AIEnter_Seek_NBG(bot_state_t *bs);
int __cdecl AINode_Seek_NBG(bot_state_t *bs);
int __cdecl AIEnter_Seek_LTG(bot_state_t *bs);
int __cdecl AINode_Seek_LTG(bot_state_t *bs);
int __cdecl AIEnter_Battle_Fight(bot_state_t *bs);
int __cdecl AINode_Battle_Fight(bot_state_t *bs);
void __cdecl AIEnter_Battle_Chase(bot_state_t *bs);
int __cdecl AINode_Battle_Chase(bot_state_t *bs);
int __cdecl AIEnter_Battle_Retreat(bot_state_t *bs);
int __cdecl AINode_Battle_Retreat(bot_state_t *bs);
int __cdecl AIEnter_Battle_NBG(bot_state_t *bs);
int __cdecl AINode_Battle_NBG(bot_state_t *bs);
_DWORD *__cdecl BotEntityInfo(bot_state_t *bs, _DWORD *info);
char *__cdecl sub_10020FE0(bot_state_t *bs, bot_weaponstate_t *ws);
void __cdecl BotUpdateInventory(bot_state_t *bs);
int __cdecl BotUpdateBattleInventory(bot_state_t *bs, int enemy);
void __cdecl BotBattleUseItems(bot_state_t *bs);
void __cdecl sub_100215E0(bot_state_t *bs);
int __cdecl BotCTFCarryingFlag(bot_state_t *bs);
BOOL __cdecl BotIsDead(bot_state_t *bs);
BOOL __cdecl BotIsObserver(bot_state_t *bs);
BOOL __cdecl BotIntermission(bot_state_t *bs);
BOOL __cdecl sub_10021710(int *a1);
BOOL __cdecl EntityIsShooting(intptr_t a1);
char *__cdecl stristr(char *str, char *charset);
char *__cdecl EasyClientName(int client, char *buf);
bot_waypoint_t *__cdecl BotCreateWayPoint(const char *name, vec3_t origin, int areanum);
bot_waypoint_t *__cdecl BotFindWayPoint(bot_waypoint_t *waypoints, char *name);
void            __cdecl BotFreeWaypoints(bot_waypoint_t *wp);
BOOL __cdecl BotValidChatPosition(bot_state_t *bs);
BOOL __cdecl BotChat_EnterGame(bot_state_t *bs);
int __cdecl BotChat_ExitGame(bot_state_t *bs);
int __cdecl BotChat_StartLevel(bot_state_t *bs);
int __cdecl BotChat_EndLevel(bot_state_t *bs);
int __cdecl BotChat_Death(int *bs);
BOOL __cdecl BotChat_Kill(int *bs);
int __cdecl BotChat_Random(bot_state_t *bs);
double __cdecl BotChatTime(bot_state_t *bs);
float __cdecl BotAggression(bot_state_t *bs);
BOOL __cdecl BotWantsToRetreat(int *bs);
BOOL __cdecl BotWantsToChase(int *bs);
float *__cdecl BotRoamGoal(bot_state_t *bs, float *goal);
bot_moveresult_t __cdecl BotAttackMove(bot_state_t *bs, int a3);
int __cdecl BotCTFTeam(bot_state_t *bs);
BOOL __cdecl BotSameTeam(bot_state_t *bs, int entnum);
int __cdecl BotNumTeamMates(bot_state_t *bs);
int __cdecl BotFindEnemy(bot_state_t *bs);
int *__cdecl BotEntityToActivate(int a1);
int __cdecl BotSetMovedir(float *angles, float *movedir);
void __cdecl BotAIBlocked(bot_state_t *bs, bot_moveresult_t *moveresult, int activate);
void __cdecl sub_100262C0(_DWORD *a1, bot_goal_t *a2);
void __cdecl BotCTFRetreatGoals(bot_state_t *bs);
void __cdecl BotCTFSeekGoals(bot_state_t *bs);
BOOL TeamPlayIsOn();
BOOL __cdecl BotGetItemTeamGoal(char *goalname, bot_goal_t *goal);
int __cdecl BotGetMessageTeamGoal(bot_state_t *bs, char *goalname, bot_goal_t *goal);
float __cdecl BotGetTime(bot_match_t *match);
int __cdecl BotGetPatrolWaypoints(bot_state_t *bs, bot_match_t *match);
int __cdecl BotAddressedToBot(bot_state_t *bs, bot_match_t *match);
int __cdecl BotMatchMessage(bot_state_t *bs, char *message);
void __cdecl BotCheckConsoleMessages(bot_state_t *bs);
float *__cdecl sub_100289A0(bot_state_t *bs, float a2);
int BotDeathmatchAI(bot_state_t *bs, float a2);
int __cdecl sub_10028A40(bot_state_t *bs, float a2);
void BotSetupDeathmatchAI();
int __cdecl ClientFromName(const char *name);
char *__cdecl ClientName(int client);
char *__cdecl ClientSkin(int client);
int NumBots();
float __cdecl AngleDifference(float ang1, float ang2);
int __cdecl BotChangeViewAngles(bot_state_t *bs, float thinktime);
void sub_100292E0();
int __cdecl BotSetupClient(int, char *Source);
int __cdecl BotShutdownClient(int a1);
int __cdecl BotMoveClient(int a1, int a2);
int __cdecl BotUpdateClient(int a1, const void *a2);
int __cdecl BotClientSettings(int a1, const void *a2);
int __cdecl BotConsoleMessage(int, int, char *Source);
int __cdecl BotSettings(int a1, const void *a2);
int __cdecl BotResetState(bot_state_t *bs);
int sub_10029C10();
int BotShutdownLibrary();
bot_character_t *__cdecl BotLoadCharacter(char *charfile, const char *a2);
void __cdecl sub_1002A590(int a1);
int __cdecl CheckCharacteristicIndex(bot_character_t *character, int index);
float __cdecl Characteristic_BFloat(bot_character_t *character, int index, float min, float max);
int __cdecl Characteristic_BInteger(bot_character_t *character, int index, int min, int max);
char *__cdecl Characteristic_String(bot_character_t *character, int index);
bot_consolemessage_t *AllocConsoleMessage();
int __cdecl FreeConsoleMessage(bot_consolemessage_t *message);
int __cdecl BotRemoveConsoleMessage(bot_chatstate_t *chatstate, bot_consolemessage_t *msg);
int __cdecl BotQueueConsoleMessage(bot_chatstate_t *chatstate, int type, char *message);
bot_consolemessage_t *__cdecl BotNextConsoleMessage(bot_chatstate_t *cs);
int __cdecl BotNumConsoleMessages(bot_chatstate_t *chatstate);
BOOL __cdecl IsWhiteSpace(char c);
void __cdecl UnifyWhiteSpaces(void *string);
int __cdecl FindClientByName(char *name);  /* 1-arg roster substring search (sub_100268D0); was incorrectly 3-arg */
const char *__cdecl StringContains(const char *str1, const char *str2, int casesensitive);  /* 0x1002ACF0 — substring search */
const char *__cdecl StringContainsWord(const char *str1, const char *str2, int casesensitive);
void __cdecl StringReplaceWords(const char *string, const char *synonym, const char *replacement);
bot_synonymlist_t *__cdecl BotLoadSynonyms(char *filename);
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
int *__cdecl ItemWeightIndex(weightconfig_t *iwc, itemconfig_t *ic);
_DWORD *__cdecl AllocLevelItem(void);
void __cdecl FreeLevelItem(levelitem_t *li);
levelitem_t *__cdecl AddLevelItemToList(levelitem_t *li);
levelitem_t *__cdecl RemoveLevelItemFromList(levelitem_t *li);
char *__cdecl BotGoalName(int number);
int __cdecl BotResetAvoidGoals(bot_goalstate_t *goalstate);
void __cdecl BotDumpAvoidGoals(bot_goalstate_t *goalstate);
void __cdecl BotAddToAvoidGoals(bot_goalstate_t *gs, int number, float avoidtime);
float __cdecl BotAvoidGoalTime(bot_goalstate_t *goalstate, int number);
int __cdecl BotGetLevelItemGoal(int index, char *name, bot_goal_t *goal);
void BotUpdateEntityItems(void);
void __cdecl BotDumpGoalStack(bot_goalstate_t *goalstate);
int __cdecl BotPushGoal(bot_goalstate_t *goalstate, const void *goal);
int __cdecl BotPopGoal(bot_goalstate_t *goalstate);
void __cdecl BotEmptyGoalStack(bot_goalstate_t *goalstate);
void *__cdecl BotGetTopGoal(bot_goalstate_t *goalstate);
void *__cdecl BotGetSecondGoal(bot_goalstate_t *goalstate);
int __cdecl BotChooseLTGItem(bot_goalstate_t *goalstate, vec3_t origin, char *inventory, int travelflags);
int __cdecl BotChooseNBGItem(bot_goalstate_t *goalstate, vec3_t origin, char *inventory, int travelflags, bot_goal_t *ltg, float maxtime);
int __cdecl BotTouchingGoal(vec3_t origin, float *goal);
BOOL __cdecl BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, bot_goal_t *goal);
int __cdecl BotLoadItemWeights(bot_goalstate_t *goalstate, char *filename);
void __cdecl BotFreeItemWeights(bot_goalstate_t *goalstate);
int __cdecl BotResetGoalState(bot_goalstate_t *goalstate);
int BotSetupGoalAI();
int BotShutdownGoalAI();
double __cdecl AngleDiff(float ang1, float ang2);
int __cdecl BotReachabilityArea(int *origin, int client);
BOOL __cdecl BotOnMover(float *origin, int entnum, aas_reachability_t* reach);
BOOL __cdecl MoverDown(aas_reachability_t* reach);
BOOL __cdecl BotValidTravel(float *a1, int a2, aas_reachability_t *a3, int a4);
void __cdecl BotAddToAvoidReach(intptr_t ms_, int number, float avoidtime);
int __cdecl BotGetReachabilityToGoal(float *origin, int areanum, int entnum, int lastgoalareanum, int lastareanum, int *avoidreach, float *avoidreachtimes, int *avoidreachtries, bot_goal_t *goal, int travelflags);
int __cdecl BotMovementViewTarget(bot_movestate_t *ms, bot_goal_t *goal, int travelflags, float *target);
void __cdecl MoverBottomCenter(aas_reachability_t *reach, vec3_t bottomcenter);
float __cdecl BotGapDistance(bot_movestate_t *ms, float *dir);
int __cdecl BotCheckBarrierJump(bot_movestate_t *ms, float *dir, float speed);
int __cdecl BotSwimInDirection(bot_movestate_t *ms, float *dir, float speed, int type);
int __cdecl BotWalkInDirection(bot_movestate_t *ms, float *dir, float speed, int type);
int __cdecl BotMoveInDirection(bot_movestate_t *movestate, float *dir, float speed, int type);
int __cdecl BotCheckBlocked(bot_movestate_t *ms, float *dir, bot_moveresult_t *moveresult);
bot_moveresult_t *__cdecl BotClearMoveResult(bot_moveresult_t *moveresult);
bot_moveresult_t __cdecl BotTravel_Walk(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Crouch(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_BarrierJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_BarrierJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Swim(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_WaterJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_WaterJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_WalkOffLedge(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_WalkOffLedge(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Jump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_Jump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Ladder(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Teleport(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_Elevator(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_Elevator(bot_movestate_t *ms, aas_reachability_t *reach);
int __cdecl GrappleState(bot_movestate_t *ms, aas_reachability_t *reach);
void __cdecl BotResetGrapple(bot_movestate_t *ms);
bot_moveresult_t __cdecl BotTravel_Grapple(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotTravel_RocketJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotFinishTravel_WeaponJump(bot_movestate_t *ms, aas_reachability_t *reach);
bot_moveresult_t __cdecl BotMoveInGoalArea(bot_movestate_t *ms, bot_goal_t *goal);
_DWORD *__cdecl BotResetAvoidReach(_DWORD *movestate);
void __cdecl BotResetLastAvoidReach(intptr_t movestate);
int __cdecl BotResetMoveState(void *movestate);
_DWORD *__cdecl WeaponWeightIndex(weightconfig_t *wwc, weaponconfig_t *wc);
void __cdecl BotFreeWeaponWeights(bot_weaponstate_t *weaponstate);
int __cdecl BotLoadWeaponWeights(bot_weaponstate_t *weaponstate, const char *filename);
weaponinfo_t *__cdecl sub_100354B0(bot_weaponstate_t *ws);
void __cdecl BotChooseBestFightWeapon(bot_weaponstate_t *ws);
int __cdecl BotResetWeaponState(bot_weaponstate_t *weaponstate);
int BotSetupWeaponAI();
int BotShutdownWeaponAI();
int __cdecl ReadValue(source_t *source, float *value);
int __cdecl ReadFuzzyWeight(source_t *source, fuzzyseperator_t *fs);
void __cdecl FreeFuzzySeperators_r(fuzzyseperator_t *fs);
fuzzyseperator_t *__cdecl ReadFuzzySeperators_r(source_t *source);
void              __cdecl FreeWeightConfig2(weightconfig_t *config);
weightconfig_t   *__cdecl ReadWeightConfig(char *filename);
qboolean __cdecl WriteFuzzyWeight(FILE *fp, fuzzyseperator_t * fs);
qboolean __cdecl WriteFuzzySeperators_r(FILE *fp, int, int);
int __cdecl FindFuzzyWeight(weightconfig_t *wc, const char *name);
double __cdecl FuzzyWeight_r(int *inventory, fuzzyseperator_t *fs);
double __cdecl FuzzyWeightUndecided_r(int *inventory, fuzzyseperator_t *fs);
double __cdecl FuzzyWeight(int *facts, weight_t *w);
double __cdecl FuzzyWeightUndecided(int *facts, weight_t *w);
void __cdecl EvolveFuzzySeperator_r(fuzzyseperator_t *fs);
void __cdecl ScaleFuzzySeperator_r(fuzzyseperator_t *fs, float scale);
int __cdecl InterbreedFuzzySeperator_r(fuzzyseperator_t *fs1, fuzzyseperator_t *fs2);
void __cdecl EA_Say(int client, char *str);
void __cdecl EA_SayTeam(int client, char *str);
void __cdecl EA_UseItem(int client, char *it);
void __cdecl sub_100371B0(int client, int sequence);
int __cdecl EA_Command(int client, char *command, ...);
int __cdecl EA_Attack(int client);
int __cdecl EA_Respawn(int client);
char __cdecl EA_Jump(int client);
int __cdecl EA_DelayedJump(int client);
int __cdecl EA_Crouch(int client);
int __cdecl EA_MoveUp(int client);
int __cdecl EA_MoveForward(int client);
void __cdecl EA_Move(int client, vec3_t dir, float speed); /* EA_Move impl */
void __cdecl EA_View(int client, vec3_t viewangles); /* EA_View impl */
void __cdecl EA_EndRegular(int client, float thinktime);
int EA_Setup();
void EA_Shutdown();
void __cdecl sub_100376B0(char *String1, __int16);
int __cdecl sub_100377E0(char *String1, __int16);
int __cdecl sub_10037850(char *String1, const unsigned char *, int);
int Sys_MilliSeconds();
qboolean __cdecl ValidClientNumber(int num, const char *str);
qboolean __cdecl ValidEntityNumber(int num, const char *str);
qboolean __cdecl BotLibSetup(const char *str);
int BotSetupMoveAI();
int __cdecl Export_BotLibStartFrame(float time);
int __cdecl Export_BotLibConsoleMessage(int client, int a2, char *message);
_WORD *__cdecl CRC_Init(_WORD *crcvalue);
__int16 __cdecl CRC_Value(__int16 crcvalue);
__int16 __cdecl CRC_Block(const unsigned char *data, int length);
float __cdecl LibVarStringValue(char *string);
libvar_t *__cdecl LibVarAlloc(const char *var_name);
void      __cdecl LibVarDeAlloc(libvar_t *v);
libvar_t *__cdecl LibVarGet(const char *var_name);
char     *__cdecl LibVarGetString(const char *var_name);
float     __cdecl LibVarGetValue(const char *var_name);
char     *__cdecl LibVarString(char *var_name, char *value);   /* returns libvar->string */
void Log_Open(char *FileName);  
FILE *Log_Shutdown();
FILE *Log_Write(char *Format, ...);
FILE *Log_FilePointer();
void Log_Flush();
void *__cdecl GetMemory(int size);
void *__cdecl GetClearedMemory(unsigned int size);
int __cdecl FreeMemory(void *ptr);  /* dummy `int` return; see definition */
int __cdecl MemoryByteSize(void *ptr);
void PrintMemoryLabels(void);
void DumpMemory(void);
int SourceError(source_t *src, char *Format, ...);
int SourceWarning(source_t *src, char *Format, ...);
indent_t *__cdecl PC_PushIndent(source_t *source, int type, int skip);
indent_t *__cdecl PC_PopIndent(source_t *source, int *type, int *skip);
void __cdecl PC_PushScript(source_t *source, script_t *script);
int __cdecl PC_ReadSourceToken(source_t *source, token_t *token); /* l_precomp.c: reads one token from source, handling pushed-back tokens */
int __cdecl PC_ReadDefineParms(source_t *source, define_t *define, token_t **parms, int maxparms);
int __cdecl PC_StringizeTokens(token_t *tokens, token_t *token);
int __cdecl PC_MergeTokens(token_t *t1, token_t *t2);
unsigned int __cdecl PC_NameHash(const char *name);
unsigned int __cdecl PC_AddDefineToHash(define_t *define, define_t **definehash);
bot_stringlist_t *__cdecl BotFindStringInList(bot_stringlist_t *list, const char *string);
int __cdecl PC_FindDefine(define_t *defines, const char *name);
int __cdecl PC_FindDefineParm(define_t *define, const char *name);
void __cdecl PC_FreeDefine(define_t *define);
define_t *__cdecl PC_FindHashedDefine(define_t **definehash, const char *name);
int __cdecl PC_ExpandBuiltinDefine(source_t *src, define_t *define, char **a3, char **a4);
int __cdecl PC_ExpandDefine(source_t *src, define_t *define, char **firsttoken, char **lasttoken);
int __cdecl PC_ExpandDefineIntoSource(source_t *src, define_t *define);
void __cdecl PC_ConvertPath(char *path);
int __cdecl PC_Directive_include(source_t *source);
BOOL __cdecl PC_WhiteSpaceBeforeToken(token_t *token);
token_t *__cdecl PC_ClearTokenWhiteSpace(token_t *token);
int __cdecl PC_Directive_undef(source_t *source);
int __cdecl PC_Directive_define(source_t *source);
define_t *__cdecl PC_DefineFromString(const char *string);
int __cdecl PC_AddGlobalDefine(const char *string);
define_t *__cdecl PC_CopyDefine(define_t *define);
void __cdecl PC_AddGlobalDefinesToSource(source_t *source);
int __cdecl PC_Directive_ifdef(source_t *src, int type);
int __cdecl PC_Directive_else(source_t *source);
int __cdecl PC_Directive_endif(source_t *source);
int __cdecl PC_OperatorPriority(int op);
int __cdecl PC_EvaluateTokens(source_t *source, token_t *tokens, int *intvalue, double *floatvalue, int integer);
int __cdecl PC_Evaluate(source_t *source, int *intvalue, double *floatvalue, int integer);
int __cdecl PC_Directive_elif(source_t *source);
int __cdecl PC_Directive_if(source_t *source);
int __cdecl PC_Directive_line(source_t * source); /* #line handler */
int __cdecl PC_Directive_error(source_t *source);
int __cdecl PC_Directive_pragma(source_t *source);
int __cdecl UnreadSignToken(source_t *source);
int __cdecl PC_Directive_eval(source_t *source);
int __cdecl PC_Directive_evalfloat(source_t *source);
int __cdecl PC_ReadDirective(source_t *source);
int __cdecl PC_DollarDirective_evalint(source_t *source);
int __cdecl PC_DollarDirective_evalfloat(source_t *source);
int __cdecl PC_ReadDollarDirective(source_t *source);
int __cdecl PC_ReadTokenHandle(source_t *source, _DWORD *pc_token);
int __cdecl PC_ExpectTokenString(source_t *source, const char *string);
int __cdecl PC_ExpectTokenType(source_t *source, int type, int subtype, intptr_t token);
int __cdecl PC_ExpectAnyToken(source_t *source, intptr_t token);
int __cdecl PC_CheckTokenString(source_t *source, const char *string);
int __cdecl PC_UnreadLastToken(source_t *source);
source_t *__cdecl LoadSourceFile(char *Source, int Offset, size_t ElementSize);
void      __cdecl FreeSource(source_t *source);
void __cdecl PS_CreatePunctuationTable(script_t *script, punctuation_t *punctuations);
void ScriptError(int script, char *Format, ...);
void ScriptWarning(int script, char *Format, ...);
void __cdecl SetScriptPunctuations(script_t *script, punctuation_t *p);
int __cdecl PS_ReadWhiteSpace(script_t *script);
int __cdecl PS_ReadEscapeCharacter(script_t *script, _BYTE *ch);
int __cdecl PS_ReadString(script_t *script, token_t *token, int quote);
int __cdecl PS_ReadName(script_t *script, intptr_t a2);
void __cdecl NumberValue(char *string, int subtype, int *intvalue, double *floatvalue);
int __cdecl PS_ReadNumber(script_t *script, token_t *token);
int __cdecl PS_ReadPunctuation(script_t *script, char *token);
int __cdecl PS_ReadPrimitive(script_t *script, intptr_t token);
int __cdecl PS_ReadToken(script_t *script, char *Destination);
int __cdecl PS_ExpectTokenType(script_t *script, int type, int subtype, token_t *token);
int __cdecl PS_ExpectAnyToken(int script, int token);
void __cdecl StripDoubleQuotes(char *string);
void __cdecl StripSingleQuotes(char *string);
void __cdecl SetScriptFlags(script_t *script, int flags);
BOOL __cdecl EndOfScript(script_t *script);
int __cdecl FileLength(FILE *fp);
script_t *__cdecl LoadScriptFile(char *FileName, int Offset, size_t ElementSize);
script_t *__cdecl LoadScriptMemory(const void *ptr, unsigned int length, const char *name);
void      __cdecl FreeScript(script_t *script);
const char **__cdecl FindField(const char **defs, const char *name);
int __cdecl ReadNumber(source_t *source, char **fd, float *p);
int __cdecl ReadChar(source_t *source, char **fd, float *p);
int __cdecl ReadString(source_t * source, char ** fd, char *p);
int __cdecl ReadStructure(source_t *source, structdef_t *def, char *structure);
int __cdecl WriteIndent(FILE *fp, int indent);
int __cdecl WriteStructWithIndent(FILE *fp, structdef_t * def, int structure, int indent);
int __cdecl WriteStructure(FILE *fp, int def, int structure);
BOOL __cdecl sub_10041240(int a1, const char *a2, int a3);  /* stub: no ZIP support */
int __stdcall sub_100415E0(int a1);
void sub_10041600(void);
LPSTR __stdcall sub_10041680(unsigned int a1, unsigned int a2, unsigned __int16 a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13);
int __stdcall sub_10041740(int a1, int a2, int a3, int a4);
int __stdcall sub_10041760(const char *a1, int a2);
int __cdecl vectoangles(float *value1, float *angles);
char __cdecl sub_100418D0(_BYTE *a1);
void __cdecl sub_10041900(const char *a1, int a2);
int __cdecl sub_10041970(char *FileName, const char *, bot_fileref_t *);
BOOL __cdecl sub_10041F60(char *a1, bot_fileref_t *a2);
void sub_10042380();
int __stdcall sub_100423B0(int a1, int a2, int a3, int a4);
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



#endif /* BOTLIB_LOCAL_H */
