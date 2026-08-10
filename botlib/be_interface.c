/*
 * be_interface.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x100376B0..0x10038480; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * Its own interface is in the matching .h; botlib_local.h, which that header
 * pulls in, carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs, the side-band scheme and the externs for
 * botlib.c's remaining globals).
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
#include "be_interface.h"
#include "be_aas_bspq2.h"
#include "be_aas_entity.h"
#include "be_aas_light.h"
#include "be_aas_main.h"
#include "be_aas_sound.h"
#include "be_ai2_main.h"
#include "be_ea.h"
#include "l_crc.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_precomp.h"

botimport_block_t botimport;   /* block 2 @0x10063FE0 — engine import callbacks */

botstate_block_t  botstate;    /* block 1 @0x10064020 — setup flag + counts + libvars */

bot_export_t      bot_exports; /* block 3 @0x10063F80 — exported API table */

/* filecrcs (name recovered from the Linux gladi386.so .dynsym, where it is a
 * 736-byte global whose contents are byte-identical to this table) — 91 entries
 * of { uint16 crc16, uint16 pad, uint32 flags } plus a NULL terminator.
 * This is the config-file CRC whitelist of the integrity subsystem that also
 * owns CRC_Block / sub_10037820 and the "You are not allowed to modify the bot
 * characters" message, NOT the weapon table an earlier annotation guessed; the
 * flags column (1/3/4/6/8/0x20/0x40) is an unidentified file category.
 * sub_100377E0 scans it up to &unk_1005E958, so that symbol MUST stay
 * immediately after this array (guaranteed by declaration order in a TU). */
int filecrcs[] = {
    0x0000A991, 0x00000001,
    0x0000A757, 0x00000001,
    0x00007267, 0x00000001,
    0x00007A0D, 0x00000001,
    0x0000937C, 0x00000001,
    0x0000CF9B, 0x00000001,
    0x0000C661, 0x00000001,
    0x0000AAA3, 0x00000001,
    0x00009795, 0x00000001,
    0x00009C59, 0x00000001,
    0x00002528, 0x00000001,
    0x000055B2, 0x00000001,
    0x0000879D, 0x00000001,
    0x0000AE75, 0x00000001,
    0x0000E512, 0x00000001,
    0x0000218B, 0x00000001,
    0x00008E97, 0x00000001,
    0x00007437, 0x00000001,
    0x00000AE2, 0x00000001,
    0x000007C1, 0x00000001,
    0x00005CAD, 0x00000001,
    0x000074D6, 0x00000001,
    0x0000694A, 0x00000001,
    0x00000E67, 0x00000001,
    0x0000F2C4, 0x00000001,
    0x0000EB92, 0x00000001,
    0x00006322, 0x00000001,
    0x0000B8A5, 0x00000001,
    0x0000E1CC, 0x00000001,
    0x00004E75, 0x00000001,
    0x00002BB0, 0x00000001,
    0x0000C54F, 0x00000001,
    0x0000CDD2, 0x00000001,
    0x0000DD83, 0x00000001,
    0x00000CA7, 0x00000001,
    0x0000107E, 0x00000001,
    0x00002874, 0x00000001,
    0x0000CE27, 0x00000001,
    0x0000DADC, 0x00000001,
    0x000097A9, 0x00000001,
    0x0000A84B, 0x00000001,
    0x000036FC, 0x00000001,
    0x000090DA, 0x00000001,
    0x00005214, 0x00000001,
    0x0000D714, 0x00000001,
    0x00009384, 0x00000001,
    0x00006490, 0x00000001,
    0x00001617, 0x00000001,
    0x00007113, 0x00000001,
    0x0000CEFE, 0x00000001,
    0x000060E6, 0x00000001,
    0x00001F50, 0x00000001,
    0x0000C7F8, 0x00000001,
    0x0000568B, 0x00000001,
    0x00007CF6, 0x00000001,
    0x00000A17, 0x00000001,
    0x00005491, 0x00000001,
    0x00002920, 0x00000001,
    0x0000C438, 0x00000001,
    0x0000B379, 0x00000001,
    0x00003418, 0x00000001,
    0x0000AC0B, 0x00000006,
    0x000035FB, 0x00000006,
    0x00005FC8, 0x00000006,
    0x0000A486, 0x00000006,
    0x00009AAF, 0x00000006,
    0x000020C2, 0x00000006,
    0x0000FB60, 0x00000006,
    0x00004FDE, 0x00000006,
    0x0000F0AB, 0x00000004,
    0x0000A9D4, 0x00000004,
    0x0000DF88, 0x00000004,
    0x0000E5CC, 0x00000004,
    0x00000ED6, 0x00000004,
    0x00008BE0, 0x00000006,
    0x0000A236, 0x00000006,
    0x00000BCB, 0x00000006,
    0x0000CC7C, 0x00000006,
    0x00003E22, 0x00000006,
    0x00000E04, 0x00000006,
    0x00004578, 0x00000006,
    0x0000343F, 0x00000006,
    0x0000FE11, 0x00000040,
    0x00008C2E, 0x00000040,
    0x0000C665, 0x00000040,
    0x00008AC0, 0x00000003,
    0x0000B1B7, 0x00000006,
    0x00006A8E, 0x00000008,
    0x00008DF3, 0x00000008,
    0x0000BC7D, 0x00000008,
    0x0000E488, 0x00000020,
    0x00000000, 0x00000000,   /* NULL terminator (entry 91) */
};
/* End-of-table sentinel — must immediately follow filecrcs. */
int unk_1005E958 = 0;

/* dword_10063F2C — head of the filename-sorted scriptcrc_t list (an int in
 * the 32-bit original; must be a real pointer here).  Referenced only from
 * this TU (sub_100376B0 / sub_100377E0 / sub_10037880), which is also where
 * the .so puts its neighbours filecrcs and dumpcrcs. */
struct scriptcrc_s *dword_10063F2C; // weak

/* The bot_export_t wrappers below (0x100379E0..0x10038460) were held in a
 * separate botlib_exports.c during reconstruction; every one of their
 * addresses falls inside this TU's range, and Q3's be_interface.c holds the
 * same wrappers next to GetBotLibAPI, so they are back where they belong. */

//----- (100376B0) --------------------------------------------------------
/* Register or look up a (filename, CRC) pair in the sorted dword_10063F2C
 * list: on a name hit return the strcmpi result of that record, else allocate a
 * scriptcrc_t, store hash+name and insert it alphabetically. */
void __cdecl sub_100376B0(char *String1, __int16 a2)
{
  scriptcrc_t *v2; // esi
  scriptcrc_t *v5;
  scriptcrc_t *v6; // edi (prev)
  scriptcrc_t *v4; // ebx (new record)
  int result;

  /* Get-or-insert: walk the sorted list, break on a name match, then let ONE
   * shared `if (v2) return;` distinguish match from exhaustion.  An inline
   * early-return drops that shared re-check. */
  for ( v2 = dword_10063F2C; v2; v2 = v2->next )
  {
    if ( !_strcmpi(String1, v2->name) )
      break;
  }
  if ( v2 )
    return;
  v4 = (scriptcrc_t *)GetClearedMemory(sizeof(scriptcrc_t));
  v4->hash = a2;
  strcpy(v4->name, String1);
  result = 0;
  v6 = NULL;
  v5 = dword_10063F2C;
  if ( v5 )
  {
    while ( 1 )
    {
      result = _strcmpi(v4->name, v5->name);
      if ( result < 0 )
        break;
      v6 = v5;
      v5 = v5->next;
      if ( !v5 )
      {
        if ( !v6 )
          goto LABEL_14;
        v6->next = v4;
        v4->next = NULL;
        { (void)(result); return; }
      }
    }
    v4->next = v5;
    if ( v6 )
      v6->next = v4;
    else
      dword_10063F2C = v4;
    { (void)(result); return; }
  }
  else
  {
LABEL_14:
    dword_10063F2C = v4;
  }
  v4->next = NULL;
  { (void)(result); return; }
}

//----- (100377E0) --------------------------------------------------------
int __cdecl sub_100377E0(char *String1, __int16 a2)
{
  _WORD *v2;

  v2 = &filecrcs;
  while ( (int)v2 < (int)&unk_1005E958 )
  {
    if ( (_WORD)a2 == *v2 )
      break;
    v2 += 4;
  }
  sub_100376B0(String1, a2);
  return 256;
}

//----- (10037820) --------------------------------------------------------
/* CRC-hash the buffer and register the (name, crc) pair — byte-identical to
 * sub_10037850 below; /INCREMENTAL kept two compiled copies of the same
 * source function. */
int __cdecl sub_10037820(char *name, const unsigned char *buf, int len)
{
  __int16 crc; // ax

  crc = CRC_Block(buf, len);
  return sub_100377E0(name, crc);
}

//----- (10037850) --------------------------------------------------------
int __cdecl sub_10037850(char *String1, const unsigned char *a2, int a3)
{
  __int16 v3; // ax

  v3 = CRC_Block(a2, a3);
  return sub_100377E0(String1, v3);
}

//----- (10037880) --------------------------------------------------------
// Walks the dword_10063F2C scriptcrc linked
// list and Log_Write's each entry as a C array initializer line:
//   \t{0x%04X, 1}, //name
// Format string at 0x1005E9EC, next-pointer at scriptcrc_t offset +0x94
// (+148 on the 32-bit binary).  This was a dev tool for dumping the
// known-CRC table to be pasted into a config; dead in the shipped DLL.
void __cdecl sub_10037880(void)
{
  scriptcrc_t *p;

  for ( p = dword_10063F2C; p; p = p->next )
    Log_Write("\t{0x%04X, 1}, //%s", (unsigned int)(unsigned __int16)p->hash, p->name);
}

//----- (100378C0) --------------------------------------------------------
int Sys_MilliSeconds()
{
  return clock() * 1000 / CLOCKS_PER_SEC;
}

//----- (10037900) --------------------------------------------------------
qboolean __cdecl ValidClientNumber(int num, const char *str)
{
  if ( num >= 0 && num <= botstate.num_clients )
    return 1;
  botimport.Print(PRT_ERROR, "%s: invalid client number %d, [0, %d]\n", str, num, botstate.num_clients);
  return 0;
}

//----- (10037950) --------------------------------------------------------
qboolean __cdecl ValidEntityNumber(int num, const char *str)
{
  if ( num >= 0 && num <= botstate.num_entities )
    return 1;
  botimport.Print(PRT_ERROR, "%s: invalid entity number %d, [0, %d]\n", str, num, botstate.num_entities);
  return 0;
}

//----- (100379A0) --------------------------------------------------------
qboolean __cdecl BotLibSetup(const char *str)
{
  if ( !botstate.setup )
  {
    botimport.Print(PRT_ERROR, "%s: bot library used before being setup\n", str);
    return 0;
  }
  return 1;
}

/* ---- SLOT 0 — BotVersion ------------------------------------------------ */
//----- (100379E0) --------------------------------------------------------
char *Export_BotVersion(void)
{
    return "BotLib v0.96";
}

//----- (10037A00) --------------------------------------------------------
int BotSetupMoveAI()
{
  libvar_t *result;

  libvar_sv_friction = LibVar("sv_friction", (char *)"6");
  libvar_sv_stopspeed = LibVar("sv_stopspeed", (char *)"100");
  libvar_sv_gravity = LibVar("sv_gravity", (char *)"800");
  libvar_sv_waterfriction = LibVar("sv_waterfriction", (char *)"1");
  libvar_sv_watergravity = LibVar("sv_watergravity", (char *)"400");
  libvar_sv_maxvelocity = LibVar("sv_maxvelocity", (char *)"300");
  libvar_sv_maxwalkvelocity = LibVar("sv_maxwalkvelocity", (char *)"300");
  libvar_sv_maxcrouchvelocity = LibVar("sv_maxcrouchvelocity", (char *)"100");
  libvar_sv_maxswimvelocity = LibVar("sv_maxswimvelocity", (char *)"150");
  libvar_sv_maxaccelerate = LibVar("sv_maxacceleration", (char *)"2200");
  libvar_sv_airaccelerate = LibVar("sv_airaccelerate", (char *)"0");
  libvar_sv_step = LibVar("sv_step", (char *)"18");
  libvar_sv_maxbarrier = LibVar("sv_maxbarrier", (char *)"50");
  libvar_sv_maxsteepness = LibVar("sv_maxsteepness", (char *)"0.7");
  libvar_sv_jumpvel = LibVar("sv_jumpvel", (char *)"224");
  libvar_sv_maxwaterjump = LibVar("sv_maxwaterjump", (char *)"21");
  result = libvar_sv_maxwaterjump;
  return (intptr_t)result;
}

/* ---- SLOT 1 — BotSetupLibrary (0x10037BB0) ------------------------------ */
//----- (10037BB0) --------------------------------------------------------
int Export_BotSetupLibrary(void)
{
    if (botstate.setup) {
        botimport.Print(3, "bot library already setup\n");
        return 2;
    }

    Log_Open("botlib.log");
    botimport.Print(1, "------- BotLib Initialization -------\n");
    botimport.Print(1, "BotLib v0.96\n");

    Swap_Init();
    botstate.setup = 1;

    botstate.num_clients  = (int)LibVarValue("maxclients",  "4");
    botstate.num_entities = (int)LibVarValue("maxentities", "1024");

    BotSetupMoveAI();

    errno = sub_1000EDC0(botstate.num_entities, botstate.num_clients);
    if (errno) return errno;

    errno = BotSetupLibrary();
    if (errno) return errno;

    EA_Setup();

    botimport.Print(1, "-------------------------------------\n");
    return 0;
}

/* ---- SLOT 2 — BotShutdownLibrary (0x10037CF0) --------------------------- */
//----- (10037CF0) --------------------------------------------------------
int Export_BotShutdownLibrary(void)
{
    if (!botstate.setup) {
        botimport.Print(3, "library not setup\n");
        return 1;
    }

    BotShutdownLibrary();
    AAS_Shutdown();
    EA_Shutdown();
    Log_Close();
    DumpMemory();

    /* Three rep stos (20 / 10 / 20 dwords) over the contiguous interface
     * blocks, then the scalar botlibsetup = 0.  sizeof keeps the clears
     * 64-bit-correct; see botlib_state.h for the block layout. */
    memset(&botstate,    0, sizeof(botstate));     /* block 1 @0x10064020, 20 dwords */
    memset(&botimport,   0, sizeof(botimport));    /* block 2 @0x10063FE0, 10 dwords */
    memset(&bot_exports, 0, sizeof(bot_exports));  /* block 3 @0x10063F80, 20 dwords */
    botstate.setup = 0;

    return 0;
}

/* ---- SLOT 3 — BotLibraryInitialized (0x10037D80); tail-jmp AAS_Initialized */
//----- (10037D80) --------------------------------------------------------
int Export_BotLibraryInitialized(void)
{
    return AAS_Initialized();
}

/* ---- SLOT 4 — BotLibVarSet (0x10037DA0).  Returns 0 unconditionally; the
 * original has no null-check. --------------------------------------------- */
//----- (10037DA0) --------------------------------------------------------
int Export_BotLibVarSet(char *var_name, char *value)
{
    LibVarSet(var_name, value);
    return 0;
}

/* ---- SLOT 5 — BotDefine (0x10037DD0) ----------------------------------- */
//----- (10037DD0) --------------------------------------------------------
int Export_BotDefine(char *string)
{
    if (!PC_AddGlobalDefine(string))
        botimport.Print(3, "couldn't add define %s\n", string);
    return 0;
}

/* ---- SLOT 6 — BotLoadMap (0x10037E10).  A NULL mapname is the unload
 * path. ------------------------------------------------------------------- */
//----- (10037E10) --------------------------------------------------------
int Export_BotLoadMap(char *mapname, int modelindexes, char **modelindex,
                     int soundindexes, char **soundindex,
                     int imageindexes, char **imageindex)
{
    if (!BotLibSetup("BotLoadMap")) return 1;

    if (!mapname) {
        return BotLoadMap(0, modelindexes, modelindex,
                          soundindexes, soundindex,
                          imageindexes, imageindex);
    }

    botimport.Print(1, "------------ Map Loading ------------\n");

    errno = BotLoadMap(mapname, modelindexes, modelindex,
                       soundindexes, soundindex,
                       imageindexes, imageindex);
    if (errno) return errno;

    sub_10029C10();
    botimport.Print(1, "-------------------------------------\n");
    return 0;
}

/* ---- SLOT 7 — BotSetupClient (0x10037F00) ------------------------------ */
//----- (10037F00) --------------------------------------------------------
int Export_BotSetupClient(int client, void *settings)
{
    int r;
    if (!BotLibSetup("BotSetupClient")) return 0;
    if (!ValidClientNumber(client, "BotSetupClient")) return 0;
    sub_100085F0();
    r = BotSetupClient(client, settings);
    return r != 0;
}

/* ---- SLOT 8 — BotShutdownClient (0x10037F70) --------------------------- */
//----- (10037F70) --------------------------------------------------------
int Export_BotShutdownClient(int client)
{
    if (!BotLibSetup("BotShutdownClient")) return 1;
    if (!ValidClientNumber(client, "BotShutdownClient")) return 3;
    return BotShutdownClient(client);
}

/* ---- SLOT 9 — BotMoveClient (0x10037FE0) ------------------------------- */
//----- (10037FE0) --------------------------------------------------------
int Export_BotMoveClient(int oldclnum, int newclnum)
{
    if (!BotLibSetup("BotMoveClient")) return 1;
    if (!ValidClientNumber(oldclnum, "BotMoveClient, parm0")) return 3;
    if (!ValidClientNumber(newclnum, "BotMoveClient, parm1")) return 3;
    return BotMoveClient(oldclnum, newclnum);
}

/* ---- SLOT 10 — BotClientSettings (0x10038070); body at 0x10029920 ------ */
//----- (10038070) --------------------------------------------------------
int Export_BotClientSettings(int client, void *settings)
{
    if (!BotLibSetup("BotClientSettings")) return 1;
    if (!ValidClientNumber(client, "BotClientSettings")) return 3;
    return BotClientSettings(client, settings);
}

/* ---- SLOT 11 — BotSettings (0x100380E0); body at 0x100299D0 ------------ */
//----- (100380E0) --------------------------------------------------------
int Export_BotSettings(int client, void *settings)
{
    if (!BotLibSetup("BotSettings")) return 1;
    if (!ValidClientNumber(client, "BotSettings")) return 3;
    return BotSettings(client, settings);
}

/* Slots 12, 17 and 18 live in botlib.c (0x10038150 / 0x10038380 /
 * 0x100383F0). */

//----- (10038150) --------------------------------------------------------
int __cdecl Export_BotLibStartFrame(float time)
{
  if ( !BotLibSetup("BotStartFrame") )
    return 1;
  *(float *)&botstate.bottime = time;
  return AAS_StartFrame(time);
}

/* ---- SLOT 13 — BotUpdateClient (0x10038190) ---------------------------- */
//----- (10038190) --------------------------------------------------------
int Export_BotUpdateClient(int client, void *buc)
{
    if (!BotLibSetup("BotUpdateClient")) return 1;
    if (!ValidClientNumber(client, "BotUpdateClient")) return 3;
    return BotUpdateClient(client, buc);
}

/* ---- SLOT 14 — BotUpdateEntity (0x10038200) ---------------------------- */
//----- (10038200) --------------------------------------------------------
int Export_BotUpdateEntity(int ent, void *bue)
{
    if (!BotLibSetup("BotUpdateEntity")) return 1;
    if (!ValidEntityNumber(ent, "BotUpdateEntity")) return 4;
    return AAS_UpdateEntity(ent, (bot_updateentity_t *)bue);
}

/* ---- SLOT 15 — BotAddSound (0x10038270).  The context string really is
 * "BotUpdateSound".  The first six args reach sub_1001CE20 as raw 32-bit
 * values (float bits through int slots); only timeofs is a real float. ---- */
//----- (10038270) --------------------------------------------------------
int Export_BotAddSound(int *origin, int ent, int channel, int soundindex,
                       float volume, float attenuation, float timeofs)
{
    if (!BotLibSetup("BotUpdateSound")) return 1;
    if (!ValidEntityNumber(ent, "BotUpdateSound")) return 4;
    return sub_1001CE20((intptr_t)origin, ent, channel, soundindex,
                        *(int *)&volume,
                        *(int *)&attenuation,
                        timeofs);
}

/* ---- SLOT 16 — BotAddPointLight (0x100382F0); floats passed as bits ---- */
//----- (100382F0) --------------------------------------------------------
int Export_BotAddPointLight(int *origin, int ent, float radius,
                            float r, float g, float b,
                            float time, float decay)
{
    if (!BotLibSetup("BotAddPointLight")) return 1;
    if (!ValidEntityNumber(ent, "BotAddPointLight")) return 4;
    /* Straight pass-through: BotAddPointLight takes floats, so the arguments
     * are re-pushed as the dwords they already are.  The bit-punned form this
     * replaced only worked against a hand-written `extern` in the old
     * botlib_exports.c that declared the callee as taking ints; against the
     * real declaration it made MSVC convert each one back (fild/fstp), which
     * is +6 instructions the original does not have. */
    return BotAddPointLight((vec_t *)origin, ent, radius, r, g, b, time, decay);
}

//----- (10038380) --------------------------------------------------------
int Export_BotLibAI(int a1, float a2)
{
  if ( !BotLibSetup("BotAI") )
    return BLERR_LIBRARYNOTSETUP;
  if ( !ValidClientNumber(a1, "BotAI") )
    return BLERR_INVALIDCLIENTNUMBER;
  return Export_BotAIFrame(a1, a2);
}

//----- (100383F0) --------------------------------------------------------
int __cdecl Export_BotLibConsoleMessage(int client, int a2, char *message)
{
  if ( !BotLibSetup("BotConsoleMessage") )
    return BLERR_LIBRARYNOTSETUP;
  if ( !ValidClientNumber(client, "BotConsoleMessage") )
    return BLERR_INVALIDCLIENTNUMBER;
  return BotConsoleMessage(client, a2, message);
}

/* ---- SLOT 19 — Test (0x10038460); returns 0 ---------------------------- */
//----- (10038460) --------------------------------------------------------
int Export_Test(int parm0, char *parm1, float *parm2, float *parm3)
{
    (void)parm0; (void)parm1; (void)parm2; (void)parm3;
    return 0;
}

//----- (10038480) --------------------------------------------------------
/* The original ends with a plain `ret`, not `ret 4`, so this export is __cdecl
 * even though gladq2_src/bl_main.c declares the function pointer WINAPI.  Both
 * sides are individually faithful; the mismatch is authentic. */
bot_export_t *GetBotAPI(bot_import_t *import)
{
  /* One shot: botimport_block_t is exactly the 10 import callbacks, so this is
   * the original's `rep movs` of 10 dwords, and sizeof keeps it 64-bit-correct.
   * GetBotAPI makes no other call — the SEH crash-handler install belongs in
   * botlib_debug.c's DllMain, where the original had it. */
  memcpy(&botimport, import, sizeof(botimport));

  bot_exports.BotVersion           = Export_BotVersion;
  bot_exports.BotSetupLibrary      = Export_BotSetupLibrary;
  bot_exports.BotShutdownLibrary   = Export_BotShutdownLibrary;
  bot_exports.BotLibraryInitialized = Export_BotLibraryInitialized;
  bot_exports.BotLibVarSet         = Export_BotLibVarSet;
  bot_exports.BotDefine            = Export_BotDefine;
  bot_exports.BotLoadMap           = (void *)Export_BotLoadMap;
  bot_exports.BotSetupClient       = (void *)Export_BotSetupClient;
  bot_exports.BotShutdownClient    = Export_BotShutdownClient;
  bot_exports.BotMoveClient        = Export_BotMoveClient;
  bot_exports.BotClientSettings    = (void *)Export_BotClientSettings;
  bot_exports.BotSettings          = (void *)Export_BotSettings;
  bot_exports.BotStartFrame        = Export_BotLibStartFrame;
  bot_exports.BotUpdateClient      = (void *)Export_BotUpdateClient;
  bot_exports.BotUpdateEntity      = (void *)Export_BotUpdateEntity;
  bot_exports.BotAddSound          = (void *)Export_BotAddSound;
  bot_exports.BotAddPointLight     = (void *)Export_BotAddPointLight;
  bot_exports.BotAI                = Export_BotLibAI;
  bot_exports.BotConsoleMessage    = Export_BotLibConsoleMessage;
  bot_exports.Test                 = (void *)Export_Test;
  return &bot_exports;
}


/* `memory`/`totalmemorysize`/`numblocks` and the ten memory-tracker functions
 * live in their own TU: botlib/l_memory.c (l_memory.obj, DLL
 * 0x10038F10..0x100391FF -- see .claude/memory/tu_partition.md). */
