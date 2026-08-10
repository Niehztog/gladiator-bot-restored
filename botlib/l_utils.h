/*
 * l_utils.h — interface of l_utils.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_L_UTILS_H
#define BOTLIB_L_UTILS_H

/* bot_fileref_t: the pak/file reference this TU's search path builds. */
/* bot_fileref_t — file location returned by sub_10041F60 / sub_10041BA0.
 * Q2-specific (Q3's transparent VFS handles paks internally).  The
 * decompilations show it as "int Offset[38]" or as three separate locals. */
typedef struct bot_fileref_s {
    int  fileofs;              /* file offset within pak  (0 = file is directly on disk)  */
    int  filelen;              /* file length within pak  (0 = read whole file from disk)  */
    char path[144];            /* pak path, or direct file path when fileofs == 0;
                                  144 = MAX_FILEPATH from gladq2_src/botlib.h              */
} bot_fileref_t;               /* sizeof = 152 */

/* The Win32 UnZip windll path lives entirely in this TU, so its kernel32
 * imports and the UNZIP32.DLL DCL/USERFUNCTIONS layouts belong here. */
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

/* The two locked-block handles, defined in l_utils.c; their types are just
 * above, which is why these declarations cannot live in botlib_local.h. */
#ifdef _WIN32
extern LPDCL dword_1006296C;
extern LPUSERFUNCTIONS dword_100639F0;
#endif



/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
int __cdecl sub_10041BA0(char *a1, char *Source, char *a3, bot_fileref_t *a4); /* search basePath+subdir+paks for file */
BOOL __cdecl sub_10041240(int a1, const char *a2, int a3);  /* stub: no ZIP support */
extern CHAR aWindllUnzip[];
extern CHAR FileName[];
extern _UNKNOWN unk_10061280;
extern _UNKNOWN unk_10061298;
extern char byte_1006294C;
extern HGLOBAL dword_10062968;
extern HGLOBAL dword_10062970;
extern HGLOBAL hMem;
/* The byte-order fn-ptr slots (0x100637CC..E0) and `bigendien` (0x10063884)
 * live in game/q_shared.c. */
extern HMODULE hLibModule;

BOOL __cdecl sub_10041240(int a1, const char *a2, int a3);
int __stdcall sub_100415E0(int a1);
void sub_10041600(void);
static int sub_10041650(void);
LPSTR __stdcall sub_10041680(unsigned int a1, unsigned int a2, unsigned __int16 a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13);
int __stdcall sub_10041740(int a1, int a2, int a3, int a4);
int __stdcall sub_10041760(const char *a1, int a2);
char __cdecl sub_100418D0(_BYTE *a1);
void __cdecl sub_10041900(const char *a1, int a2);
int __cdecl sub_10041970(char *FileName, const char *, bot_fileref_t *);
int __cdecl sub_10041BA0(char *a1, char *Source, char *a3, bot_fileref_t *a4);
BOOL __cdecl sub_10041F60(char *a1, bot_fileref_t *a2);
int __cdecl sub_10041FF0(const char *zipfile, const char *file_to_archive);
void sub_10042380();
int __stdcall sub_100423B0(int a1, int a2, int a3, int a4);
int __cdecl sub_100423D0(int a1, int a2);
void __stdcall sub_100423F0(char *p);
void __cdecl vectoangles(float *value1, float *angles);

#endif /* BOTLIB_L_UTILS_H */
