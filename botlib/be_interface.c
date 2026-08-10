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

#include "be_interface.h"
#include "be_aas_main.h"
#include "be_ai2_main.h"
#include "l_crc.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"

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
//----- (10038150) --------------------------------------------------------
int __cdecl Export_BotLibStartFrame(float time)
{
  if ( !BotLibSetup("BotStartFrame") )
    return 1;
  *(float *)&botstate.bottime = time;
  return AAS_StartFrame(time);
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
