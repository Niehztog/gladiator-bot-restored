/*
 * be_ea.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10037090..0x10037690; the boundary evidence -- per-object .text and
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
#include "be_ea.h"
#include "be_interface.h"
#include "l_memory.h"

ea_state_t *ea_controls; /* per-client EA state array, sized 36 * maxclients */

//----- (10037090) --------------------------------------------------------
void __cdecl EA_Say(int client, char *str)
{
  botimport.BotClientCommand(client, "say", str, (char *)NULL);
}
//----- (100370C0) --------------------------------------------------------
void __cdecl EA_SayTeam(int client, char *str)
{
  botimport.BotClientCommand(client, "say_team", str, (char *)NULL);
}
//----- (100370F0) --------------------------------------------------------
void __cdecl EA_UseItem(int client, char *it)
{
  botimport.BotClientCommand(client, "use", it, (char *)NULL);
}
//----- (10037120) --------------------------------------------------------
void __cdecl EA_DropItem(int client, char *it)
{
  botimport.BotClientCommand(client, "drop", it, (char *)NULL);
}
//----- (10037150) --------------------------------------------------------
// Pushes 0, item, "invuse", client
// then tail-calls bi_BotClientCommand.  Matches the sibling family
// EA_Say/EA_SayTeam/EA_UseItem/EA_DropItem exactly.
void __cdecl EA_UseInv(int client, char *inv)
{
  botimport.BotClientCommand(client, "invuse", inv, (char *)NULL);
}
//----- (10037180) --------------------------------------------------------
// Mirror of EA_UseInv with the
// "invdrop" command string (0x1005E63C).
void __cdecl EA_DropInv(int client, char *inv)
{
  botimport.BotClientCommand(client, "invdrop", inv, (char *)NULL);
}
//----- (100371B0) --------------------------------------------------------
void __cdecl sub_100371B0(int client, int sequence)
{
  char Buffer[128]; // [esp+0h] [ebp-80h] BYREF

  sprintf(Buffer, "%d", sequence);
  botimport.BotClientCommand(client, "wave", Buffer, (char *)NULL);
}
//----- (10037200) --------------------------------------------------------
int __cdecl EA_Command(int client, char *command, ...)
{
  va_list ap;
  char *args[10];
  int   n;

  args[0] = command;
  n = 1;
  va_start(ap, command);
  while ( n < 10 )
  {
    args[n] = va_arg(ap, char *);
    if ( !args[n] )
      break;
    ++n;
  }
  va_end(ap);
  if ( n >= 10 )
    botimport.Print(PRT_ERROR, "EA_Command: too many arguments");
  return botimport.BotClientCommand(client, args[0], args[1], args[2], args[3], args[4],
                              args[5], args[6], args[7], args[8], args[9], 0);
}
// 10037233: conditional instruction was optimized away because esi.4<A
//----- (100372C0) --------------------------------------------------------
/* EA_Attack — set ACTION_ATTACK so the engine fires the bot's weapon next
 * frame.  As Q3's be_ea.c::EA_Attack. */
void __cdecl EA_Attack(int client)
{
  ea_state_t *ea = &ea_controls[client];
  ea->flags |= ACTION_ATTACK;
}
//----- (100372F0) --------------------------------------------------------
/* EA_Use — set ACTION_USE.  DEAD in Gladiator: the mod issues USE through
 * bi_BotClientCommand instead of the EA layer. */
void __cdecl EA_Use(int client)
{
  ea_state_t *ea = &ea_controls[client];
  ea->flags |= ACTION_USE;
}
//----- (10037320) --------------------------------------------------------
/* EA_Respawn — set ACTION_RESPAWN. */
void __cdecl EA_Respawn(int client)
{
  ea_state_t *ea = &ea_controls[client];
  ea->flags |= ACTION_RESPAWN;
}
//----- (10037350) --------------------------------------------------------
/* EA_Jump — set ACTION_JUMP only while the EA_JUMPEDLASTFRAME latch is clear;
 * if it is set, clear ACTION_JUMP instead so the engine sees a key release. */
char __cdecl EA_Jump(int client)
{
  ea_state_t *ea = &ea_controls[client];
  if ( ea->flags & EA_JUMPEDLASTFRAME )
    ea->flags &= ~ACTION_JUMP;
  else
    ea->flags |= ACTION_JUMP;
  return (char)ea->flags;
}
//----- (10037390) --------------------------------------------------------
/* EA_DelayedJump — set ACTION_DELAYEDJUMP (0x200), jump-latch gated like
 * EA_Jump.  The `BYTE1(v) |= 2u` form is how the original writes bit 9. */
int __cdecl EA_DelayedJump(int client)
{
  ea_state_t *ea = &ea_controls[client];
  int v = ea->flags;
  if ( v & EA_JUMPEDLASTFRAME )
    v &= ~ACTION_DELAYEDJUMP;
  else
    v |= ACTION_DELAYEDJUMP;
  ea->flags = v;
  return v;
}
//----- (100373D0) --------------------------------------------------------
void __cdecl EA_Crouch(int client)
{
  ea_state_t *ea = &ea_controls[client];
  ea->flags |= ACTION_CROUCH;
}
//----- (10037400) --------------------------------------------------------
/* EA_MoveUp — set ACTION_MOVEUP, which aliases ACTION_JUMP's bit (0x008).
 * Unlike EA_Jump this is unconditional, with no EA_JUMPEDLASTFRAME gate, which
 * is what BotTravel_WaterJump needs to jump out of water regardless of the
 * jump cooldown. */
void __cdecl EA_MoveUp(int client)
{
  ea_state_t *ea = &ea_controls[client];
  ea->flags |= ACTION_MOVEUP;
}
//----- (10037430) --------------------------------------------------------
/* EA_MoveDown — set ACTION_MOVEDOWN, which aliases ACTION_CROUCH's bit (0x10),
 * so this is bit-for-bit EA_Crouch.  DEAD in Gladiator, which always calls
 * EA_Crouch instead. */
void __cdecl EA_MoveDown(int client)
{
  ea_state_t *ea = &ea_controls[client];
  ea->flags |= ACTION_MOVEDOWN;
}
//----- (10037460) --------------------------------------------------------
/* EA_MoveForward — set ACTION_MOVEFORWARD. */
void __cdecl EA_MoveForward(int client)
{
  ea_state_t *ea = &ea_controls[client];
  ea->flags |= ACTION_MOVEFORWARD;
}
//----- (10037490) --------------------------------------------------------
/* EA_MoveBack — set ACTION_MOVEBACK (0x40).  DEAD in Gladiator. */
void __cdecl EA_MoveBack(int client)
{
  ea_state_t *ea = &ea_controls[client];
  ea->flags |= ACTION_MOVEBACK;
}
//----- (100374C0) --------------------------------------------------------
/* EA_MoveLeft — set ACTION_MOVELEFT (0x80).  DEAD in Gladiator. */
void __cdecl EA_MoveLeft(int client)
{
  ea_state_t *ea = &ea_controls[client];
  ea->flags |= ACTION_MOVELEFT;
}
//----- (100374F0) --------------------------------------------------------
/* EA_MoveRight — set ACTION_MOVERIGHT (0x100).  DEAD in Gladiator. */
void __cdecl EA_MoveRight(int client)
{
  ea_state_t *ea = &ea_controls[client];
  ea->flags |= ACTION_MOVERIGHT;
}
//----- (10037520) --------------------------------------------------------
void __cdecl EA_Move(int client, vec3_t dir, float speed)
{
  ea_state_t *ea = &ea_controls[client];
  VectorCopy(dir, ea->dir);
  if ( speed > 565.0f )
  {
    ea->speed = 565.0f;
    return;
  }
  if ( speed < -565.0f )
  {
    ea->speed = -565.0f;
    return;
  }
  ea->speed = speed;
}
//----- (100375A0) --------------------------------------------------------
void __cdecl EA_View(int client, vec3_t viewangles)
{
  ea_state_t *ea = &ea_controls[client];
  VectorCopy(viewangles, ea->angles);
}
//----- (100375E0) --------------------------------------------------------
/* EA_EndRegular — flush this frame's accumulated EA state to the engine via the
 * BotInput callback, then clear the EA fields for the next frame.  If the jump
 * bit was set, latch EA_JUMPEDLASTFRAME so the next EA_Jump emits a key
 * release — the same press/release pattern Q3 uses.
 *
 * The second parameter is `float thinktime`, NOT an int: ea_state_t matches the
 * engine's bot_input_t only with thinktime@0, and on aarch64 an int declaration
 * silently truncates the float to 0, giving ucmd.msec = 0 and a bot frozen at
 * spawn. */
void __cdecl EA_EndRegular(int client, float thinktime)
{
  ea_state_t *ea = &ea_controls[client];
  qboolean jumped_this_frame;
  ea->flags &= ~EA_JUMPEDLASTFRAME;  /* original: `and cl, 0x7F` — clear bit 7 in low byte, preserve upper bits */
  ea->thinktime = thinktime;
  botimport.BotInput(client, ea);
  /* Clear the non-flag fields FIRST, so MSVC6 materialises the shared zero
   * before the `ea->flags & ACTION_JUMP` mask: the mask then sets the ZF the
   * trailing `if` consumes, and these stores preserve it.  Masking first forces
   * a flag-safe `mov eax,0` plus a second zero for the return. */
  ea->thinktime = 0.0f;
  ea->dir[0] = ea->dir[1] = ea->dir[2] = 0.0f;
  ea->speed    = 0.0f;
  jumped_this_frame = ea->flags & ACTION_JUMP;
  ea->flags    = 0;
  if ( jumped_this_frame )
    ea->flags = EA_JUMPEDLASTFRAME;
}
//----- (10037660) --------------------------------------------------------
int EA_Setup()
{
  ea_controls = (ea_state_t *)GetClearedMemory(sizeof(ea_state_t) * botstate.num_clients);
  return (int)ea_controls;
}
//----- (10037690) --------------------------------------------------------
void EA_Shutdown()
{
  FreeMemory(ea_controls);
}
