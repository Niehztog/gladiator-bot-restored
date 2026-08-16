/*
 * be_ea.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10037090..0x10037690.
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
#include "be_ea.h"
#include "be_interface.h"
#include "l_memory.h"

ea_state_t *botinputs; /* per-client EA state array, sized 36 * maxclients */

// gladiator.dll: 10037090..100370AB
// gladi386.so:   0004840C..0004843C
void __cdecl EA_Say(int client, char *str)
{
  botimport.BotClientCommand(client, "say", str, (char *)NULL);
}
// gladiator.dll: 100370C0..100370DB
// gladi386.so:   0004843C..0004846C
void __cdecl EA_SayTeam(int client, char *str)
{
  botimport.BotClientCommand(client, "say_team", str, (char *)NULL);
}
// gladiator.dll: 100370F0..1003710B
// gladi386.so:   0004846C..0004849C
void __cdecl EA_UseItem(int client, char *it)
{
  botimport.BotClientCommand(client, "use", it, (char *)NULL);
}
// gladiator.dll: 10037120..1003713B
// gladi386.so:   0004849C..000484CC
void __cdecl EA_DropItem(int client, char *it)
{
  botimport.BotClientCommand(client, "drop", it, (char *)NULL);
}
// gladiator.dll: 10037150..1003716B
// gladi386.so:   000484CC..000484FC
// Pushes 0, item, "invuse", client then tail-calls bi_BotClientCommand.  Matches the
// sibling family EA_Say/EA_SayTeam/EA_UseItem/EA_DropItem exactly.
void __cdecl EA_UseInv(int client, char *inv)
{
  botimport.BotClientCommand(client, "invuse", inv, (char *)NULL);
}
// gladiator.dll: 10037180..1003719B
// gladi386.so:   000484FC..0004852C
// Mirror of EA_UseInv with the
// "invdrop" command string (0x1005E63C).
void __cdecl EA_DropInv(int client, char *inv)
{
  botimport.BotClientCommand(client, "invdrop", inv, (char *)NULL);
}
// gladiator.dll: 100371B0..100371EB
// gladi386.so:   0004852C..00048584
void __cdecl sub_100371B0(int client, int sequence)
{
  char Buffer[128]; // [esp+0h] [ebp-80h] BYREF

  sprintf(Buffer, "%d", sequence);
  botimport.BotClientCommand(client, "wave", Buffer, (char *)NULL);
}
// gladiator.dll: 10037200..1003728C
// gladi386.so:   00048584..00048625
int __cdecl EA_Command(int client, char *command, ...)
{
  va_list ap;
  char *args[10];
  int   n;

  args[0] = command;
  va_start(ap, command);
  n = 1;
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
// gladiator.dll: 100372C0..100372D5
// gladi386.so:   00048628..0004864B
/* EA_Attack — set ACTION_ATTACK so the engine fires the bot's weapon next
 * frame.  As Q3's be_ea.c::EA_Attack. */
void __cdecl EA_Attack(int client)
{
  ea_state_t *ea = &botinputs[client];
  ea->flags |= ACTION_ATTACK;
}
// gladiator.dll: 100372F0..10037305
// gladi386.so:   0004864C..0004866F
/* EA_Use — set ACTION_USE.  DEAD in Gladiator: the mod issues USE through
 * bi_BotClientCommand instead of the EA layer. */
void __cdecl EA_Use(int client)
{
  ea_state_t *ea = &botinputs[client];
  ea->flags |= ACTION_USE;
}
// gladiator.dll: 10037320..10037335
// gladi386.so:   00048670..00048693
/* EA_Respawn — set ACTION_RESPAWN. */
void __cdecl EA_Respawn(int client)
{
  ea_state_t *ea = &botinputs[client];
  ea->flags |= ACTION_RESPAWN;
}
// gladiator.dll: 10037350..10037373
// gladi386.so:   00048694..000486CB
/* EA_Jump — set ACTION_JUMP only while the EA_JUMPEDLASTFRAME latch is clear;
 * if it is set, clear ACTION_JUMP instead so the engine sees a key release. */
void __cdecl EA_Jump(int client)
{
  ea_state_t *ea = &botinputs[client];
  if ( ea->flags & EA_JUMPEDLASTFRAME )
    ea->flags &= ~ACTION_JUMP;
  else
    ea->flags |= ACTION_JUMP;
}
// gladiator.dll: 10037390..100373B5
// gladi386.so:   000486CC..00048704
/* EA_DelayedJump — set ACTION_DELAYEDJUMP (0x200), jump-latch gated like
 * EA_Jump.  The `BYTE1(v) |= 2u` form is how the original writes bit 9. */
void __cdecl EA_DelayedJump(int client)
{
  ea_state_t *ea = &botinputs[client];
  if ( ea->flags & EA_JUMPEDLASTFRAME )
    ea->flags &= ~ACTION_DELAYEDJUMP;
  else
    ea->flags |= ACTION_DELAYEDJUMP;
}
// gladiator.dll: 100373D0..100373E5
// gladi386.so:   00048704..00048727
void __cdecl EA_Crouch(int client)
{
  ea_state_t *ea = &botinputs[client];
  ea->flags |= ACTION_CROUCH;
}
// gladiator.dll: 10037400..10037415
// gladi386.so:   00048728..0004874B
/* Set ACTION_MOVEUP, which aliases ACTION_JUMP's bit (0x008).  Unlike EA_Jump this is
 * unconditional, with no EA_JUMPEDLASTFRAME gate, which is what BotTravel_WaterJump
 * needs to jump out of water regardless of the jump cooldown. */
void __cdecl EA_MoveUp(int client)
{
  ea_state_t *ea = &botinputs[client];
  ea->flags |= ACTION_MOVEUP;
}
// gladiator.dll: 10037430..10037445
// gladi386.so:   0004874C..0004876F
/* Set ACTION_MOVEDOWN, which aliases ACTION_CROUCH's bit (0x10), so this is
 * bit-for-bit EA_Crouch.  DEAD in Gladiator, which always calls EA_Crouch instead. */
void __cdecl EA_MoveDown(int client)
{
  ea_state_t *ea = &botinputs[client];
  ea->flags |= ACTION_MOVEDOWN;
}
// gladiator.dll: 10037460..10037475
// gladi386.so:   00048770..00048793
/* EA_MoveForward — set ACTION_MOVEFORWARD. */
void __cdecl EA_MoveForward(int client)
{
  ea_state_t *ea = &botinputs[client];
  ea->flags |= ACTION_MOVEFORWARD;
}
// gladiator.dll: 10037490..100374A5
// gladi386.so:   00048794..000487B7
/* EA_MoveBack — set ACTION_MOVEBACK (0x40).  DEAD in Gladiator. */
void __cdecl EA_MoveBack(int client)
{
  ea_state_t *ea = &botinputs[client];
  ea->flags |= ACTION_MOVEBACK;
}
// gladiator.dll: 100374C0..100374DA
// gladi386.so:   000487B8..000487DB
/* EA_MoveLeft — set ACTION_MOVELEFT (0x80).  DEAD in Gladiator. */
void __cdecl EA_MoveLeft(int client)
{
  ea_state_t *ea = &botinputs[client];
  ea->flags |= ACTION_MOVELEFT;
}
// gladiator.dll: 100374F0..1003750A
// gladi386.so:   000487DC..00048802
/* EA_MoveRight — set ACTION_MOVERIGHT (0x100).  DEAD in Gladiator. */
void __cdecl EA_MoveRight(int client)
{
  ea_state_t *ea = &botinputs[client];
  ea->flags |= ACTION_MOVERIGHT;
}
// gladiator.dll: 10037520..1003757F
// gladi386.so:   00048804..00048878
void __cdecl EA_Move(int client, vec3_t dir, float speed)
{
  ea_state_t *ea = &botinputs[client];
  VectorCopy(dir, ea->dir);
  if ( speed > 565.0f )
    speed = 565.0f;
  else if ( speed < -565.0f )
    speed = -565.0f;
  ea->speed = speed;
}
// gladiator.dll: 100375A0..100375C6
// gladi386.so:   00048878..000488AE
void __cdecl EA_View(int client, vec3_t viewangles)
{
  ea_state_t *ea = &botinputs[client];
  VectorCopy(viewangles, ea->angles);
}
// gladiator.dll: 100375E0..10037633
// gladi386.so:   000488B0..00048929
/* Flush this frame's accumulated EA state to the engine via the BotInput callback, then
 * clear the EA fields for the next frame.  If the jump bit was set, latch
 * EA_JUMPEDLASTFRAME so the next EA_Jump emits a key release — the same press/release
 * pattern Q3 uses.
 *
 * The second parameter is `float thinktime`, NOT an int: ea_state_t matches the engine's
 * bot_input_t only with thinktime@0, and on aarch64 an int declaration silently
 * truncates the float to 0, giving ucmd.msec = 0 and a bot frozen at spawn. */
void __cdecl EA_EndRegular(int client, float thinktime)
{
  ea_state_t *ea = &botinputs[client];
  qboolean jumped_this_frame;
  ea->flags &= ~EA_JUMPEDLASTFRAME;  /* original: `and cl, 0x7F` — clear bit 7 in low byte, preserve upper bits */
  ea->thinktime = thinktime;
  botimport.BotInput(client, ea);
  /* Clear the non-flag fields FIRST, so MSVC6 materialises the shared zero before the
   * `ea->flags & ACTION_JUMP` mask: the mask then sets the ZF the trailing `if`
   * consumes, and these stores preserve it.  Masking first forces a flag-safe
   * `mov eax,0` plus a second zero for the return. */
  ea->thinktime = 0.0f;
  ea->dir[0] = ea->dir[1] = ea->dir[2] = 0.0f;
  ea->speed    = 0.0f;
  jumped_this_frame = ea->flags & ACTION_JUMP;
  ea->flags    = 0;
  if ( jumped_this_frame )
    ea->flags = EA_JUMPEDLASTFRAME;
}
// gladiator.dll: 10037660..1003767A
// gladi386.so:   0004892C..0004895D
int EA_Setup()
{
  botinputs = (ea_state_t *)GetClearedMemory(sizeof(ea_state_t) * botlibglobals.num_clients);
  return (int)botinputs;
}
// gladiator.dll: 10037690..1003769D
// gladi386.so:   00048960..00048980
void EA_Shutdown()
{
  FreeMemory(botinputs);
}
