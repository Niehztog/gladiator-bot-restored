/*
 * be_ai2_main.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10028E80..0x10029DA0.
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
#include "be_ai2_main.h"
#include "be_aas_bspq2.h"
#include "be_aas_main.h"
#include "be_ai2_dmq2.h"
#include "be_ai_char.h"
#include "be_ai_chat.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"
#include "be_ai_weap.h"
#include "be_ea.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_memory.h"
#include "l_utils.h"

int numbots; /* active-bot count, ++/-- in BotSetupClient/BotShutdownClient, returned by
                NumBots(); name recovered from the tourney-2.5 Linux gladi386.so .dynsym */
bsp_entity_t *entities; // BSP entity list head (parsed by AAS_ParseBSPEntities)

#if BOTLIB_NEED_SIDEBAND
bot_character_t **botcharacters;
#else
#endif

#if BOTLIB_NEED_SIDEBAND
void **botgoalstate_p0;  /* weightconfig_t* */
void **botgoalstate_p1;  /* iteminfo weight table */
#else
#endif

#if BOTLIB_NEED_SIDEBAND
bot_weaponstate_t **botweaponstates;
#else
#endif

#if BOTLIB_NEED_SIDEBAND
void **botchatdumps;
#else
#endif

#if BOTLIB_NEED_SIDEBAND
chatmsg_links_t *botchatmsglinks;
#else
#endif

#if BOTLIB_NEED_SIDEBAND
ai_node_fn_t *botainodes;
#else
#endif

#if BOTLIB_NEED_SIDEBAND
bot_waypoint_t **botcheckpoints;
bot_waypoint_t **botpatrolpoints;
bot_waypoint_t **botcurpatrolpoint;
#else
#endif

int gametype; // weak
bot_state_t *botstates; // base array of maxclients bot states
float regularupdate_time; // weak

// gladiator.dll: 10028EA0..10028F01
// gladi386.so:   0003744C..000374A9
int __cdecl ClientFromName(const char *name)
{
  int v1;

  for ( v1 = 0; v1 < botlibglobals.num_clients; ++v1 )
  {
    if ( !strcmp(name, clientsettings[v1].netname) )
      return v1;
  }
  return 0;
}

// gladiator.dll: 10028F30..10028F66
// gladi386.so:   000374AC..000374FC
char *__cdecl ClientName(int client)
{
  if ( client < 0 || client >= botlibglobals.num_clients )
  {
    botimport.Print(PRT_WARNING, "ClientName: client %d out of range\n", client);
    return "";
  }
  return clientsettings[client].netname;
}

// gladiator.dll: 10028F80..10028FB8
// gladi386.so:   000374FC..0003754F
char *__cdecl ClientSkin(int client)
{
  if ( client < 0 || client >= botlibglobals.num_clients )
  {
    botimport.Print(PRT_WARNING, "ClientSkin: client %d out of range\n", client);
    return "";
  }
  return clientsettings[client].skin;
}

// gladiator.dll: 10028FD0..10028FD6
// gladi386.so:   00037550..00037567
int NumBots()
{
  return numbots;
}

// gladiator.dll: 10028FF0..1002902F
// gladi386.so:   00037568..000375FB
float __cdecl AngleDifference(float ang1, float ang2)
{
  float diff;

  diff = ang1 - ang2;
  if ( ang1 > ang2 )
  {
    if ( diff > 180.0 )
      diff -= 360.0;
  }
  else if ( diff < -180.0 )
  {
    diff += 360.0;
  }
  return diff;
}

// gladiator.dll: 10029040..10029109
// gladi386.so:   000375FC..00037714
float BotChangeViewAngle(float angle, float ideal_angle, float speed)
{
  float move;

  angle = anglemod(angle);
  ideal_angle = anglemod(ideal_angle);
  if ( angle == ideal_angle )
    return angle;
  move = ideal_angle - angle;
  if ( ideal_angle > angle )
  {
    if ( move > 180.0 )
      move = move - 360.0;
  }
  else
  {
    if ( move < -180.0 )
      move = move + 360.0;
  }
  if ( move > 0.0f )
  {
    if ( move > speed )
      move = speed;
  }
  else
  {
    if ( move < -speed )
      move = -speed;
  }
  return anglemod(angle + move);
}

// gladiator.dll: 10029150..10029289
// gladi386.so:   00037714..000379F3
/* All nine callers push (bs, thinktime), but the body reads bs->thinktime straight from
 * the struct instead of the parameter — probably declared for API symmetry with Q3.
 * `thinktime` is deliberately unused. */
int __cdecl BotChangeViewAngles(bot_state_t *bs, float thinktime)
{
  float v2; // st7 (was double)
  float *v3; // esi
  int v4; // ebx
  float v5; // st7 (was double)
  float v6; // st6 (was double)
  float v7; // st6 (was double)
  float v9; // [esp+10h] [ebp-4h]
  float v10; // [esp+18h] [ebp+4h] — reused in place for v10*thinktime

  (void)thinktime;
  v2 = bs->ideal_viewangles[0];
  if ( v2 > 180.0f )
  {
    v2 = bs->ideal_viewangles[0] - 360.0f;
    bs->ideal_viewangles[0] = v2;
  }
  if ( bs->enemy )
  {
    /* Both Characteristic_BFloat results come back on the FPU stack: #9 into v10, and #10
     * stays in ST(0) until the post-merge `v2 * bs->thinktime` consumes it — recaptured
     * into v2 here, which the else-branch overwrites with 150.0f. */
    v10 = Characteristic_BFloat(BotCharacter(bs), 9, 0.1f, 1800.0f);
    v2  = Characteristic_BFloat(BotCharacter(bs), 10, 0.1f, 1800.0f);
  }
  else
  {
    v10 = 100.0f;
    v2 = 150.0f;
  }
  v3 = bs->viewanglespeed;  /* One walk over three parallel vec3s: *v3 is viewanglespeed[i],
                              * v3-6 is viewangles[i] and v3-3 ideal_viewangles[i], all
                              * advanced in lockstep. */
  v4 = 2;
  v10 = v10 * bs->thinktime;
  v9 = v2 * bs->thinktime;
  do
  {
    /* AngleDifference's FPU return feeds straight into the int conversion and
     * abs: v5 = abs((int)AngleDifference(viewangles[i], ideal_viewangles[i])). */
    v5 = (float)abs((int)AngleDifference(*(v3 - 6), *(v3 - 3)));
    if ( v5 > *v3 )
    {
      v6 = v10 + *v3;
      *v3 = v6;
      if ( v6 > v5 )
        *v3 = v5;
    }
    else if ( v5 < *v3 )
    {
      v7 = *v3 - v9;
      *v3 = v7;
      if ( v7 < v5 )
        *v3 = v5;
    }
    v5 = BotChangeViewAngle(*(v3 - 6), *(v3 - 3), *v3);
    *(v3 - 6) = v5;
    ++v3;
    --v4;
  }
  while ( v4 );
  EA_View(bs->client, bs->viewangles);
}

// gladiator.dll: 100292E0..10029309
// gladi386.so:   000379F4..00037A34
void sub_100292E0()
{
  if ( regularupdate_time < AAS_Time() )
  {
    BotUpdateEntityItems();
    regularupdate_time = AAS_Time() + 1.0f;
  }
}

// gladiator.dll: 10029320..10029375
// gladi386.so:   00037A34..00037AD6
int Export_BotAIFrame(int a1, float a2)
{
  bot_state_t *bs;

  if ( !AAS_Initialized() )
    return BLERR_NOERROR;

  bs = &botstates[a1];
  if ( !bs->inuse )
  {
    botimport.Print(PRT_FATAL, "client %d hasn't been setup\n", a1);
    return BLERR_AICLIENTNOTSETUP;
  }
  BotDeathmatchAI(bs, a2);
  sub_100292E0();
  return BLERR_NOERROR;
}

// gladiator.dll: 100293A0..1002944E
// gladi386.so:   00037AD8..00037BA5
// Per-bot memory-usage dumper: reports the byte size of every allocation the bot owns
// (character / item weights / item index / weapon weights / weapon index / chat file),
// then calls PrintUsedMemorySize for the totals.
// DEAD in Gladiator — /INCREMENTAL; debug-only telemetry.
// Field offsets, raw: the bot_state_t labels in this region (chatstate /
// weaponweights) do not match these handle slots one-to-one:
//   +0x688  = character handle
//   +0xbc0  = item weights handle (== goalstate.itemweightconfig; read via BotGoalP0
//             below, since on 64-bit the real pointer lives in the sideband array)
//   +0xbc4  = item index handle (== goalstate.itemweightindex; BotGoalP1)
//   +0x1044 = chat file handle
//   +0x1050 = weapon weights handle
//   +0x1054 = weapon index handle
void sub_100293A0(bot_state_t *bs)
{
  botimport.Print(PRT_MESSAGE, "%6d bytes character\n",
           MemoryByteSize(BotCharacter(bs)));
  botimport.Print(PRT_MESSAGE, "%6d bytes item weights\n",
           MemoryByteSize(BotGoalP0(bs)));
  botimport.Print(PRT_MESSAGE, "%6d bytes item index\n",
           MemoryByteSize(BotGoalP1(bs)));
  botimport.Print(PRT_MESSAGE, "%6d bytes weapon weights\n",
           MemoryByteSize(BotWS(bs)->weightconfig));
  botimport.Print(PRT_MESSAGE, "%6d bytes weapon index\n",
           MemoryByteSize(BotWS(bs)->itemweights));
  botimport.Print(PRT_MESSAGE, "%6d bytes chat file\n",
           MemoryByteSize(BotChatDumpSlot(&bs->chatstate)));
  PrintUsedMemorySize();
}

// gladiator.dll: 10029480..10029614
// gladi386.so:   00037BA8..00037DD1
int __cdecl BotSetupClient(int a1, char *Source)
{
  bot_state_t *bs;
  bot_character_t *char_handle;
  char *weights_handle;
  char *chat_path;
  char *chat_arg;
  _DWORD *chat_state_ptr;
  char gender;

  bs = &botstates[a1];
  if ( bs->inuse )
  {
    botimport.Print(PRT_FATAL, "client %d already setup\n", a1);
    return 0;
  }
  char_handle = BotLoadCharacter(Source, Source + 144);
  BotCharacter(bs) = char_handle;
  if ( !char_handle )
  {
    botimport.Print(PRT_FATAL, "couldn't load bot character %s from %s\n", Source + 144, Source);
    return 0;
  }
  memcpy(bs->settings, Source, 0x1B0u);
  weights_handle = Characteristic_String(BotCharacter(bs), 28);
  if ( BotLoadItemWeights(&bs->goalstate, weights_handle) )
    return 0;
  weights_handle = Characteristic_String(BotCharacter(bs), 5);
#if BOTLIB_NEED_SIDEBAND
  /* On 32-bit BotWS(bs) already aliases &bs->weaponweights[0], so no separate
   * allocation happens — exactly as in the original. */
  if ( !BotWS(bs) )
    BotWS(bs) = (bot_weaponstate_t *)GetClearedMemory(sizeof(bot_weaponstate_t));
#endif
  if ( BotLoadWeaponWeights(BotWS(bs), weights_handle) )
  {
    BotFreeItemWeights(&bs->goalstate);
    return 0;
  }
  chat_path = Characteristic_String(BotCharacter(bs), 12);
  chat_arg = Characteristic_String(BotCharacter(bs), 13);
  chat_state_ptr = (_DWORD *)&bs->chatstate;
  if ( BotLoadChatFile(&bs->chatstate, chat_path, chat_arg) )
  {
    BotFreeItemWeights(&bs->goalstate);
    BotFreeWeaponWeights(BotWS(bs));
    return 0;
  }
  gender = *(_BYTE *)Characteristic_String(BotCharacter(bs), 3);
  if ( gender == 102 || gender == 70 )
    *chat_state_ptr = 1;
  else if ( gender == 109 || gender == 77 )
    *chat_state_ptr = 2;
  else
    *chat_state_ptr = 0;
  bs->inuse = 1;
  bs->client = a1;
  bs->entitynum = a1 + 1;
  bs->inuse_marker = 1;
  bs->setup_time = AAS_Time();
  ++numbots;
  return 1;
}

// gladiator.dll: 10029690..10029767
// gladi386.so:   00037DD4..00037ECA
int __cdecl BotShutdownClient(int a1)
{
  bot_state_t *bs;

  bs = &botstates[a1];
  if ( !bs->inuse )
  {
    botimport.Print(PRT_ERROR, "client %d already shutdown\n", a1);
    return BLERR_AICLIENTALREADYSHUTDOWN;
  }
  if ( BotChat_ExitGame(bs) )
    BotEnterChat(&bs->chatstate, bs->client, 0);
  BotFreeChatState(&bs->chatstate);
  BotFreeWeaponWeights(BotWS(bs));
#if BOTLIB_NEED_SIDEBAND
  /* 64-bit only: on 32-bit BotWS(bs) is inline and the memset below clears it. */
  if ( BotWS(bs) ) { FreeMemory(BotWS(bs)); BotWS(bs) = 0; }
#endif
  BotFreeItemWeights(&bs->goalstate);
  sub_1002A590(bs->character);
  BotFreeWaypoints(BotCheckpoints(bs));
  BotCheckpoints(bs) = 0;
  BotFreeWaypoints(BotPatrolpoints(bs));
  BotPatrolpoints(bs) = 0;
  memset(bs, 0, 0x11D0u);
  bs->inuse = 0;
  --numbots;
  return BLERR_NOERROR;
}

// gladiator.dll: 100297B0..10029842
// gladi386.so:   00037ECC..00037F9D
int __cdecl BotMoveClient(int a1, int a2)
{
  if ( !botstates[a1].inuse )
  {
    botimport.Print(PRT_FATAL, "tried to move inactive bot client\n");
    return BLERR_AIMOVEINACTIVECLIENT;
  }
  if ( botstates[a2].inuse )
  {
    botimport.Print(PRT_FATAL, "tried to move client to active client\n");
    return BLERR_AIMOVETOACTIVECLIENT;
  }
  memcpy(&botstates[a2], &botstates[a1], sizeof(bot_state_t));
  memset(&botstates[a1], 0, sizeof(bot_state_t));
  botstates[a1].inuse = 0;
  return BLERR_NOERROR;
}

// gladiator.dll: 10029880..100298F1
// gladi386.so:   00037FA0..0003807E
int __cdecl BotUpdateClient(int a1, const void *a2)
{
  bot_state_t *v2; // eax
  int v5; // edi

  v2 = &botstates[a1];
  if ( !v2->inuse )
  {
    botimport.Print(PRT_FATAL, "tried to updated inactive bot client\n");
    return BLERR_AIUPDATEINACTIVECLIENT;
  }
  memcpy(&v2->snapshot, a2, sizeof(v2->snapshot));
  /* The original computes viewangles[i] and delta_angles[i] via a shared scaled-index
   * register per iteration (`fld [ebp+esi*4+0x1080]` / `fld [ebp+esi*4+0x30]`, esi =
   * 0,1,2), not a raw incrementing pointer. */
  for ( v5 = 0; v5 < 3; v5++ )
    v2->viewangles[v5] = anglemod(v2->viewangles[v5] + v2->snapshot.delta_angles[v5]);
  return BLERR_NOERROR;
}

// gladiator.dll: 10029920..10029943
// gladi386.so:   00038080..000380B5
int __cdecl BotClientSettings(int a1, const void *a2)
{
  memcpy(&clientsettings[a1], a2, sizeof(bot_clientsettings_t));
  return 0;
}

// gladiator.dll: 10029960..100299B0
// gladi386.so:   000380B8..00038126
int __cdecl BotConsoleMessage(int a1, int a2, char *Source)
{
  bot_state_t *v3; // eax

  v3 = &botstates[a1];
  if ( !v3->inuse )
  {
    botimport.Print(PRT_ERROR, "recieved console message for inactive bot client\n");
    return BLERR_AICMFORINACTIVECLIENT;
  }
  BotQueueConsoleMessage(&v3->chatstate, a2, Source);
  return BLERR_NOERROR;
}

// gladiator.dll: 100299D0..10029A1D
// gladi386.so:   00038128..00038190
int __cdecl BotSettings(int a1, const void *a2)
{
  bot_state_t *v2; // eax

  v2 = &botstates[a1];
  if ( !v2->inuse )
  {
    botimport.Print(PRT_FATAL, "tried to update settings of inactive client\n");
    return BLERR_SETTINGSINACTIVECLIENT;
  }
  memcpy(v2->settings, a2, sizeof(v2->settings));
  return BLERR_NOERROR;
}

// gladiator.dll: 10029A40..10029BA2
// gladi386.so:   00038190..000383DD
int __cdecl BotResetState(bot_state_t *bs)
{
  int inuse;
  int client;
  int entitynum;
  int character;
  char movestate[128];
  int goalstate[243];
  int weaponstate[7];
  bot_chatstate_t chatstate;
  char settings[432];

  memcpy(settings, bs->settings, sizeof(settings));
  inuse = bs->inuse;
  client = bs->client;
  entitynum = bs->entitynum;
  character = bs->character;
  memcpy(movestate, &bs->ms, sizeof(movestate));
  memcpy(goalstate, &bs->goalstate, sizeof(goalstate));
  memcpy(weaponstate, bs->weaponweights, sizeof(weaponstate));
  memcpy(&chatstate, &bs->chatstate, sizeof(chatstate));
  BotFreeWaypoints(BotCheckpoints(bs));
  BotFreeWaypoints(BotPatrolpoints(bs));
#if BOTLIB_NEED_SIDEBAND
  /* 64-bit only: the memset below clears the inline slots, but the real heads
   * live outside bs and would stay dangling.  Same shape as BotWS above. */
  BotCheckpoints(bs) = 0;
  BotPatrolpoints(bs) = 0;
#endif
  memset(bs, 0, sizeof(*bs));
  memcpy(&bs->ms, movestate, sizeof(movestate));
  memcpy(&bs->goalstate, goalstate, sizeof(goalstate));
  memcpy(bs->weaponweights, weaponstate, sizeof(weaponstate));
  memcpy(&bs->chatstate, &chatstate, sizeof(chatstate));
  memcpy(bs->settings, settings, sizeof(settings));
  bs->inuse = inuse;
  bs->client = client;
  bs->entitynum = entitynum;
  bs->character = character;
  BotResetMoveState((int *)&bs->ms);
  BotResetGoalState(&bs->goalstate);
  BotResetWeaponState(BotWS(bs));
  BotResetAvoidGoals(&bs->goalstate);
  BotResetAvoidReach((int *)&bs->ms);
}

// gladiator.dll: 10029C10..10029C6B
// gladi386.so:   000383E0..00038459
int sub_10029C10()
{
  int i;

  for ( i = 0; i < botlibglobals.num_clients; i++ )
    BotResetState(&botstates[i]);
  BotInitLevelItems();
  if ( entities )
    AAS_FreeBSPEntities(entities);
  entities = AAS_ParseBSPEntities();
  BotSetupDeathmatchAI();
  return 0;
}

// gladiator.dll: 10029C90..10029D59
// gladi386.so:   0003845C..00038523
/* Windows genuinely writes each sub-init's return code through the CRT global `errno`
 * (three separate `_errno()` calls).  The Linux .so has none of that traffic — just a
 * plain register test after each call — so the Linux original used a local variable
 * here instead.  A genuine two-week source-drift split, not a compiler tie: a
 * shared-body local achieves a full ELF byte match but regresses the PE from 63 insns
 * to 37. */
int BotSetupLibrary()
{
#ifdef _WIN32
  srand(time(0));
  errno = BotSetupWeaponAI();
  if ( errno )
    return errno;
  errno = BotSetupGoalAI();
  if ( errno )
    return errno;
  errno = BotSetupChatAI();
  if ( errno )
    return errno;
#else
  int result;

  srand(time(0));
  result = BotSetupWeaponAI();
  if ( result )
    return result;
  result = BotSetupGoalAI();
  if ( result )
    return result;
  result = BotSetupChatAI();
  if ( result )
    return result;
#endif
  botstates = (bot_state_t *)GetClearedMemory(sizeof(bot_state_t) * botlibglobals.num_clients);
#if BOTLIB_NEED_SIDEBAND
  botcharacters = (bot_character_t **)GetClearedMemory(sizeof(bot_character_t *) * botlibglobals.num_clients);
  botgoalstate_p0 = (void **)GetClearedMemory(sizeof(void *) * botlibglobals.num_clients);
  botgoalstate_p1 = (void **)GetClearedMemory(sizeof(void *) * botlibglobals.num_clients);
  botweaponstates = (bot_weaponstate_t **)GetClearedMemory(sizeof(void *) * botlibglobals.num_clients);
  botchatdumps = (void **)GetClearedMemory(sizeof(void *) * botlibglobals.num_clients);
  botchatmsglinks = (chatmsg_links_t *)GetClearedMemory(sizeof(chatmsg_links_t) * botlibglobals.num_clients);
  botainodes = (ai_node_fn_t *)GetClearedMemory(sizeof(ai_node_fn_t) * botlibglobals.num_clients);
  botcheckpoints    = (bot_waypoint_t **)GetClearedMemory(sizeof(bot_waypoint_t *) * botlibglobals.num_clients);
  botpatrolpoints   = (bot_waypoint_t **)GetClearedMemory(sizeof(bot_waypoint_t *) * botlibglobals.num_clients);
  botcurpatrolpoint = (bot_waypoint_t **)GetClearedMemory(sizeof(bot_waypoint_t *) * botlibglobals.num_clients);
#endif
  clientsettings = GetClearedMemory(sizeof(bot_clientsettings_t) * botlibglobals.num_clients);
  gametype = (int)LibVarValue("gametype", (char *)"0");
  return BLERR_NOERROR;
}

// gladiator.dll: 10029DA0..10029DED
// gladi386.so:   00038524..00038589
void BotShutdownLibrary(void)
{
  BotShutdownDeathmatchAI();
  BotShutdownChatAI();
  BotShutdownGoalAI();
  BotShutdownWeaponAI();
  if ( clientsettings )
    FreeMemory(clientsettings);
  clientsettings = 0;
  if ( botstates )
    FreeMemory(botstates);
  botstates = 0;
#if BOTLIB_NEED_SIDEBAND
  if ( botcharacters )
    FreeMemory(botcharacters);
  botcharacters = 0;
  if ( botgoalstate_p0 )
    FreeMemory(botgoalstate_p0);
  botgoalstate_p0 = 0;
  if ( botgoalstate_p1 )
    FreeMemory(botgoalstate_p1);
  botgoalstate_p1 = 0;
  if ( botweaponstates )
    FreeMemory(botweaponstates);
  botweaponstates = 0;
  if ( botchatdumps )
    FreeMemory(botchatdumps);
  botchatdumps = 0;
  if ( botchatmsglinks )
    FreeMemory(botchatmsglinks);
  botchatmsglinks = 0;
  if ( botainodes )
    FreeMemory(botainodes);
  botainodes = 0;
  if ( botcheckpoints )
    FreeMemory(botcheckpoints);
  botcheckpoints = 0;
  if ( botpatrolpoints )
    FreeMemory(botpatrolpoints);
  botpatrolpoints = 0;
  if ( botcurpatrolpoint )
    FreeMemory(botcurpatrolpoint);
  botcurpatrolpoint = 0;
#endif
}
