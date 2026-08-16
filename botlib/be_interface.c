/*
 * be_interface.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x100376B0..0x10038480.
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

/* filecrcs (name from gladi386.so's .dynsym, where it is a 736-byte global whose
 * contents are byte-identical to this table) — 91 entries of { uint16 crc16, uint16
 * pad, uint32 flags } plus a NULL terminator.  This is the config-file CRC whitelist
 * of the integrity subsystem that also owns CRC_Block / sub_10037820 and the "You are
 * not allowed to modify the bot characters" message; the flags column
 * (1/3/4/6/8/0x20/0x40) is an unidentified file category.  sub_100377E0 scans it up
 * to &unk_1005E958, so that symbol MUST stay immediately after this array. */
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

/* dword_10063F2C — head of the filename-sorted scriptcrc_t list (an int in the 32-bit
 * original; must be a real pointer here).  Referenced only from this TU, which is
 * also where the .so puts its neighbours filecrcs and dumpcrcs. */
struct scriptcrc_s *dword_10063F2C; // weak

/* The bot_export_t wrappers below (0x100379E0..0x10038460) all fall inside this TU's
 * range, and Q3's be_interface.c holds the same wrappers next to GetBotLibAPI. */

// gladiator.dll: 100376B0..10037791
// gladi386.so:   00048980..00048A76
/* Register or look up a (filename, CRC) pair in the sorted dword_10063F2C list: on a
 * name hit return the strcmpi result of that record, else allocate a scriptcrc_t,
 * store hash+name and insert it alphabetically. */
void __cdecl sub_100376B0(char *String1, unsigned __int16 a2)
{
  scriptcrc_t *v2; // esi
  scriptcrc_t *v5;
  scriptcrc_t *v6; // edi (prev)
  scriptcrc_t *v4; // ebx (new record)
  int result;

  /* Get-or-insert: walk the sorted list, break on a name match, then let ONE shared
   * `if (v2) return;` distinguish match from exhaustion.  An inline early-return drops
   * that shared re-check. */
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

// gladiator.dll: 100377E0..1003780C
// gladi386.so:   00048A78..00048B74
int __cdecl sub_100377E0(char *String1, unsigned __int16 a2)
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

// gladiator.dll: 10037820..1003783E
// gladi386.so:   00048B74..00048BA7
/* CRC-hash the buffer and register the (name, crc) pair — byte-identical to
 * sub_10037850 below; /INCREMENTAL kept two compiled copies of the same source
 * function. */
int __cdecl sub_10037820(char *name, const unsigned char *buf, int len)
{
  unsigned short crc; // ax

  crc = CRC_Block(buf, len);
  return sub_100377E0(name, crc);
}

// gladiator.dll: 10037850..1003786E
// gladi386.so:   00048BA8..00048BDB
int __cdecl sub_10037850(char *String1, const unsigned char *a2, int a3)
{
  unsigned short v3; // ax

  v3 = CRC_Block(a2, a3);
  return sub_100377E0(String1, v3);
}

// gladiator.dll: 10037880..100378AE
// gladi386.so:   00048BDC..00048C1C
// Walk the dword_10063F2C scriptcrc list and Log_Write each entry as a C array
// initializer line:  \t{0x%04X, 1}, //name
// Format string at 0x1005E9EC, next-pointer at scriptcrc_t offset +0x94.  A dev tool
// for dumping the known-CRC table into a config; dead in the shipped DLL.
void __cdecl sub_10037880(void)
{
  scriptcrc_t *p;

  for ( p = dword_10063F2C; p; p = p->next )
    Log_Write("\t{0x%04X, 1}, //%s", (unsigned int)(unsigned __int16)p->hash, p->name);
}

// gladiator.dll: 100378C0..100378E5
// gladi386.so:   00048C1C..00048C38
int Sys_MilliSeconds()
{
  return clock() * 1000 / CLOCKS_PER_SEC;
}

// gladiator.dll: 10037900..10037932
// gladi386.so:   00048C38..00048C90
/* Q3's form, and the two are NOT interchangeable: the guarded-Print shape puts the
 * Print in the fall-through and the `return 1` epilogue at the cold end, which is what
 * gladi386.so has.  The inverted `if (ok) return 1;` spelling swaps the two and, once
 * inlined, folds the 0/1 the seven Export_* callers test. */
qboolean __cdecl ValidClientNumber(int num, const char *str)
{
  if ( num < 0 || num > botstate.num_clients )
  {
    botimport.Print(PRT_ERROR, "%s: invalid client number %d, [0, %d]\n", str, num, botstate.num_clients);
    return 0;
  }
  return 1;
}

// gladiator.dll: 10037950..10037982
// gladi386.so:   00048C90..00048CE8
qboolean __cdecl ValidEntityNumber(int num, const char *str)
{
  if ( num < 0 || num > botstate.num_entities )
  {
    botimport.Print(PRT_ERROR, "%s: invalid entity number %d, [0, %d]\n", str, num, botstate.num_entities);
    return 0;
  }
  return 1;
}

// gladiator.dll: 100379A0..100379C7
// gladi386.so:   00048CE8..00048D27
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
// gladiator.dll: 100379E0..100379E6
// gladi386.so:   00048D28..00048D3D
char *Export_BotVersion(void)
{
    return "BotLib v0.96";
}

// gladiator.dll: 10037A00..10037B47
// gladi386.so:   00048D40..00048F33
void BotSetupMoveAI()
{
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
}

/* ---- SLOT 1 — BotSetupLibrary (0x10037BB0) ------------------------------ */
// gladiator.dll: 10037BB0..10037CA6
// gladi386.so:   00048F34..00049066
int Export_BotSetupLibrary(void)
{
#ifndef _WIN32
    int result;
#endif

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

#ifdef _WIN32
    errno = sub_1000EDC0(botstate.num_entities, botstate.num_clients);
    if (errno) return errno;

    errno = BotSetupLibrary();
    if (errno) return errno;
#else
    result = sub_1000EDC0(botstate.num_entities, botstate.num_clients);
    if (result) return result;

    result = BotSetupLibrary();
    if (result) return result;
#endif

    EA_Setup();

    botimport.Print(1, "-------------------------------------\n");
    return 0;
}

/* ---- SLOT 2 — BotShutdownLibrary (0x10037CF0) --------------------------- */
// gladiator.dll: 10037CF0..10037D56
// gladi386.so:   00049068..000490FC
int Export_BotShutdownLibrary(void)
{
    if (!botstate.setup) {
        botimport.Print(3, "bot library already shutdown\n");
        return 1;
    }

    BotShutdownLibrary();
    AAS_Shutdown();
    EA_Shutdown();
    Log_Shutdown();
    DumpMemory();

    /* Three rep stos (20 / 10 / 20 dwords) over the contiguous interface blocks, then
     * the scalar botlibsetup = 0.  sizeof keeps the clears 64-bit-correct; see
     * botlib_state.h for the block layout. */
    memset(&botstate,    0, sizeof(botstate));     /* block 1 @0x10064020, 20 dwords */
    memset(&botimport,   0, sizeof(botimport));    /* block 2 @0x10063FE0, 10 dwords */
    memset(&bot_exports, 0, sizeof(bot_exports));  /* block 3 @0x10063F80, 20 dwords */
    botstate.setup = 0;

    return 0;
}

/* ---- SLOT 3 — BotLibraryInitialized (0x10037D80); tail-jmp AAS_Initialized */
// gladiator.dll: 10037D80..10037D85
// gladi386.so:   000490FC..00049110
int Export_BotLibraryInitialized(void)
{
    return AAS_Initialized();
}

/* ---- SLOT 4 — BotLibVarSet (0x10037DA0).  Returns 0 unconditionally; the
 * original has no null-check. --------------------------------------------- */
// gladiator.dll: 10037DA0..10037DB5
// gladi386.so:   00049110..00049133
int Export_BotLibVarSet(char *var_name, char *value)
{
    LibVarSet(var_name, value);
    return 0;
}

/* ---- SLOT 5 — BotDefine (0x10037DD0) ----------------------------------- */
// gladiator.dll: 10037DD0..10037DF7
// gladi386.so:   00049134..00049170
int Export_BotDefine(char *string)
{
    if (!PC_AddGlobalDefine(string))
        botimport.Print(3, "couldn't add define %s\n", string);
    return 0;
}

/* ---- SLOT 6 — BotLoadMap (0x10037E10).  A NULL mapname is the unload
 * path. ------------------------------------------------------------------- */
// gladiator.dll: 10037E10..10037EC3
// gladi386.so:   00049170..00049255
int Export_BotLoadMap(char *mapname, int modelindexes, char **modelindex,
                     int soundindexes, char **soundindex,
                     int imageindexes, char **imageindex)
{
#ifndef _WIN32
    int result;
#endif

    if (!BotLibSetup("BotLoadMap")) return 1;

    if (!mapname) {
        return BotLoadMap(0, modelindexes, modelindex,
                          soundindexes, soundindex,
                          imageindexes, imageindex);
    }

    botimport.Print(1, "------------ Map Loading ------------\n");

#ifdef _WIN32
    errno = BotLoadMap(mapname, modelindexes, modelindex,
                       soundindexes, soundindex,
                       imageindexes, imageindex);
    if (errno) return errno;
#else
    result = BotLoadMap(mapname, modelindexes, modelindex,
                       soundindexes, soundindex,
                       imageindexes, imageindex);
    if (result) return result;
#endif

    sub_10029C10();
    botimport.Print(1, "-------------------------------------\n");
    return 0;
}

/* ---- SLOT 7 — BotSetupClient (0x10037F00) ------------------------------ */
// gladiator.dll: 10037F00..10037F47
// gladi386.so:   00049258..0004930D
int Export_BotSetupClient(int client, void *settings)
{
    int r;
    if (!BotLibSetup("BotSetupClient")) return 0;
    if (!ValidClientNumber(client, "BotSetupClient")) return 0;
    sub_100085F0();
    r = BotSetupClient(client, settings);
    if (!r) goto fail;
    return 1;
fail:
    return 0;
}

/* ---- SLOT 8 — BotShutdownClient (0x10037F70) --------------------------- */
// gladiator.dll: 10037F70..10037FB1
// gladi386.so:   00049310..000493C0
int Export_BotShutdownClient(int client)
{
    if (!BotLibSetup("BotShutdownClient")) return 1;
    if (!ValidClientNumber(client, "BotShutdownClient")) return 3;
    return BotShutdownClient(client);
}

/* ---- SLOT 9 — BotMoveClient (0x10037FE0) ------------------------------- */
// gladiator.dll: 10037FE0..10038044
// gladi386.so:   000493C0..000494C5
int Export_BotMoveClient(int oldclnum, int newclnum)
{
    if (!BotLibSetup("BotMoveClient")) return 1;
    if (!ValidClientNumber(oldclnum, "BotMoveClient, parm0")) return 3;
    if (!ValidClientNumber(newclnum, "BotMoveClient, parm1")) return 3;
    return BotMoveClient(oldclnum, newclnum);
}

/* ---- SLOT 10 — BotClientSettings (0x10038070); body at 0x10029920 ------ */
// gladiator.dll: 10038070..100380B6
// gladi386.so:   000494C8..0004957D
int Export_BotClientSettings(int client, void *settings)
{
    if (!BotLibSetup("BotClientSettings")) return 1;
    if (!ValidClientNumber(client, "BotClientSettings")) return 3;
    return BotClientSettings(client, settings);
}

/* ---- SLOT 11 — BotSettings (0x100380E0); body at 0x100299D0 ------------ */
// gladiator.dll: 100380E0..10038126
// gladi386.so:   00049580..00049635
int Export_BotSettings(int client, void *settings)
{
    if (!BotLibSetup("BotSettings")) return 1;
    if (!ValidClientNumber(client, "BotSettings")) return 3;
    return BotSettings(client, settings);
}

/* Slots 12, 17 and 18 live in botlib.c (0x10038150 / 0x10038380 /
 * 0x100383F0). */

// gladiator.dll: 10038150..1003817C
// gladi386.so:   00049638..0004969F
int __cdecl Export_BotLibStartFrame(float time)
{
  if ( !BotLibSetup("BotStartFrame") )
    return 1;
  *(float *)&botstate.bottime = time;
  return AAS_StartFrame(time);
}

/* ---- SLOT 13 — BotUpdateClient (0x10038190) ---------------------------- */
// gladiator.dll: 10038190..100381D6
// gladi386.so:   000496A0..00049755
int Export_BotUpdateClient(int client, void *buc)
{
    if (!BotLibSetup("BotUpdateClient")) return 1;
    if (!ValidClientNumber(client, "BotUpdateClient")) return 3;
    return BotUpdateClient(client, buc);
}

/* ---- SLOT 14 — BotUpdateEntity (0x10038200) ---------------------------- */
// gladiator.dll: 10038200..10038246
// gladi386.so:   00049758..0004980D
int Export_BotUpdateEntity(int ent, void *bue)
{
    if (!BotLibSetup("BotUpdateEntity")) return 1;
    if (!ValidEntityNumber(ent, "BotUpdateEntity")) return 4;
    return AAS_UpdateEntity(ent, (bot_updateentity_t *)bue);
}

/* ---- SLOT 15 — BotAddSound (0x10038270).  The context string really is
 * "BotUpdateSound".  volume/attenuation/timeofs all reach sub_1001CE20 as plain
 * floats, matching game/botlib.h's export signature verbatim. ---- */
// gladiator.dll: 10038270..100382CF
// gladi386.so:   00049810..000498F5
int Export_BotAddSound(int *origin, int ent, int channel, int soundindex,
                       float volume, float attenuation, float timeofs)
{
    if (!BotLibSetup("BotUpdateSound")) return 1;
    if (!ValidEntityNumber(ent, "BotUpdateSound")) return 4;
    return sub_1001CE20((intptr_t)origin, ent, channel, soundindex,
                        volume,
                        attenuation,
                        timeofs);
}

/* ---- SLOT 16 — BotAddPointLight (0x100382F0); floats passed as bits ---- */
// gladiator.dll: 100382F0..10038354
// gladi386.so:   000498F8..000499CD
int Export_BotAddPointLight(int *origin, int ent, float radius,
                            float r, float g, float b,
                            float time, float decay)
{
    if (!BotLibSetup("BotAddPointLight")) return 1;
    if (!ValidEntityNumber(ent, "BotAddPointLight")) return 4;
    /* Straight pass-through: BotAddPointLight takes floats, so the arguments are
     * re-pushed as the dwords they already are.  A bit-punned form only works against
     * an `extern` declaring the callee as taking ints; against the real declaration it
     * makes MSVC convert each one back (fild/fstp), +6 instructions. */
    return BotAddPointLight((vec_t *)origin, ent, radius, r, g, b, time, decay);
}

// gladiator.dll: 10038380..100383C6
// gladi386.so:   000499D0..00049A84
int Export_BotLibAI(int a1, float a2)
{
  if ( !BotLibSetup("BotAI") )
    return BLERR_LIBRARYNOTSETUP;
  if ( !ValidClientNumber(a1, "BotAI") )
    return BLERR_INVALIDCLIENTNUMBER;
  return Export_BotAIFrame(a1, a2);
}

// gladiator.dll: 100383F0..1003843B
// gladi386.so:   00049A84..00049B46
int __cdecl Export_BotLibConsoleMessage(int client, int a2, char *message)
{
  if ( !BotLibSetup("BotConsoleMessage") )
    return BLERR_LIBRARYNOTSETUP;
  if ( !ValidClientNumber(client, "BotConsoleMessage") )
    return BLERR_INVALIDCLIENTNUMBER;
  return BotConsoleMessage(client, a2, message);
}

/* ---- SLOT 19 — Test (0x10038460); returns 0 ---------------------------- */
// gladiator.dll: 10038460..10038463
// gladi386.so:   00049B48..00049B4B
int Export_Test(int parm0, char *parm1, float *parm2, float *parm3)
{
    (void)parm0; (void)parm1; (void)parm2; (void)parm3;
    return 0;
}

// gladiator.dll: 10038480..10038562
// gladi386.so:   00049B4C..00049CA2
/* The original ends with a plain `ret`, not `ret 4`, so this export is __cdecl even
 * though gladq2_src/bl_main.c declares the function pointer WINAPI.  Both sides are
 * individually faithful; the mismatch is authentic. */
bot_export_t *GetBotAPI(bot_import_t *import)
{
  /* One shot: botimport_block_t is exactly the 10 import callbacks, so this is the
   * original's `rep movs` of 10 dwords, and sizeof keeps it 64-bit-correct.  GetBotAPI
   * makes no other call — the SEH crash-handler install belongs in botlib_debug.c's
   * DllMain, where the original had it. */
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


/* `memory`/`totalmemorysize`/`numblocks` and the ten memory-tracker functions live in
 * their own TU: botlib/l_memory.c (DLL 0x10038F10..0x100391FF). */
