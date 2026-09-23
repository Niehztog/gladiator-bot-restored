/*
 * be_ai2_dmq2.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10020ED0..0x10028C30.
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
#include "be_ai2_dmq2.h"
#include "be_aas_bspq2.h"
#include "be_aas_debug.h"
#include "be_aas_entity.h"
#include "be_aas_light.h"
#include "be_aas_main.h"
#include "be_aas_reach.h"
#include "be_aas_sample.h"
#include "be_ai2_dmnet.h"
#include "be_ai2_main.h"
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

bot_clientsettings_t *clientsettings; /* per-client {netname[16], skin[128]} = 144 B */
libvar_t *ctf; /* libvar handle */

bot_goal_t ctf_flag2; /* 0x100643E0 blue flag goal (ai_dmq3.c; was unk_100643E0) */

bot_goal_t ctf_flag1;  /* 0x10064420 red flag goal  (ai_dmq3.c; was unk_10064420) */

libvar_t *usehook; /* libvar handle */

libvar_t *techs; /* libvar handle */

libvar_t *rocketjump; /* libvar handle */

/* G_SetMovedir's four direction constants, 12 bytes each, in the original .data
 * order — the same order and values as game/g_utils.c:342-345, from which
 * BotSetMovedir was copied.  Non-static in Mr. Elusive's sources, so all four
 * survive by name in gladi386.so's .dynsym, each a 12-byte OBJECT, confirming
 * float[3] rather than three separate scalars. */
float VEC_UP[3]       = { 0.0f, -1.0f,  0.0f };
float MOVEDIR_UP[3]   = { 0.0f,  0.0f,  1.0f };
float VEC_DOWN[3]     = { 0.0f, -2.0f,  0.0f };
float MOVEDIR_DOWN[3] = { 0.0f,  0.0f, -1.0f };

libvar_t *ch; /* libvar handle */
libvar_t *teamplay; /* libvar handle */
libvar_t *ra; /* libvar handle */
int mapchange; // weak
libvar_t *dmflags; /* libvar handle */
libvar_t *nochat; /* libvar handle */
libvar_t *fastchat; /* libvar handle */
libvar_t *assimilation; /* libvar handle */
int modelindex3_flag1; // weak
libvar_t *teamplay_shell; /* libvar handle */
int modelindex3_flag2; // weak
int modelindex_tech4; // weak
int modelindex_tech3; // weak
int modelindex_tech2; // weak
int modelindex_tech1; // weak

// gladiator.dll: 10020ED0..10020F9F
// gladi386.so:   0002BA08..0002BAD3
void __cdecl BotEntityInfo(bot_state_t *bs, _DWORD *info)
{

  unsigned int v3; // edx
  unsigned int v4; // edx
  unsigned int v5; // edx

  *info = (*(int *)&bs->snapshot.origin[0]);
  info[1] = (*(int *)&bs->snapshot.origin[1]);
  info[2] = (*(int *)&bs->snapshot.origin[2]);
  info[3] = (*(int *)&bs->snapshot.velocity[0]);
  info[4] = (*(int *)&bs->snapshot.velocity[1]);
  info[5] = (*(int *)&bs->snapshot.velocity[2]);
  info[6] = (*(int *)&bs->snapshot.viewoffset[0]);
  info[7] = (*(int *)&bs->snapshot.viewoffset[1]);
  info[8] = (*(int *)&bs->snapshot.viewoffset[2]);
  info[9] = bs->entitynum;
  info[10] = bs->client;
  info[11] = *(int *)&bs->thinktime;
  v3 = info[24] & 0xFFFFFFFD;
  info[24] = v3;
  if ( (bs->snapshot.pm_flags & 4) != 0 )
    info[24] = v3 | 2;
  v4 = info[24] & 0xFFFFFFDF;
  info[24] = v4;
  if ( (bs->snapshot.pm_flags & 0x20) != 0 && bs->snapshot.pm_time > 0 )
    info[24] = v4 | 0x20;
  v5 = info[24] & 0xFFFFFFEF;
  info[24] = v5;
  if ( (bs->snapshot.pm_flags & 8) != 0 && bs->snapshot.pm_time > 0 )
    info[24] = v5 | 0x10;
  if ( (bs->snapshot.pm_flags & 1) != 0 )
    info[12] = 4;
  else
    info[12] = 2;
  info[13] = *(int *)&bs->viewangles[0];
  info[14] = *(int *)&bs->viewangles[1];
  info[15] = *(int *)&bs->viewangles[2];
}

// gladiator.dll: 10020FE0..10021008
// gladi386.so:   0002BAD4..0002BB0A
char *__cdecl sub_10020FE0(bot_state_t *bs, bot_weaponstate_t *ws)
{

  char *result; // eax

  ws->client = bs->client;
  ws->inventory = bs->inventory;
  result = AAS_ModelFromIndex(bs->snapshot.gunindex);
  ws->modelname = result;
  return result;
}

// gladiator.dll: 10021020..1002120B
// gladi386.so:   0002BB0C..0002BD98
void __cdecl BotUpdateInventory(bot_state_t *bs)
{
  int *inventory;
  short *stats;
  char *name;

  inventory = bs->inventory;
  stats = bs->snapshot.stats;
  inventory[INVENTORY_HEALTH] = stats[1];
  if ( stats[9] )
  {
    name = AAS_ImageFromIndex(stats[9]);
    if ( !_strcmpi(name, "p_quad") )
      bs->quad_endtime = AAS_Time() + (float)stats[10];
    else if ( !_strcmpi(name, "p_invulnerability") )
      bs->invulnerability_endtime = AAS_Time() + (float)stats[10];
    else if ( !_strcmpi(name, "p_rebreather") )
      bs->rebreather_endtime = AAS_Time() + (float)stats[10];
    else if ( !_strcmpi(name, "p_envirosuit") )
      bs->enviro_endtime = AAS_Time() + (float)stats[10];
  }
  inventory[QUAD_SECONDS] = bs->quad_endtime - AAS_Time();
  if ( inventory[QUAD_SECONDS] <= 0 )
    inventory[QUAD_SECONDS] = 0;
  inventory[INVULNERABILITY_SECONDS] = bs->invulnerability_endtime - AAS_Time();
  if ( inventory[INVULNERABILITY_SECONDS] <= 0 )
    inventory[INVULNERABILITY_SECONDS] = 0;
  inventory[REBREATHER_SECONDS] = bs->rebreather_endtime - AAS_Time();
  if ( inventory[REBREATHER_SECONDS] <= 0 )
    inventory[REBREATHER_SECONDS] = 0;
  inventory[ENVIROSUIT_SECONDS] = bs->enviro_endtime - AAS_Time();
  if ( inventory[ENVIROSUIT_SECONDS] <= 0 )
    inventory[ENVIROSUIT_SECONDS] = 0;
  if ( stats[4] )
  {
    name = AAS_ImageFromIndex(stats[4]);
    if ( !_strcmpi(name, "i_powershield") )
      bs->powerscreen_seen_time = AAS_Time();
    if ( bs->powerscreen_seen_time > AAS_Time() - 0.9 )
    {
      inventory[POWER_SCREEN_CELLS] = inventory[20];  /* inventory[20] = power shield cells */
      inventory[POWER_SHIELD_CELLS] = inventory[20];
    }
    else
    {
      inventory[POWER_SCREEN_CELLS] = 0;
      inventory[POWER_SHIELD_CELLS] = 0;
    }
  }
}

// gladiator.dll: 10021290..10021427
// gladi386.so:   0002BD98..0002C01B
void __cdecl BotUpdateBattleInventory(bot_state_t *bs, int enemy)
{

  vec3_t dir; // [esp+8h] [ebp-88h] BYREF
  /* Properly-typed struct, not `float entinfo[31]` + a type-punned cast assignment:
   * writing AAS_EntityInfo()'s struct-by-value return through a cast to a
   * differently-declared object defeats gcc's return-slot forwarding, forcing a
   * hidden temp plus an extra 124-byte rep-movs copy. */
  aas_entityinfo_t entinfo; // [esp+14h] [ebp-7Ch] BYREF

  entinfo = AAS_EntityInfo(enemy);
  VectorSubtract(entinfo.origin, bs->origin, dir);
  bs->inventory[ENEMY_HEIGHT] = (int)dir[2];
  dir[2] = 0.0;
  bs->inventory[ENEMY_HORIZONTAL_DIST] = (int)VectorLength(dir);
  bs->inventory[ENEMY_WEAPON_BLASTER] = 0;
  bs->inventory[ENEMY_WEAPON_SHOTGUN] = 0;
  bs->inventory[ENEMY_WEAPON_SUPERSHOTGUN] = 0;
  bs->inventory[ENEMY_WEAPON_MACHINEGUN] = 0;
  bs->inventory[ENEMY_WEAPON_CHAINGUN] = 0;
  bs->inventory[ENEMY_WEAPON_GRENADES] = 0;
  bs->inventory[ENEMY_WEAPON_GRENADELAUNCHER] = 0;
  bs->inventory[ENEMY_WEAPON_ROCKETLAUNCHER] = 0;
  bs->inventory[ENEMY_WEAPON_HYPERBLASTER] = 0;
  bs->inventory[ENEMY_WEAPON_RAILGUN] = 0;
  bs->inventory[ENEMY_WEAPON_BFG] = 0;
  bs->inventory[ENEMY_WEAPON_PHALANX] = 0;
  /* Not `v2 = (...); ...; v3 = v2 - 1; switch(v3)`: the original computes the weapon
   * index in one shot right before the switch dispatch (movzx+lea immediately
   * preceding `cmp edx,0xb`), not interleaved with the zero-writes above. */
  switch ( ((entinfo.skinnum >> 8) & 0xFF) - 1 )
  {
    case 0:
      bs->inventory[ENEMY_WEAPON_BLASTER] = 1;
      break;
    case 1:
      bs->inventory[ENEMY_WEAPON_SHOTGUN] = 1;
      break;
    case 2:
      bs->inventory[ENEMY_WEAPON_SUPERSHOTGUN] = 1;
      break;
    case 3:
      bs->inventory[ENEMY_WEAPON_MACHINEGUN] = 1;
      break;
    case 4:
      bs->inventory[ENEMY_WEAPON_CHAINGUN] = 1;
      break;
    case 5:
      bs->inventory[ENEMY_WEAPON_GRENADES] = 1;
      break;
    case 6:
      bs->inventory[ENEMY_WEAPON_GRENADELAUNCHER] = 1;
      break;
    case 7:
      bs->inventory[ENEMY_WEAPON_ROCKETLAUNCHER] = 1;
      break;
    case 8:
      bs->inventory[ENEMY_WEAPON_HYPERBLASTER] = 1;
      break;
    case 9:
      bs->inventory[ENEMY_WEAPON_RAILGUN] = 1;
      break;
    case 10:
      bs->inventory[ENEMY_WEAPON_BFG] = 1;
      break;
    case 11:
      bs->inventory[ENEMY_WEAPON_PHALANX] = 1;
      break;
    default:
      break;
  }
  /* Not `v8 = entinfo.effects; v5 = BYTE1(entinfo.effects);`: the original re-reads
   * `entinfo.effects` from memory independently for each bit test, at whatever
   * sub-width fits that mask (BYTE at +2 for 0x10000, a WORD sign-test at +0 for
   * 0x8000, BYTE at +1 for 0x200) — three separate full-width source expressions. */
  if ( (entinfo.effects & 0x10000) != 0 )
    bs->inventory[ENEMY_INVULNERABILITY] = 1;
  else
    bs->inventory[ENEMY_INVULNERABILITY] = 0;
  if ( (entinfo.effects & 0x8000) != 0 )
    bs->inventory[ENEMY_QUAD] = 1;
  else
    bs->inventory[ENEMY_QUAD] = 0;
  /* void, not `int` returning 1: the disasm never sets eax=1 anywhere (the switch
   * dispatch is the last eax write and its leftover jump-table address falls through
   * to `ret` unused) and the sole caller discards the result.  The `int`/`return 1`
   * reading was MSVC6's unrelated eax=1/ecx=0 register-caching for the inventory
   * writes above, which happens to leave eax=1 at the tail.  Two physical
   * fall-to-own-epilogue tails below, not one shared one, matches the original's
   * separate copies for the POWERSCREEN true/false paths. */
  if ( (entinfo.effects & 0x200) != 0 )
  {
    bs->inventory[ENEMY_POWERSCREEN] = 1;
    return;
  }
  bs->inventory[ENEMY_POWERSCREEN] = 0;
}

// gladiator.dll: 100214E0..100214EC
// gladi386.so:   0002C01C..0002C028
/* Returns the signed 16-bit snapshot.stats[16].  DEAD in Gladiator, so no call
 * site confirms the parameter type; bot_state_t * matches both neighbours. */
int __cdecl sub_100214E0(bot_state_t *p)
{
  return p->snapshot.stats[16];
}

// gladiator.dll: 10021500..100215A4
// gladi386.so:   0002C028..0002C0DB
void __cdecl BotBattleUseItems(bot_state_t *bs)
{
  if ( bs->inventory[25] > 0 )                     /* +1828 silencer ammo */
    EA_UseItem(bs->client, "Silencer");
  if ( (AAS_PointContents(bs->eye) & 0x38) != 0
       && !bs->inventory[REBREATHER_SECONDS]
       && bs->inventory[26] > 0 )                  /* +1832 rebreather charges */
    EA_UseItem(bs->client, "Rebreather");
  if ( !bs->inventory[POWER_SHIELD_CELLS] && bs->inventory[6] > 0 )   /* +1752 */
    EA_UseItem(bs->client, "Power Shield");
  if ( !bs->inventory[POWER_SCREEN_CELLS] && bs->inventory[5] > 0 )   /* +1748 */
    EA_UseItem(bs->client, "Power Screen");
}

// gladiator.dll: 100215E0..10021630
// gladi386.so:   0002C0DC..0002C13B
void __cdecl sub_100215E0(bot_state_t *bs)
{
  if ( !bs->inventory[QUAD_SECONDS] && bs->inventory[23] > 0 )   /* +1820 quad ammo */
  {
    EA_UseItem(bs->client, "Quad Damage");
    return;
  }
  if ( !bs->inventory[INVULNERABILITY_SECONDS] && bs->inventory[24] > 0 ) /* +1824 invuln ammo */
    EA_UseItem(bs->client, "Invulnerability");
}

// gladiator.dll: 10021650..10021690
// gladi386.so:   0002C13C..0002C18C
int __cdecl BotCTFCarryingFlag(bot_state_t *bs)
{
  if ( ctf->value == 0.0f )
    return 0;
  /* inventory[43]=RED FLAG, inventory[44]=BLUE FLAG (Q2 CTF item indices).  Returns
   * 1 (red), 2 (blue), or 0.  Four separate `return` statements, not a ternary on the
   * last pair: gcc cross-jumps the trailing `return 0` back onto the first one, where
   * a ternary would pre-zero eax. */
  if ( bs->inventory[43] > 0 )
    return 1;
  if ( bs->inventory[44] > 0 )
    return 2;
  return 0;
}

// gladiator.dll: 100216A0..100216BA
// gladi386.so:   0002C18C..0002C1A2
BOOL __cdecl BotIsDead(bot_state_t *bs)
{
  int v1; // eax

  v1 = bs->snapshot.pm_type;
  return v1 == 2 || v1 == 3;
}

// gladiator.dll: 100216D0..100216DE
// gladi386.so:   0002C1A4..0002C1B5
BOOL __cdecl BotIsObserver(bot_state_t *bs)
{
  return bs->snapshot.pm_type == 1;
}

// gladiator.dll: 100216F0..100216FE
// gladi386.so:   0002C1B8..0002C1C9
BOOL __cdecl BotIntermission(bot_state_t *bs)
{
  return bs->snapshot.pm_type == 4;
}

// gladiator.dll: 10021710..10021752
// gladi386.so:   0002C1CC..0002C223
BOOL __cdecl sub_10021710(int *a1)
{
  if ( (a1[29] & 0x4002) != 0 )
    return 1;
  if ( a1[3] >= 1 && a1[3] <= botlibglobals.num_clients )
  {
    if ( a1[23] != 255 )
      return 1;
    if ( a1[27] >= 173 && a1[27] <= 197 )
      return 1;
    return 0;
  }
  return 1;
}

// gladiator.dll: 10021780..100217A3
// gladi386.so:   0002C224..0002C245
BOOL __cdecl EntityIsShooting(intptr_t a1)
{
  aas_entityinfo_t *ent = (aas_entityinfo_t *)a1;

  if ( ent->modelindex != 255 )
    return 0;
  if ( ent->frame >= 46 && ent->frame <= 53 )
    return 1;
  return 0;
}

// gladiator.dll: 100217C0..10021836
// gladi386.so:   0002C248..0002C2DB
char *__cdecl stristr(char *str, char *charset)
{
  int i;

  while ( *str )
  {
    for ( i = 0; charset[i] && str[i]; i++ )
    {
      if ( toupper(charset[i]) != toupper(str[i]) )
        break;
    }
    if ( !charset[i] )
      return str;
    str++;
  }
  return 0;
}

// gladiator.dll: 10021860..10021A12
// gladi386.so:   0002C2DC..0002C4AA
char *__cdecl EasyClientName(int client, char *buf)
{
  int ci; // eax (strength-reduced walk-pointer over Str)
  char *i; // edx
  char *str1; // esi
  char *str2; // eax
  char *ptr; // esi
  char c; // al
  /* One 128-byte stack buffer (the original's `sub esp,0x80`). */
  char Str[128]; // [esp+8h] [ebp-80h] BYREF

  strcpy(Str, (const char *)ClientName(client));
  for ( ci = 0; Str[ci]; ci++ )
    Str[ci] &= 0x7F;
  for ( i = strstr(Str, " "); i; i = strstr(Str, " ") )
    memmove(i, i + 1, strlen(i + 1) + 1);  /* memmove, not strcpy: the ranges overlap */
  str1 = strstr(Str, "[");
  str2 = strstr(Str, "]");
  if ( str1 && str2 )
  {
    /* overlapping shift-left: use memmove (was strcpy — UB on aarch64) */
    if ( str2 > str1 )
      memmove(str1, str2 + 1, strlen(str2 + 1) + 1);
    else
      memmove(str2, str1 + 1, strlen(str1 + 1) + 1);
  }
  if ( (Str[0] == 109 || Str[0] == 77) && (Str[1] == 114 || Str[1] == 82) )
  {
    memmove(Str, &Str[2], strlen(&Str[2]) + 1);  /* was strcpy — UB on aarch64 */
  }
  ptr = Str;
  if ( *ptr )
  {
    do
    {
      c = *ptr;
      if ( c >= 97 && c <= 122 || c >= 48 && c <= 57 || c == 95 )
      {
        ++ptr;
      }
      else if ( c >= 65 && c <= 90 )
      {
        *ptr++ = c + 32;
      }
      else
      {
        memmove(ptr, ptr + 1, strlen(ptr + 1) + 1);
      }
    }
    while ( *ptr );
  }
  strcpy(buf, Str);
  return buf;
}

// gladiator.dll: 10021A90..10021B1A
// gladi386.so:   0002C4AC..0002C57E
bot_waypoint_t *__cdecl BotCreateWayPoint(const char *name, vec3_t origin, int areanum)
{
  /* The original allocates strlen(name)+1+68 with a hand-laid header; a real struct
   * here, with the name buffer still inline right after the node so one FreeMemory
   * releases both. */
  bot_waypoint_t *wp;
  vec3_t mins = { -8.0f, -8.0f, -8.0f };
  vec3_t maxs = {  8.0f,  8.0f,  8.0f };
  size_t namelen = strlen(name);

  wp = (bot_waypoint_t *)GetMemory((unsigned)(sizeof(bot_waypoint_t) + namelen + 1));
  wp->name = (char *)(wp + 1);
  strcpy(wp->name, name);
  VectorCopy(origin, wp->goal.origin);
  VectorCopy(mins, wp->goal.mins);
  VectorCopy(maxs, wp->goal.maxs);
  wp->goal.areanum = areanum;
  wp->next = NULL;
  wp->prev = NULL;
  return wp;
}

// gladiator.dll: 10021B50..10021B7F
// gladi386.so:   0002C580..0002C5C1
bot_waypoint_t *__cdecl BotFindWayPoint(bot_waypoint_t *waypoints, char *name)
{
  bot_waypoint_t *wp;

  for ( wp = waypoints; wp; wp = wp->next )
  {
    if ( !_strcmpi(wp->name, name) )
      return wp;
  }
  return NULL;
}

// gladiator.dll: 10021B90..10021BAD
// gladi386.so:   0002C5C4..0002C5F1
void __cdecl BotFreeWaypoints(bot_waypoint_t *wp)
{
  bot_waypoint_t *next;

  for ( ; wp; wp = next )
  {
    next = wp->next;
    FreeMemory(wp);
  }
}

// gladiator.dll: 10021BC0..10021D13
// gladi386.so:   0002C5F4..0002C79E
BOOL __cdecl BotValidChatPosition(bot_state_t *bs)
{

  char v4; // al
  char v7; // al
  /* Real vec3_t, written with Q3's exact statements
   * (`VectorCopy(bs->origin, X); X[2] += K;`): MSVC6 forwards the [2] copy into the
   * arithmetic and interleaves the two surviving integer copies between the fld and
   * the fadd.  Copying start and end from the SAME source produces the original's
   * register reuse. */
  vec3_t point; // [esp+4h] [ebp-90h] BYREF
  vec3_t start; // [esp+10h] [ebp-84h] BYREF
  vec3_t end;   // [esp+1Ch] [ebp-78h] BYREF
  _DWORD mins[3]; // [esp+34h] [ebp-60h] BYREF
  _DWORD maxs[3]; // [esp+28h] [ebp-6Ch] BYREF
  bsp_trace_t trace; // [esp+40h] [ebp-54h] BYREF

  if ( BotIsDead(bs) )
    return 1;
  if ( (bs->snapshot.pm_flags & 4) == 0 )
    return 0;
  VectorCopy(bs->origin, point);
  point[2] = point[2] - 24.0f;
  v4 = (char)AAS_PointContents(point);
  if ( (v4 & 0x18) != 0 )           /* CONTENTS_LAVA(8) | CONTENTS_SLIME(16) */
    return 0;
  VectorCopy(bs->origin, point);
  point[2] = point[2] + 32.0f;
  v7 = (char)AAS_PointContents(point);
  if ( (v7 & 0x38) != 0 )           /* CONTENTS_LAVA(8) | SLIME(16) | WATER(32) */
    return 0;
  VectorCopy(bs->origin, start);
  VectorCopy(bs->origin, end);
  start[2] = start[2] + 1.0f;
  end[2] = end[2] - 100.0f;
  AAS_PresenceTypeBoundingBox(4, (float *)mins, (float *)maxs);
  trace = AAS_Trace(start, (float*)mins, (float*)maxs, end, 4, bs->client);
  if ( trace.ent != 0 )
    return 0;
  return 1;
}

// gladiator.dll: 10021D80..10021E44
// gladi386.so:   0002C7A0..0002C888
BOOL __cdecl BotChat_EnterGame(bot_state_t *bs)
{

  float v1; // st7
  BOOL result; // eax
  float rnd; // [esp+4h] [ebp-24h]
  char name[32]; // [esp+8h] [ebp-20h] BYREF

  v1 = nochat->value;
  if ( v1 != 0.0f )
    return 0;
  rnd = (float)Characteristic_BFloat(BotCharacter(bs), 18, 0.0, 1.0);
  if ( fastchat->value == 0.0f )
  {
    if ( random() > rnd )
      return 0;
  }
  result = BotValidChatPosition(bs);
  if ( !result )
    return result;
  BotInitialChat(&bs->chatstate, "enter_game", EasyClientName(bs->client, name),
                 (char *)0);
  return 1;
}

// gladiator.dll: 10021E90..10021F42
// gladi386.so:   0002C888..0002C958
int __cdecl BotChat_ExitGame(bot_state_t *bs)
{

  float rnd; // [esp+4h] [ebp-24h]
  char name[32]; // [esp+8h] [ebp-20h] BYREF

  if ( nochat->value != 0.0f )
    return 0;
  rnd = (float)Characteristic_BFloat(BotCharacter(bs), 18, 0.0, 1.0);
  if ( fastchat->value == 0.0f )
  {
    if ( random() > rnd )
      return 0;
  }
  BotInitialChat(&bs->chatstate, "exit_game", EasyClientName(bs->client, name),
                 (char *)0);
  return 1;
}

// gladiator.dll: 10021F80..10022032
// gladi386.so:   0002C958..0002CA28
int __cdecl BotChat_StartLevel(bot_state_t *bs)
{

  float rnd; // [esp+4h] [ebp-24h]
  char name[32]; // [esp+8h] [ebp-20h] BYREF

  if ( nochat->value != 0.0f )
    return 0;
  rnd = (float)Characteristic_BFloat(BotCharacter(bs), 17, 0.0, 1.0);
  if ( fastchat->value == 0.0f )
  {
    if ( random() > rnd )
      return 0;
  }
  BotInitialChat(&bs->chatstate, "start_level",
                 EasyClientName(bs->client, name), (char *)0);
  return 1;
}

// gladiator.dll: 10022070..10022122
// gladi386.so:   0002CA28..0002CAF8
int __cdecl BotChat_EndLevel(bot_state_t *bs)
{

  float rnd; // [esp+4h] [ebp-24h]
  char name[32]; // [esp+8h] [ebp-20h] BYREF

  if ( nochat->value != 0.0f )
    return 0;
  rnd = (float)Characteristic_BFloat(BotCharacter(bs), 17, 0.0, 1.0);
  if ( fastchat->value == 0.0f )
  {
    if ( random() > rnd )
      return 0;
  }
  BotInitialChat(&bs->chatstate, "end_level", EasyClientName(bs->client, name),
                 (char *)0);
  return 1;
}

// gladiator.dll: 10022160..1002228C
// gladi386.so:   0002CAF8..0002CC50
int __cdecl BotChat_Death(int *bs)
{
  float v1; // st7
  int v4; // eax
  float v6; // [esp+4h] [ebp-24h]
  float v5; // st7
  _BYTE v7[32]; // [esp+8h] [ebp-20h] BYREF
  float v8; // [esp+2Ch] [ebp+4h]

  v1 = nochat->value;
  if ( v1 != 0.0f )
    return 0;
  v6 = (float)Characteristic_BFloat(BotCharacter((bot_state_t *)bs), 20, 0.0, 1.0);
  if ( fastchat->value == 0.0f )
  {
    if ( random() > v6 )
      return 0;
  }
  v4 = bs[1049];
  if ( v4 )
    EasyClientName(v4 - 1, v7);
  else
    strcpy(v7, "");
  if ( bs[693] == 12 )
  {
    BotInitialChat(bs + 995, "death_bfg", v7, (char *)0);
  }
  else
  {
    v5 = random();
    /* Characteristic 15 is the praise-vs-insult probability. */
    v8 = (float)Characteristic_BFloat(BotCharacter((bot_state_t *)bs), 15, 0.0, 1.0);
    if ( v5 < v8 )
      BotInitialChat(bs + 995, "death_insult", v7, (char *)0);
    else
      BotInitialChat(bs + 995, "death_praise", v7, (char *)0);
  }
  return 1;
}

// gladiator.dll: 100222E0..1002241E
// gladi386.so:   0002CC50..0002CDB8
BOOL __cdecl BotChat_Kill(int *bs)
{
  float v1; // st7
  BOOL result; // eax
  int v4; // eax
  float rnd; // [esp+4h] [ebp-24h]
  float v5; // st7
  _BYTE name[32]; // [esp+8h] [ebp-20h] BYREF
  float v8; // [esp+2Ch] [ebp+4h]

  v1 = nochat->value;
  if ( v1 != 0.0f )
    return 0;
  rnd = (float)Characteristic_BFloat(BotCharacter((bot_state_t *)bs), 19, 0.0, 1.0);
  if ( fastchat->value == 0.0f )
  {
    if ( random() > rnd )
      return 0;
  }
  result = BotValidChatPosition((bot_state_t *)bs);
  if ( !result )
    return result;
  v4 = bs[1049];
  if ( v4 )
    EasyClientName(v4 - 1, name);
  else
    strcpy(name, "");
  if ( bs[692] == 13 )
  {
    BotInitialChat(bs + 995, "kill_telefrag", name, (char *)0);
  }
  else
  {
    v5 = random();
    /* Characteristic 15 is the praise-vs-insult probability. */
    v8 = (float)Characteristic_BFloat(BotCharacter((bot_state_t *)bs), 15, 0.0, 1.0);
    if ( v5 < v8 )
      BotInitialChat(bs + 995, "kill_insult", name, (char *)0);
    else
      BotInitialChat(bs + 995, "kill_praise", name, (char *)0);
  }
  return 1;
}

// gladiator.dll: 10022470..100225EE
// gladi386.so:   0002CDB8..0002CF54
int __cdecl BotChat_Random(bot_state_t *bs)
{

  float v1; // st7
  float rnd; // [esp+4h] [ebp-4h]

  v1 = nochat->value;
  if ( v1 != 0.0f )
    return 0;
  if ( bs->ltgtype == 1 || bs->ltgtype == 2 || bs->ltgtype == 5 )
    return 0;
  /* Characteristic 21 is the random-chat probability. */
  rnd = (float)Characteristic_BFloat(BotCharacter(bs), 21, 0.0f, 1.0f);
  if ( random() > bs->thinktime * 0.1 )
    return 0;
  if ( fastchat->value == 0.0f )
  {
    if ( random() > rnd || random() > 0.25 )
      return 0;
  }
  if ( !BotValidChatPosition(bs) )
    return 0;
  if ( random() < Characteristic_BFloat(BotCharacter(bs), 16, 0.0f, 1.0f) )
  {
    BotInitialChat(&bs->chatstate, "random_misc", (char *)0);
    return 1;
  }
  BotInitialChat(&bs->chatstate, "random_insult", (char *)0);
  return 1;
}

// gladiator.dll: 10022650..10022693
// gladi386.so:   0002CF54..0002CFA7
float __cdecl BotChatTime(bot_state_t *bs)
{

  int cpm; // [esp+4h] [ebp-4h]

  cpm = Characteristic_BInteger(BotCharacter(bs), 14, 1, 4000);
  return (int)BotChatLength(&bs->chatstate) * 30.0f / cpm;
}

// gladiator.dll: 100226C0..10022849
// gladi386.so:   0002CFA8..0002D0F4
float __cdecl BotAggression(bot_state_t *bs)
{
  int v2; // ecx

  if ( bs->inventory[INVULNERABILITY_SECONDS] )
    return 100.0f;
  if ( bs->inventory[ENEMY_INVULNERABILITY] )
    return 0.0f;
  if ( bs->inventory[ENEMY_QUAD] && !bs->inventory[QUAD_SECONDS] )
    return 0.0f;
  if ( bs->inventory[ENEMY_POWERSCREEN] && (!bs->inventory[POWER_SCREEN_CELLS] || ((int *)bs)[452] < 50) )
    return 0.0f;
  if ( bs->inventory[ENEMY_HEIGHT] > 200 )
    return 0.0f;
  v2 = bs->inventory[INVENTORY_HEALTH];
  if ( v2 < 40 || v2 < 70 && ((int *)bs)[433] < 40 && ((int *)bs)[434] < 50 && ((int *)bs)[435] < 60 )
    return 0.0f;
  if ( ((int *)bs)[449] > 0 && ((int *)bs)[452] > 50 )
    return 100.0f;
  if ( ((int *)bs)[448] > 0 && ((int *)bs)[454] > 5 )
    return 100.0f;
  if ( ((int *)bs)[447] > 0 && ((int *)bs)[452] > 50 )
    return 100.0f;
  if ( ((int *)bs)[446] > 0 && ((int *)bs)[453] > 5 )
    return 100.0f;
  if ( ((int *)bs)[445] > 0 && ((int *)bs)[444] > 10 )
    return 100.0f;
  if ( ((int *)bs)[443] > 0 && ((int *)bs)[451] > 100 )
    return 100.0f;
  if ( ((int *)bs)[442] > 0 && ((int *)bs)[451] > 75 )
    return 100.0f;
  if ( ((int *)bs)[441] > 0 && ((int *)bs)[450] > 20 )
    return 100.0f;
  return 0.0f;
}

// gladiator.dll: 100228C0..1002290A
// gladi386.so:   0002D0F4..0002D173
BOOL __cdecl BotWantsToRetreat(int *bs)
{
  if ( BotCTFCarryingFlag(bs) )
    return 1;
  if ( bs[1065] == 4 )
    return 1;
  if ( BotAggression((bot_state_t *)bs) < 50.0f )
    return 1;
  return 0;
}

// gladiator.dll: 10022930..10022953
// gladi386.so:   0002D174..0002D1AF
BOOL __cdecl BotWantsToChase(int *bs)
{
  if ( BotAggression((bot_state_t *)bs) > 50.0f )
    return 1;
  return 0;
}

// gladiator.dll: 10022970..10022976
// gladi386.so:   0002D1B0..0002D1B6
/* Always-true predicate between BotWantsToChase and BotCanAndWantsToRocketJump —
 * probably a feature toggle that ended up hard-coded.  DEAD. */
int __cdecl BotWantsToHelp(bot_state_t *bs)
{
  return 1;
}

// gladiator.dll: 10022990..10022A23
// gladi386.so:   0002D1B8..0002D259
BOOL BotCanAndWantsToRocketJump(bot_state_t *bs)
{
  int v3;

  if ( ((int *)bs)[446] <= 0 )
    return 0;
  if ( ((int *)bs)[453] < 3 )
    return 0;
  if ( bs->inventory[QUAD_SECONDS] )
    return 0;
  if ( bs->inventory[INVULNERABILITY_SECONDS] )
    return 1;
  v3 = bs->inventory[INVENTORY_HEALTH];
  if ( v3 < 60 )
    return 0;
  if ( v3 < 90 && ((int *)bs)[433] < 40 && ((int *)bs)[434] < 50 && ((int *)bs)[435] < 60 )
    return 0;
  if ( Characteristic_BFloat(BotCharacter(bs), 26, 0.0, 1.0) < 0.5 )
    return 0;
  return 1;
}

// gladiator.dll: 10022A60..10022D42
// gladi386.so:   0002D25C..0002D5C1
void __cdecl BotRoamGoal(bot_state_t *bs, vec3_t goal)
{
  int pc;
  float len, rnd, sign, i;
  vec3_t dir, bestorg, belowbestorg;
  bsp_trace_t trace;

  for ( i = 0; i < 10; i++ )
  {
    VectorCopy(bs->origin, bestorg);
    rnd = random();
    if ( rnd < 0.8 )
    {
      if ( random() < 0.5 ) sign = -1;
      else sign = 1;
      bestorg[0] += sign * 700 * random() + 50;
    }
    if ( rnd > 0.2 )
    {
      if ( random() < 0.5 ) sign = -1;
      else sign = 1;
      bestorg[1] += sign * 700 * random() + 50;
    }
    bestorg[2] += random() * 144 - 96 - 1;
    trace = AAS_Trace(bs->origin, NULL, NULL, bestorg, bs->entitynum, 3);
    VectorSubtract(bestorg, bs->origin, dir);
    len = VectorNormalize(dir);
    if ( len > 100 )
    {
      VectorScale(dir, len * trace.fraction - 40, dir);
      VectorAdd(bs->origin, dir, bestorg);
      VectorCopy(bestorg, belowbestorg);
      belowbestorg[2] -= 800;
      trace = AAS_Trace(bestorg, NULL, NULL, belowbestorg, bs->entitynum, 3);
      if ( !trace.startsolid )
      {
        trace.endpos[2]++;
        pc = AAS_PointContents(trace.endpos);
        if ( !(pc & 0x18) )
        {
          VectorCopy(bestorg, goal);
          return;
        }
      }
    }
  }
  VectorCopy(bestorg, goal);
}

// gladiator.dll: 10022E10..100233A0
// gladi386.so:   0002D5C4..0002DE0C
bot_moveresult_t __cdecl BotAttackMove(bot_state_t *bs, int tfl)
{
  int movetype, i;
  float attack_skill, jumper, croucher, dist, strafechange_time;
  vec3_t forward, backward, sideward, hordir, up = {0, 0, 1};
  aas_entityinfo_t entinfo;
  bot_moveresult_t moveresult;
  bot_goal_t goal;

  if ( bs->attackchase_time > AAS_Time() )
  {
    goal.entitynum = bs->enemy;
    goal.areanum = bs->lastenemyareanum;
    VectorCopy(bs->lastenemyorigin, goal.origin);
    VectorSet(goal.mins, -8, -8, -8);
    VectorSet(goal.maxs, 8, 8, 8);
    BotEntityInfo(bs, (_DWORD *)&bs->ms);
    return BotMoveToGoal((bot_movestate_t *)&bs->ms, &goal, tfl);
  }
  memset(&moveresult, 0, sizeof(bot_moveresult_t));
  if ( random() < Characteristic_BFloat(BotCharacter(bs), 48, 0, 1) )
    return moveresult;
  attack_skill = Characteristic_BFloat(BotCharacter(bs), 4, 0, 1);
  jumper = Characteristic_BFloat(BotCharacter(bs), 25, 0, 1);
  croucher = Characteristic_BFloat(BotCharacter(bs), 24, 0, 1);
  if ( attack_skill < 0.2 )
    return moveresult;
  BotEntityInfo(bs, (_DWORD *)&bs->ms);
  entinfo = AAS_EntityInfo(bs->enemy);
  VectorSubtract(entinfo.origin, bs->origin, forward);
  dist = VectorLength(forward);
  VectorNormalize(forward);
  backward[0] = -forward[0];
  backward[1] = -forward[1];
  backward[2] = -forward[2];
  movetype = 1;
  if ( bs->attackcrouch_time < AAS_Time() - 1 )
  {
    if ( random() < jumper )
      movetype = 4;
    else if ( bs->attackcrouch_time < AAS_Time() - 1 && random() < croucher )
      bs->attackcrouch_time = AAS_Time() + croucher * 5;
  }
  if ( bs->attackcrouch_time > AAS_Time() )
    movetype = 2;
  if ( movetype == 4 )
  {
    if ( bs->flags & 4 )
    {
      bs->flags &= ~4;
      movetype = 1;
    }
    else
      bs->flags |= 4;
  }
  if ( attack_skill <= 0.4 )
  {
    if ( dist > 180 )
    {
      if ( BotMoveInDirection((bot_movestate_t *)&bs->ms, forward, 400, movetype) )
        return moveresult;
    }
    if ( dist < 100 )
    {
      if ( BotMoveInDirection((bot_movestate_t *)&bs->ms, backward, 400, movetype) )
        return moveresult;
    }
    return moveresult;
  }
  bs->attackstrafe_drift += 0.1;
  strafechange_time = 0.4 + (1 - attack_skill) * 0.2;
  if ( attack_skill > 0.7 )
    strafechange_time += crandom() * 0.1;
  if ( bs->attackstrafe_drift > strafechange_time )
  {
    if ( random() > 0.935 )
    {
      bs->flags ^= 1;
      bs->attackstrafe_drift = 0;
    }
  }
  for ( i = 0; i < 2; i++ )
  {
    hordir[0] = forward[0];
    hordir[1] = forward[1];
    hordir[2] = 0;
    VectorNormalize(hordir);
    CrossProduct(hordir, up, sideward);
    if ( bs->flags & 1 )
    {
      sideward[0] = -sideward[0];
      sideward[1] = -sideward[1];
      sideward[2] = -sideward[2];
    }
    if ( random() > 0.9 )
    {
      VectorAdd(sideward, backward, sideward);
    }
    else
    {
      if ( dist > 180 )
        VectorAdd(sideward, forward, sideward);
      else if ( dist < 100 )
        VectorAdd(sideward, backward, sideward);
    }
    if ( BotMoveInDirection((bot_movestate_t *)&bs->ms, sideward, 400, movetype) )
      return moveresult;
    bs->flags ^= 1;
    bs->attackstrafe_drift = 0;
  }
  return moveresult;
}

// gladiator.dll: 10023510..10023533
// gladi386.so:   0002DE0C..0002DE4B
int __cdecl BotCTFTeam(bot_state_t *bs)
{
  const char *v1; // eax

  v1 = (const char *)ClientSkin(bs->client);
  if ( strstr(v1, "ctf_r") )
    return 1;
  return 2;
}

// gladiator.dll: 10023550..1002382D
// gladi386.so:   0002DE4C..0002E15D
BOOL __cdecl BotSameTeam(bot_state_t *bs, int entnum)
{
  aas_entityinfo_t entinfo;
  aas_entityinfo_t botinfo;
  char *team1, *team2;
  unsigned int len1, len2;

  entinfo = AAS_EntityInfo(entnum);
  if ( !entinfo.number )
    return 0;
  if ( teamplay_shell->value != 0.0f )
  {
    botinfo = AAS_EntityInfo(bs->entitynum);
    return (botinfo.renderfx & 0x1C00) == (entinfo.renderfx & 0x1C00);
  }
  if ( ch->value != 0.0f )
  {
    botinfo = AAS_EntityInfo(bs->entitynum);
    if ( botinfo.modelindex3 != entinfo.modelindex3 )
      return 1;
  }
  else if ( teamplay->value != 0.0f )
  {
    if ( !_strcmpi(ClientSkin(bs->client), ClientSkin(entinfo.number - 1)) )
      return 1;
    return 0;
  }
  else if ( ((int)dmflags->value & 0x40) || ctf->value != 0.0f )
  {
    team1 = strchr(ClientSkin(bs->client), '/');
    if ( !team1 )
      team1 = ClientSkin(bs->client);
    team2 = strchr(ClientSkin(entinfo.number - 1), '/');
    if ( !team2 )
      team2 = ClientSkin(entinfo.number - 1);
    if ( !_strcmpi(team1, team2) )
      return 1;
  }
  else if ( (int)dmflags->value & 0x80 )
  {
    team1 = strchr(ClientSkin(bs->client), '/');
    if ( team1 )
      len1 = team1 - ClientSkin(bs->client);
    else
      len1 = strlen(ClientSkin(bs->client));
    team2 = strchr(ClientSkin(entinfo.number - 1), '/');
    if ( team2 )
      len2 = team2 - ClientSkin(entinfo.number - 1);
    else
      len2 = strlen(ClientSkin(entinfo.number - 1));
    if ( len1 == len2 )
    {
      if ( !strncmp(ClientSkin(bs->client), ClientSkin(entinfo.number - 1), len1) )
        return 1;
    }
  }
  return 0;
}

// gladiator.dll: 100238F0..10023949
// gladi386.so:   0002E160..0002E1DD
int __cdecl BotNumTeamMates(bot_state_t *bs)
{
  int numplayers; // ebp
  int i; // esi

  numplayers = 0;
  for ( i = 0; i < botlibglobals.num_clients; i++ )
  {
    if ( strlen(clientsettings[i].netname) )
    {
      if ( BotSameTeam(bs, i + 1) )
        ++numplayers;
    }
  }
  return numplayers;
}

// gladiator.dll: 10023970..10023C30
// gladi386.so:   0002E1E0..0002E558
int __cdecl BotFindEnemy(bot_state_t *bs)
{
  int i, numents, alertness, healthdecrease;
  float dist, fov;
  int ents[16];
  aas_entityinfo_t entinfo;
  vec3_t dir, angles;

  alertness = Characteristic_BInteger(BotCharacter(bs), 45, 0, 1);
  //check if the health decreased
  healthdecrease = bs->lasthealth > bs->inventory[INVENTORY_HEALTH];
  //remember the current health value
  bs->lasthealth = bs->inventory[INVENTORY_HEALTH];
  numents = sub_1000BAA0(bs->entitynum, bs->eye, bs->viewangles, 360, 16, ents);
  for ( i = 0; i < numents; i++ )
  {
    entinfo = AAS_EntityInfo(ents[i]);
    //if the enemy isn't dead and the enemy isn't the bot self
    if ( sub_10021710((int *)&entinfo) || entinfo.number == bs->entitynum )
      continue;
    //calculate the distance towards the enemy
    VectorSubtract(entinfo.origin, bs->origin, dir);
    dist = VectorLength(dir);
    if ( !alertness && dist > 900 )
      continue;
    //if the bot's health decreased
    if ( healthdecrease )
      fov = 360;
    else
      fov = 360 - (270 - (dist > 810 ? 810 : dist) / 3);
    Vector2Angles(dir, angles);
    if ( !InFieldOfVision(bs->viewangles, fov, angles) )
      continue;
    //if on the same team
    if ( BotSameTeam(bs, ents[i]) )
      continue;
    if ( !healthdecrease || dist > 300 )
    {
      if ( AAS_PointLight(entinfo.origin, 0, 0, 0) < 5 )
        continue;
      if ( dist > 300 && !EntityIsShooting((intptr_t)&entinfo) )
      {
        //check if we can avoid this enemy
        VectorSubtract(bs->origin, entinfo.origin, dir);
        Vector2Angles(dir, angles);
        //if the bot isn't in the fov of the enemy
        if ( !InFieldOfVision(entinfo.angles, 160, angles) )
        {
          //update some stuff for this enemy
          BotUpdateBattleInventory(bs, ents[i]);
          //if the bot doesn't really want to fight
          if ( BotWantsToRetreat((int *)bs) )
            continue;
        }
      }
    }
    //found an enemy
    bs->enemy = entinfo.number;
    bs->enemysight_time = AAS_Time();
    return 1;
  }
  return 0;
}

// gladiator.dll: 10023CE0..100243C4
// gladi386.so:   0002E558..0002EDBF
void BotAimAtEnemy(bot_state_t *bs)
{
  int i;
  float dist, aim_skill, aim_accuracy, speed;
  /* Declaration order of the by-reference locals is Q3's own order in
   * ai_dmq3.c's BotAimAtEnemy -- `vec3_t dir, bestorigin, end, start,
   * groundtarget, ...;` then `mins`/`maxs`, then `entinfo` and `trace`.  It is
   * recoverable because gcc 2.7 lays the address-taken group out top-down in
   * declaration order, and the reference .so's slot order matches Q3's list
   * exactly.  The float spill slots (dist, aim_skill, aim_accuracy) follow the
   * same rule.  MSVC /O2 assigns slots in first-reference order, which makes
   * the DLL blind to this. */
  vec3_t dir, bestorigin, end, start, groundtarget;
  vec3_t mins = {-4, -4, -4}, maxs = {4, 4, 4};
  weaponinfo_t *wi;
  aas_entityinfo_t entinfo;
  bsp_trace_t trace;

  if ( bs->enemy )
  {
    aim_skill = Characteristic_BFloat(BotCharacter(bs), 7, 0, 1);
    aim_accuracy = Characteristic_BFloat(BotCharacter(bs), 8, 0, 1);
    //
    if ( aim_accuracy <= 0 )
      aim_accuracy = 0.0001f;
    //get the weapon information
    wi = sub_100354B0(BotWS(bs));
    if ( !_strcmpi(wi->name, "Rocket Launcher") )
      aim_accuracy = sqrt(aim_accuracy);
    //get the enemy entity information
    entinfo = AAS_EntityInfo(bs->enemy);
    //the bot's aim point
    VectorCopy(entinfo.origin, bestorigin);
    bestorigin[2] += 8;
    //get the start point shooting from
    //NOTE: the x and y projectile start offsets are ignored
    VectorCopy(bs->origin, start);
    start[2] += bs->snapshot.viewoffset[2];
    start[2] += wi->offset[2];
    //
    trace = AAS_Trace(start, mins, maxs, bestorigin, bs->entitynum, 100663299);
    //if the enemy is NOT hit
    if ( trace.fraction <= 1 && trace.ent != entinfo.number )
      bestorigin[2] += 16;
    //if the weapon has a speed and the bot is skilled enough
    if ( wi->speed && aim_skill > 0.4 )
    {
      //direction towards the enemy
      VectorSubtract(entinfo.origin, bs->origin, dir);
      //distance towards the enemy
      dist = VectorLength(dir);
      //direction the enemy is moving in
      dir[0] = entinfo.origin[0] - entinfo.lastvisorigin[0];
      dir[1] = entinfo.origin[1] - entinfo.lastvisorigin[1];
      dir[2] = 0;
      //
      speed = VectorNormalize(dir) / entinfo.update_time;
      //best spot to aim at
      VectorMA(entinfo.origin, (dist / wi->speed) * speed, dir, bestorigin);
    }
    //if the projectile does radial damage
    if ( aim_skill > 0.6 && (wi->proj->damagetype & 2) )
    {
      //if the enemy isn't standing significantly higher than the bot
      if ( entinfo.origin[2] < bs->origin[2] + 16 )
      {
        //try to aim at the ground in front of the enemy
        VectorCopy(entinfo.origin, end);
        end[2] -= 64;
        trace = AAS_Trace(entinfo.origin, NULL, NULL, end, entinfo.number, 100663299);
        //
        VectorCopy(bestorigin, groundtarget);
        if ( trace.startsolid )
          groundtarget[2] = entinfo.origin[2] - 16;
        else
          groundtarget[2] = trace.endpos[2] - 8;
        //trace a line from projectile start to ground target
        trace = AAS_Trace(start, NULL, NULL, groundtarget, bs->entitynum, 100663299);
        //if hitpoint is not vertically too far from the ground target
        if ( fabs(trace.endpos[2] - groundtarget[2]) < 50 )
        {
          VectorSubtract(trace.endpos, groundtarget, dir);
          //if the hitpoint is near enough the ground target
          if ( VectorLength(dir) < 60 )
          {
            VectorSubtract(trace.endpos, start, dir);
            //if the hitpoint is far enough from the bot
            if ( VectorLength(dir) > 150 )
            {
              //check if the bot is visible from the ground target
              trace = AAS_Trace(trace.endpos, NULL, NULL, entinfo.origin, entinfo.number, 100663299);
              if ( trace.fraction >= 1 )
                VectorCopy(groundtarget, bestorigin);
            }
          }
        }
      }
    }
    /* Q3's own phrasing, and it is load-bearing for the ELF: under
     * -ffast-math gcc 2.7's fold() rewrites `(C * crandom()) * x` into
     * `crandom() * (x * C)`, which is exactly the reference's evaluation
     * order, and CSEs the inline `(1 - aim_accuracy)` into the double spill
     * slot the reference has.  cl.exe reaches the DLL's `crandom() * x * C`
     * from the same text. */
    bestorigin[0] += 20 * crandom() * (1 - aim_accuracy);
    bestorigin[1] += 20 * crandom() * (1 - aim_accuracy);
    bestorigin[2] += 10 * crandom() * (1 - aim_accuracy);
    //get aim direction
    VectorSubtract(bestorigin, bs->eye, dir);
    //add some random stuff to the aim direction
    if ( !_strcmpi(wi->name, "Railgun") )
    {
      VectorNormalize(dir);
      for ( i = 0; i < 3; i++ )
        dir[i] += 0.3 * crandom() * (1 - aim_accuracy);
    }
    //set the ideal view angles
    Vector2Angles(dir, bs->ideal_viewangles);
    //take the weapon spread into account for lower skilled bots
    bs->ideal_viewangles[0] += 6 * wi->vspread * crandom() * (1 - aim_accuracy);
    bs->ideal_viewangles[0] = anglemod(bs->ideal_viewangles[0]);
    bs->ideal_viewangles[1] += 6 * wi->hspread * crandom() * (1 - aim_accuracy);
    bs->ideal_viewangles[1] = anglemod(bs->ideal_viewangles[1]);
    BotChangeViewAngles(bs, bs->thinktime);
    if ( aim_accuracy > 0.8 )
    {
      //set the view angles directly
      VectorCopy(bs->ideal_viewangles, bs->viewangles);
      EA_View(bs->client, bs->viewangles);
    }
  }
}

// gladiator.dll: 10024590..10024915
// gladi386.so:   0002EDC0..0002F17E
void BotCheckAttack(bot_state_t *bs)
{

  /* reactiontime/fov are declared BEFORE the pointers: MSVC6 coalesces the two into
   * one slot (their lifetimes do not overlap, which is why IDA shows both at
   * [esp+10h]), gcc 2.7 gives them one spill slot too -- and the ELF original puts
   * that slot ABOVE `wi`'s, which only this declaration order produces. */
  float reactiontime; // [esp+10h] [ebp-1A4h] — Characteristic_BFloat/AAS_Time splash-attack
                       // timer (Q3 ai_dmq3 BotCheckAttack 'reactiontime'); this and the
                       // fov local below are SEPARATE variables sharing one stack slot
                       // (non-overlapping lifetimes), not one variable reused.
  float fov; // [esp+10h] [ebp-1A4h] — visibility check distance (Q3 ai_dmq3 BotCheckAttack 'fov');
             // shares reactiontime's stack slot, see above.
  weaponinfo_t *wi; // ebx
  projectileinfo_t *v6; // ecx
  float points; // st — register-only (Q3 ai_dmq3 BotCheckAttack 'points')
  /* Q3 BotCheckAttack's declaration order, with mins/maxs LAST and mins before maxs:
   * gcc 2.7 lays the frame out in reverse declaration order, and the ELF original has
   * maxs BELOW mins, which only that order produces. */
  vec3_t forward; // [esp+20h] [ebp-194h] BYREF
  vec3_t right; // [esp+44h] [ebp-170h] BYREF
  vec3_t start; // [esp+14h] [ebp-1A0h] BYREF — trace start; as separate locals the
                // y/z stores get dead-store-eliminated and VectorMA/AAS_Trace see
                // garbage in [1]/[2]
  vec3_t end; // [esp+5Ch] [ebp-158h] BYREF
  vec3_t dir; // [esp+38h] [ebp-17Ch] BYREF
  bsp_trace_t trace; // [esp+68h] [ebp-14Ch] BYREF
  aas_entityinfo_t entinfo; // [esp+BCh] [ebp-7Ch] BYREF
  vec3_t mins; // [esp+50h] [ebp-164h] BYREF
  vec3_t maxs; // [esp+2Ch] [ebp-188h] BYREF

  /* Float literals: mins/maxs are float[3], not raw bit patterns.  No `attackentity`
   * local: the original never caches bs->enemy here — it is a direct
   * `cmp [bs+enemy],0` right after the mins/maxs stores. */
  mins[0] = -8.0f;
  mins[1] = -8.0f;
  mins[2] = -8.0f;
  maxs[0] = 8.0f;
  maxs[1] = 8.0f;
  maxs[2] = 8.0f;
  if ( bs->enemy )
  {
    /* Characteristic_BFloat's FPU return drives the splash-attack timer. */
    reactiontime = Characteristic_BFloat(BotCharacter(bs), 11, 0.0, 1.0);
    if ( AAS_Time() - reactiontime >= bs->enemysight_time )
    {
      entinfo = AAS_EntityInfo(bs->enemy);
      VectorSubtract(entinfo.origin, bs->origin, dir);
      if ( VectorLength(dir) < 100.0f )
        fov = 120.0f;
      else
        fov = 50.0f;
      if ( BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, fov, bs->enemy) )
      {
        wi = sub_100354B0(BotWS(bs));
        if ( wi )
        {
          VectorCopy(bs->origin, start);
          start[2] += bs->snapshot.viewoffset[2];
          AngleVectors(bs->viewangles, forward, right, 0);
          start[0] += forward[0] * wi->offset[0] + right[0] * wi->offset[1];
          start[1] += forward[1] * wi->offset[0] + right[1] * wi->offset[1];
          start[2] += forward[2] * wi->offset[0] + right[2] * wi->offset[1] + wi->offset[2];
          VectorMA(start, 1000.0, forward, end);
          VectorMA(start, -12.0, forward, start);
          trace = AAS_Trace(start, (float*)mins, (float*)maxs, (float*)(end), bs->entitynum, 100663299);
          if ( trace.ent == bs->enemy
            || (trace.ent <= 0 || trace.ent > botlibglobals.num_clients || !BotSameTeam(bs, trace.ent))
            && ((v6 = wi->proj, (v6->damagetype & 2) == 0)
             || trace.fraction * 1000.0f >= v6->radius
             || (points = ((double)v6->damage - trace.fraction * 500.0) * 0.5, points <= 0)) )
          {
            if ( (trace.contents & 2) != 0 )
            {
              entinfo = AAS_EntityInfo(bs->enemy);
              trace = AAS_Trace(trace.endpos, (float*)(uintptr_t)(0), (float*)(uintptr_t)(0),
                                entinfo.origin, bs->entitynum, 100663299);
              if ( trace.ent != bs->enemy )
                return;
            }
            if ( (wi->flags & 1) != 0 )
            {
              if ( (*(unsigned char *)&bs->flags & 2) != 0 )
                EA_Attack(bs->client);
            }
            else
            {
              EA_Attack(bs->client);
            }
            bs->flags ^= 2u;
          }
        }
      }
    }
  }
}

// gladiator.dll: 10024A10..10024E9D
// gladi386.so:   0002F180..0002F53D
bsp_entity_t *__cdecl BotEntityToActivate(int entitynum)
{
  int i;
  char *model, *tmpmodel, *classname, *target, *targetname[10];
  bsp_entity_t *ent, *cur_entities[10];
  aas_entityinfo_t entinfo;

  entinfo = AAS_EntityInfo(entitynum);
  model = AAS_ModelFromIndex(entinfo.modelindex);
  for ( ent = entities; ent; ent = ent->next )
  {
    tmpmodel = AAS_ValueForBSPEpairKey(ent, "model");
    if ( !tmpmodel )
      continue;
    if ( !strcmp(model, tmpmodel) )
      break;
  }
  if ( !ent )
  {
    botimport.Print(PRT_ERROR, "BotEntityToActivate: no entity found with model %s\n", model);
    return NULL;
  }
  classname = AAS_ValueForBSPEpairKey(ent, "classname");
  if ( !classname )
  {
    botimport.Print(PRT_ERROR, "BotEntityToActivate: entity with model %s has no classname\n", model);
    return NULL;
  }
  if ( !strcmp(classname, "func_door_secret") )
  {
    targetname[0] = AAS_ValueForBSPEpairKey(ent, "targetname");
    target = AAS_ValueForBSPEpairKey(ent, "spawnflags");
    if ( !targetname[0] || (atoi(target) & 1) )
      return ent;
  }
  if ( !strcmp(classname, "func_door") )
  {
    if ( AAS_FloatForBSPEpairKey(ent, "health") )
      return ent;
  }
  targetname[0] = AAS_ValueForBSPEpairKey(ent, "targetname");
  if ( !targetname[0] )
    return NULL;
  cur_entities[0] = entities;
  for ( i = 0; i >= 0 && i < 10; )
  {
    for ( ent = cur_entities[i]; ent; ent = ent->next )
    {
      target = AAS_ValueForBSPEpairKey(ent, "target");
      if ( !target )
        continue;
      if ( !strcmp(targetname[i], target) )
      {
        cur_entities[i] = ent->next;
        break;
      }
    }
    if ( !ent )
    {
      botimport.Print(PRT_ERROR, "BotEntityToActivate: no entity with target \"%s\"\n", targetname[i]);
      i--;
      continue;
    }
    classname = AAS_ValueForBSPEpairKey(ent, "classname");
    if ( !classname )
    {
      botimport.Print(PRT_ERROR, "BotEntityToActivate: entity with target \"%s\" has no classname\n", targetname[i]);
      return NULL;
    }
    if ( !strcmp(classname, "trigger_counter") || !strcmp(classname, "trigger_relay") )
    {
      if ( i >= 9 )
      {
        botimport.Print(PRT_ERROR, "BotEntityToActivate: stacked up more than %d trigger_counter or trigger_relay\n", i);
        return NULL;
      }
      targetname[++i] = AAS_ValueForBSPEpairKey(ent, "targetname");
      cur_entities[i] = entities;
      continue;
    }
    if ( !strcmp(classname, "func_button") )
      return ent;
    if ( !strcmp(classname, "trigger_multiple") )
      return ent;
    if ( !strcmp(classname, "trigger_once") )
      return ent;
    if ( !strcmp(classname, "func_door_rotating") )
      return ent;
    if ( !strcmp(classname, "trigger_key") )
      return NULL;
    i--;
  }
  botimport.Print(PRT_ERROR, "BotEntityToActivate: unkown activator with classname \"%s\"\n", classname);
  return NULL;
}

// gladiator.dll: 10024FD0..1002504D
// gladi386.so:   0002F540..0002F5E2
void __cdecl BotSetMovedir(float *angles, float *movedir)
{
  if ( VectorCompare(angles, VEC_UP) )
  {
    VectorCopy(MOVEDIR_UP, movedir);
  }
  else if ( VectorCompare(angles, VEC_DOWN) )
  {
    VectorCopy(MOVEDIR_DOWN, movedir);
  }
  else
  {
    AngleVectors(angles, movedir, NULL, NULL);
  }
}

// gladiator.dll: 10025070..1002545E
// gladi386.so:   0002F5E4..0002FBCD
/* Debug visualiser for func_button entities: walk the BSP entity list, filter by
 * classname == "func_button", read the brush's model AABB plus the "angle"/"health"
 * keys, and draw permanent debug crosses — one at the shoot point for shootable
 * buttons, three for touch/use buttons.  Capped at the first 6 matches.
 *
 * The body is Q3 BotFuncButtonActivateGoal's, statement for statement: `lip` is read
 * and defaulted but never used (the .so keeps a dead `fld 0.0; fstp st(0)` from the
 * `if (!lip)` test), and BSPModelMinsMaxs is called with zero-initialised `angles`,
 * so the bbox is the model's local-space AABB rather than a world-rotated one.
 * The vec3 declaration order is the .so's frame read top-down (gcc 2.7 lays out
 * address-taken locals in declaration order); cl.exe is indifferent to it. */
void __cdecl sub_10025070(void)
{
  bsp_entity_t *ent;
  int drawn;
  char *classname;
  char *model_str;
  int modelnum;
  vec3_t mins;
  vec3_t maxs;
  vec3_t size;
  vec3_t origin;
  vec3_t angles;
  vec3_t movedir;
  vec3_t goalorigin;
  vec3_t start;
  vec3_t end;
  vec3_t bboxmins;
  vec3_t bboxmaxs;
  aas_trace_t trace;
  float dist;
  float lip;

  drawn = 0;
  ent   = entities;
  if ( !ent )
    return;

  do
  {
    /* Do NOT remove the two (const char *) casts below, even though they trip
     * -Wdiscarded-qualifiers and are codegen-neutral here: dropping them perturbs
     * MSVC6's whole-TU scheduler enough to flip a tie-break in PC_ReadDefineParms and
     * cost that function its byte-match. */
    classname = (const char *)AAS_ValueForBSPEpairKey(ent, "classname");
    if ( !strcmp(classname, "func_button") )
    {
      model_str = (const char *)AAS_ValueForBSPEpairKey(ent, "model");
      modelnum  = AAS_IndexFromModel(model_str);
      if ( !modelnum )
        modelnum = atoi(model_str + 1);

      VectorClear(angles);
      AAS_BSPModelMinsMaxsOrigin(modelnum - 1, angles, mins, maxs, NULL);

      lip = AAS_FloatForBSPEpairKey(ent, "lip");
      if (!lip) lip = 4;
      VectorSet(angles, 0, AAS_FloatForBSPEpairKey(ent, "angle"), 0);
      BotSetMovedir(angles, movedir);

      VectorSubtract(maxs, mins, size);
      VectorAdd(mins, maxs, origin);
      VectorScale(origin, 0.5f, origin);
      dist = fabs(movedir[0]) * size[0] + fabs(movedir[1]) * size[1] + fabs(movedir[2]) * size[2];
      dist *= 0.5;

      if ( AAS_FloatForBSPEpairKey(ent, "health") )
      {
        VectorMA(origin, -dist, movedir, goalorigin);
        AAS_DrawPermanentCross(goalorigin, 4.0f, (int)0xf3f3f1f1);
      }
      else
      {
        int i;

        AAS_PresenceTypeBoundingBox(4, bboxmins, bboxmaxs);
        for (i = 0; i < 3; i++)
        {
          if (movedir[i] < 0) dist += fabs(movedir[i]) * fabs(bboxmaxs[i]);
          else dist += fabs(movedir[i]) * fabs(bboxmins[i]);
        }
        VectorMA(origin, -dist, movedir, goalorigin);

        VectorCopy(goalorigin, start);
        start[2] += 24.0f;
        VectorSet(end, start[0], start[1], start[2] - 100.0f);
        trace = AAS_TraceClientBBox(start, end, 4, -1);
        if ( !trace.startsolid )
          VectorCopy(trace.endpos, goalorigin);
        AAS_DrawPermanentCross(goalorigin, 4.0f, (int)0xdcdddedf);

        VectorSubtract(mins, origin, mins);
        VectorSubtract(maxs, origin, maxs);

        VectorAdd(mins, origin, start);
        AAS_DrawPermanentCross(start, 4.0f, (int)0xf3f3f1f1);

        VectorAdd(maxs, origin, start);
        AAS_DrawPermanentCross(start, 4.0f, (int)0xf3f3f1f1);
      }

      if ( ++drawn > 5 )
        return;
    }
    ent = ent->next; /* next entity (typed bsp_entity_t) */
  }
  while ( ent );
}

// gladiator.dll: 10025560..1002600F
// gladi386.so:   0002FBD0..000308CB
/* Genuinely void, as in Q3: each exit path just leaves whatever is in eax and
 * every caller ignores it. */
void __cdecl BotAIBlocked(bot_state_t *bs, bot_moveresult_t *moveresult, int activate)
{
  int i, modelnum;
  char *classname, *model;
  float dist, lip;
  bsp_entity_t *ent;
  vec3_t mins, maxs, triggerorigin, start, end, angles, movedir, hordir, sideward;
  vec3_t size, origin, goalorigin, bboxmins, bboxmaxs;
  vec3_t up = {0, 0, 1};
  vec3_t extramins = {-5, -5, -5}, extramaxs = {5, 5, 5};
  aas_entityinfo_t entinfo;
  aas_trace_t trace;
  bsp_trace_t bsptrace;  /* Q3's declaration, unused here; gcc 2.7 still gives it its frame slot */

  if ( !moveresult->blocked )
    return;
  entinfo = AAS_EntityInfo(moveresult->blockentity);
  if ( entinfo.solid == 3 && activate )
  {
    ent = BotEntityToActivate(entinfo.number);
    if ( !ent )
      classname = "";
    else
      classname = AAS_ValueForBSPEpairKey(ent, "classname");
    if ( !strcmp(classname, "func_door_secret") || !strcmp(classname, "func_door") )
    {
      model = AAS_ValueForBSPEpairKey(ent, "model");
      modelnum = AAS_IndexFromModel(model);
      if ( !modelnum )
        return;
      VectorClear(angles);
      AAS_BSPModelMinsMaxsOrigin(modelnum - 1, angles, mins, maxs, NULL);
      VectorAdd(maxs, mins, goalorigin);
      VectorScale(goalorigin, 0.5, goalorigin);
      VectorSubtract(goalorigin, bs->origin, movedir);
      Vector2Angles(movedir, moveresult->ideal_viewangles);
      moveresult->flags |= 1;
      EA_UseItem(bs->client, "Blaster");
      EA_Attack(bs->client);
      return;
    }
    if ( !strcmp(classname, "func_button") )
    {
      model = AAS_ValueForBSPEpairKey(ent, "model");
      modelnum = AAS_IndexFromModel(model);
      if ( !modelnum )
        return;
      VectorClear(angles);
      AAS_BSPModelMinsMaxsOrigin(modelnum - 1, angles, mins, maxs, NULL);
      lip = AAS_FloatForBSPEpairKey(ent, "lip");
      if (!lip) lip = 4;
      VectorSet(angles, 0, AAS_FloatForBSPEpairKey(ent, "angle"), 0);
      BotSetMovedir(angles, movedir);
      VectorSubtract(maxs, mins, size);
      VectorAdd(mins, maxs, origin);
      VectorScale(origin, 0.5, origin);
      dist = fabs(movedir[0]) * size[0] + fabs(movedir[1]) * size[1] + fabs(movedir[2]) * size[2];
      dist *= 0.5;
      if ( AAS_FloatForBSPEpairKey(ent, "health") )
      {
        VectorMA(origin, -dist, movedir, goalorigin);
        VectorSubtract(goalorigin, bs->origin, movedir);
        Vector2Angles(movedir, moveresult->ideal_viewangles);
        moveresult->flags |= 1;
        EA_UseItem(bs->client, "Blaster");
        EA_Attack(bs->client);
        return;
      }
      AAS_PresenceTypeBoundingBox(4, bboxmins, bboxmaxs);
      for (i = 0; i < 3; i++)
      {
        if (movedir[i] < 0) dist += fabs(movedir[i]) * fabs(bboxmaxs[i]);
        else dist += fabs(movedir[i]) * fabs(bboxmins[i]);
      }
      VectorMA(origin, -dist, movedir, goalorigin);
      VectorCopy(goalorigin, start);
      start[2] += 24;
      VectorSet(end, start[0], start[1], start[2] - 100);
      trace = AAS_TraceClientBBox(start, end, 4, -1);
      if ( !trace.startsolid )
        VectorCopy(trace.endpos, goalorigin);
      VectorCopy(origin, bs->activategoal.origin);
      bs->activategoal.areanum = AAS_PointAreaNum(goalorigin);
      VectorSubtract(mins, origin, bs->activategoal.mins);
      VectorSubtract(maxs, origin, bs->activategoal.maxs);
      VectorAdd(bs->activategoal.mins, extramins, bs->activategoal.mins);
      VectorAdd(bs->activategoal.maxs, extramaxs, bs->activategoal.maxs);
      bs->activategoal.entitynum = entinfo.number;
      bs->activategoal.number = 0;
      bs->activategoal.flags = 0;
      bs->activategoal_time = AAS_Time() + 10;
      if ( !AAS_AreaReachability(bs->activategoal.areanum) )
      {
        if ( BotAINode(bs) == AINode_Seek_NBG )
          bs->nbg_time = 0;
        else if ( BotAINode(bs) == AINode_Seek_LTG )
          bs->ltg_time = 0;
        return;
      }
      AIEnter_Seek_ActivateEntity(bs);
      return;
    }
    if ( !strcmp(classname, "trigger_multiple") || !strcmp(classname, "trigger_once") )
    {
      model = AAS_ValueForBSPEpairKey(ent, "model");
      modelnum = AAS_IndexFromModel(model);
      if ( !modelnum )
        modelnum = atoi(model + 1);
      VectorClear(angles);
      AAS_BSPModelMinsMaxsOrigin(modelnum - 1, angles, mins, maxs, NULL);
      VectorAdd(mins, maxs, triggerorigin);
      VectorScale(triggerorigin, 0.5, triggerorigin);
      VectorCopy(triggerorigin, start);
      start[2] = maxs[2] + 24;
      VectorSet(end, start[0], start[1], start[2] - 100);
      trace = AAS_TraceClientBBox(start, end, 4, -1);
      if ( !trace.startsolid )
      {
        VectorCopy(trace.endpos, goalorigin);
        VectorCopy(triggerorigin, bs->activategoal.origin);
        bs->activategoal.areanum = AAS_PointAreaNum(goalorigin);
        VectorSubtract(mins, triggerorigin, bs->activategoal.mins);
        VectorSubtract(maxs, triggerorigin, bs->activategoal.maxs);
        bs->activategoal.entitynum = entinfo.number;
        bs->activategoal.number = 0;
        bs->activategoal.flags = 0;
        bs->activategoal_time = AAS_Time() + 10;
        if ( !AAS_AreaReachability(bs->activategoal.areanum) )
        {
          if ( BotAINode(bs) == AINode_Seek_NBG )
            bs->nbg_time = 0;
          else if ( BotAINode(bs) == AINode_Seek_LTG )
            bs->ltg_time = 0;
          return;
        }
        AIEnter_Seek_ActivateEntity(bs);
      }
      return;
    }
  }
  hordir[0] = moveresult->movedir[0];
  hordir[1] = moveresult->movedir[1];
  hordir[2] = 0;
  VectorNormalize(hordir);
  VectorCopy(bs->origin, start);
  start[2] += libvar_sv_step->value;
  VectorMA(start, 5, hordir, end);
  VectorSet(mins, -16, -16, -24);
  VectorSet(maxs, 16, 16, 4);
  CrossProduct(hordir, up, sideward);
  if ( bs->flags & 0x10 )
  {
    sideward[0] = -sideward[0];
    sideward[1] = -sideward[1];
    sideward[2] = -sideward[2];
  }
  if ( !BotMoveInDirection((bot_movestate_t *)&bs->ms, sideward, 400, 1) )
  {
    bs->flags ^= 0x10;
    sideward[0] = -sideward[0];
    sideward[1] = -sideward[1];
    sideward[2] = -sideward[2];
    BotMoveInDirection((bot_movestate_t *)&bs->ms, sideward, 400, 1);
  }
  if ( BotAINode(bs) == AINode_Seek_NBG )
    bs->nbg_time = 0;
  else if ( BotAINode(bs) == AINode_Seek_LTG )
    bs->ltg_time = 0;
}

// gladiator.dll: 100262C0..1002638D
// gladi386.so:   000308CC..000309C2
void __cdecl sub_100262C0(_DWORD *a1, bot_goal_t *a2)
{
  aas_entityinfo_t info; // [esp+8h] [ebp-7Ch] BYREF

  if ( ctf->value != 0.0f )
  {
    if ( a2->entitynum )
    {
      info = AAS_EntityInfo(a2->entitynum);
      if ( (info.modelindex == modelindex_tech1 || info.modelindex == modelindex_tech2 || info.modelindex == modelindex_tech3 || info.modelindex == modelindex_tech4)
        && ((int)a1[477] > 0 && info.modelindex != modelindex_tech1
         || (int)a1[478] > 0 && info.modelindex != modelindex_tech2
         || (int)a1[479] > 0 && info.modelindex != modelindex_tech3
         || (int)a1[480] > 0 && info.modelindex != modelindex_tech3) )
      {
        EA_DropItem(a1[1], "tech");
      }
    }
  }
}

// gladiator.dll: 100263D0..10026414
// gladi386.so:   000309C4..00030A41
void __cdecl BotCTFRetreatGoals(bot_state_t *bs)
{

  float v1; // st7

  if ( BotCTFCarryingFlag(bs) )
  {
    if ( bs->ltgtype != 5 )
    {
      bs->ltgtype = 5;
      v1 = AAS_Time();
      bs->teamgoal_time = v1 + 120.0f;
      *(int *)&bs->rushbaseaway_time = 0;
    }
  }
}

// gladiator.dll: 10026440..10026604
// gladi386.so:   00030A44..00030CBF
void __cdecl BotCTFSeekGoals(bot_state_t *bs)
{

  int v3; // eax
  double v5; // st7

  if ( BotCTFCarryingFlag(bs) )
  {
    if ( bs->ltgtype != 5 )
    {
      bs->ltgtype = 5;
      bs->teamgoal_time = AAS_Time() + 120.0f;
      *(int *)&bs->rushbaseaway_time = 0;
    }
  }
  else if ( AAS_Time() >= bs->ctfroam_time )
  {
    v3 = bs->ltgtype;
    if ( v3 != 1 && v3 != 2 && v3 != 3 && v3 != 4 && v3 != 5 && v3 != 6 && v3 != 7 && BotAggression(bs) >= 50.0f )
    {
      bs->teammessage_time = AAS_Time() + 2 * random();
      v5 = (rand() & 0x7FFF) * 0.0000305185f;
      if ( v5 < 0.33f && ctf_flag1.areanum && ctf_flag2.areanum )
      {
        bs->ltgtype = 4;
        bs->teamgoal_time = AAS_Time() + 180.0f;
      }
      else if ( v5 < 0.66 && ctf_flag1.areanum && ctf_flag2.areanum )
      {
        if ( BotCTFTeam(bs) == 1 )
          memcpy(&bs->teamgoal, &ctf_flag1, 0x38u);
        else
          memcpy(&bs->teamgoal, &ctf_flag2, 0x38u);
        bs->ltgtype = 3;
        bs->teamgoal_time = AAS_Time() + 120.0f;
        *(int *)&bs->defendaway_time = 0;
      }
      else
      {
        bs->ltgtype = 0;
        bs->ctfroam_time = AAS_Time() + 60.0f;
      }
    }
  }
}

// gladiator.dll: 10026690..100266D6
// gladi386.so:   00030CC0..00030D42
BOOL TeamPlayIsOn()
{
  return ((int)dmflags->value & 0xC0) != 0
      || ctf->value != 0.0f
      || teamplay->value != 0.0f;
}

// gladiator.dll: 10026700..10026746
// gladi386.so:   00030D44..00030D92
BOOL __cdecl BotGetItemTeamGoal(char *goalname, bot_goal_t *goal)
{
  int i;

  if ( !strlen(goalname) )
    return 0;
  i = -1;
  /* Keep the do/while: the original carries the second iteration path even
   * though a positive first result returns immediately. */
  do
  {
    i = BotGetLevelItemGoal(i, goalname, goal);
    if ( i > 0 )
      return 1;
  }
  while ( i > 0 );
  return 0;
}

// gladiator.dll: 10026770..100267BF
// gladi386.so:   00030D94..00030E29
int __cdecl BotGetMessageTeamGoal(bot_state_t *bs, char *goalname, bot_goal_t *goal)
{
  int cp; // eax

  if ( BotGetItemTeamGoal(goalname, goal) )
    return 1;
  cp = BotFindWayPoint(BotCheckpoints(bs), goalname);
  if ( cp )
  {
    memcpy((void *)goal, (const void *)(cp + 4), 0x38u);
    return 1;
  }
  return 0;
}

// gladiator.dll: 100267E0..1002689B
// gladi386.so:   00030E2C..00030EF0
float __cdecl BotGetTime(bot_match_t *match)
{
  float v1; // st7
  float t; // [esp+0h] [ebp-18Ch]
  /* One bot_match_t (240 B), filled by BotFindMatch — see chat_state.h. */
  bot_match_t timematch; // [esp+9Ch] [ebp-F0h] BYREF
  char timestring[152]; // [esp+4h] [ebp-188h] BYREF

  if ( (match->subtype & 0x10) != 0 )
  {
    BotMatchVariable(match, 5, timestring);
    if ( BotFindMatch(timestring, &timematch, 8) )
    {
      BotMatchVariable(&timematch, 5, timestring);
      if ( timematch.type == 105 )
      {
        v1 = atof(timestring) * 60.0;
        t = v1;
      }
      else if ( timematch.type == 106 )
      {
        v1 = atof(timestring);
        t = v1;
      }
      if ( t > 0.0f )
        return AAS_Time() + t;
    }
  }
  return 0.0f;
}

// gladiator.dll: 100268D0..10026953
// gladi386.so:   00030EF0..00031037
int __cdecl FindClientByName(char *name)
{
  int i;

  for ( i = 0; i < botlibglobals.num_clients; i++ )
  {
    if ( !_strcmpi(clientsettings[i].netname, name) )
      return i;
  }
  for ( i = 0; i < botlibglobals.num_clients; i++ )
  {
    if ( stristr(clientsettings[i].netname, name) )
      return i;
  }
  return -1;
}

// gladiator.dll: 10026990..10026B5F
// gladi386.so:   00031038..0003139B
int __cdecl BotGetPatrolWaypoints(bot_state_t *bs, bot_match_t *match)
{
  int patrolflags; // edi (patrol flags accumulator)
  bot_waypoint_t *newpatrolpoints; // esi (head of new patrol list)
  bot_waypoint_t *newwp; // eax (newly-allocated node)
  bot_waypoint_t *wp; // ecx (tail walker)
  char Destination[152]; // [esp+48h] [ebp-188h] BYREF
  /* One bot_match_t (240 B), filled by BotFindMatch — see chat_state.h. */
  bot_match_t keyareamatch; // [esp+E0h] [ebp-F0h] BYREF
  bot_goal_t goal; // [esp+10h] [ebp-1C0h] BYREF — parsed goal for current keypoint name

  newpatrolpoints = NULL;
  patrolflags = 0;
  BotMatchVariable(match, 4, Destination);
  while ( 1 )
  {
    if ( !BotFindMatch(Destination, &keyareamatch, 64) )
    {
      EA_SayTeam(bs->client, "what do you say?");
      BotFreeWaypoints(newpatrolpoints);
      BotPatrolpoints(bs) = NULL;
      return 0;
    }
    BotMatchVariable(&keyareamatch, 4, Destination);
    if ( !BotGetMessageTeamGoal(bs, Destination, &goal) )
    {
      BotInitialChat(&bs->chatstate, "cannotfind", Destination, (char *)0);
      BotEnterChat(&bs->chatstate, bs->client, 1);
      BotFreeWaypoints(newpatrolpoints);
      BotPatrolpoints(bs) = NULL;
      return 0;
    }
    /* Thunk 0x10001401 goes to BotCreateWayPoint, not BotResetState. */
    newwp = BotCreateWayPoint(Destination, goal.origin, goal.areanum);
    newwp->next = NULL;
    for ( wp = newpatrolpoints; wp && wp->next; wp = wp->next )
      ;
    if ( !wp )
    {
      newpatrolpoints = newwp;
      newwp->prev = NULL;
    }
    else
    {
      wp->next = newwp;
      newwp->prev = wp;
    }
    if ( (keyareamatch.subtype & 0x200) != 0 )
    {
      patrolflags = 1;
      break;
    }
    else if ( (keyareamatch.subtype & 0x400) != 0 )
    {
      patrolflags = 2;
      break;
    }
    else if ( (keyareamatch.subtype & 0x100) != 0 )
    {
      BotMatchVariable(&keyareamatch, 5, Destination);
    }
    else
    {
      break;
    }
  }
  if ( !newpatrolpoints || !newpatrolpoints->next )
  {
    EA_SayTeam(bs->client, "I need more key points to patrol\n");
    BotFreeWaypoints(newpatrolpoints);
    return 0;
  }
  else
  {
    BotFreeWaypoints(BotPatrolpoints(bs));
    BotPatrolpoints(bs) = newpatrolpoints;
    BotCurPatrolPoint(bs) = newpatrolpoints;
    bs->patrolflags = patrolflags;
    return 1;
  }
}

// gladiator.dll: 10026BE0..10026DBC
// gladi386.so:   0003139C..000315C2
int __cdecl BotAddressedToBot(bot_state_t *bs, bot_match_t *match)
{
  int client; // eax
  int result; // eax
  const char *botname; // esi (Q2 ClientName returns char*)
  float v5; // [esp+Ch] [ebp-2BCh]
  char addressedto[152]; // [esp+A8h] [ebp-220h] BYREF
  char netname[152]; // [esp+230h] [ebp-98h] BYREF
  char name[152]; // [esp+10h] [ebp-2B8h] BYREF
  /* One bot_match_t (240 B), filled by BotFindMatch — see chat_state.h. */
  bot_match_t addresseematch; // [esp+140h] [ebp-188h] BYREF

  BotMatchVariable(match, 0, netname);
  client = ClientFromName(netname);
  if ( client < 0 )
    return 0;
  result = BotSameTeam(bs, client + 1);
  if ( !result )
    return result;
  if ( (match->subtype & 2) != 0 )
  {
    BotMatchVariable(match, 1, addressedto);
    botname = ClientName(bs->client);
    while ( BotFindMatch(addressedto, &addresseematch, 32) )
    {
      if ( addresseematch.type == 101 )
      {
        return 1;
      }
      if ( addresseematch.type == 102 )
      {
        BotMatchVariable(&addresseematch, 3, name);
        if ( StringContains(botname, name, 0) )
          return 1;
        if ( StringContains(bs->teamleader, name, 0) )
          return 1;
        BotMatchVariable(&addresseematch, 5, addressedto);
      }
      else
      {
        BotMatchVariable(&addresseematch, 3, name);
        if ( StringContains(botname, name, 0) )
          return 1;
        if ( StringContains(bs->teamleader, name, 0) )
          return 1;
        break;
      }
    }
    return 0;
  }
  else
  {
    v5 = random();
    if ( 1.0f / (float)(BotNumTeamMates(bs) - 1) < v5 )
      return 0;
  }
  return 1;
}

// gladiator.dll: 10026E40..10026ED9
// gladi386.so:   000315C4..000316B2
// Q3's BotGPSToPosition.  DEAD in Gladiator — /INCREMENTAL.
int __cdecl BotGPSToPosition(char *buf, float *position)
{
  int i;
  int j = 0;
  int num;
  int sign;

  for ( i = 0; i < 3; i++ )
  {
    num = 0;
    while ( buf[j] == ' ' )
      j++;
    if ( buf[j] == '-' )
    {
      j++;
      sign = -1;
    }
    else
    {
      sign = 1;
    }
    while ( buf[j] )
    {
      if ( buf[j] >= '0' && buf[j] <= '9' )
      {
        num = num * 10 + buf[j] - '0';
        j++;
      }
      else
      {
        j++;
        break;
      }
    }
    botimport.Print(PRT_MESSAGE, "%d\n", sign * num);
    position[i] = (float)sign * num;
  }
  return 1;
}

// gladiator.dll: 10026F10..10028198
// gladi386.so:   000316B4..00032D6B
int __cdecl BotMatchMessage(bot_state_t *bs, char *message)
{
  int v3; // eax
  int v4; // ebx
  int v5; // eax
  int v6; // eax
  int v10; // ax
  float v11; // st7
  int v12; // eax
  float v13; // st7
  int v14; // ax
  float v16; // st7
  int v18; // ebx
  int v24; // ax
  float v26; // st7
  int v27; // eax
  int v31; // ax
  float v33; // st7
  float v34; // st7
  int v35; // ax
  float v37; // st7
  int v38; // ax
  float v40; // st7
  int v41; // ebx
  bot_waypoint_t *v42; // eax
  bot_waypoint_t *v43; // ecx
  bot_waypoint_t *v44; // ecx
  bot_waypoint_t *v45; // esi
  bot_waypoint_t *v46; // eax
  float v47; // st7
  int v48; // eax
  int v49; // eax
  int v50; // eax
  /* ONE slot shared by the mutually-exclusive switch cases; kept as a union so
   * MSVC packs every assignment onto the same 4 bytes. */
  int u54i;   /* int view (checkpoint num) */
  float u54f; /* float view of the same slot */
#define v54 u54i
#define v55 u54f
  float v56;
  float v57;
  float v58;
  float v59;
  float v60;
  vec3_t origin; // [esp+2Ch] [ebp-5C0h] BYREF — checkpoint origin parsed from chat (sscanf input to AAS_PointAreaNum)
  /* The full 240-byte bot_match_t (chat_state.h), including the variables[] capture
   * array, so the BotFindMatch / StringsMatch / BotMatchVariable interfaces
   * type-check. */
  bot_match_t match; // [esp+38h] [ebp-5B4h] -- the entire match struct
  char Destination[152]; // [esp+128h] [ebp-4C4h] BYREF
  aas_entityinfo_t entinfo; // [esp+1C0h] [ebp-42Ch] BYREF
  char Source[152]; // [esp+23Ch] [ebp-3B0h] BYREF
  char Buffer[152]; // [esp+2D4h] [ebp-318h] BYREF
  char String2[152]; // [esp+36Ch] [ebp-280h] BYREF
  /* One bot_match_t (240 B), filled by BotFindMatch — see chat_state.h. */
  bot_match_t teammatematch; // [esp+4FCh] [ebp-F0h] BYREF

  match.type = 0;
  if ( !BotFindMatch(message, &match, 7) )
    return 0;
  switch ( match.type )
  {
    case 1:
      BotMatchVariable(&match, 0, Buffer);
      v3 = ClientFromName(Buffer);
      if ( v3 == bs->client )
      {
        bs->botdeathtype = match.subtype;
        return 1;
      }
      else
      {
        if ( v3 + 1 != bs->enemy )
          return 1;
        bs->enemydeathtype = match.subtype;
        bs->killedenemy_time = AAS_Time();
        return 1;
      }
    case 2:
    case 17:
      return 1;
    case 3:
    case 4:
      if ( !TeamPlayIsOn() || !BotAddressedToBot(bs, &match) )
        return 1;
      BotMatchVariable(&match, 3, Source);
      if ( BotFindMatch(Source, &teammatematch, 16) && teammatematch.type == 100 )
      {
        BotMatchVariable(&match, 0, Destination);
        v4 = ClientFromName(Destination) + 1;
        v5 = 0;
      }
      else
      {
        v4 = FindClientByName(Source) + 1;
        if ( v4 == bs->entitynum )
          return 1;
        v5 = 1;
      }
      v54 = v5;
      if ( !v4 )
      {
        if ( v5 )
          BotInitialChat(&bs->chatstate, "whois", Source, (char *)0);
        else
          BotInitialChat(&bs->chatstate, "whois", Destination, (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        return 1;
      }
      bs->teamgoal.entitynum = 0;
      entinfo = AAS_EntityInfo(v4);
      if ( entinfo.valid )
      {
        v6 = AAS_PointAreaNum(entinfo.origin);
        if ( v6 )
        {
          if ( AAS_AreaReachability(v6) )
          {
            bs->teamgoal.origin[2] = entinfo.origin[2];
            bs->teamgoal.entitynum = v4;
            bs->teamgoal.mins[0] = -8.0f;
            bs->teamgoal.mins[1] = -8.0f;
            bs->teamgoal.mins[2] = -8.0f;
            bs->teamgoal.areanum = v6;
            bs->teamgoal.origin[0] = entinfo.origin[0];
            bs->teamgoal.origin[1] = entinfo.origin[1];
            bs->teamgoal.maxs[0] = 8.0f;
            bs->teamgoal.maxs[1] = 8.0f;
            bs->teamgoal.maxs[2] = 8.0f;
          }
        }
      }
      if ( bs->teamgoal.entitynum )
        goto LABEL_32;
      if ( (match.subtype & 1) == 0 || (BotMatchVariable(&match, 2, String2), BotGetMessageTeamGoal(bs, String2, &bs->teamgoal)) )
      {
        if ( !bs->teamgoal.entitynum )
        {
          if ( v54 )
            BotInitialChat(&bs->chatstate, "whereis", Source, (char *)0);
          else
            BotInitialChat(&bs->chatstate, "whereareyou", Destination,
                           (char *)0);
          BotEnterChat(&bs->chatstate, bs->client, 1);
          return 1;
        }
LABEL_32:
        bs->teammate = v4;
        bs->teammatevisible_time = AAS_Time();
        v10 = rand();
        v55 = (float)(v10 & 0x7FFF) * 0.000030518509f;
        v55 = v55 + v55;
        bs->teammessage_time = AAS_Time() + v55;
        v11 = BotGetTime(&match);
        v12 = match.type;
        bs->teamgoal_time = v11;
        if ( v12 == 3 )
        {
          bs->ltgtype = 1;
          if ( v11 == 0 )
          {
            v13 = AAS_Time();
            bs->teamgoal_time = v13 + 60;
          }
          return 1;
        }
        else
        {
          bs->ltgtype = 2;
          if ( v11 == 0 )
            bs->teamgoal_time = AAS_Time() + 240;
          bs->formation_dist = 3.5 * 32;
          *(int *)&bs->arrive_time = 0;
          return 1;
        }
      }
      else
      {
        BotInitialChat(&bs->chatstate, "cannotfind", String2, (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        return 1;
      }
    case 5:
      if ( !TeamPlayIsOn() || !BotAddressedToBot(bs, &match) )
        return 1;
      BotMatchVariable(&match, 4, String2);
      if ( !BotGetMessageTeamGoal(bs, String2, &bs->teamgoal) )
      {
        BotInitialChat(&bs->chatstate, "cannotfind", String2, (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        return 1;
      }
      v14 = rand();
      v56 = (float)(v14 & 0x7FFF) * 0.000030518509f;
      v56 = v56 + v56;
      bs->teammessage_time = AAS_Time() + v56;
      bs->ltgtype = 3;
      v16 = BotGetTime(&match);
      bs->teamgoal_time = v16;
      if ( v16 == 0 )
        bs->teamgoal_time = AAS_Time() + 120;
      *(int *)&bs->defendaway_time = 0;
      return 1;
    case 19:
      if ( !TeamPlayIsOn() || !BotAddressedToBot(bs, &match) )
        return 1;
      BotMatchVariable(&match, 0, Destination);
      v18 = FindClientByName(Destination) + 1;
      if ( !v18 )
      {
        BotInitialChat(&bs->chatstate, "whois", Destination, (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        return 1;
      }
      BotMatchVariable(&match, 4, String2);
      if ( (match.subtype & 0x40) != 0 )
      {
        bs->teamgoal.entitynum = bs->entitynum;
        bs->teamgoal.areanum = bs->ms.areanum;
        VectorCopy(bs->origin, bs->teamgoal.origin);
        bs->teamgoal.mins[0] = -8.0f;
        bs->teamgoal.mins[1] = -8.0f;
        bs->teamgoal.mins[2] = -8.0f;
        bs->teamgoal.maxs[0] = 8.0f;
        bs->teamgoal.maxs[1] = 8.0f;
        bs->teamgoal.maxs[2] = 8.0f;
      }
      else if ( (match.subtype & 0x20) != 0 )
      {
        if ( v18 == bs->entitynum )
          return 1;
        bs->teamgoal.entitynum = 0;
        entinfo = AAS_EntityInfo(v18);
        if ( entinfo.valid )
        {
          v27 = AAS_PointAreaNum(entinfo.origin);
          if ( v27 )
          {
            if ( AAS_AreaReachability(v27) && BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0, v18) )
            {
              bs->teamgoal.origin[2] = entinfo.origin[2];
              bs->teamgoal.entitynum = v18;
              bs->teamgoal.mins[0] = -8.0f;
              bs->teamgoal.mins[1] = -8.0f;
              bs->teamgoal.mins[2] = -8.0f;
              bs->teamgoal.areanum = v27;
              bs->teamgoal.origin[0] = entinfo.origin[0];
              bs->teamgoal.origin[1] = entinfo.origin[1];
              bs->teamgoal.maxs[0] = 8.0f;
              bs->teamgoal.maxs[1] = 8.0f;
              bs->teamgoal.maxs[2] = 8.0f;
            }
          }
        }
        if ( !bs->teamgoal.entitynum )
        {
          BotInitialChat(&bs->chatstate, "whereareyou", Destination,
                         (char *)0);
          BotEnterChat(&bs->chatstate, bs->client, 1);
          return 1;
        }
      }
      else if ( !BotGetMessageTeamGoal(bs, String2, &bs->teamgoal) )
      {
        BotInitialChat(&bs->chatstate, "cannotfind", String2, (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        return 1;
      }
      v24 = rand();
      v57 = (float)(v24 & 0x7FFF) * 0.000030518509f;
      v57 = v57 + v57;
      bs->teammessage_time = AAS_Time() + v57;
      bs->ltgtype = 6;
      v26 = BotGetTime(&match);
      bs->teamgoal_time = v26;
      if ( v26 == 0 )
        bs->teamgoal_time = AAS_Time() + 300;
      bs->teammate = v18;
      *(int *)&bs->arrive_time = 0;
      return 1;
    case 21:
      if ( !TeamPlayIsOn() )
        return 1;
      if ( !BotAddressedToBot(bs, &match) )
        return 1;
      if ( !BotGetPatrolWaypoints(bs, &match) )
        return 1;
      v31 = rand();
      v58 = (float)(v31 & 0x7FFF) * 0.000030518509f;
      v58 = v58 + v58;
      bs->teammessage_time = AAS_Time() + v58;
      bs->ltgtype = 7;
      v33 = BotGetTime(&match);
      bs->teamgoal_time = v33;
      if ( v33 != 0 )
        return 1;
      v34 = AAS_Time();
      bs->teamgoal_time = v34 + 300;
      return 1;
    case 7:
      if ( ctf->value == 0.0f || !ctf_flag1.areanum || !ctf_flag2.areanum || !BotAddressedToBot(bs, &match) )
        return 1;
      v35 = rand();
      v59 = (float)(v35 & 0x7FFF) * 0.000030518509f;
      v59 = v59 + v59;
      bs->teammessage_time = AAS_Time() + v59;
      bs->ltgtype = 4;
      v37 = AAS_Time();
      bs->teamgoal_time = v37 + 180;
      return 1;
    case 6:
      if ( ctf->value == 0.0f || !ctf_flag1.areanum || !ctf_flag2.areanum || !BotAddressedToBot(bs, &match) )
        return 1;
      v38 = rand();
      v60 = (float)(v38 & 0x7FFF) * 0.000030518509f;
      v60 = v60 + v60;
      bs->teammessage_time = AAS_Time() + v60;
      bs->ltgtype = 5;
      v40 = AAS_Time();
      *(int *)&bs->rushbaseaway_time = 0;
      bs->teamgoal_time = v40 + 120;
      return 1;
    case 12:
      if ( !TeamPlayIsOn() || !BotAddressedToBot(bs, &match) )
        return 1;
      BotMatchVariable(&match, 3, Source);
      strncpy(bs->teamleader, Source, 0x20u);
      bs->teamleader[31] = 0;   /* ensure NUL-terminated */
      BotInitialChat(&bs->chatstate, "joinedteam", Source, (char *)0);
      BotEnterChat(&bs->chatstate, bs->client, 1);
      return 1;
    case 13:
      if ( !TeamPlayIsOn() || !BotAddressedToBot(bs, &match) )
        return 1;
      if ( strlen(bs->teamleader) )
        BotInitialChat(&bs->chatstate, "leftteam", bs->teamleader,
                       (char *)0);
      BotEnterChat(&bs->chatstate, bs->client, 1);
      strcpy(bs->teamleader, "");
      return 1;
    case 20:
      if ( !TeamPlayIsOn() )
        return 1;
      BotMatchVariable(&match, 4, Buffer);
      VectorClear(origin);
      sscanf(Buffer, "%f %f %f", &origin[0], &origin[1], &origin[2]);
      origin[2] = origin[2] + 0.5;
      v41 = AAS_PointAreaNum(origin);
      if ( !v41 )
      {
        if ( !BotAddressedToBot(bs, &match) )
          return 1;
        BotInitialChat(&bs->chatstate, "checkpoint_invalid", (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        return 1;
      }
      BotMatchVariable(&match, 5, Buffer);
      v42 = BotFindWayPoint(BotCheckpoints(bs), Buffer);
      if ( v42 )
      {
        v43 = v42->next;
        if ( v43 )
          v43->prev = v42->prev;
        v44 = v42->prev;
        if ( v44 )
          v44->next = v42->next;
        else
          BotCheckpoints(bs) = v42->next;
        FreeMemory(v42);
      }
      /* thunk 0x10001401 -> BotCreateWayPoint */
      v45 = BotCreateWayPoint(Buffer, origin, v41);
      v45->next = BotCheckpoints(bs);
      v46 = BotCheckpoints(bs);
      if ( v46 )
        v46->prev = v45;
      BotCheckpoints(bs) = v45;
      if ( BotAddressedToBot(bs, &match) )
      {
        sprintf(Buffer, "%1.0f %1.0f %1.0f", v45->goal.origin[0], v45->goal.origin[1], v45->goal.origin[2]);
        BotInitialChat(&bs->chatstate, "checkpoint_confirm", v45->name, Buffer,
                       (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        return 1;
      }
      return 1;
    case 14:
      EA_SayTeam(bs->client,
                 "the part of my brain to create formations has been damaged");
      return 1;
    case 15:
      EA_SayTeam(bs->client,
                 "the part of my brain to create formations has been damaged");
      return 1;
    case 16:
      if ( !TeamPlayIsOn() || !BotAddressedToBot(bs, &match) )
        return 1;
      BotMatchVariable(&match, 4, Buffer);
      if ( (match.subtype & 8) != 0 )
        v47 = atof(Buffer) * 9.7536;
      else
        v47 = atof(Buffer) * 32.0;
      if ( v47 < 48 || v47 > 500 )
        v47 = 100;
      bs->formation_dist = v47;
      return 1;
    case 18:
      if ( !TeamPlayIsOn() )
        return 1;
      if ( !BotAddressedToBot(bs, &match) )
        return 1;
      v48 = bs->ltgtype;
      if ( v48 != 2 && v48 != 1 )
        return 1;
      bs->ltgtype = 0;
      return 1;
    case 8:
      if ( !TeamPlayIsOn() )
        return 1;
      BotMatchVariable(&match, 3, Source);
      if ( (match.subtype & 0x80u) != 0 )
      {
        strncpy(bs->formation_teammate, Source, 0x10u);
        bs->formation_teammate[15] = 0;
        return 1;
      }
      v49 = FindClientByName(Source);
      if ( v49 < 0 )
        return 1;
      strcpy(bs->formation_teammate, (const char *)ClientName(v49));
      return 1;
    case 9:
      if ( !TeamPlayIsOn() )
        return 1;
      BotMatchVariable(&match, 3, Source);
      if ( (match.subtype & 0x80u) != 0 )
      {
        BotMatchVariable(&match, 0, Destination);
        v50 = FindClientByName(Destination);
      }
      else
      {
        v50 = FindClientByName(Source);
      }
      if ( v50 < 0 )
        return 1;
      if ( _strcmpi(bs->formation_teammate, (const char *)ClientName(v50)) )
        return 1;
      bs->formation_teammate[0] = 0;
      return 1;
    case 11:
      if ( !BotAddressedToBot(bs, &match) )
        return 1;
      switch ( bs->ltgtype )
      {
        case 1:
          BotMatchVariable(&match, 0, Destination);
          EasyClientName(bs->teammate - 1, Destination);
          BotInitialChat(&bs->chatstate, "helping", Destination, (char *)0);
          BotEnterChat(&bs->chatstate, bs->client, 1);
          return 1;
        case 2:
          BotMatchVariable(&match, 0, Destination);
          EasyClientName(bs->teammate - 1, Destination);
          BotInitialChat(&bs->chatstate, "accompanying", Destination,
                         (char *)0);
          BotEnterChat(&bs->chatstate, bs->client, 1);
          return 1;
        case 3:
          BotInitialChat(&bs->chatstate, "defending", BotGoalName(bs->teamgoal.number), (char *)0);
          BotEnterChat(&bs->chatstate, bs->client, 1);
          return 1;
        case 6:
          BotInitialChat(&bs->chatstate, "camping", (char *)0);
          break;
        case 7:
          BotInitialChat(&bs->chatstate, "patrolling", (char *)0);
          break;
        case 4:
          BotInitialChat(&bs->chatstate, "capturingflag", (char *)0);
          break;
        case 5:
          BotInitialChat(&bs->chatstate, "rushingbase", (char *)0);
          break;
        default:
          return 0;
      }
      BotEnterChat(&bs->chatstate, bs->client, 1);
      return 1;
    default:
      botimport.Print(PRT_MESSAGE, "unknown match type\n");
      return 1;
  }
}

#undef v54
#undef v55
#undef v56
#undef v57
#undef v58
#undef v59
#undef v60
// gladiator.dll: 10028650..100288E8
// gladi386.so:   00032D6C..000330B5
void __cdecl BotCheckConsoleMessages(bot_state_t *bs)
{
  char *botname, *ptr;
  float chat_reply;
  int context;
  bot_consolemessage_t *m;

  botname = ClientName(bs->client);
  for ( m = BotNextConsoleMessage(&bs->chatstate); m; m = BotNextConsoleMessage(&bs->chatstate) )
  {
    if ( BotNumConsoleMessages(&bs->chatstate) < 10 )
    {
      if ( m->type == 1 && m->time > AAS_Time() - (1 + random()) )
        return;
    }
    if ( m->type == 1 )
    {
      ptr = strstr(m->message, ":");
      if ( ptr )
      {
        if ( !strncmp(m->message, botname, ptr - m->message) || !strncmp(m->message + 1, botname, ptr - m->message - 2) )
        {
          BotRemoveConsoleMessage(&bs->chatstate, m);
          continue;
        }
      }
      else
      {
        BotRemoveConsoleMessage(&bs->chatstate, m);
        continue;
      }
    }
    UnifyWhiteSpaces(m->message);
    context = 3;
    if ( ctf->value )
      context = BotCTFTeam(bs) == 1 ? 7 : 11;
    BotReplaceSynonyms(m->message, context);
    if ( !BotMatchMessage(bs, m->message) )
    {
      if ( m->type == 1 && !nochat->value )
      {
        if ( BotAINode(bs) != AINode_Stand && BotValidChatPosition(bs) )
        {
          chat_reply = Characteristic_BFloat(BotCharacter(bs), 22, 0, 1);
          if ( random() < 1.5 / (NumBots() + 1) && random() < chat_reply )
          {
            ptr = strstr(m->message, ":");
            if ( ptr )
            {
              memmove(m->message, ptr + 1, strlen(ptr + 1) + 1);
              UnifyWhiteSpaces(m->message);
              if ( BotReplyChat(&bs->chatstate, m->message) )
              {
                BotRemoveConsoleMessage(&bs->chatstate, m);
                bs->stand_time = AAS_Time() + BotChatTime(bs);
                AIEnter_Stand(bs);
                return;
              }
            }
          }
        }
      }
    }
    BotRemoveConsoleMessage(&bs->chatstate, m);
  }
}

// gladiator.dll: 100289A0..10028A15
// gladi386.so:   000330B8..0003312C
void __cdecl sub_100289A0(bot_state_t *bs, float a2)
{
  bs->ltime += a2;
  bs->thinktime = a2;
  VectorCopy(bs->snapshot.origin, bs->origin);
  VectorAdd(bs->snapshot.origin, bs->snapshot.viewoffset, bs->eye);
  memcpy(bs->inventory, bs->snapshot.inventory, 0x400u);
}

// gladiator.dll: 10028A40..10028A56
// gladi386.so:   0003312C..00033150
int __cdecl sub_10028A40(bot_state_t *bs, float a2)
{
  return ((int (__cdecl *)(int, float))EA_EndRegular)(bs->client, a2);
}

// gladiator.dll: 10028A70..10028BCF
// gladi386.so:   00033150..0003341C
int BotDeathmatchAI(bot_state_t *bs, float thinktime)
{
  int i; // edi

  sub_100289A0(bs, thinktime);
  if ( mapchange && AAS_Initialized() )
    mapchange = 0;
  if ( bs->inuse_marker )
  {
    char *characteristic_string;

    characteristic_string = Characteristic_String(BotCharacter(bs), 3);
    EA_Command(bs->client, "gender", characteristic_string, (char *)0);
    if ( LibVarValue("altnames", (char *)"0") != 0.0f )
    {
      characteristic_string = Characteristic_String(BotCharacter(bs), 1);
      EA_Command(bs->client, "name", characteristic_string, (char *)0);
    }
    bs->inuse_marker = 0;
  }
  BotUpdateInventory(bs);
  BotCheckConsoleMessages(bs);
  if ( !BotAINode(bs) )
    AIEnter_Seek_LTG(bs);
  if ( AAS_Time() - 8.0f < bs->setup_time && BotChat_EnterGame(bs) )
  {
    bs->stand_time = AAS_Time() + BotChatTime(bs);
    AIEnter_Stand(bs);
  }
  BotResetNodeSwitches();
  for ( i = 0; i < 50; ++i )
  {
    if ( (BotAINode(bs))(bs) )
      break;
  }
  if ( i >= 50 )
  {
    BotDumpGoalStack(&bs->goalstate);
    BotDumpAvoidGoals(&bs->goalstate);
    BotDumpNodeSwitches(bs);
  }
  if ( *(_DWORD *)bs )
    return sub_10028A40(bs, thinktime);
}

// gladiator.dll: 10028C30..10028DF7
// gladi386.so:   0003341C..0003366C
void BotSetupDeathmatchAI()
{
  dmflags = LibVar("dmflags", (char *)"0");
  ctf = LibVar("ctf", (char *)"0");
  ch = LibVar("ch", (char *)"0");
  ra = LibVar("ra", (char *)"0");
  fastchat = LibVar("fastchat", (char *)"0");
  nochat = LibVar("nochat", (char *)"0");
  teamplay = LibVar("teamplay", (char *)"0");
  usehook = LibVar("usehook", (char *)"0");
  rocketjump = LibVar("rocketjump", (char *)"1");
  techs = LibVar("runes", (char *)"0");
  teamplay_shell = LibVar("teamplay_shell", (char *)"0");
  assimilation = LibVar("assimilation", (char *)"0");
  if ( ctf->value != 0.0f )
  {
    if ( BotGetLevelItemGoal(-1, "Red Flag", &ctf_flag1) < 0 )
      botimport.Print(PRT_WARNING, "CTF without Red Flag\n");
    if ( BotGetLevelItemGoal(-1, "Blue Flag", &ctf_flag2) < 0 )
      botimport.Print(PRT_WARNING, "CTF without Blue Flag\n");
    modelindex3_flag1 = AAS_IndexFromModel("players/male/flag1.md2");
    modelindex3_flag2 = AAS_IndexFromModel("players/male/flag2.md2");
    modelindex_tech1 = AAS_IndexFromModel("models/ctf/resistance/tris.md2");
    modelindex_tech2 = AAS_IndexFromModel("models/ctf/strength/tris.md2");
    modelindex_tech3 = AAS_IndexFromModel("models/ctf/haste/tris.md2");
    modelindex_tech4 = AAS_IndexFromModel("models/ctf/regeneration/tris.md2");
  }
  mapchange = 1;
}

// gladiator.dll: 10028E80..10028E81
// gladi386.so:   0003366C..0003366D
/* Empty in the original.  Q3 pairs BotShutdownDeathmatchAI right after
 * BotSetupDeathmatchAI in the same file, and the ELF oracle confirms the same
 * adjacency here (F814 ends exactly where the 1-byte F815 begins).  Defining it in
 * be_ai2_main.c lets gcc -O6 auto-inline the empty body away at its only call site;
 * keeping it here, where that TU cannot see the definition, forces the real
 * out-of-line call both binaries have. */
void BotShutdownDeathmatchAI(void) { /* empty body — original returns immediately */ }
