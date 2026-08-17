/*
 * l_utils.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10041240..0x100423F0.
 */

#include "botlib_port.h"
#include "l_libvar.h"
#undef VectorNegate
#include "be_ea.h"
#include "q2files.h"
#include "aasfile.h"
#include "be_aas_def.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "be_ai_def.h"
#include "be_interface.h"
#include "struct_sizes_asserts.h"
#include "l_utils.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"

/* ---- UnZip/ZIP32 windll state — WINDOWS ONLY -----------------------------
 * All of it belongs inside the same `#ifdef _WIN32` as the code that uses it
 * (below, and the ZIP32 block further down).  It used to sit outside the gate,
 * so the Linux build emitted nine globals it never references -- which is
 * exactly what `dataaudit.py`'s EXPORTED-but-not-in-real check reports, and
 * the arithmetic is unambiguous: our `.data` was 148 B larger than
 * gladi386.so's and our surplus `.data` globals totalled 141 B, while real has
 * 2 B of unnamed `.data` in the whole image and therefore no room for them as
 * statics either.  gladi386.so has no unzip support at all (it imports no
 * dlopen and no zlib); .aas files are loaded directly by BotLibLoadMap.
 * (2026-08-17.) */
#ifdef _WIN32
CHAR FileName[] = "UNZIP32.DLL"; // idb

_UNKNOWN unk_10061280; // weak

_UNKNOWN unk_10061298; // weak

LPDCL dword_1006296C = NULL; /* locked DCL option block — UnZip windll, Windows-only */

LPUSERFUNCTIONS dword_100639F0; /* locked USERFUNCTIONS callback table — UnZip windll, Windows-only */

CHAR aWindllUnzip[] = "windll_unzip"; // idb
HGLOBAL dword_10062968 = NULL; // idb
HGLOBAL dword_10062970 = NULL; // idb
HGLOBAL hMem = NULL; // idb
int (__stdcall *windll_unzip)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD); // weak
HMODULE hLibModule; // idb
#endif /* _WIN32 — UnZip/ZIP32 windll state */

#ifdef _WIN32  /* ---- UnZip windll path (UNZIP32.DLL) ----
                * Windows-only.  Linux gladi386.so has no unzip support (imports no dlopen and
                * no zlib); .aas files are loaded directly by BotLibLoadMap. */
// gladiator.dll: 10041240..1004151C
// gladi386.so:   absent
BOOL __cdecl sub_10041240(int a1, const char *a2, int a3)
{
  HGLOBAL v3; // eax
  HGLOBAL v5; // eax
  USERFUNCTIONS *v6; // eax
  HMODULE LibraryA; // eax
  HGLOBAL v8; // ebp
  HGLOBAL v9; // esi
  char **v10; // ebx
  char *v11; // edi
  int v12; // esi
  HGLOBAL hMem; // [esp+18h] [ebp-88h]
  LPSTR FilePart; // [esp+1Ch] [ebp-84h] BYREF
  CHAR Buffer[128]; // [esp+20h] [ebp-80h] BYREF

  if ( !a1 )
    return 0;
  v3 = GlobalAlloc(0x40u, 0x44u);
  dword_10062970 = v3;
  if ( !v3 )
    return 0;
  dword_1006296C = GlobalLock(v3);
  if ( !dword_1006296C )
  {
    GlobalFree(dword_10062970);
    return 0;
  }
  v5 = GlobalAlloc(0x40u, 0x28u);
  dword_10062968 = v5;
  if ( !v5 )
  {
    GlobalUnlock(dword_10062970);
    GlobalFree(dword_10062970);
    return 0;
  }
  v6 = GlobalLock(v5);
  dword_100639F0 = v6;
  if ( !v6 )
  {
    GlobalUnlock(dword_10062970);
    GlobalFree(dword_10062970);
    GlobalFree(dword_10062968);
    return 0;
  }
  v6->password = (intptr_t)sub_10041740;
  dword_100639F0->print = (intptr_t)sub_10041760;
  dword_100639F0->sound = 0;
  dword_100639F0->replace = (intptr_t)sub_100415E0;
  dword_100639F0->SendApplicationMessage = (intptr_t)sub_10041680;
  if ( !SearchPathA(0, FileName, 0, 0x80u, Buffer, &FilePart)
    || (LibraryA = LoadLibraryA(FileName), (hLibModule = LibraryA) == 0) )
  {
    sub_10041600();
    return 0;
  }
  windll_unzip = (int (__stdcall *)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD))GetProcAddress(
                                                                                      LibraryA,
                                                                                      aWindllUnzip);
  dword_1006296C->ncflag = 0;
  dword_1006296C->fQuiet = 2;
  dword_1006296C->ntflag = 0;
  dword_1006296C->nvflag = 0;
  dword_1006296C->nUflag = 0;
  dword_1006296C->nzflag = 0;
  dword_1006296C->ndflag = 0;
  dword_1006296C->noflag = 1;
  dword_1006296C->naflag = 0;
  dword_1006296C->lpszZipFN = a1;
  dword_1006296C->lpszExtractDir = a3;
  dword_1006296C->C_flag = 1;
  v8 = GlobalAlloc(2u, 0x28u);
  if ( !v8 )
  {
    sub_10041600();
    FreeLibrary(hLibModule);
    return 0;
  }
  v9 = GlobalAlloc(2u, 0x104u);
  hMem = v9;
  if ( !v9 )
  {
    sub_10041600();
    FreeLibrary(hLibModule);
    return 0;
  }
  v10 = (char **)GlobalLock(v8);
  memset(v10, 0, 0x28u);
  v11 = (char *)GlobalLock(v9);
  *v10 = v11;
  memset(v11, 0, 0x104u);
  strcpy(*v10, a2);
  v12 = windll_unzip(1, (intptr_t)v10, 0, 0, (intptr_t)dword_1006296C, (intptr_t)dword_100639F0);
  GlobalUnlock(hMem);
  GlobalFree(hMem);
  GlobalUnlock(v8);
  GlobalFree(v8);
  sub_10041600();
  FreeLibrary(hLibModule);
  return v12 == 0;
}
// 1006296C: resolved to LPDCL dword_1006296C (UnZip windll option block)
// 100639F0: resolved to LPUSERFUNCTIONS dword_100639F0 (UnZip windll callback table)
// gladiator.dll: 100415E0..100415E8
// gladi386.so:   absent
int __stdcall sub_100415E0(int a1)
{
  return 1;
}
// gladiator.dll: 10041600..1004163A
// gladi386.so:   absent
/* void: ref 10041600 falls through to one bare `pop edi; pop esi; ret`. */
void sub_10041600(void)
{

  if ( dword_10062970 )
  {
    GlobalUnlock(dword_10062970);
    GlobalFree(dword_10062970);
  }
  if ( dword_10062968 )
  {
    GlobalUnlock(dword_10062968);
    GlobalFree(dword_10062968);
  }
}
// gladiator.dll: 10041650..1004166F
// gladi386.so:   absent
/* Cached MSVC _osplatform-style helper: 1 on Windows NT (top bit of GetVersion()
 * clear), 0 on Windows 9x.  The cache slot starts at -1.  DEAD in Gladiator. */
static int sub_10041650(void)
{
  static unsigned int cached = 0xFFFFFFFFu;
  if ( cached == 0xFFFFFFFFu )
    cached = 1; /* GetVersion unavailable cross-platform; dead code anyway */
  return (int)cached;
}
// gladiator.dll: 10041680..1004170A
// gladi386.so:   absent
LPSTR __stdcall sub_10041680(
        unsigned int a1,
        unsigned int a2,
        unsigned __int16 a3,
        int a4,
        int a5,
        int a6,
        int a7,
        int a8,
        int a9,
        int a10,
        int a11,
        int a12,
        int a13)
{
#ifdef _WIN32
  /* Two stack-local string buffers initialised from .rdata literals, then
   * lstrcpyA / sprintf. */
  char Format[] = "%c%d%%"; // [esp+8h] [ebp-14h] BYREF
  CHAR String2[] = "100%%"; // [esp+0h] [ebp-1Ch] BYREF
  CHAR String1[12]; // [esp+10h] [ebp-Ch] BYREF
  char sign;

  if ( a1 < a2 )
    sign = 45;
  else
    sign = 32;
  if ( a3 == 100 )
    return lstrcpyA(String1, String2);
  else
    return (LPSTR)sprintf(String1, Format, sign, a3);
#else
  /* Progress callback.  No lstrcpyA off-Windows, so sprintf into a static buffer
   * — safe because this callback is unreachable in the Linux build. */
  static CHAR buf[16];
  if (a3 == 100)
    strcpy(buf, "100%%");
  else
    sprintf(buf, "%c%d%%", a1 < a2 ? 45 : 32, a3);
  return buf;
#endif
}
// gladiator.dll: 10041740..10041748
// gladi386.so:   absent
int __stdcall sub_10041740(int a1, int a2, int a3, int a4)
{
  return 1;
}
// gladiator.dll: 10041760..10041777
// gladi386.so:   absent
int __stdcall sub_10041760(const char *a1, int a2)
{
  botimport.Print(PRT_MESSAGE, a1);
  return a2;
}

#endif /* _WIN32 — UnZip windll path */
// gladiator.dll: 10041790..10041883
// gladi386.so:   00053C3C..00053D94
void __cdecl vectoangles(float *value1, float *angles)
{
  float forward;
  float yaw, pitch;

  if ( value1[1] == 0 && value1[0] == 0 )
  {
    yaw = 0;
    if ( value1[2] > 0 )
      pitch = 90;
    else
      pitch = 270;
  }
  else
  {
    yaw = (float)(int)(atan2(value1[1], value1[0]) * 57.29577951308232);
    if ( yaw < 0 )
      yaw += 360;
    forward = sqrt(value1[0]*value1[0] + value1[1]*value1[1]);
    pitch = (float)(int)(atan2(value1[2], forward) * 57.29577951308232);
    if ( pitch < 0 )
      pitch += 360;
  }
  angles[0] = -pitch;
  angles[1] = yaw;
  angles[2] = 0;
}
// gladiator.dll: 100418D0..100418EE
// gladi386.so:   00053D94..00053DB4
/* In-place path-separator normalisation.  The original folds both '/' and '\\' to
 * '\\'; POSIX must fold to '/' instead, or access()/fopen() reject the path.  The
 * _WIN32 branch is verbatim. */
#ifdef _WIN32
#  define BOTLIB_PATHSEP '\\'
#else
#  define BOTLIB_PATHSEP '/'
#endif

void __cdecl sub_100418D0(_BYTE *a1)
{
  _BYTE *v1; // ecx

  v1 = a1;
  if ( *v1 )
  {
    do
    {
      if ( *v1 == '/' || *v1 == '\\' )
        *v1 = BOTLIB_PATHSEP;
      ++v1;
    }
    while ( *v1 );
  }
}
// gladiator.dll: 10041900..10041943
// gladi386.so:   00053DB4..00053E05
/* Genuinely void: every guard routes to the same bare epilogue and eax is never
 * set — any apparent return value is just the last compare's residue. */
void __cdecl sub_10041900(const char *a1, int a2)
{
  char result; // al
  unsigned int v4; // esi

  v4 = strlen(a1);
  if ( strlen(a1) )
  {
    if ( a2 - (int)v4 > 1 )
    {
      result = a1[v4 - 1];
      if ( result != '/' && result != '\\' )
      {
        ((char *)a1)[v4] = BOTLIB_PATHSEP;
        ((char *)a1)[v4 + 1] = 0;
      }
    }
  }
}

/* PAK directory entry — 64 bytes on either word width.  Declared above the docblock so
 * the ref-funcmap generator attributes the address to the function. */
typedef struct pak_direntry_s {
    char    name[56];
    int32_t filepos;
    int32_t filelen;
} pak_direntry_t;
// gladiator.dll: 10041970..10041B29
// gladi386.so:   00053E08..00054014
int __cdecl sub_10041970(char *FileName, const char *a2, bot_fileref_t *a3)
{
  FILE *v3; // eax
  FILE *v4; // esi
  signed int v7; // ebp
  char *v8; // edi
  signed int v10; // ebx
  char *v11; // esi
  void **v12; // ebx
  char v16[144]; // [esp+20h] [ebp-90h] BYREF
  int Buffer[3]; // [esp+14h] [ebp-9Ch] BYREF

  v3 = fopen(FileName, "rb");
  v4 = v3;
  if ( !v3 ) { return 0; }
  /* 12-byte PAK header: magic(4), dir_offset(4), dir_size(4). */
  if ( fread(Buffer, 1u, 0xCu, v3) != 12
    || Buffer[0] != 1262698832   /* "PACK" magic = 0x4B434150 */
    || fseek(v4, LittleLong(Buffer[1]), SEEK_SET) )  /* seek to directory (Buffer[1] = dir_offset) */
  {
    fclose(v4);
    return 0;
  }
  v7 = (unsigned int)LittleLong(Buffer[2]) >> 6;                    /* number of entries: dir_size / 64 */
  v8 = (char *)GetMemory(v7 << 6); /* allocate nentries*64 bytes */
  if ( fread(v8, 0x40u, v7, v4) != v7 )
  {
    fclose(v4);
    FreeMemory(v8);
    return 0;
  }
  fclose(v4);
  strcpy(v16, a2);
  sub_100418D0(v16);
  v10 = 0;
  if ( v7 <= 0 )
  {
LABEL_11:
    FreeMemory(v8);
    return 0;
  }
  else
  {
    v11 = v8;
    while ( 1 )
    {
      sub_100418D0(v11);
      /* `Q_strcasecmp`, not `Q_stricmp` — the real .so calls the former here
       * (contentseq, 2026-08-16).  They are distinct functions in q_shared.c. */
      if ( !Q_strcasecmp(v11, v16) )
        break;
      ++v10;
      v11 += 64;
      if ( v10 >= v7 )
        goto LABEL_11;
    }
    strcpy(a3->path, FileName);
    /* This lone goto is load-bearing: the shared FreeMemory/return-0 tail plus the v8
     * stack spill (edi is the strcpy rep-movs scratch) is only reproduced by this form —
     * every structured equivalent makes MSVC6 cache v8 in edi. */
    a3->fileofs = LittleLong(((pak_direntry_t *)v8)[v10].filepos);
    a3->filelen = LittleLong(((pak_direntry_t *)v8)[v10].filelen);
    (void)v12;
    FreeMemory(v8);
    return 1;
  }
}
// gladiator.dll: 10041BA0..10041E9A
// gladi386.so:   00054014..00054456
/* Search for file `a3` under base path `a1`, first in the gamedir `Source`, then
 * "baseq2", trying both loose files and pak archives; the result goes to the
 * bot_fileref_t out-param. */
int __cdecl sub_10041BA0(char *a1, char *Source, char *a3, bot_fileref_t *a4)
{
  int v5; // esi
  int v7; // [esp+10h] [ebp-244h]
  /* `subdirs` MUST be one [3][144] array, not [2][144] plus a named third buffer: the
   * loop walks a char* across all three rows, but only rows 0 (subdir) and 1 ("baseq2")
   * are populated and searched, so a separately-named row 2 gets dead-eliminated by /O2
   * — the array's escaping address is what keeps it live.  Each buffer's init is
   * byte[0] = '\0' plus memset of the remaining 143, i.e. a 144-byte zero-fill. */
  char *v4; // ebp
  char FileName[144]; // [esp+14h] [ebp-240h] BYREF
  char subdirs[3][144]; // [esp+A4h] [ebp-1A0h] BYREF — was subdirs[2][144] + v17_buf

  strcpy(subdirs[0], "");
  strcpy(FileName, "");
  memset(&subdirs[0][1], 0, 143);
  strcpy(subdirs[1], "");
  memset(&subdirs[1][1], 0, 143);
  strcpy(subdirs[2], "");
  memset(&subdirs[2][1], 0, 143);
  memset(&FileName[1], 0, 143);
  if ( Source )
    strncpy(subdirs[0], Source, 0x90u);
  strncpy(subdirs[1], "baseq2", 0x90u);
  v7 = 0;
  v4 = subdirs[0];
  do
  {
    FileName[0] = 0;
    if ( a1 && strlen(a1) )
    {
      strncpy(FileName, a1, 0x90u);
      sub_10041900(FileName, 144);
    }
    if ( strlen(v4) )
    {
      strncat(FileName, v4, 144 - strlen(FileName));
      sub_10041900(FileName, 144);
    }
    strncat(FileName, a3, 144 - strlen(FileName));
    sub_100418D0(FileName);
    Log_Write("accessing %s", FileName);
    if ( _access(FileName, 4) )
    {
      v5 = 0;
      while ( 1 )
      {
        FileName[0] = 0;
        if ( a1 && strlen(a1) )
        {
          strncpy(FileName, a1, 0x90u);
          sub_10041900(FileName, 144);
        }
        if ( strlen(v4) )
        {
          strncat(FileName, v4, 144 - strlen(FileName));
          sub_10041900(FileName, 144);
        }
        sprintf(&FileName[strlen(FileName)], "pak%d.pak", v5);
        if ( !_access(FileName, 4) )
        {
          Log_Write("searching %s in %s", a3, FileName); /* "searching %s in %s": file, pak */
          if ( sub_10041970(FileName, a3, a4) )
            return 1;
        }
        if ( ++v5 >= 10 )
          break;
      }
      v4 += 144;  /* step from subdirs[0] to subdirs[1]; array guarantees 144-byte stride */
    }
    else
    {
      strcpy(a4->path, FileName);
      a4->filelen = 0;
      a4->fileofs = 0;
      return 1;
    }
  }
  while ( ++v7 < 2 );
  a4->fileofs = 0;
  a4->filelen = 0;
  return 0;
}
// gladiator.dll: 10041F60..10041FCB
// gladi386.so:   00054458..000544E2
BOOL __cdecl sub_10041F60(char *a1, bot_fileref_t *a2)
{
  if ( sub_10041BA0(
         LibVarGetString("basedir"),
         LibVarGetString("gamedir"),
         a1,
         a2) )
    return 1;
  if ( sub_10041BA0(
         LibVarGetString("cddir"),
         LibVarGetString("gamedir"),
         a1,
         a2) )
    return 1;
  return 0;
}

#ifdef _WIN32  /* ---- ZIP32 windll archive path (ZIP32.DLL): sub_10041FF0 + helpers/callbacks ----
                * Windows-only, and dead even there (no live caller).  Absent on Linux. */
// gladiator.dll: 10041FF0..100422BD
// gladi386.so:   absent
/* Add one file to a zip archive by loading Info-ZIP's ZIP32.DLL and driving its
 * ZpInit / ZpSetOptions / ZpArchive entry points, mirroring the UnZip sibling
 * sub_10041240 above.  Off-Windows the shimmed SearchPathA returns 0, so the body
 * bails before touching any DLL.
 *
 * ZPOPT (248 bytes) and ZCL (12 bytes) are passed BY VALUE, with pointer fields as
 * 4-byte int slots so the byte image is width-independent.  Field names are
 * offset-derived; only the two TRUE flags and the embedded getcwd buffer carry meaning.
 *
 * DEAD in Gladiator — its only caller was an unreachable debug menu entry. */
typedef struct {
  int  o00, o04, o08, o0c, o10, o14;  /* +0x00..+0x14                     */
  int  o18, o1c;                      /* +0x18,+0x1c  (never set -> 0)    */
  int  o20, o24, o28, o2c, o30, o34;  /* +0x20..+0x34 (o24,o30 = TRUE)    */
  int  o38, o3c, o40, o44, o48, o4c;  /* +0x38..+0x4c                     */
  int  o50, o54, o58, o5c;            /* +0x50..+0x5c                     */
  char c60[13];                       /* +0x60..+0x6c                     */
  char c6d;                           /* +0x6d                            */
  char c6e[8];                        /* +0x6e..+0x75                     */
  char rootdir[130];                  /* +0x76..+0xf7  (getcwd target)    */
} ZPOPT;                              /* sizeof == 0xf8 (248)             */

typedef struct {
  int argc;       /* +0x00  number of file names                          */
  int lpszZipFN;  /* +0x04  LPSTR — archive name (4-byte slot)            */
  int FNV;        /* +0x08  char ** — file-name vector (4-byte slot)      */
} ZCL;                                /* sizeof == 0xc (12)               */

typedef struct {
  int print;      /* +0x00  DLLPRNT*    -> sub_100423D0                    */
  int password;   /* +0x04  DLLPASSWORD*-> sub_100423F0                    */
  int comment;    /* +0x08  DLLCOMMENT* -> sub_100423B0                    */
} ZIPUSERFUNCTIONS;                   /* sizeof == 0xc (12)               */

typedef int (__stdcall *ZpArchive_t)(ZCL);
typedef int (__stdcall *ZpSetOptions_t)(ZPOPT);
typedef int (__stdcall *ZpInit_t)(void *);

int __cdecl sub_100423D0(int a1, int a2);
void __stdcall sub_100423F0(char *p);

static ZPOPT zopt;                    /* ds:0x100638e0 */
static ZCL  zcl;                      /* ds:0x10063890 */
static HGLOBAL FNV_handle;            /* ds:0x100639d8 */
static ZIPUSERFUNCTIONS *cbtable;     /* ds:0x100639dc */
static void *zip32_module;            /* ds:0x100639e0 */
static ZpArchive_t   ZpArchive;       /* ds:0x100639e4 */
static ZpSetOptions_t ZpSetOptions;   /* ds:0x100639e8 */
static ZpInit_t      ZpInit;          /* ds:0x100639ec */

int __cdecl sub_10041FF0(const char *zipfile, const char *file_to_archive)
{
  char **FNV;          // esi
  char *fname;         // edi
  int rc;              // esi
  LPSTR FilePart;      // [esp+10h] BYREF
  CHAR Buffer[128];    // [esp+14h] BYREF

  if ( !file_to_archive || !zipfile )
    return 0;
  hMem = GlobalAlloc(0x40u, 0xcu);
  if ( !hMem )
    return 0;
  cbtable = (ZIPUSERFUNCTIONS *)GlobalLock(hMem);
  if ( !cbtable )
  {
    GlobalFree(hMem);
    return 0;
  }
  cbtable->print = (intptr_t)sub_100423D0;
  cbtable->comment = (intptr_t)sub_100423B0;
  cbtable->password = (intptr_t)sub_100423F0;
  if ( !SearchPathA(0, "ZIP32.DLL", 0, 0x80u, Buffer, &FilePart)
    || (zip32_module = LoadLibraryA("ZIP32.DLL")) == 0 )
  {
    sub_10042380();
    return 0;
  }
  ZpArchive = (ZpArchive_t)GetProcAddress(zip32_module, "ZpArchive");
  ZpSetOptions = (ZpSetOptions_t)GetProcAddress(zip32_module, "ZpSetOptions");
  if ( !ZpArchive || !ZpSetOptions )
  {
    sub_10042380();
    return 0;
  }
  ZpInit = (ZpInit_t)GetProcAddress(zip32_module, "ZpInit");
  if ( !ZpInit )
  {
    FreeLibrary(zip32_module);
    sub_10042380();
    return 0;
  }
  if ( !ZpInit(cbtable) )
  {
    FreeLibrary(zip32_module);
    sub_10042380();
    return 0;
  }
  zopt.o00 = 0; zopt.o04 = 0; zopt.o08 = 0; zopt.o0c = 0; zopt.o10 = 0; zopt.o14 = 0;
  zopt.o20 = 0; zopt.o24 = 1; zopt.o28 = 0; zopt.o2c = 0; zopt.o30 = 1; zopt.o34 = 0;
  zopt.o38 = 0; zopt.o3c = 0; zopt.o40 = 0;
  zopt.o48 = 0; zopt.o4c = 0; zopt.o50 = 0; zopt.o54 = 0; zopt.o58 = 0; zopt.o5c = 0;
  zopt.o44 = 0;
  zopt.c6d = 0;
  getcwd_locked(zopt.rootdir, 0x104);
  zcl.argc = 1;
  zcl.lpszZipFN = (intptr_t)zipfile;
  FNV_handle = GlobalAlloc(0x40u, 0x10000u);
  if ( FNV_handle )
    FNV = (char **)GlobalLock(FNV_handle);
  else
    FNV = (char **)FilePart;
  fname = (char *)&FNV[zcl.argc];
  lstrlenA(file_to_archive);
  lstrcpyA(fname, file_to_archive);
  FNV[0] = fname;
  zcl.FNV = (intptr_t)FNV;
  ZpSetOptions(zopt);
  rc = ZpArchive(zcl);
  if ( rc )
    botimport.Print(PRT_ERROR, "Error during archiving.\nUnable to create \"%s\"\n", zipfile);
  GlobalUnlock(FNV_handle);
  GlobalFree(FNV_handle);
  sub_10042380();
  FreeLibrary(zip32_module);
  return rc == 0;
}
// gladiator.dll: 10042380..1004239D
// gladi386.so:   absent
/* void: ref 10042380's only exit is the shared `ret` at 1004239c. */
void sub_10042380()
{

  if ( hMem )
  {
    GlobalUnlock(hMem);
    GlobalFree(hMem);
  }
}
// gladiator.dll: 100423B0..100423B8
// gladi386.so:   absent
int __stdcall sub_100423B0(int a1, int a2, int a3, int a4)
{
  return 1;
}
// gladiator.dll: 100423D0..100423D5
// gladi386.so:   absent
/* Returns its second argument unchanged — a placeholder identity passthrough.
 * DEAD in Gladiator. */
int __cdecl sub_100423D0(int a1, int a2)
{
  (void)a1;
  return a2;
}
// gladiator.dll: 100423F0..100423FA
// gladi386.so:   absent
/* Writes an empty string at `p` — the ZIP32 password callback.
 * DEAD in Gladiator. */
void __stdcall sub_100423F0(char *p)
{
  p[0] = 0;
}

#endif /* _WIN32 — ZIP32 windll archive path */
