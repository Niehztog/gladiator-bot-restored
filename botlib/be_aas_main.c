/*
 * be_aas_main.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x1000D7E0..0x1000EE30; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * Its own interface is in the matching .h; botlib_local.h, which that header
 * pulls in, carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs, the side-band scheme and the externs for
 * botlib.c's remaining globals).
 *
 * Every function below carries a two-line address annotation: its extent in the
 * 1999 Windows `gladiator.dll` (PE32) and in the 1999 Linux `gladi386.so`
 * (ELF32, glibc build), each start..end with the end exclusive.  `absent` means
 * the Linux image has no counterpart.  CLAUDE.md, "Function address
 * annotations", records how both ranges are derived.
 */

#include "botlib_port.h"
#include "l_libvar.h"      /* libvar_t: 24-byte botlib cvar (reconstructed) */
#undef VectorNegate
#include "be_ea.h"    /* ea_state_t: 36-byte per-client EA struct (reconstructed) */
#include "q2files.h"       /* Q2 BSP file format (+ the BSP header) */
#include "aasfile.h"       /* AAS file format: aas_lump_t, aas_header_t */
#include "be_aas_def.h"   /* aas_t, aas_area_t etc. (reconstructed from aasworld_* globals) */
#include "l_script.h"      /* token_t, script_t, punctuation_t */
#include "l_precomp.h"     /* source_t, define_t */
#include "l_struct.h"      /* structdef_t */
#include "l_utils.h"       /* bot_fileref_t + the Win32 UnZip declarations */
#include "be_ai_def.h"     /* the bot-AI structures and interfaces */
#include "be_interface.h"   /* botimport / botstate / bot_exports + libvar aliases */
#include "struct_sizes_asserts.h" /* compile-time struct-layout guard */
#include "be_aas_main.h"
#include "be_aas_bspq2.h"
#include "be_aas_cluster.h"
#include "be_aas_entity.h"
#include "be_aas_file.h"
#include "be_aas_light.h"
#include "be_aas_optimize.h"
#include "be_aas_reach.h"
#include "be_aas_route.h"
#include "be_aas_routealt.h"
#include "be_aas_sample.h"
#include "be_aas_sound.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_utils.h"

#if BOTLIB_NEED_SIDEBAND
aas_link_t **aasentity_arealinks;
bsp_link_t **aasentity_bsplinks;
#else
#endif

aas_world_t aasworld;

// gladiator.dll: 1000D7E0..1000D811
// gladi386.so:   000181F8..00018240
int AAS_Error(char *Format, ...)
{
  char Buffer[1024]; // [esp+0h] [ebp-400h] BYREF
  va_list va; // [esp+408h] [ebp+8h] BYREF

  va_start(va, Format);
  vsprintf(Buffer, Format, va);
  return botimport.Print(PRT_FATAL, Buffer);
}
// gladiator.dll: 1000D830..1000D8AF
// gladi386.so:   00018240..000182D7
char *__cdecl AAS_StringFromIndex(const char *indexname, indexlist_t *list, int index)
{
  if ( !aasworld.indexes_loaded )
  {
    botimport.Print(PRT_ERROR, "%s: index %d not setup\n", indexname, index);
    return "";
  }
  if ( index < 0 || index >= list->numindexes )
  {
    botimport.Print(PRT_ERROR, "%s: index %d out of range\n", indexname, index);
    return "";
  }
  if ( !list->indexes[index] )
  {
    if ( index )
    {
      botimport.Print(PRT_ERROR, "%s: reference to unused index %d\n", indexname, index);
    }
    return "";
  }
  return list->indexes[index];
}
// gladiator.dll: 1000D8D0..1000D936
// gladi386.so:   000182D8..00018358
int __cdecl AAS_IndexFromString(const char *indexname, indexlist_t *list, char *String2)
{
  int i; // esi
  const char *v5; // eax

  if ( !aasworld.indexes_loaded )
  {
    botimport.Print(PRT_ERROR, "%s: index not setup \"%s\"\n", indexname, String2);
    return 0;
  }
  for ( i = 0; i < list->numindexes; i++ )
  {
    v5 = list->indexes[i];
    if ( v5 && !_strcmpi(v5, String2) )
      return i;
  }
  return 0;
}
// gladiator.dll: 1000D960..1000D97A
// gladi386.so:   00018358..000183F3
char *__cdecl AAS_ModelFromIndex(int index)
{
  return AAS_StringFromIndex("ModelFromIndex", aasworld.modelindex_table, index);
}
// gladiator.dll: 1000D990..1000D9AA
// gladi386.so:   000183F4..00018478
int __cdecl IndexFromModel(char *String2)
{
  return AAS_IndexFromString("IndexFromModel", aasworld.modelindex_table,
                             String2);
}
// gladiator.dll: 1000D9C0..1000D9DA
// gladi386.so:   00018478..00018513
// Pushes "SoundFromIndex" + aasworld.soundindex_table,
// tail-calls AAS_StringFromIndex thunk at 0x10001E01 -> 0x1000D830.
char *__cdecl AAS_SoundFromIndex(int index)
{
  return AAS_StringFromIndex("SoundFromIndex", aasworld.soundindex_table, index);
}
// gladiator.dll: 1000D9F0..1000DA0A
// gladi386.so:   00018514..00018598
// Mirror of IndexFromModel against the
// soundindex_table; tail-calls AAS_IndexFromString thunk at 0x100012C1.
int __cdecl AAS_IndexFromSound(char *String2)
{
  return AAS_IndexFromString("IndexFromSound", aasworld.soundindex_table,
                             String2);
}
// gladiator.dll: 1000DA20..1000DA3A
// gladi386.so:   00018598..00018633
char *__cdecl AAS_ImageFromIndex(int index)
{
  return AAS_StringFromIndex("ImageFromIndex", aasworld.imageindex_table, index);
}
// gladiator.dll: 1000DA50..1000DA6A
// gladi386.so:   00018634..000186B8
// Mirror of IndexFromModel against the
// imageindex_table; tail-calls AAS_IndexFromString thunk at 0x100012C1.
int __cdecl AAS_IndexFromImage(char *String2)
{
  return AAS_IndexFromString("IndexFromImage", aasworld.imageindex_table,
                             String2);
}
// gladiator.dll: 1000DA80..1000DB06
// gladi386.so:   000186B8..00018A1E
indexlist_t *__cdecl sub_1000DA80(int numindexes, char **names)
{
  indexlist_t *list; // ebp
  int v4; // ebx
  const char *v5; // edi

  list = (indexlist_t *)GetClearedMemory(sizeof(indexlist_t) + numindexes * sizeof(char *));
  list->indexes = (char **)(list + 1);
  list->numindexes = numindexes;
  for ( v4 = 0; v4 < numindexes; ++v4 )
  {
    v5 = names[v4];
    if ( v5 )
    {
      list->indexes[v4] = (char *)GetMemory(strlen(v5) + 1);
      strcpy(list->indexes[v4], names[v4]);
    }
  }
  return list;
}
// gladiator.dll: 1000DB40..1000DBB0
// gladi386.so:   00018A20..00018BCE
void __cdecl sub_1000DB40(indexlist_t *list, int numindexes, char **names)
{
  int i; // ebx
  const char *v5; // edi

  for ( i = 0; i < numindexes; ++i )
  {
    v5 = names[i];
    if ( v5 )
    {
      if ( !list->indexes[i] )
      {
        list->indexes[i] = (char *)GetMemory(strlen(v5) + 1);
        strcpy(list->indexes[i], names[i]);
      }
    }
  }
  { (void)(numindexes); return; }
}
// gladiator.dll: 1000DBD0..1000DC03
// gladi386.so:   00018BD0..00018C11
int __cdecl sub_1000DBD0(indexlist_t *list)
{
  int i; // esi

  for ( i = 0; i < list->numindexes; ++i )
  {
    if ( list->indexes[i] )
      FreeMemory(list->indexes[i]);
  }
  return FreeMemory(list);
}
// gladiator.dll: 1000DC20..1000DCA0
// gladi386.so:   00018C14..000199D5
void __cdecl sub_1000DC20(int a1, char **a2, int a3, char **a4, int a5, char **a6)
{
  if ( aasworld.modelindex_table )
    sub_1000DBD0(aasworld.modelindex_table);
  if ( aasworld.soundindex_table )
    sub_1000DBD0(aasworld.soundindex_table);
  if ( aasworld.imageindex_table )
    sub_1000DBD0(aasworld.imageindex_table);
  aasworld.modelindex_table = sub_1000DA80(a1, a2);
  aasworld.soundindex_table = sub_1000DA80(a3, a4);
  aasworld.imageindex_table = sub_1000DA80(a5, a6);
  aasworld.indexes_loaded = 1;
}
// gladiator.dll: 1000DCC0..1000DD6E
// gladi386.so:   000199D8..0001AE25
void __cdecl sub_1000DCC0(int a1, char **a2, int a3, char **a4, int a5, char **a6)
{
  if ( aasworld.modelindex_table )
    sub_1000DB40(aasworld.modelindex_table, a1, a2);
  else
    aasworld.modelindex_table = sub_1000DA80(a1, a2);
  if ( aasworld.soundindex_table )
    sub_1000DB40(aasworld.soundindex_table, a3, a4);
  else
    aasworld.soundindex_table = sub_1000DA80(a3, a4);
  if ( aasworld.imageindex_table )
    sub_1000DB40(aasworld.imageindex_table, a5, a6);
  else
    aasworld.imageindex_table = sub_1000DA80(a5, a6);
  aasworld.indexes_loaded = 1;
}
// gladiator.dll: 1000DDA0..1000DE97
// gladi386.so:   0001AE28..0001AEE1
/* AAS_PresenceTypeBoundingBox (Q3 be_aas_sample.c).  The Q2 player bbox is
 * 32x32 (-16..16), not Q3's 30x30.  Presence types here are 4=NORMAL,
 * 2=CROUCH (Q3 uses 1/2).  Declared int (unlike Q3's void) but never
 * returns a value; falls off the end, leaving whatever the index*12
 * array-offset arithmetic last left in eax as the (unused) return value. */
int __cdecl AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs)
{
  int    index;
  vec3_t boxmins[3] = { {0.0f, 0.0f, 0.0f}, {-16.0f, -16.0f, -24.0f}, {-16.0f, -16.0f, -24.0f} };
  vec3_t boxmaxs[3] = { {0.0f, 0.0f, 0.0f}, { 16.0f,  16.0f,  32.0f}, { 16.0f,  16.0f,   8.0f} };

  if ( presencetype == 4 )
    index = 1;
  else if ( presencetype == 2 )
    index = 2;
  else
  {
    botimport.Print(PRT_FATAL,
             "AAS_PresenceTypeBoundingBox: unknown presence type\n");
    index = 2;
  }
  VectorCopy(boxmins[index], mins);
  VectorCopy(boxmaxs[index], maxs);
}
// gladiator.dll: 1000DEE0..1000DEE6
// gladi386.so:   0001AEE4..0001AEFC
int AAS_Initialized()
{
  return aasworld.initialized;
}
// gladiator.dll: 1000DF00..1000DF1B
// gladi386.so:   0001AEFC..0001AF2F
// Sets aasworld.initialized = 1, then prints "AAS initialized." at level 1.
// Q3 be_aas_main.c AAS_SetInitialized: sets aasworld.initialized and prints
// "AAS initialized.\n". Called from AAS_ContinueInit's tail (see 1000DF30).
// Returns bi_Print's value (Q3 declares it void).
int __cdecl AAS_SetInitialized(void)
{
  aasworld.initialized = 1;
  return botimport.Print(PRT_MESSAGE, "AAS initialized.\n");
}
// gladiator.dll: 1000DF30..1000DFD4
// gladi386.so:   0001AF30..0001B057
int AAS_ContinueInit(float time)
{
  int result; // eax

  result = aasworld.loaded;
  if ( aasworld.loaded )
  {
    result = aasworld.initialized;
    if ( !aasworld.initialized )
    {
      result = AAS_ContinueInitReachability(time);
      if ( !result )
      {
        AAS_InitClustering();
        if ( aasworld.savefile || (unsigned int)(int)LibVarGetValue("forcewrite") )
        {
          if ( !(unsigned int)(int)LibVarGetValue("nooptimize") )
            AAS_Optimize();
          if ( AAS_WriteAASFile(aasworld.filename) )
            botimport.Print(PRT_MESSAGE, "%s written succesfully\n", aasworld.filename);
          else
            botimport.Print(PRT_ERROR, "couldn't write %s\n", aasworld.filename);
        }
        AAS_InitRouting();
        return AAS_SetInitialized();
      }
    }
  }
  return result;
}
// gladiator.dll: 1000E010..1000E0D6
// gladi386.so:   0001B058..0001B150
int AAS_StartFrame(float time)
{
  aasworld.time = time;
  AAS_InvalidateEntities();
  sub_1001CFA0(time);
  sub_1000D4E0(time);
  AAS_ContinueInit(time);
  aasworld.frameroutingupdates = 0;
  if ( LibVarGetValue("showcacheupdates") != 0.0f )
  {
    AAS_RoutingInfo();
    LibVarSet("showcacheupdates", (char *)"0");
  }
  if ( LibVarGetValue("showmemoryusage") != 0.0f )
  {
    PrintUsedMemorySize();
    LibVarSet("showmemoryusage", (char *)"0");
  }
  if ( LibVarGetValue("memorydump") != 0.0f )
  {
    PrintMemoryLabels();
    LibVarSet("memorydump", (char *)"0");
  }
  return 0;
}
// gladiator.dll: 1000E120..1000E127
// gladi386.so:   0001B150..0001B168
float AAS_Time()
{
  return aasworld.time;
}

#ifdef _WIN32  /* ---- winbspc spawn (sub_1000E140) + aasN.zip search (sub_1000E430): Windows-only ----
                * The Linux botlib has neither: BotLibLoadMap loads .aas directly and, on
                * failure, just reports "no AAS file available" (see its #else give-up path). */
// gladiator.dll: 1000E140..1000E38A
// gladi386.so:   absent
intptr_t __cdecl sub_1000E140(char *Source)
{
  intptr_t result; // eax
  char FileName[144]; // [esp+8h] [ebp-240h] BYREF
  char Arguments[144]; // [esp+98h] [ebp-1B0h] BYREF
  char Destination[144]; // [esp+128h] [ebp-120h] BYREF
  char Buffer[144]; // [esp+1B8h] [ebp-90h] BYREF

  strncpy(Destination, Source, 0x90u);
  strncat(Destination, ".bsp", 144 - strlen(Destination));
  strncpy(FileName, (const char *)LibVarGetString("basedir"), 0x90u);
  sub_10041900(FileName, 144 - strlen(FileName));
  strncat(FileName, (const char *)LibVarGetString("gamedir"), 144 - strlen(FileName));
  sub_10041900(FileName, 144 - strlen(FileName));
  strncpy(Arguments, FileName, 0x90u);
  strncat(FileName, "maps", 144 - strlen(FileName));
  if ( _access(FileName, 4) )
    FileName[strlen(FileName) - 4] = 0;  /* maps dir not accessible: strip the "maps" just appended */
  else
    sub_10041900(FileName, 144 - strlen(FileName));
  strncat(FileName, Source, 144 - strlen(FileName));
  strncat(FileName, ".aas", 144 - strlen(FileName));
  strncat(Arguments, "winbspc.exe", 144 - strlen(Arguments));
  Log_Write("spawning \"%s\"", Arguments);
  sprintf(Buffer, "bsp2aas(%s,%s);", Destination, FileName);
  result = SpawnProcess(1, Arguments, Arguments, Buffer, 0);
  if ( result < 0 )
    return botimport.Print(PRT_ERROR, "can't execute WinBSPC\n");
  return result;
}
// gladiator.dll: 1000E430..1000E79C
// gladi386.so:   absent
int __cdecl sub_1000E430(char *Source)
{
  const char *v1; // esi
  const char *v2; // ebx
  int v3; // ebp
  char *v4; // ebx
  int i; // esi
  int v7; // esi
  /* Each of these buffers is 144 bytes, so the strncpy/strncat bounds of 144
   * cannot overrun. */
  char Destination[144]; // [esp+10h] [ebp-360h] BYREF
  char ArgList[144]; // [esp+A0h] [ebp-2D0h] BYREF
  char Path[144]; // [esp+130h] [ebp-240h] BYREF
  /* The three search-dir buffers are ONE contiguous char[3][144] array, walked
   * by a char* stepping 144.  All three rows are zero-inited even though only
   * dirs[0] (gamedir) and dirs[1] ("baseq2") are ever populated or searched.
   * It MUST be a real array, not three named locals: the escaping address is
   * what keeps /O2 from dead-eliminating the unused dirs[2]. */
  char dirs[3][144]; // [esp+1C0h] [ebp-1B0h] BYREF

  strcpy(dirs[0], "");
  strcpy(Destination, "");
  memset(&dirs[0][1], 0, 143);
  strcpy(dirs[1], "");
  memset(&dirs[1][1], 0, 143);
  strcpy(dirs[2], "");
  memset(&dirs[2][1], 0, 143);
  memset(&Destination[1], 0, 143);
  v1 = (const char *)LibVarGetString("basedir");
  v2 = (const char *)LibVarGetString("gamedir");
  if ( v1 && strlen(v1) )
  {
    strncat(Destination, v1, 0x90u);
    sub_10041900(Destination, 144);
  }
  if ( v2 && strlen(v2) )
  {
    strncat(Destination, v2, 144 - strlen(Destination));
    sub_10041900(Destination, 144);
  }
  getcwd_locked((int)Path, 144);
  _chdir(Destination);
  if ( v2 )
    strncpy(dirs[0], v2, 0x90u);
  strncpy(dirs[1], "baseq2", 0x90u);
  strncpy(ArgList, Source, 144 - strlen(ArgList));
  strncat(ArgList, ".aas", 144 - strlen(ArgList));
  v3 = 0;
  v4 = dirs[0];
  for ( ; v3 < 2; ++v3, v4 += 144 )
  {
    for ( i = 0; i < 10; ++i )
    {
      strcpy(Destination, "..");
      sub_10041900(Destination, 144);
      if ( strlen(v4) )
      {
        strncat(Destination, v4, 144 - strlen(Destination));
        sub_10041900(Destination, 144);
      }
      sprintf(&Destination[strlen(Destination)], "aas%d.zip", i);
      if ( !_access(Destination, 4) )
      {
        Log_Write("searching %s in %s", ArgList, Destination); /* "searching %s in %s" */
        if ( sub_10041240((int)Destination, ArgList, 0) )
        {
          v7 = AAS_LoadAASFile(ArgList, 0, 0);
          errno = v7;
          if ( errno )
            return errno;
          remove_file(ArgList);
          botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", Destination, ArgList);
          Log_Write("found %s in %s", ArgList, Destination); /* "found %s in %s" */
          _chdir(Path);
          return 0;
        }
        Log_Write("could not find %s in %s", ArgList, Destination); /* "could not find %s in %s" */
      }
    }
  }
  /* v4 walks the next dir row (gamedir -> "baseq2"); array contiguity makes this portable */
  _chdir(Path);
  return BLERR_NOAASFILE;
}

#endif /* _WIN32 — winbspc spawn + aasN.zip search */
// gladiator.dll: 1000E880..1000EBE2
// gladi386.so:   0001B170..0001B531
int BotLibLoadMap(char *Source)
{
  int v2; // esi
  int v4; // esi
#ifdef _WIN32
  int v5; // esi  (holds sub_1000E430 result — Windows-only aasN.zip fallback)
#endif
  int errnum; // esi
  bot_fileref_t v7; // [esp+Ch] [ebp-1B8h] BYREF
  char Destination[144]; // [esp+A4h] [ebp-120h] BYREF
  char aasfile[144]; // [esp+134h] [ebp-90h] BYREF

  strcpy(aasworld.mapname, Source);
  AAS_ResetEntityLinks();
  memset(&v7, 0, sizeof(v7));
  strncpy(Destination, "maps\\", 0x90u);
  strncat(Destination, Source, 144 - strlen(Destination));
  strncat(Destination, ".bsp", 144 - strlen(Destination));
  if ( sub_10041F60(Destination, &v7) )
  {
    v2 = AAS_LoadBSPFile(v7.path, v7.fileofs, v7.filelen);
    errno = v2;
    if ( errno )
    {
      return errno;
    }
    else
    {
      if ( v7.filelen )
        botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", v7.path, Destination);
      else
        botimport.Print(PRT_MESSAGE, "loaded %s\n", Destination);
      memset(&v7, 0, sizeof(v7));
      v4 = 0;
      while ( 1 )
      {
        if ( v4 )
          strncpy(aasfile, "maps\\", 0x90u);
        else
          strncpy(aasfile, "", 0x90u);
        strncat(aasfile, Source, 144 - strlen(aasfile));
        strncat(aasfile, ".aas", 144 - strlen(aasfile));
        if ( sub_10041F60(aasfile, &v7) )
          break;
        if ( ++v4 >= 2 )
        {
#ifdef _WIN32
          v5 = sub_1000E430(Source);
          errno = v5;
          if ( !errno )
            return BLERR_NOERROR;
          if ( LibVarValue("autolaunchbspc", (char *)"0") != 0 )
          {
            sub_1000E140(Source);
            botimport.Print(
              5,
              "\n"
              "creating AAS for %s...\n"
              "\n"
              "This may take several minutes\n"
              "\n"
              "You cannot play the map %s with\n"
              "bots before AAS (%s.aas) has been\n"
              "created.\n"
              "\n"
              "You probably want to close Quake2 now\n"
              "to free up processing power for the\n"
              "tool which creates the AAS file.\n"
              "\n",
              Source,
              Source,
              Source);
          }
#else
          /* Faithful Linux give-up path: the Linux botlib has no UNZIP32/ZIP32 windll
           * and no winbspc spawn, so it never tries the aasN.zip fallback — it sets
           * errno=5 and the autolaunchbspc branch only reports that BSPC is a Win32
           * program. */
          errno = 5;
          if ( LibVarValue("autolaunchbspc", (char *)"0") != 0 )
            botimport.Print(PRT_MESSAGE, "the BSPC tool is a Win32 program\n");
#endif
          botimport.Print(PRT_FATAL, "no AAS file available\n");
          return BLERR_NOAASFILE;
        }
      }
      errnum = AAS_LoadAASFile(v7.path, v7.fileofs, v7.filelen);
      errno = errnum;
      if ( errno )
        return errno;
      if ( v7.fileofs )
        botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", v7.path, aasfile);
      else
        botimport.Print(PRT_MESSAGE, "loaded %s\n", v7.path);
      if ( v7.fileofs )
        strncpy(aasworld.filename, aasfile, 0x90u);
      else
        strncpy(aasworld.filename, v7.path, 0x90u);
      return BLERR_NOERROR;
    }
  }
  else
  {
    botimport.Print(PRT_FATAL, "couldn't find the bsp file %s\n", Destination);
    return BLERR_NOBSPFILE;
  }
}
// gladiator.dll: 1000ECD0..1000ED81
// gladi386.so:   0001B534..0001B5EB
int __cdecl BotLoadMap(char *Source, int a2, char **a3, int a4, char **a5, int a6, char **a7)
{
  if ( !Source )
  {
    sub_1000DCC0(a2, a3, a4, a5, a6, a7);
    return BLERR_NOERROR;
  }
  aasworld.initialized = 0;
  sub_1000DC20(a2, a3, a4, a5, a6, a7);
  AAS_FreeRoutingCaches();
  errno = BotLibLoadMap(Source);
  if ( errno )
  {
    aasworld.loaded = 0;
    return errno;
  }
  AAS_InitAASLinkHeap();
  AAS_InitAASLinkedEntities();
  AAS_InitReachability();
  sub_1001D140();
  sub_1001AB80();
  return BLERR_NOERROR;
}
// gladiator.dll: 1000EDC0..1000EE0C
// gladi386.so:   0001B5EC..0001B663
int __cdecl sub_1000EDC0(int a1, int a2)
{
  aasworld.numentities = a1;
  aasworld.aas_maxclients = a2;
  if ( aasworld.entities )
    FreeMemory(aasworld.entities);
  aasworld.entities = (aas_entity_t *)GetClearedMemory(sizeof(aas_entity_t) * a1);
#if BOTLIB_NEED_SIDEBAND
  if ( aasentity_arealinks )
    FreeMemory(aasentity_arealinks);
  aasentity_arealinks = (aas_link_t **)GetClearedMemory(sizeof(aas_link_t *) * a1);
  if ( aasentity_bsplinks )
    FreeMemory(aasentity_bsplinks);
  aasentity_bsplinks = (bsp_link_t **)GetClearedMemory(sizeof(bsp_link_t *) * a1);
#endif
  sub_1001D260();
  AAS_InvalidateEntities();
  return 0;
}
// gladiator.dll: 1000EE30..1000EE81
// gladi386.so:   0001B664..0001B6DC
int AAS_Shutdown()
{
  AAS_FreeRoutingCaches();
  sub_1001D290();
  AAS_FreeAASLinkHeap();
  AAS_FreeAASLinkedEntities();
  AAS_DumpAASData();
  if ( aasworld.entities )
    FreeMemory(aasworld.entities);
  /* Zero the whole 676-byte state; sizeof(aasworld) == the original's 0x2A4. */
  memset(&aasworld, 0, sizeof(aasworld));
  aasworld.initialized = 0;
  return botimport.Print(PRT_MESSAGE, "AAS shutdown.\n");
}

/* ------------------------------------------------------------------------
 * Present in gladi386.so, ABSENT from gladiator.dll.  See the identical note
 * in be_aas_route.c for why these are gated rather than added outright.
 * ------------------------------------------------------------------------ */
#ifndef _WIN32
/* F184 @ 0x0001b168, 6 bytes -- `mov eax,5; ret`, nothing else.  It has no
 * callers anywhere in the image and no string, constant or call to identify
 * it by, so there is no name to recover and none is invented: the identifier
 * is the symbol gladi386.so ships.  It sits between AAS_Time (F183) and
 * BotLibLoadMap (F185).  The 5 is almost certainly a presence-type or version
 * constant, but "almost certainly" is not evidence and it is not recorded as
 * one. */
int __cdecl F184(void)
{
  return 5;
} //end of the function F184
#endif /* !_WIN32 -- gladi386.so-only */
