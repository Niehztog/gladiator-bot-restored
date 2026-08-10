/*
 * be_ai2_dmq2.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10020ED0..0x10028C30; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * botlib_local.h carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs and every prototype), so this file compiles in
 * exactly the environment these functions had before the split.
 */

#include "botlib_local.h"

libvar_t *libvar_ch; /* libvar handle */
libvar_t *libvar_teamplay; /* libvar handle */
libvar_t *libvar_ra; /* libvar handle */
int dword_1006446C; // weak
libvar_t *libvar_dmflags; /* libvar handle */
libvar_t *libvar_nochat; /* libvar handle */
libvar_t *libvar_fastchat; /* libvar handle */
libvar_t *libvar_assimilation; /* libvar handle */
int dword_10064484; // weak
libvar_t *libvar_teamplay_shell; /* libvar handle */
int dword_1006448C; // weak
int dword_10064490; // weak
int dword_10064494; // weak
int dword_10064498; // weak
int dword_1006449C; // weak

//----- (10020ED0) --------------------------------------------------------
_DWORD *__cdecl BotEntityInfo(bot_state_t *bs, _DWORD *info)
{

  _DWORD *result; // eax
  unsigned int v3; // edx
  unsigned int v4; // edx
  unsigned int v5; // edx

  result = info;
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
  return result;
}
//----- (10020FE0) --------------------------------------------------------
char *__cdecl sub_10020FE0(bot_state_t *bs, bot_weaponstate_t *ws)
{

  char *result; // eax

  ws->client = bs->client;
  ws->inventory = bs->inventory;
  result = AAS_ModelFromIndex(bs->snapshot.gunindex);
  ws->modelname = result;
  return result;
}
//----- (10021020) --------------------------------------------------------
void __cdecl BotUpdateInventory(bot_state_t *bs)
{
  __int16 v1; // ax
  char *v2; // edi
  int v3; // eax
  int v4; // eax
  int v5; // eax
  int v6; // eax
  __int16 v7; // ax
  char *v8; // eax
  int v9; // eax
  double v10; // [esp+Ch] [ebp-8h]

  bs->inventory_health = bs->snapshot.stats[1];
  v1 = bs->snapshot.stats[9];
  if ( v1 )
  {
    v2 = AAS_ImageFromIndex(v1);
    if ( !_strcmpi(v2, "p_quad") )
      bs->quad_endtime = AAS_Time() + (float)bs->snapshot.stats[10];
    else if ( !_strcmpi(v2, "p_invulnerability") )
      bs->invulnerability_endtime = AAS_Time() + (float)bs->snapshot.stats[10];
    else if ( !_strcmpi(v2, "p_rebreather") )
      bs->rebreather_endtime = AAS_Time() + (float)bs->snapshot.stats[10];
    else if ( !_strcmpi(v2, "p_envirosuit") )
      bs->enviro_endtime = AAS_Time() + (float)bs->snapshot.stats[10];
  }
  v3 = (int)(bs->quad_endtime - AAS_Time());
  bs->quad_seconds = v3;
  if ( v3 <= 0 )
    bs->quad_seconds = 0;
  v4 = (int)(bs->invulnerability_endtime - AAS_Time());
  bs->invuln_seconds = v4;
  if ( v4 <= 0 )
    bs->invuln_seconds = 0;
  v5 = (int)(bs->rebreather_endtime - AAS_Time());
  bs->rebreather_seconds = v5;
  if ( v5 <= 0 )
    bs->rebreather_seconds = 0;
  v6 = (int)(bs->enviro_endtime - AAS_Time());
  bs->enviro_seconds = v6;
  if ( v6 <= 0 )
    bs->enviro_seconds = 0;
  v7 = bs->snapshot.stats[4];
  if ( v7 )
  {
    v8 = AAS_ImageFromIndex(v7);
    if ( !_strcmpi(v8, "i_powershield") )
      bs->powerscreen_seen_time = AAS_Time();
    v10 = bs->powerscreen_seen_time;
    if ( AAS_Time() - 0.9 < v10 )
    {
      v9 = bs->inventory[20];  /* inventory[20] = power shield cells */
      bs->power_screen_active_cells = v9;
      bs->power_shield_active_cells = v9;
    }
    else
    {
      bs->power_screen_active_cells = 0;
      bs->power_shield_active_cells = 0;
    }
  }
}
//----- (10021290) --------------------------------------------------------
int __cdecl BotUpdateBattleInventory(bot_state_t *bs, int enemy)
{

  int v2; // eax
  int v3; // edx
  char v5; // dh
  int v8; // eax
  vec3_t dir; // [esp+8h] [ebp-88h] BYREF
  float entinfo[31]; // [esp+14h] [ebp-7Ch] BYREF

  *(aas_entityinfo_t *)entinfo = AAS_EntityInfo(enemy);
  dir[0] = entinfo[4] - bs->origin[0];
  dir[1] = entinfo[5] - bs->origin[1];
  bs->enemy_height = (int)(entinfo[6] - bs->origin[2]);
  dir[2] = 0.0;
  bs->enemy_horizontal_dist = (int)VectorLength(dir);
  v2 = (SLODWORD(entinfo[28]) >> 8) & 0xFF;
  bs->enemy_weapon_blaster = 0;
  bs->enemy_weapon_shotgun = 0;
  v3 = v2 - 1;
  bs->enemy_weapon_supershotgun = 0;
  bs->enemy_weapon_machinegun = 0;
  bs->enemy_weapon_chaingun = 0;
  bs->enemy_weapon_grenades = 0;
  bs->enemy_weapon_grenadelauncher = 0;
  bs->enemy_weapon_rocketlauncher = 0;
  bs->enemy_weapon_hyperblaster = 0;
  bs->enemy_weapon_railgun = 0;
  bs->enemy_weapon_bfg = 0;
  bs->enemy_weapon_phalanx = 0;
  switch ( v3 )
  {
    case 0:
      bs->enemy_weapon_blaster = 1;
      break;
    case 1:
      bs->enemy_weapon_shotgun = 1;
      break;
    case 2:
      bs->enemy_weapon_supershotgun = 1;
      break;
    case 3:
      bs->enemy_weapon_machinegun = 1;
      break;
    case 4:
      bs->enemy_weapon_chaingun = 1;
      break;
    case 5:
      bs->enemy_weapon_grenades = 1;
      break;
    case 6:
      bs->enemy_weapon_grenadelauncher = 1;
      break;
    case 7:
      bs->enemy_weapon_rocketlauncher = 1;
      break;
    case 8:
      bs->enemy_weapon_hyperblaster = 1;
      break;
    case 9:
      bs->enemy_weapon_railgun = 1;
      break;
    case 10:
      bs->enemy_weapon_bfg = 1;
      break;
    case 11:
      bs->enemy_weapon_phalanx = 1;
      break;
    default:
      break;
  }
  v8 = LODWORD(entinfo[29]);
  v5 = BYTE1(entinfo[29]);
  if ( (v8 & 0x10000) != 0 )
    bs->enemy_invulnerability = 1;
  else
    bs->enemy_invulnerability = 0;
  if ( (v5 & 0x80) != 0 )
    bs->enemy_quad = 1;
  else
    bs->enemy_quad = 0;
  if ( (v5 & 2) != 0 )
    bs->enemy_powerscreen = 1;
  else
    bs->enemy_powerscreen = 0;
  return 1;
}
//----- (100214E0) --------------------------------------------------------
/* Returns the signed 16-bit snapshot.stats[16].  DEAD in Gladiator, so no call
 * site confirms the parameter type; bot_state_t * matches both neighbours. */
int __cdecl sub_100214E0(bot_state_t *p)
{
  return p->snapshot.stats[16];
}
//----- (10021500) --------------------------------------------------------
void __cdecl BotBattleUseItems(bot_state_t *bs)
{
  if ( bs->inventory[25] > 0 )                     /* +1828 silencer ammo */
    EA_UseItem(bs->client, "Silencer");
  if ( (sub_10003080(bs->eye) & 0x38) != 0
       && !bs->rebreather_seconds
       && bs->inventory[26] > 0 )                  /* +1832 rebreather charges */
    EA_UseItem(bs->client, "Rebreather");
  if ( !bs->power_shield_active_cells && bs->inventory[6] > 0 )   /* +1752 */
    EA_UseItem(bs->client, "Power Shield");
  if ( !bs->power_screen_active_cells && bs->inventory[5] > 0 )   /* +1748 */
    EA_UseItem(bs->client, "Power Screen");
}
//----- (100215E0) --------------------------------------------------------
void __cdecl sub_100215E0(bot_state_t *bs)
{
  if ( !bs->quad_seconds && bs->inventory[23] > 0 )   /* +1820 quad ammo */
  {
    EA_UseItem(bs->client, "Quad Damage");
    return;
  }
  if ( !bs->invuln_seconds && bs->inventory[24] > 0 ) /* +1824 invuln ammo */
    EA_UseItem(bs->client, "Invulnerability");
}
//----- (10021650) --------------------------------------------------------
int __cdecl BotCTFCarryingFlag(bot_state_t *bs)
{
  if ( libvar_ctf->value == 0.0f )
    return 0;
  /* inventory[43]=RED FLAG, inventory[44]=BLUE FLAG (Q2 CTF item indices).
   * Returns 1 (red), 2 (blue), or 0 (not carrying). */
  if ( bs->inventory[43] > 0 )
    return 1;
  return bs->inventory[44] > 0 ? 2 : 0;
}
//----- (100216A0) --------------------------------------------------------
BOOL __cdecl BotIsDead(bot_state_t *bs)
{
  int v1; // eax

  v1 = bs->snapshot.pm_type;
  return v1 == 2 || v1 == 3;
}
//----- (100216D0) --------------------------------------------------------
BOOL __cdecl BotIsObserver(bot_state_t *bs)
{
  return bs->snapshot.pm_type == 1;
}
//----- (100216F0) --------------------------------------------------------
BOOL __cdecl BotIntermission(bot_state_t *bs)
{
  return bs->snapshot.pm_type == 4;
}
//----- (10021710) --------------------------------------------------------
BOOL __cdecl sub_10021710(int *a1)
{
  if ( (a1[29] & 0x4002) != 0 )
    return 1;
  if ( a1[3] < 1 || a1[3] > botstate.num_clients || a1[23] != 255 )
    return 1;
  if ( a1[27] < 173 || a1[27] > 197 )
    return 0;
  return 1;
}
//----- (10021780) --------------------------------------------------------
BOOL __cdecl EntityIsShooting(intptr_t a1)
{
  int v1; // eax
  aas_entityinfo_t *ent = (aas_entityinfo_t *)a1;

  if ( ent->modelindex != 255 )
    return 0;
  v1 = ent->frame;
  if ( v1 < 46 || v1 > 53 )
    return 0;
  return 1;
}
//----- (100217C0) --------------------------------------------------------
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
//----- (10021860) --------------------------------------------------------
char *__cdecl EasyClientName(int client, char *buf)
{
  int ci; // eax (strength-reduced walk-pointer over Str)
  char *i; // edx
  char *str1; // esi
  char *str2; // eax
  char v7; // al
  char *ptr; // esi
  const char *v9; // ebx
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
  v7 = Str[0];
  if ( (Str[0] == 109 || Str[0] == 77) && (Str[1] == 114 || Str[1] == 82) )
  {
    memmove(Str, &Str[2], strlen(&Str[2]) + 1);  /* was strcpy — UB on aarch64 */
    v7 = Str[0];
  }
  ptr = Str;
  if ( v7 )
  {
    v9 = &Str[1];
    do
    {
      c = *ptr;
      if ( *ptr >= 97 && c <= 122 || c >= 48 && c <= 57 || c == 95 )
      {
        ++ptr;
        ++v9;
      }
      else if ( c < 65 || c > 90 )
      {
        memmove(ptr, v9, strlen(v9) + 1);
      }
      else
      {
        *ptr++ = c + 32;
        ++v9;
      }
    }
    while ( *ptr );
  }
  strcpy(buf, Str);
  return buf;
}
//----- (10021A90) --------------------------------------------------------
bot_waypoint_t *__cdecl BotCreateWayPoint(const char *name, vec3_t origin, int areanum)
{
  /* The original allocates strlen(name)+1+68 with a hand-laid header; a real
   * struct here, with the name buffer still inline right after the node so one
   * FreeMemory releases both. */
  bot_waypoint_t *wp;
  size_t namelen = strlen(name);

  wp = (bot_waypoint_t *)GetMemory((unsigned)(sizeof(bot_waypoint_t) + namelen + 1));
  wp->name = (char *)(wp + 1);
  strcpy(wp->name, name);
  VectorCopy(origin, wp->goal.origin);
  wp->goal.areanum   = areanum;
  wp->goal.mins[0] = -8.0f;
  wp->goal.mins[1] = -8.0f;
  wp->goal.mins[2] = -8.0f;
  wp->goal.maxs[0] =  8.0f;
  wp->goal.maxs[1] =  8.0f;
  wp->goal.maxs[2] =  8.0f;
  wp->next = NULL;
  wp->prev = NULL;
  return wp;
}
//----- (10021B50) --------------------------------------------------------
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
//----- (10021B90) --------------------------------------------------------
void __cdecl BotFreeWaypoints(bot_waypoint_t *wp)
{
  bot_waypoint_t *next;

  while ( wp )
  {
    next = wp->next;
    FreeMemory(wp);
    wp = next;
  }
}
//----- (10021BC0) --------------------------------------------------------
BOOL __cdecl BotValidChatPosition(bot_state_t *bs)
{

  char v4; // al
  char v7; // al
  /* Real vec3_t, written with Q3's exact statements
   * (`VectorCopy(bs->origin, X); X[2] += K;`): MSVC6 forwards the [2] copy into
   * the arithmetic and interleaves the two surviving integer copies between the
   * fld and the fadd.  Copying start and end from the SAME source is what
   * produces the original's register reuse. */
  vec3_t point; // [esp+4h] [ebp-90h] BYREF
  vec3_t start; // [esp+10h] [ebp-84h] BYREF
  vec3_t end;   // [esp+1Ch] [ebp-78h] BYREF
  _DWORD maxs[3]; // [esp+28h] [ebp-6Ch] BYREF
  _DWORD mins[3]; // [esp+34h] [ebp-60h] BYREF
  int trace[21]; // [esp+40h] [ebp-54h] BYREF

  if ( BotIsDead(bs) )
    return 1;
  if ( (bs->snapshot.pm_flags & 4) == 0 )
    return 0;
  VectorCopy(bs->origin, point);
  point[2] = point[2] - 24.0f;
  v4 = (char)sub_10003080(point);
  if ( (v4 & 0x18) != 0 )           /* CONTENTS_LAVA(8) | CONTENTS_SLIME(16) */
    return 0;
  VectorCopy(bs->origin, point);
  point[2] = point[2] + 32.0f;
  v7 = (char)sub_10003080(point);
  if ( (v7 & 0x38) != 0 )           /* CONTENTS_LAVA(8) | SLIME(16) | WATER(32) */
    return 0;
  VectorCopy(bs->origin, start);
  VectorCopy(bs->origin, end);
  start[2] = start[2] + 1.0f;
  end[2] = end[2] - 100.0f;
  AAS_PresenceTypeBoundingBox(4, (float *)mins, (float *)maxs);
  *(bsp_trace_t *)trace = AAS_Trace(start, (float*)mins, (float*)maxs, end, 4, bs->client);
  return trace[20] == 0;
}
//----- (10021D80) --------------------------------------------------------
BOOL __cdecl BotChat_EnterGame(bot_state_t *bs)
{

  float v1; // st7
  BOOL result; // eax
  float rnd; // [esp+4h] [ebp-24h]
  char name[32]; // [esp+8h] [ebp-20h] BYREF

  v1 = libvar_nochat->value;
  if ( v1 != 0.0f )
    return 0;
  rnd = (float)Characteristic_BFloat(BotCharacter(bs), 18, 0.0, 1.0);
  if ( libvar_fastchat->value == 0.0f )
  {
    if ( (float)(rand() & 0x7FFF) * 0.000030518509f > rnd )
      return 0;
  }
  result = BotValidChatPosition(bs);
  if ( !result )
    return result;
  BotInitialChat(&bs->chatstate, "enter_game", EasyClientName(bs->client, name),
                 (char *)0);
  return 1;
}
//----- (10021E90) --------------------------------------------------------
int __cdecl BotChat_ExitGame(bot_state_t *bs)
{

  float rnd; // [esp+4h] [ebp-24h]
  char name[32]; // [esp+8h] [ebp-20h] BYREF

  if ( libvar_nochat->value != 0.0f )
    return 0;
  rnd = (float)Characteristic_BFloat(BotCharacter(bs), 18, 0.0, 1.0);
  if ( libvar_fastchat->value == 0.0f )
  {
    if ( (float)(rand() & 0x7FFF) * 0.000030518509f > rnd )
      return 0;
  }
  BotInitialChat(&bs->chatstate, "exit_game", EasyClientName(bs->client, name),
                 (char *)0);
  return 1;
}
//----- (10021F80) --------------------------------------------------------
int __cdecl BotChat_StartLevel(bot_state_t *bs)
{

  float rnd; // [esp+4h] [ebp-24h]
  char name[32]; // [esp+8h] [ebp-20h] BYREF

  if ( libvar_nochat->value != 0.0f )
    return 0;
  rnd = (float)Characteristic_BFloat(BotCharacter(bs), 17, 0.0, 1.0);
  if ( libvar_fastchat->value == 0.0f )
  {
    if ( (float)(rand() & 0x7FFF) * 0.000030518509f > rnd )
      return 0;
  }
  BotInitialChat(&bs->chatstate, "start_level",
                 EasyClientName(bs->client, name), (char *)0);
  return 1;
}
//----- (10022070) --------------------------------------------------------
int __cdecl BotChat_EndLevel(bot_state_t *bs)
{

  float rnd; // [esp+4h] [ebp-24h]
  char name[32]; // [esp+8h] [ebp-20h] BYREF

  if ( libvar_nochat->value != 0.0f )
    return 0;
  rnd = (float)Characteristic_BFloat(BotCharacter(bs), 17, 0.0, 1.0);
  if ( libvar_fastchat->value == 0.0f )
  {
    if ( (float)(rand() & 0x7FFF) * 0.000030518509f > rnd )
      return 0;
  }
  BotInitialChat(&bs->chatstate, "end_level", EasyClientName(bs->client, name),
                 (char *)0);
  return 1;
}
//----- (10022160) --------------------------------------------------------
int __cdecl BotChat_Death(int *bs)
{
  float v1; // st7
  int v4; // eax
  float v5; // st7
  float v6; // [esp+4h] [ebp-24h]
  _BYTE v7[32]; // [esp+8h] [ebp-20h] BYREF
  float v8; // [esp+2Ch] [ebp+4h]

  v1 = libvar_nochat->value;
  if ( v1 != 0.0f )
    return 0;
  v6 = (float)Characteristic_BFloat(BotCharacter((bot_state_t *)bs), 20, 0.0, 1.0);
  if ( libvar_fastchat->value == 0.0f )
  {
    if ( (float)(rand() & 0x7FFF) * 0.000030518509f > v6 )
      return 0;
  }
  v4 = bs[1049];
  if ( v4 )
    EasyClientName(v4 - 1, v7);
  else
    v7[0] = byte_1006294C;
  if ( bs[693] == 12 )
  {
    BotInitialChat(bs + 995, "death_bfg", v7, (char *)0);
  }
  else
  {
    v5 = (float)(rand() & 0x7FFF) * 0.000030518509f;
    /* Characteristic 15 is the praise-vs-insult probability. */
    v8 = (float)Characteristic_BFloat(BotCharacter((bot_state_t *)bs), 15, 0.0, 1.0);
    if ( v5 < v8 )
      BotInitialChat(bs + 995, "death_insult", v7, (char *)0);
    else
      BotInitialChat(bs + 995, "death_praise", v7, (char *)0);
  }
  return 1;
}
//----- (100222E0) --------------------------------------------------------
BOOL __cdecl BotChat_Kill(int *bs)
{
  float v1; // st7
  BOOL result; // eax
  int v4; // eax
  float v5; // st7
  float rnd; // [esp+4h] [ebp-24h]
  _BYTE name[32]; // [esp+8h] [ebp-20h] BYREF
  float v8; // [esp+2Ch] [ebp+4h]

  v1 = libvar_nochat->value;
  if ( v1 != 0.0f )
    return 0;
  rnd = (float)Characteristic_BFloat(BotCharacter((bot_state_t *)bs), 19, 0.0, 1.0);
  if ( libvar_fastchat->value == 0.0f )
  {
    if ( (float)(rand() & 0x7FFF) * 0.000030518509f > rnd )
      return 0;
  }
  result = BotValidChatPosition((bot_state_t *)bs);
  if ( !result )
    return result;
  v4 = bs[1049];
  if ( v4 )
    EasyClientName(v4 - 1, name);
  else
    name[0] = byte_1006294C;
  if ( bs[692] == 13 )
  {
    BotInitialChat(bs + 995, "kill_telefrag", name, (char *)0);
  }
  else
  {
    v5 = (float)(rand() & 0x7FFF) * 0.000030518509f;
    /* Characteristic 15 is the praise-vs-insult probability. */
    v8 = (float)Characteristic_BFloat(BotCharacter((bot_state_t *)bs), 15, 0.0, 1.0);
    if ( v5 < v8 )
      BotInitialChat(bs + 995, "kill_insult", name, (char *)0);
    else
      BotInitialChat(bs + 995, "kill_praise", name, (char *)0);
  }
  return 1;
}
//----- (10022470) --------------------------------------------------------
int __cdecl BotChat_Random(bot_state_t *bs)
{

  float v1; // st7
  float rnd; // [esp+4h] [ebp-4h]

  v1 = libvar_nochat->value;
  if ( v1 != 0.0f )
    return 0;
  if ( bs->ltgtype == 1 || bs->ltgtype == 2 || bs->ltgtype == 5 )
    return 0;
  /* Characteristic 21 is the random-chat probability. */
  rnd = (float)Characteristic_BFloat(BotCharacter(bs), 21, 0.0f, 1.0f);
  if ( (float)(rand() & 0x7FFF) * 0.000030518509f > bs->thinktime * 0.1 )
    return 0;
  if ( libvar_fastchat->value == 0.0f )
  {
    if ( (float)(rand() & 0x7FFF) * 0.000030518509f > rnd || (float)(rand() & 0x7FFF) * 0.000030518509f > 0.25 )
      return 0;
  }
  if ( !BotValidChatPosition(bs) )
    return 0;
  if ( (float)(rand() & 0x7FFF) * 0.000030518509f < Characteristic_BFloat(BotCharacter(bs), 16, 0.0f, 1.0f) )
  {
    BotInitialChat(&bs->chatstate, "random_misc", (char *)0);
    return 1;
  }
  BotInitialChat(&bs->chatstate, "random_insult", (char *)0);
  return 1;
}
//----- (10022650) --------------------------------------------------------
double __cdecl BotChatTime(bot_state_t *bs)
{

  int cpm; // [esp+4h] [ebp-4h]

  cpm = Characteristic_BInteger(BotCharacter(bs), 14, 1, 4000);
  return (int)BotChatLength(&bs->chatstate) * 30.0f / cpm;
}
//----- (100226C0) --------------------------------------------------------
float __cdecl BotAggression(bot_state_t *bs)
{
  int v2; // ecx

  if ( bs->invuln_seconds )
    return 100.0f;
  if ( bs->enemy_invulnerability || bs->enemy_quad && !bs->quad_seconds )
    return 0.0f;
  if ( bs->enemy_powerscreen && (!bs->power_screen_active_cells || ((int *)bs)[452] < 50) )
    return 0.0f;
  if ( bs->enemy_height > 200 )
    return 0.0f;
  v2 = bs->inventory_health;
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
  if ( ((int *)bs)[441] <= 0 || ((int *)bs)[450] <= 20 )
    return 0.0f;
  else
    return 100.0f;
}
//----- (100228C0) --------------------------------------------------------
BOOL __cdecl BotWantsToRetreat(int *bs)
{
  if ( BotCTFCarryingFlag(bs) )
    return 1;
  if ( bs[1065] == 4 )
    return 1;
  return BotAggression((bot_state_t *)bs) < 50.0f;
}
//----- (10022930) --------------------------------------------------------
BOOL __cdecl BotWantsToChase(int *bs)
{
  return BotAggression((bot_state_t *)bs) > 50.0f;
}
//----- (10022970) --------------------------------------------------------
/* Always-true predicate between BotWantsToChase and
 * BotCanAndWantsToRocketJump — probably a feature toggle that ended up
 * hard-coded.  DEAD in Gladiator. */
int __cdecl BotWantsToHelp(bot_state_t *bs)
{
  return 1;
}
//----- (10022990) --------------------------------------------------------
BOOL BotCanAndWantsToRocketJump(bot_state_t *bs)
{
  int v3;

  if ( ((int *)bs)[446] <= 0 )
    return 0;
  if ( ((int *)bs)[453] < 3 )
    return 0;
  if ( bs->quad_seconds )
    return 0;
  if ( !bs->invuln_seconds )
  {
    v3 = bs->inventory_health;
    if ( v3 < 60 )
      return 0;
    if ( v3 < 90 && ((int *)bs)[433] < 40 && ((int *)bs)[434] < 50 && ((int *)bs)[435] < 60 )
      return 0;
    if ( Characteristic_BFloat(BotCharacter(bs), 26, 0.0, 1.0) < 0.5 )
      return 0;
  }
  return 1;
}
//----- (10022A60) --------------------------------------------------------
float *__cdecl BotRoamGoal(bot_state_t *bs, float *goal)
{
  int *v2; // ebp
  int v6; // ax
  int v7; // ax
  float len; // st7
  char pc; // al
  float *result; // eax
  int v17; // [esp-Ch] [ebp-154h]
  int v18; // [esp+0h] [ebp-148h]
  float v19; // [esp+0h] [ebp-148h]
  int v20; // [esp+18h] [ebp-130h]
  vec3_t endpos; // [esp+1Ch] [ebp-12Ch] BYREF — randomized trace endpoint, refined to predicted path destination
  float v24; // [esp+28h] [ebp-120h]
  float rnd; // [esp+2Ch] [ebp-11Ch]
  float i; // [esp+30h] [ebp-118h]
  vec3_t dir; // [esp+34h] [ebp-114h] BYREF — direction vector (endpos - origin) for VectorNormalize/Scale
  vec3_t belowbestorg; // [esp+40h] [ebp-108h] BYREF
  int trace[21]; // [esp+4Ch] [ebp-FCh] BYREF

  i = 0.0;
  v2 = (int *)bs->origin;
  do
  {
    *(_DWORD *)&endpos[0] = *v2;
    *(_DWORD *)&endpos[1] = *(int *)&bs->origin[1];
    *(_DWORD *)&endpos[2] = *(int *)&bs->origin[2];
    rnd = (float)(rand() & 0x7FFF) * 0.000030518509f;
    if ( rnd < 0.8 )
    {
      v6 = rand();
      v24 = -1.0;
      if ( (float)(v6 & 0x7FFF) * 0.000030518509f >= 0.5 )
        v24 = 1.0;
      v19 = (float)(rand() & 0x7FFF) * 0.000030518509f;
      endpos[0] = v19 * v24 * 700.0f + endpos[0] + 50.0f;
    }
    if ( rnd > 0.2 )
    {
      v7 = rand();
      v24 = -1.0;
      if ( (float)(v7 & 0x7FFF) * 0.000030518509f >= 0.5 )
        v24 = 1.0;
      v19 = (float)(rand() & 0x7FFF) * 0.000030518509f;
      endpos[1] = v19 * v24 * 700.0f + endpos[1] + 50.0f;
    }
    v20 = rand() & 0x7FFF;
    v18 = bs->entitynum;
    v19 = (float)v20 * 0.000030518509f;
    endpos[2] = v19 * 144.0f - 96.0f - 1.0f + endpos[2];
    *(bsp_trace_t *)trace = AAS_Trace((float*)(v2), (float*)(uintptr_t)(0), (float*)(uintptr_t)(0), (float*)(endpos), v18, 3);
    VectorSubtract(endpos, bs->origin, dir);
    len = VectorNormalize(dir);
    if ( len > 100.0f )
    {
      v19 = *(float *)&trace[2] * len - 40.0f;
      VectorScale((float *)dir, v19, (float *)dir);
      v17 = bs->entitynum;
      endpos[0] = dir[0] + *(float *)v2;
      endpos[1] = dir[1] + bs->origin[1];
      endpos[2] = dir[2] + bs->origin[2];
      VectorCopy(endpos, belowbestorg);
      belowbestorg[2] = belowbestorg[2] - 800.0f;
      *(bsp_trace_t *)trace = AAS_Trace((float*)(endpos), (float*)(uintptr_t)(0), (float*)(uintptr_t)(0), (float*)(belowbestorg), v17, 3);
      if ( !trace[1] )
      {
        *(float *)&trace[5] = *(float *)&trace[5] + 1.0f;
        pc = sub_10003080((float *)&trace[3]);   /* trace-endpoint lava/slime check */
        if ( (pc & 0x18) == 0 )
          break;
      }
    }
    i = i + 1.0f;
  }
  while ( i < 10.0f );
  result = goal;
  *goal = endpos[0];
  goal[1] = endpos[1];
  goal[2] = endpos[2];
  return result;
}
//----- (10022E10) --------------------------------------------------------
bot_moveresult_t __cdecl BotAttackMove(bot_state_t *bs, int a3)
{
  float v10; // st7
  float jitter;
  int movetype; // edi
  float strafechange_time; // st7
  int v16; // eax
  int i; // esi
  float v18; // st7
  int v19; // edx
  float jumper; // [esp+10h] [ebp-134h]
  /* vec3_t: passed by address to CrossProduct / VectorNormalize /
   * VectorLength / BotMoveInDirection, all of which read three floats. */
  vec3_t sideward; // [esp+14h] [ebp-130h] BYREF
  float croucher; // [esp+20h] [ebp-124h]
  float dist; // [esp+24h] [ebp-120h]
  float attack_skill; // [esp+28h] [ebp-11Ch]
  vec3_t forward; // [esp+2Ch] [ebp-118h] BYREF
  vec3_t backward; // [esp+3Ch] [ebp-108h] BYREF
  vec3_t hordir; // [esp+48h] [ebp-FCh] BYREF
  vec3_t up = { 0, 0, 1.0f }; // [esp+54h] [ebp-F0h] BYREF
  bot_goal_t goal; // [esp+60h] [ebp-E4h] BYREF (was float[14]; the in-line chase goal)
  bot_moveresult_t moveresult; // [esp+98h] [ebp-ACh] BYREF (was int[12]; the move-result output buffer)
  float entinfo[31]; // [esp+C8h] [ebp-7Ch] BYREF

  if ( AAS_Time() < bs->attackchase_time )
  {
    goal.entitynum = bs->enemy;
    goal.areanum = bs->lastenemyareanum;
    VectorCopy(bs->lastenemyorigin, goal.origin);
    goal.mins[0] = -8.0f;
    goal.mins[1] = -8.0f;
    goal.mins[2] = -8.0f;
    goal.maxs[0] = 8.0f;
    goal.maxs[1] = 8.0f;
    goal.maxs[2] = 8.0f;
    BotEntityInfo(bs, (_DWORD *)bs->movestate);
    return *BotMoveToGoal(&moveresult, (bot_movestate_t *)bs->movestate, &goal, a3);
  }
  memset(&moveresult, 0, sizeof(moveresult));
  v10 = (float)(rand() & 0x7FFF) * 0.000030518509f;
  /* Each Characteristic_BFloat result must be captured from the FPU return, as
   * the four `fstp [esp+…]` stores in the original do — otherwise every
   * probability gate below degenerates to `random <= random`. */
  if ( Characteristic_BFloat(BotCharacter(bs), 48, 0.0f, 1.0f) <= v10 )
  {
    attack_skill = Characteristic_BFloat(BotCharacter(bs), 4, 0.0f, 1.0f);
    jumper = Characteristic_BFloat(BotCharacter(bs), 25, 0.0f, 1.0f);
    croucher = Characteristic_BFloat(BotCharacter(bs), 24, 0.0f, 1.0f);
    if ( attack_skill >= 0.2 )
    {
      BotEntityInfo(bs, (_DWORD *)bs->movestate);
      *(aas_entityinfo_t *)entinfo = AAS_EntityInfo(bs->enemy);
      forward[0] = entinfo[4] - bs->origin[0];
      forward[1] = entinfo[5] - bs->origin[1];
      forward[2] = entinfo[6] - bs->origin[2];
      dist = VectorLength(forward);
      VectorNormalize(forward);
      backward[0] = -forward[0];
      backward[1] = -forward[1];
      backward[2] = -forward[2];
      movetype = 1;
      if ( AAS_Time() - 1.0f > bs->attackcrouch_time )
      {
        if ( (float)(rand() & 0x7FFF) * 0.000030518509f < jumper )
        {
          movetype = 4;
        }
        else if ( AAS_Time() - 1.0f > bs->attackcrouch_time && (float)(rand() & 0x7FFF) * 0.000030518509f < croucher )
        {
          bs->attackcrouch_time = AAS_Time() + croucher * 5.0f;
        }
      }
      if ( AAS_Time() < bs->attackcrouch_time )
      {
        movetype = 2;
      }
      else
      {
        if ( movetype == 4 )
        {
          if ( (bs->flags & 4) != 0 )
          {
            bs->flags &= 0xFFFFFFFB;
            movetype = 1;
          }
          else
          {
            bs->flags |= 4;
          }
        }
      }
      if ( attack_skill <= 0.4 )
      {
        if ( (dist <= 180.0f || !BotMoveInDirection((bot_movestate_t *)bs->movestate, forward, 400.0f, movetype)) && dist < 100.0f )
          BotMoveInDirection((bot_movestate_t *)bs->movestate, backward, 400.0f, movetype);
        return moveresult;
      }
      bs->attackstrafe_drift = bs->attackstrafe_drift + 0.1;
      strafechange_time = (1.0f - attack_skill) * 0.2 + 0.4;
      if ( attack_skill > 0.7 )
      {
        jitter = (float)(rand() & 0x7FFF) * 0.000030518509f - 0.5;
        strafechange_time += (jitter + jitter) * 0.1;
      }
      if ( strafechange_time < bs->attackstrafe_drift && (float)(rand() & 0x7FFF) * 0.000030518509f > 0.935 )
      {
        v16 = bs->flags;
        *(_DWORD *)&bs->attackstrafe_drift = 0;
        bs->flags = v16 ^ 1;
      }
      i = 0;
      while ( 1 )
      {
        hordir[0] = forward[0];
        hordir[1] = forward[1];
        hordir[2] = 0;
        VectorNormalize(hordir);
        CrossProduct(hordir, up, sideward);
        if ( (*(unsigned char *)&bs->flags & 1) != 0 )
        {
          sideward[0] = -sideward[0];
          sideward[1] = -sideward[1];
          sideward[2] = -sideward[2];
        }
        if ( (float)(rand() & 0x7FFF) * 0.000030518509f > 0.9 )
          goto LABEL_35;
        if ( dist <= 180.0f )
          break;
        sideward[0] = sideward[0] + forward[0];
        sideward[1] = sideward[1] + forward[1];
        v18 = sideward[2] + forward[2];
LABEL_36:
        sideward[2] = v18;
LABEL_37:
        if ( !BotMoveInDirection((bot_movestate_t *)bs->movestate, sideward, 400.0f, movetype) )
        {
          v19 = bs->flags;
          *(_DWORD *)&bs->attackstrafe_drift = 0;
          bs->flags = v19 ^ 1;
          ++i;
          if ( i < 2 )
            continue;
        }
        return moveresult;
      }
      if ( dist >= 100.0f )
        goto LABEL_37;
LABEL_35:
      sideward[0] = sideward[0] + backward[0];
      sideward[1] = sideward[1] + backward[1];
      v18 = sideward[2] + backward[2];
      goto LABEL_36;
    }
  }
  return moveresult;
}
//----- (10023510) --------------------------------------------------------
int __cdecl BotCTFTeam(bot_state_t *bs)
{
  const char *v1; // eax

  v1 = (const char *)ClientSkin(bs->client);
  if ( strstr(v1, "ctf_r") )
    return 1;
  return 2;
}
//----- (10023550) --------------------------------------------------------
BOOL __cdecl BotSameTeam(bot_state_t *bs, int entnum)
{
  int v2; // ebx
  int v5; // ecx
  char *v7; // esi
  int v8; // ebx
  char *v10; // esi
  unsigned int v11; // ecx
  char *v14; // edi
  char *v16; // eax
  unsigned int MaxCount; // [esp+10h] [ebp-FCh]
  int v20[31]; // [esp+14h] [ebp-F8h] BYREF
  int v21[31]; // [esp+90h] [ebp-7Ch] BYREF

  *(aas_entityinfo_t *)v21 = AAS_EntityInfo(entnum);
  v2 = v21[3];
  if ( v21[3] )
  {
    if ( libvar_teamplay_shell->value != 0.0f )
    {
      *(aas_entityinfo_t *)v20 = AAS_EntityInfo(*(_DWORD *)((char *)bs + 8));
      return ((LOWORD(v20[30]) ^ LOWORD(v21[30])) & 0x1C00) == 0;
    }
    if ( libvar_ch->value != 0.0f )
    {
      *(aas_entityinfo_t *)v20 = AAS_EntityInfo(*(_DWORD *)((char *)bs + 8));
      if ( v20[25] != v21[25] )
        return 1;
    }
    else
    {
      if ( libvar_teamplay->value != 0.0f )
      {
        return _strcmpi((const char *)ClientSkin(bs->client),
                        (const char *)ClientSkin(v21[3] - 1)) == 0;
      }
      v5 = (__int64)libvar_dmflags->value;
      if ( (v5 & 0x40) != 0 || libvar_ctf->value != 0.0f )
      {
        v14 = strchr((const char *)ClientSkin(bs->client), 47);
        if ( !v14 )
          v14 = ClientSkin(bs->client);
        v16 = strchr((const char *)ClientSkin(v2 - 1), 47);
        if ( !v16 )
          v16 = ClientSkin(v2 - 1);
        if ( !_strcmpi(v14, v16) )
          return 1;
      }
      else if ( (v5 & 0x80) != 0 )
      {
        v7 = strchr((const char *)ClientSkin(bs->client), 47);
        MaxCount = v7 ? (unsigned int)(v7 - (const char *)ClientSkin(bs->client)) : strlen((const char *)ClientSkin(bs->client));
        v8 = v2 - 1;
        v10 = strchr((const char *)ClientSkin(v8), 47);
        v11 = v10 ? (unsigned int)(v10 - (const char *)ClientSkin(v8)) : strlen((const char *)ClientSkin(v8));
        if ( MaxCount == v11 )
        {
          if ( !strncmp((const char *)ClientSkin(bs->client), (const char *)ClientSkin(v8), MaxCount) )
            return 1;
        }
      }
    }
  }
  return 0;
}
//----- (100238F0) --------------------------------------------------------
int __cdecl BotNumTeamMates(bot_state_t *bs)
{
  int numplayers; // ebp
  int i; // esi

  numplayers = 0;
  for ( i = 0; i < botstate.num_clients; i++ )
  {
    if ( strlen(clientsettings[i].netname) )
    {
      if ( BotSameTeam(bs, i + 1) )
        ++numplayers;
    }
  }
  return numplayers;
}
//----- (10023970) --------------------------------------------------------
int __cdecl BotFindEnemy(bot_state_t *bs)
{

  int v1; // eax
  int v2; // edi
  int v3; // eax
  BOOL v5; // esi
  float v6; // st7
  float v8; // [esp+10h] [ebp-168h]
  float v9; // [esp+14h] [ebp-164h]
  int v10; // [esp+18h] [ebp-160h]
  vec3_t dir; // [ebp-15Ch] BYREF — one vec3; all three components are stored
  int v14; // [esp+28h] [ebp-150h]
  int v15; // [esp+2Ch] [ebp-14Ch]
  BOOL healthdecrease; // [esp+30h] [ebp-148h]
  vec3_t angles; // [esp+34h] [ebp-144h] BYREF
  int entinfo[31]; // [esp+40h] [ebp-138h] BYREF
  int v19[16]; // [esp+BCh] [ebp-BCh] BYREF

  v1 = Characteristic_BInteger(BotCharacter(bs), 45, 0, 1);
  v2 = bs->lasthealth;
  v15 = v1;
  v3 = bs->inventory_health;
  healthdecrease = v2 > v3;
  bs->lasthealth = v3;
  v14 = sub_1000BAA0(bs->entitynum, bs->eye, bs->viewangles, 360.0f, 16, v19);
  for ( v10 = 0; v10 < v14; ++v10 )
  {
    *(aas_entityinfo_t *)entinfo = AAS_EntityInfo(v19[v10]);
    if ( !sub_10021710(entinfo) && entinfo[3] != bs->entitynum )
    {
      dir[0] = *(float *)&entinfo[4] - bs->origin[0];
      dir[1] = *(float *)&entinfo[5] - bs->origin[1];
      dir[2] = *(float *)&entinfo[6] - bs->origin[2];
      v8 = VectorLength(dir);
      if ( v15 || v8 <= 900.0f )
      {
        v5 = healthdecrease;
        if ( healthdecrease )
        {
          v9 = 360.0f;
        }
        else
        {
          v6 = v8 > 810.0f ? 810.0f : v8;
          v9 = 360.0f - (270.0f - v6 * 0.33333334f);
        }
        vectoangles(dir, (float *)angles);
        if ( InFieldOfVision(bs->viewangles, v9, angles) && !BotSameTeam(bs, v19[v10]) )
        {
          if ( v5 && v8 <= 300.0f )
            goto found;
          if ( AAS_PointLight((float *)&entinfo[4], 0, 0, 0) >= 5 )
          {
            if ( v8 <= 300.0f )
              goto found;
            if ( EntityIsShooting((intptr_t)entinfo) )
              goto found;
            dir[0] = bs->origin[0] - *(float *)&entinfo[4];
            dir[1] = bs->origin[1] - *(float *)&entinfo[5];
            dir[2] = bs->origin[2] - *(float *)&entinfo[6];
            vectoangles(dir, (float *)angles);
            if ( InFieldOfVision((float *)&entinfo[7], 160.0f, angles) )
              goto found;
            BotUpdateBattleInventory(bs, v19[v10]);
            if ( !BotWantsToRetreat((int *)bs) )
              goto found;
          }
        }
      }
    }
    continue;
found:
    bs->enemy = entinfo[3];
    bs->enemysight_time = AAS_Time();
    return 1;
  }
  return 0;
}
//----- (10023CE0) --------------------------------------------------------
void BotAimAtEnemy(bot_state_t *bs)
{

  int v2; // eax
  weaponinfo_t *wi; // ebp
  float speed; // st — Q3 'speed'; register-only, never stored
  float v12; // st7
  int v13; // ax
  int v14; // ax
  int v15; // ax
  int i; // edi
  int v18; // ax
  int v19; // ax
  float dist; // [esp+1Ch] [ebp-1ACh]
  float aim_skill; // [esp+20h] [ebp-1A8h]
  float v28; // [esp+20h] [ebp-1A8h]
  vec3_t dir; // [ebp-1A4h] BYREF — one vec3; all three components are stored
  vec3_t bestorigin; // [esp+30h] [ebp-198h] BYREF
  float aim_accuracy; // [esp+3Ch] [ebp-18Ch]
  vec3_t groundtarget; // [esp+40h] [ebp-188h] BYREF
  vec3_t start; // [esp+4Ch] [ebp-17Ch] BYREF
  vec3_t end; // [esp+58h] [ebp-170h] BYREF
  vec3_t mins; // [esp+64h] [ebp-164h] BYREF
  vec3_t maxs; // [esp+70h] [ebp-158h] BYREF
  int trace[21]; // [esp+7Ch] [ebp-14Ch] BYREF
  float entinfo[31]; // [esp+D0h] [ebp-F8h] BYREF

  /* Float literals: mins/maxs are float[3], so the original's raw ±4.0f bit
   * patterns would be converted, not reinterpreted. */
  mins[0] = -4.0f;
  v2 = bs->enemy;
  mins[1] = -4.0f;
  mins[2] = -4.0f;
  maxs[0] = 4.0f;
  maxs[1] = 4.0f;
  maxs[2] = 4.0f;
  if ( v2 )
  {
    /* Both Characteristic_BFloat results come back on the FPU stack; the second is
     * compared against 0.0 and clamped to 0.000099999997f when not positive. */
    aim_skill = (float)Characteristic_BFloat(BotCharacter(bs), 7, 0.0, 1.0);
    aim_accuracy = (float)Characteristic_BFloat(BotCharacter(bs), 8, 0.0, 1.0);
    if ( aim_accuracy <= 0.0f )
      aim_accuracy = 0.000099999997f;
    wi = sub_100354B0(BotWS(bs));
    if ( !_strcmpi(wi->name, "Rocket Launcher") )
      aim_accuracy = sqrt(aim_accuracy);
    *(aas_entityinfo_t *)entinfo = AAS_EntityInfo(bs->enemy);
    bestorigin[0] = entinfo[4];
    bestorigin[1] = entinfo[5];
    start[0] = bs->origin[0];
    start[1] = bs->origin[1];
    bestorigin[2] = entinfo[6];
    start[2] = bs->origin[2];
    bestorigin[2] += 8.0f;
    start[2] += bs->snapshot.viewoffset[2];
    start[2] += wi->offset[2];
    *(bsp_trace_t *)trace = AAS_Trace(start, (float*)mins, (float*)maxs, (float*)(bestorigin), bs->entitynum, 100663299);
    if ( *(float *)&trace[2] <= 1.0f && trace[20] != LODWORD(entinfo[3]) )
      bestorigin[2] += 16.0f;
    if ( wi->speed != 0.0f && aim_skill > 0.4 )
    {
      /* All three components are stored (three fld/fsub/fstp triples). */
      dir[0] = entinfo[4] - bs->origin[0];
      dir[1] = entinfo[5] - bs->origin[1];
      dir[2] = entinfo[6] - bs->origin[2];
      dist = VectorLength(dir);
      dir[2] = 0.0f;
      dir[0] = entinfo[4] - entinfo[13];
      dir[1] = entinfo[5] - entinfo[14];
      speed = VectorNormalize(dir) / entinfo[2];
      VectorMA((&entinfo[4]), (dist / wi->speed) * speed, dir, bestorigin);
    }
    if ( aim_skill > 0.6 && (wi->proj->damagetype & 2) != 0 && bs->origin[2] + 16.0f > entinfo[6] )
    {
      end[0] = entinfo[4];
      end[1] = entinfo[5];
      end[2] = entinfo[6];
      end[2] -= 64.0f;
      *(bsp_trace_t *)trace = AAS_Trace((&entinfo[4]), (float*)(uintptr_t)(0), (float*)(uintptr_t)(0), (float*)(end), SLODWORD(entinfo[3]), 100663299);
      VectorCopy(bestorigin, groundtarget);
      groundtarget[2] = trace[1] ? entinfo[6] - 16.0f : *(float *)&trace[5] - 8.0f;
      *(bsp_trace_t *)trace = AAS_Trace(start, (float*)(uintptr_t)(0), (float*)(uintptr_t)(0), (float*)(groundtarget), bs->entitynum, 100663299);
      v12 = *(float *)&trace[5] - groundtarget[2];
      if ( fabs(v12) < 50.0 )
      {
        /* All three components are stored. */
        dir[0] = *(float *)&trace[3] - groundtarget[0];
        dir[1] = *(float *)&trace[4] - groundtarget[1];
        dir[2] = v12;
        if ( VectorLength(dir) < 60.0f )
        {
          /* All three components are stored. */
          dir[0] = *(float *)&trace[3] - start[0];
          dir[1] = *(float *)&trace[4] - start[1];
          dir[2] = *(float *)&trace[5] - start[2];
          if ( VectorLength(dir) > 150.0f )
          {
            *(bsp_trace_t *)trace = AAS_Trace((float*)(&trace[3]), (float*)(uintptr_t)(0), (float*)(uintptr_t)(0), (&entinfo[4]), SLODWORD(entinfo[3]), 100663299);
            if ( *(float *)&trace[2] >= 1.0f )
            {
              VectorCopy(groundtarget, bestorigin);
            }
          }
        }
      }
    }
    v28 = 1.0f - aim_accuracy;
    v13 = rand();
    bestorigin[0] = (2 * ((float)(v13 & 0x7FFF) * 0.000030518509f - 0.5))
                   * v28
                   * 20.0
                   + bestorigin[0];
    v14 = rand();
    bestorigin[1] = (2 * ((float)(v14 & 0x7FFF) * 0.000030518509f - 0.5)) * v28 * 20.0
        + bestorigin[1];
    v15 = rand();
    bestorigin[2] = (2 * ((float)(v15 & 0x7FFF) * 0.000030518509f - 0.5)) * v28 * 10.0
        + bestorigin[2];
    /* All three components are stored. */
    VectorSubtract(bestorigin, bs->eye, dir);
    if ( !_strcmpi(wi->name, "Railgun") )
    {
      VectorNormalize(dir);
      for ( i = 0; i < 3; i++ )
        dir[i] += (2 * ((float)(rand() & 0x7FFF) * 0.000030518509f - 0.5)) * v28 * 0.3;
    }
    vectoangles(dir, bs->ideal_viewangles);
    v18 = rand();
    bs->ideal_viewangles[0] += (2 * ((float)(v18 & 0x7FFF) * 0.000030518509f - 0.5))
        * (wi->vspread
         * 6.0f)
        * v28;
    bs->ideal_viewangles[0] = anglemod(bs->ideal_viewangles[0]);
    v19 = rand();
    bs->ideal_viewangles[1] += (2 * ((float)(v19 & 0x7FFF) * 0.000030518509f - 0.5))
        * (wi->hspread
         * 6.0f)
        * v28;
    bs->ideal_viewangles[1] = anglemod(bs->ideal_viewangles[1]);
    BotChangeViewAngles(bs, bs->thinktime);
    if ( aim_accuracy > 0.8 )
    {
      VectorCopy(bs->ideal_viewangles, bs->viewangles);
      EA_View(bs->client, bs->viewangles);
    }
  }
}
//----- (10024590) --------------------------------------------------------
void BotCheckAttack(bot_state_t *bs)
{

  int attackentity; // eax
  weaponinfo_t *wi; // ebx
  projectileinfo_t *v6; // ecx
  float points; // st — register-only (Q3 ai_dmq3 BotCheckAttack 'points')
  float v11; // [esp+10h] [ebp-1A4h] — one local, reused
  vec3_t start; // [esp+14h] [ebp-1A0h] BYREF — trace start; as separate locals the
                // y/z stores get dead-store-eliminated and VectorMA/AAS_Trace see
                // garbage in [1]/[2]
  vec3_t forward; // [esp+20h] [ebp-194h] BYREF
  vec3_t maxs; // [esp+2Ch] [ebp-188h] BYREF
  vec3_t dir; // [esp+38h] [ebp-17Ch] BYREF
  vec3_t right; // [esp+44h] [ebp-170h] BYREF
  vec3_t mins; // [esp+50h] [ebp-164h] BYREF
  vec3_t end; // [esp+5Ch] [ebp-158h] BYREF
  float trace[21]; // [esp+68h] [ebp-14Ch] BYREF
  aas_entityinfo_t entinfo; // [esp+BCh] [ebp-7Ch] BYREF

  /* Float literals: mins/maxs are float[3], not raw bit patterns. */
  mins[0] = -8.0f;
  attackentity = bs->enemy;
  mins[1] = -8.0f;
  mins[2] = -8.0f;
  maxs[0] = 8.0f;
  maxs[1] = 8.0f;
  maxs[2] = 8.0f;
  if ( attackentity )
  {
    /* Characteristic_BFloat's FPU return drives the splash-attack timer. */
    v11 = (float)Characteristic_BFloat(BotCharacter(bs), 11, 0.0, 1.0);
    if ( AAS_Time() - v11 >= bs->enemysight_time )
    {
      entinfo = AAS_EntityInfo(bs->enemy);
      VectorSubtract(entinfo.origin, bs->origin, dir);
      if ( VectorLength(dir) < 100.0f )
        v11 = 120.0f;
      else
        v11 = 50.0f;
      if ( BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, v11, bs->enemy) )
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
          *(bsp_trace_t *)trace = AAS_Trace(start, (float*)mins, (float*)maxs, (float*)(end), bs->entitynum, 100663299);
          if ( LODWORD(trace[20]) == bs->enemy
            || (SLODWORD(trace[20]) <= 0 || SLODWORD(trace[20]) > botstate.num_clients || !BotSameTeam(bs, SLODWORD(trace[20])))
            && ((v6 = wi->proj, (v6->damagetype & 2) == 0)
             || trace[2] * 1000.0f >= v6->radius
             || (points = ((float)v6->damage - trace[2] * 500.0) * 0.5, points <= 0)) )
          {
            if ( (LOBYTE(trace[19]) & 2) != 0 )
            {
              entinfo = AAS_EntityInfo(bs->enemy);
              *(bsp_trace_t *)trace = AAS_Trace((&trace[3]), (float*)(uintptr_t)(0), (float*)(uintptr_t)(0),
                                entinfo.origin, bs->entitynum, 100663299);
              if ( LODWORD(trace[20]) != bs->enemy )
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
//----- (10024A10) --------------------------------------------------------
int *__cdecl BotEntityToActivate(int a1)
{
  char *v1; // eax
  bsp_entity_t *v2; // edi — current entity walk
  const char *v3; // ebp
  const char *v4; // eax
  const char *v5; // eax
  const char *v6; // ebx
  int v7; // esi
  const char *v8; // eax
  const char *v9; // eax
  bsp_entity_t **v10; // ebx — pointer into stack-resident heads array
  const char **v11; // ebp
  const char *v12; // eax
  const char *v13; // eax
  int v14; // [esp+10h] [ebp-D4h]
  const char *v15; // [esp+14h] [ebp-D0h]
  const char *v16[10]; // [esp+18h] [ebp-CCh] BYREF — targetname stack
  bsp_entity_t *v17[10]; // [esp+40h] [ebp-A4h] BYREF — heads stack walked via v10
  int v18[31]; // [esp+68h] [ebp-7Ch] BYREF

  *(aas_entityinfo_t *)v18 = AAS_EntityInfo(a1);
  v1 = AAS_ModelFromIndex(v18[23]);
  v2 = dword_10064398;
  v3 = v1;
  if ( !v2 )
    goto LABEL_5;
  while ( 1 )
  {
    v4 = (const char *)AAS_ValueForBSPEpairKey(v2, "model");
    if ( v4 )
    {
      if ( !strcmp(v3, v4) )
        break;
    }
    v2 = v2->next;
    if ( !v2 )
    {
      botimport.Print(PRT_ERROR, "BotEntityToActivate: no entity found with model %s\n", v3);
      return 0;
    }
  }
  if ( !v2 )
  {
LABEL_5:
    botimport.Print(PRT_ERROR, "BotEntityToActivate: no entity found with model %s\n", v3);
    return 0;
  }
  v5 = (const char *)AAS_ValueForBSPEpairKey(v2, "classname");
  v6 = v5;
  v15 = v5;
  if ( !v5 )
  {
    botimport.Print(PRT_ERROR, "BotEntityToActivate: entity with model %s has no classname\n", v3);
    return 0;
  }
  if ( !strcmp(v5, "func_door_secret") )
  {
    v7 = AAS_ValueForBSPEpairKey(v2, "targetname");
    v8 = (const char *)AAS_ValueForBSPEpairKey(v2, "spawnflags");
    if ( !v7 || (atoi(v8) & 1) != 0 )
      return (int *)v2;
  }
  if ( !strcmp(v6, "func_door") && FloatForKey(v2, "health") != 0.0f )
    return (int *)v2;
  v16[0] = (const char *)AAS_ValueForBSPEpairKey(v2, "targetname");
  if ( !v16[0] )
    return 0;
  v14 = 0;
  v17[0] = dword_10064398;
  v10 = v17;
  v11 = v16;
  while ( 1 )
  {
    if ( v14 >= 10 )
      goto LABEL_39;
    v2 = *v10;
    if ( *v10 )
    {
      while ( 1 )
      {
        v12 = (const char *)AAS_ValueForBSPEpairKey(v2, "target");
        if ( v12 )
        {
          if ( !strcmp(*v11, v12) )
            break;
        }
        v2 = v2->next;
        if ( v2 )
          continue;
        goto LABEL_26;
      }
      *v10 = v2->next;
      if ( v2 )
        break;
    }
LABEL_26:
    v9 = *v11;
    botimport.Print(PRT_ERROR, "BotEntityToActivate: no entity with target \"%s\"\n", v9);
    --v14;
    --v11;
    --v10;
LABEL_38:
    if ( v14 < 0 )
      goto LABEL_39;
  }
  v13 = (const char *)AAS_ValueForBSPEpairKey(v2, "classname");
      v15 = v13;
      if ( !v13 )
      {
        botimport.Print(PRT_ERROR, "BotEntityToActivate: entity with target \"%s\" has no classname\n", v16[v14]);
        return 0;
      }
      if ( strcmp(v13, "trigger_counter") && strcmp(v15, "trigger_relay") )
      {
        if ( !strcmp(v15, "func_button")
            || !strcmp(v15, "trigger_multiple")
            || !strcmp(v15, "trigger_once")
            || !strcmp(v15, "func_door_rotating") )
        {
          return (int *)v2;
        }
        if ( !strcmp(v15, "trigger_key") )
          return 0;
        --v14;
        --v11;
        --v10;
        goto LABEL_38;
      }
      if ( v14 < 9 )
      {
        ++v14;
        ++v11;
        ++v10;
        *v11 = (const char *)AAS_ValueForBSPEpairKey(v2, "targetname");
        *v10 = dword_10064398;
        goto LABEL_38;
      }
  botimport.Print(PRT_ERROR, "BotEntityToActivate: stacked up more than %d trigger_counter or trigger_relay\n", v14);
  return 0;
LABEL_39:
  botimport.Print(PRT_ERROR, "BotEntityToActivate: unkown activator with classname \"%s\"\n", v15);
  return 0;
}
//----- (10024FD0) --------------------------------------------------------
int __cdecl BotSetMovedir(float *angles, float *movedir)
{
  if ( VectorCompare(angles, VEC_UP) )
  {
    VectorCopy(MOVEDIR_UP, movedir);
    return (int)(intptr_t)movedir;
  }
  else if ( VectorCompare(angles, VEC_DOWN) )
  {
    VectorCopy(MOVEDIR_DOWN, movedir);
    return (int)(intptr_t)movedir;
  }
  else
  {
    /* AngleVectors is void and this return value is dead (both callers ignore
     * it), so the original just lets eax flow through from the call.  The
     * cast-call reproduces that `call; ret` instead of keeping `dir` alive
     * across the call at the cost of a callee-saved push. */
    return ((int (__cdecl *)(float *, float *, void *, void *))AngleVectors)(angles, movedir, NULL, NULL);
  }
}
//----- (10025070) --------------------------------------------------------
/* Debug visualiser for func_button entities: walk the BSP entity list, filter
 * by classname == "func_button", read the brush's model AABB plus the
 * "angle"/"health" keys, and draw permanent debug crosses — one at the shoot
 * point for shootable buttons (health != 0), three for touch/use buttons
 * (the bot's standing position plus two corner markers).  Capped at the first
 * 6 matching buttons.
 *
 * Quirks preserved: FloatForKey(ent, "lip") is called and its result
 * discarded, and BSPModelMinsMaxs is called with zero-initialised `angles`, so
 * the bbox is the model's local-space AABB rather than a world-rotated one. */
void __cdecl sub_10025070(void)
{
  bsp_entity_t *ent;
  int drawn;
  char *classname;
  char *model_str;
  int modelnum;
  struct {
    vec3_t origin;
    vec3_t mins;
    vec3_t maxs;
    vec3_t start;
    vec3_t goalorigin;
    vec3_t movedir;
    float dist;
    vec3_t angles;
    vec3_t size;
    vec3_t end;
    vec3_t bboxmins;
    vec3_t bboxmaxs;
    aas_trace_t trace;
  } l;

  drawn = 0;
  ent   = dword_10064398;
  if ( !ent )
    return;

  do
  {
    /* Do NOT remove the two (const char *) casts below, even though they trip
     * -Wdiscarded-qualifiers and are codegen-neutral here: dropping them
     * perturbs MSVC6's whole-TU scheduler enough to flip a tie-break in
     * PC_ReadDefineParms and cost that function its byte-match. */
    classname = (const char *)AAS_ValueForBSPEpairKey(ent, "classname");
    if ( !strcmp(classname, "func_button") )
    {
      model_str = (const char *)AAS_ValueForBSPEpairKey(ent, "model");
      modelnum  = IndexFromModel(model_str);
      if ( !modelnum )
        modelnum = atoi(model_str + 1);

      l.angles[2] = 0.0f;
      l.angles[1] = 0.0f;
      l.angles[0] = 0.0f;
      AAS_BSPModelMinsMaxsOrigin(modelnum - 1, l.angles, l.mins, l.maxs, NULL);

      FloatForKey(ent, "lip");
      l.angles[0] = 0.0f;
      l.angles[1] = FloatForKey(ent, "angle");
      l.angles[2] = 0.0f;
      BotSetMovedir(l.angles, l.movedir);

      VectorSubtract(l.maxs, l.mins, l.size);
      VectorAdd(l.mins, l.maxs, l.origin);
      VectorScale(l.origin, 0.5f, l.origin);
      l.dist = fabs(l.movedir[2]) * l.size[2]
             + fabs(l.movedir[1]) * l.size[1]
             + fabs(l.movedir[0]) * l.size[0];
      l.dist *= 0.5;

      if ( FloatForKey(ent, "health") )
      {
        VectorMA(l.origin, -l.dist, l.movedir, l.goalorigin);
        AAS_DrawPermanentCross(l.goalorigin, 4.0f, (int)0xf3f3f1f1);
      }
      else
      {
        float accum;
        int offset;

        AAS_PresenceTypeBoundingBox(4, l.bboxmins, l.bboxmaxs);
        /* BYTE-OFFSET walk, not `for (i = 0; i < 3; i++)`: the original shares one
         * esp-relative index across all three arrays, whereas the indexed form
         * strength-reduces to a walking pointer plus a separate counter.  This is
         * the original's own addressing — do not "fix" it. */
        offset = 0;
        accum = l.dist;
        do
        {
          float side;

          if ( *(float *)((char *)l.movedir + offset) < 0.0f )
            side = *(float *)((char *)l.bboxmaxs + offset);
          else
            side = *(float *)((char *)l.bboxmins + offset);
          accum += fabs(*(float *)((char *)l.movedir + offset)) * fabs(side);
          offset += 4;
        }
        while ( offset < 12 );
        VectorMA(l.origin, -accum, l.movedir, l.goalorigin);

        VectorCopy(l.goalorigin, l.start);
        l.start[2] += 24.0f;
        VectorSet(l.end, l.start[0], l.start[1], l.start[2] - 100.0f);
        l.trace = AAS_TraceClientBBox(l.start, l.end, 4, -1);
        if ( !l.trace.startsolid )
          VectorCopy(l.trace.endpos, l.goalorigin);
        AAS_DrawPermanentCross(l.goalorigin, 4.0f, (int)0xdcdddedf);

        VectorSubtract(l.mins, l.origin, l.mins);
        VectorSubtract(l.maxs, l.origin, l.maxs);

        VectorAdd(l.origin, l.mins, l.start);
        AAS_DrawPermanentCross(l.start, 4.0f, (int)0xf3f3f1f1);

        VectorAdd(l.origin, l.maxs, l.start);
        AAS_DrawPermanentCross(l.start, 4.0f, (int)0xf3f3f1f1);
      }

      if ( ++drawn > 5 )
        return;
    }
    ent = ent->next; /* next entity (typed bsp_entity_t) */
  }
  while ( ent );
}
//----- (10025560) --------------------------------------------------------
/* Genuinely void, as in Q3: each exit path just leaves whatever is in eax and
 * every caller ignores it. */
void __cdecl BotAIBlocked(bot_state_t *bs, bot_moveresult_t *moveresult, int activate)
{
  ai_node_fn_t result; // eax
  int modelnum;
  int *v5; // eax
  int *v6; // ebp
  const char *v7; // edi
  char *v8; // eax
  long double v9; // st7
  int i; // ecx
  long double v11; // st6
  bot_state_t *v13; // esi (originally `int v13 = a1` where `a1` aliased a global char[] - decompiler artifact; real value is bs)
  int v16; // eax
  int v18; // ecx
  char *v20; // esi
  int v21; // eax
  char *v28; // eax
  int v33; // eax
  vec3_t v38_vec; // [esp+1Ch..24h] [ebp-158h..150h] BYREF (was v38/v39/v40 vec3 split)
  vec3_t v41_vec; // [esp+28h..30h] [ebp-14Ch..144h] BYREF (was v41/v42/v43 vec3 split)
  vec3_t v44_vec; // [esp+34h..3Ch] [ebp-140h..138h] BYREF (was v44/v45/v46 vec3 split)
  vec3_t v47_vec; // [esp+40h..48h] [ebp-134h..12Ch] BYREF (was v47/v48/v49 vec3 split)
  vec3_t v50; /* was v50,v51,v52 — vec3_t for activated area check */
  vec3_t v53_vec; // [esp+58h..60h] [ebp-11Ch..114h] BYREF
  vec3_t v56_vec; // [esp+64h..6Ch] [ebp-110h..108h] BYREF (was v56/v57/v58; AAS_BSPModelMinsMaxsOrigin origin)
  vec3_t v59_vec; // [esp+70h..78h] [ebp-104h..FCh] BYREF (was v59/v60/v61 vec3 split)
  vec3_t sideward; // [esp+7Ch..84h] [ebp-F8h..F0h] BYREF (was v62 int + v63/v64 float — vec3 split for CrossProduct/BotMoveInDirection)
  float v65; // [esp+88h] [ebp-ECh]
  vec3_t v66_vec; // [esp+8Ch..94h] [ebp-E8h..E0h] BYREF
  vec3_t v69_vec; // [esp+98h..A0h] [ebp-DCh..D4h] (was v69/v70/v71 vec3 split)
  vec3_t hordir; // [esp+A4h] [ebp-D0h] BYREF
  vec3_t up; // [esp+B0h] [ebp-C4h] BYREF
  aas_trace_t trace; // [esp+BCh] [ebp-B8h] (was int v74[9])
  vec3_t v75; // [esp+E0h] [ebp-94h] BYREF
  vec3_t v76; // [esp+ECh] [ebp-88h] BYREF
  int v77[31]; // [esp+F8h] [ebp-7Ch] BYREF

  up[0] = 0;
  up[1] = 0;
  up[2] = 1.0f;   /* 1065353216 bit-pattern of 1.0f; v73 is float[3] */
  if ( !moveresult->blocked )
    return;
  *(aas_entityinfo_t *)v77 = AAS_EntityInfo(moveresult->blockentity);
  if ( v77[22] != 3 || !activate )
    goto LABEL_37;
  v5 = BotEntityToActivate(v77[3]);
  v6 = v5;
  if ( !v5 )
    v7 = &byte_1006294C;
  else
    v7 = (const char *)AAS_ValueForBSPEpairKey(v5, "classname");
  if ( !strcmp(v7, "func_door_secret") || !strcmp(v7, "func_door") )
  {
    v28 = AAS_ValueForBSPEpairKey(v6, "model");
    modelnum = IndexFromModel(v28);
    if ( modelnum )
    {
      VectorClear(v56_vec);
      AAS_BSPModelMinsMaxsOrigin(modelnum - 1, v56_vec, v44_vec, v41_vec, NULL);
      VectorAdd(v44_vec, v41_vec, v38_vec);
      VectorScale(v38_vec, 0.5f, v38_vec);
      VectorSubtract(v38_vec, bs->origin, v50);
      vectoangles(v50, moveresult->ideal_viewangles);
      moveresult->flags |= 1;
      EA_UseItem(bs->client, "Blaster");
      EA_Attack(bs->client);
      return;
    }
    return;
  }
  if ( !strcmp(v7, "func_button") )
  {
    v8 = AAS_ValueForBSPEpairKey(v6, "model");
    modelnum = IndexFromModel(v8);
    if ( !modelnum )
      return;
    VectorClear(v56_vec);
    AAS_BSPModelMinsMaxsOrigin(modelnum - 1, v56_vec, v44_vec, v41_vec, NULL);
    FloatForKey(v6, "lip");
    {
      vec3_t angles;
      angles[0] = 0;
      angles[1] = FloatForKey(v6, "angle");
      angles[2] = 0;
      BotSetMovedir(angles, v50);
    }
    VectorSubtract(v41_vec, v44_vec, v69_vec);
    VectorAdd(v44_vec, v41_vec, v59_vec);
    VectorScale(v59_vec, 0.5f, v59_vec);
    v65 = (fabs(v50[2]) * v69_vec[2] + fabs(v50[1]) * v69_vec[1] + fabs(v50[0]) * v69_vec[0]) * 0.5f;
    if ( FloatForKey(v6, "health") != 0.0f )
    {
      VectorMA(v59_vec, -v65, v50, v38_vec);
      VectorSubtract(v38_vec, bs->origin, v50);
      vectoangles(v50, moveresult->ideal_viewangles);
      moveresult->flags |= 1;
      EA_UseItem(bs->client, "Blaster");
      EA_Attack(bs->client);
      return;
    }
    AAS_PresenceTypeBoundingBox(4, v76, v75);
    v9 = v65;
    for ( i = 0; i < 3; ++i )
    {
      if ( v50[i] < 0.0f )
        v11 = v75[i];
      else
        v11 = v76[i];
      v9 = fabs(v11) * fabs(v50[i]) + v9;
    }
    VectorMA(v59_vec, -v9, v50, v38_vec);
    v53_vec[0] = v38_vec[0];
    v53_vec[1] = v38_vec[1]; /* all three start components are written */
    v53_vec[2] = v38_vec[2] + 24.0f;
    v66_vec[0] = v38_vec[0];
    v66_vec[1] = v38_vec[1]; /* all three end components are written */
    v66_vec[2] = v53_vec[2] - 100.0f; /* end.z is written here too */
    trace = AAS_TraceClientBBox(v53_vec, v66_vec, 4, -1);
    if ( !trace.startsolid )
    {
      VectorCopy(trace.endpos, v38_vec);
    }
    v13 = bs;
    VectorCopy(v59_vec, bs->activategoal.origin);
    v16 = AAS_PointAreaNum(v38_vec);
    v18 = v77[3];
    bs->activategoal.areanum = v16;
    bs->activategoal.entitynum = v18;
    bs->activategoal.number = 0;
    bs->activategoal.flags = 0;
    bs->activategoal.mins[0] = v44_vec[0] - v59_vec[0] - 5.0f;
    bs->activategoal.mins[1] = v44_vec[1] - v59_vec[1] - 5.0f;
    bs->activategoal.mins[2] = v44_vec[2] - v59_vec[2] - 5.0f;
    bs->activategoal.maxs[0] = v41_vec[0] - v59_vec[0] + 5.0f;
    bs->activategoal.maxs[1] = v41_vec[1] - v59_vec[1] + 5.0f;
    bs->activategoal.maxs[2] = v41_vec[2] - v59_vec[2] + 5.0f;
    bs->activategoal_time = AAS_Time() + 10.0f;
    if ( !AAS_AreaReachability(bs->activategoal.areanum) )
    {
      result = BotAINode(bs);
      if ( result == AINode_Seek_NBG )
      {
        bs->nbg_time = 0.0f;
        return;
      }
      if ( result == AINode_Seek_LTG )
        v13->ltg_time = 0.0f;
      return;
    }
    AIEnter_Seek_ActivateEntity(v13);
    return;
  }
  if ( strcmp(v7, "trigger_multiple") && strcmp(v7, "trigger_once") )
  {
LABEL_37:
    hordir[0] = moveresult->movedir[0];   /* raw float copy — original mov [esp+0x98],[ebx+0x18] at 0x10025e6e */
    hordir[1] = moveresult->movedir[1];   /* raw float copy — original mov [esp+0xa0],[ebx+0x1c] */
    hordir[2] = 0;
    VectorNormalize(hordir);
    VectorCopy(bs->origin, v53_vec);
    v53_vec[2] = v53_vec[2] + libvar_sv_step->value;
    VectorMA(v53_vec, 5.0f, hordir, v66_vec);
    v44_vec[0] = -16.0f;
    v44_vec[1] = -16.0f;
    v44_vec[2] = -24.0f;
    v41_vec[0] = 16.0f;
    v41_vec[1] = 16.0f;
    v41_vec[2] = 4.0f;
    CrossProduct(hordir, up, sideward);
    if ( (*(unsigned char *)&bs->flags & 0x10) != 0 )
    {
      sideward[0] = -sideward[0];
      sideward[1] = -sideward[1];
      sideward[2] = -sideward[2];
    }
    if ( !BotMoveInDirection((bot_movestate_t *)bs->movestate, (float *)(intptr_t)sideward, 400.0f, 1) )
    {
      sideward[0] = -sideward[0];
      v33 = bs->flags;   /* original reads flags mid-negation: mov eax,[esi+0xac0] at 0x10025f9b */
      sideward[1] = -sideward[1];
      sideward[2] = -sideward[2];
      bs->flags = v33 ^ 0x10;
      BotMoveInDirection((bot_movestate_t *)bs->movestate, (float *)(intptr_t)sideward, 400.0f, 1);
    }
    result = BotAINode(bs);
    if ( result == AINode_Seek_NBG )
    {
      bs->nbg_time = 0.0f;
    }
    else if ( result == AINode_Seek_LTG )
    {
      bs->ltg_time = 0.0f;
    }
    return;
  }
  v20 = AAS_ValueForBSPEpairKey(v6, "model");
  v21 = IndexFromModel(v20);
  if ( !v21 )
    v21 = atoi(v20 + 1);
  VectorClear(v56_vec);
  AAS_BSPModelMinsMaxsOrigin(v21 - 1, v56_vec, v44_vec, v41_vec, NULL);
  VectorAdd(v44_vec, v41_vec, v47_vec);
  VectorScale(v47_vec, 0.5f, v47_vec);
  v53_vec[0] = v47_vec[0];
  v53_vec[1] = v47_vec[1]; /* all three start components are written */
  v53_vec[2] = v41_vec[2] + 24.0f;
  v66_vec[0] = v47_vec[0];
  v66_vec[1] = v47_vec[1]; /* all three end components are written */
  v66_vec[2] = v53_vec[2] - 100.0f; /* end.z is written here too */
  /* thunk 0x10001861 -> AAS_TraceClientBBox */
  trace = AAS_TraceClientBBox(v53_vec, v66_vec, 4, -1);
  if ( !trace.startsolid )
  {
    v13 = bs;
    VectorCopy(trace.endpos, v38_vec);
    VectorCopy(v47_vec, bs->activategoal.origin);
    bs->activategoal.areanum = AAS_PointAreaNum(v38_vec);
    bs->activategoal.entitynum = v77[3];
    bs->activategoal.number = 0;
    bs->activategoal.flags = 0;
    VectorSubtract(v44_vec, v47_vec, bs->activategoal.mins);
    VectorSubtract(v41_vec, v47_vec, bs->activategoal.maxs);
    bs->activategoal_time = AAS_Time() + 10.0f;
    if ( !AAS_AreaReachability(bs->activategoal.areanum) )
    {
      result = BotAINode(bs);
      if ( result == AINode_Seek_NBG )
      {
        bs->nbg_time = 0.0f;
        return;
      }
      if ( result == AINode_Seek_LTG )
        v13->ltg_time = 0.0f;
      return;
    }
    AIEnter_Seek_ActivateEntity(v13);
    return;
  }
  return;
}
//----- (100262C0) --------------------------------------------------------
void __cdecl sub_100262C0(_DWORD *a1, bot_goal_t *a2)
{
  int v2[31]; // [esp+8h] [ebp-7Ch] BYREF

  if ( libvar_ctf->value != 0.0f )
  {
    if ( a2->entitynum )
    {
      *(aas_entityinfo_t *)v2 = AAS_EntityInfo(a2->entitynum);
      if ( (v2[23] == dword_1006449C || v2[23] == dword_10064498 || v2[23] == dword_10064494 || v2[23] == dword_10064490)
        && ((int)a1[477] > 0 && v2[23] != dword_1006449C
         || (int)a1[478] > 0 && v2[23] != dword_10064498
         || (int)a1[479] > 0 && v2[23] != dword_10064494
         || (int)a1[480] > 0 && v2[23] != dword_10064494) )
      {
        EA_DropItem(a1[1], "tech");
      }
    }
  }
}
//----- (100263D0) --------------------------------------------------------
void __cdecl BotCTFRetreatGoals(bot_state_t *bs)
{

  float v1; // st7

  if ( BotCTFCarryingFlag(bs) )
  {
    if ( bs->ltgtype != 5 )
    {
      bs->ltgtype = 5;
      v1 = AAS_Time();
      *(int *)&bs->rushbaseaway_time = 0;
      bs->teamgoal_time = v1 + 120.0f;
    }
  }
}
//----- (10026440) --------------------------------------------------------
void __cdecl BotCTFSeekGoals(bot_state_t *bs)
{

  int v3; // eax
  int v4; // ax
  double v5; // st7
  float v8; // [esp+8h] [ebp+4h]

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
      v4 = rand();
      v8 = (float)(v4 & 0x7FFF) * 0.000030518509f;
      v8 = v8 + v8;
      bs->teammessage_time = AAS_Time() + v8;
      v5 = (rand() & 0x7FFF) * 0.000030518509f;
      if ( v5 < 0.33f && ctf_redflag.areanum && ctf_blueflag.areanum )
      {
        bs->ltgtype = 4;
        bs->teamgoal_time = AAS_Time() + 180.0f;
      }
      else if ( v5 < 0.66 && ctf_redflag.areanum && ctf_blueflag.areanum )
      {
        if ( BotCTFTeam(bs) == 1 )
          memcpy(&bs->teamgoal, &ctf_redflag, 0x38u);
        else
          memcpy(&bs->teamgoal, &ctf_blueflag, 0x38u);
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
//----- (10026690) --------------------------------------------------------
BOOL TeamPlayIsOn()
{
  return ((int)libvar_dmflags->value & 0xC0) != 0
      || libvar_ctf->value != 0.0f
      || libvar_teamplay->value != 0.0f;
}
//----- (10026700) --------------------------------------------------------
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
// 10026728: conditional instruction was optimized away because eax.4<1
//----- (10026770) --------------------------------------------------------
int __cdecl BotGetMessageTeamGoal(bot_state_t *bs, char *goalname, bot_goal_t *goal)
{
  int cp; // eax

  if ( BotGetItemTeamGoal(goalname, goal) )
    return 1;
  cp = BotFindWayPoint(bs->checkpoints, goalname);
  if ( cp )
  {
    memcpy((void *)goal, (const void *)(cp + 4), 0x38u);
    return 1;
  }
  return 0;
}
//----- (100267E0) --------------------------------------------------------
float __cdecl BotGetTime(bot_match_t *match)
{
  float v1; // st7
  float t; // [esp+0h] [ebp-18Ch]
  char timestring[152]; // [esp+4h] [ebp-188h] BYREF
  /* One bot_match_t (240 B), filled by BotFindMatch — see chat_state.h. */
  bot_match_t timematch; // [esp+9Ch] [ebp-F0h] BYREF

  if ( (match->subtype & 0x10) == 0 )
    return 0.0f;
  BotMatchVariable(match, 5, timestring);
  if ( !BotFindMatch(timestring, &timematch, 8) )
    return 0.0f;
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
  return 0.0f;
}
//----- (100268D0) --------------------------------------------------------
int __cdecl FindClientByName(char *name)
{
  int i;

  for ( i = 0; i < botstate.num_clients; i++ )
  {
    if ( !_strcmpi(clientsettings[i].netname, name) )
      return i;
  }
  for ( i = 0; i < botstate.num_clients; i++ )
  {
    if ( stristr(clientsettings[i].netname, name) )
      return i;
  }
  return -1;
}
//----- (10026990) --------------------------------------------------------
int __cdecl BotGetPatrolWaypoints(bot_state_t *bs, bot_match_t *match)
{
  bot_waypoint_t *newpatrolpoints; // esi (head of new patrol list)
  int patrolflags; // edi (patrol flags accumulator)
  bot_waypoint_t *newwp; // eax (newly-allocated node)
  bot_waypoint_t *wp; // ecx (tail walker)
  bot_goal_t goal; // [esp+10h] [ebp-1C0h] BYREF — parsed goal for current keypoint name
  char Destination[152]; // [esp+48h] [ebp-188h] BYREF
  /* One bot_match_t (240 B), filled by BotFindMatch — see chat_state.h. */
  bot_match_t keyareamatch; // [esp+E0h] [ebp-F0h] BYREF

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
  if ( newpatrolpoints && newpatrolpoints->next )
  {
    BotFreeWaypoints(BotPatrolpoints(bs));
    bs->patrolflags = patrolflags;
    BotPatrolpoints(bs) = newpatrolpoints;
    BotCurPatrolPoint(bs) = newpatrolpoints;
    return 1;
  }
  else
  {
    EA_SayTeam(bs->client, "I need more key points to patrol\n");
    BotFreeWaypoints(newpatrolpoints);
    return 0;
  }
}
// 10026A34: conditional instruction was optimized away because edx.4!=0
//----- (10026BE0) --------------------------------------------------------
int __cdecl BotAddressedToBot(bot_state_t *bs, bot_match_t *match)
{
  int client; // eax
  int result; // eax
  const char *botname; // esi (Q2 ClientName returns char*)
  float v5; // [esp+Ch] [ebp-2BCh]
  char name[152]; // [esp+10h] [ebp-2B8h] BYREF
  char addressedto[152]; // [esp+A8h] [ebp-220h] BYREF
  /* One bot_match_t (240 B), filled by BotFindMatch — see chat_state.h. */
  bot_match_t addresseematch; // [esp+140h] [ebp-188h] BYREF
  char netname[152]; // [esp+230h] [ebp-98h] BYREF

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
    v5 = (float)(rand() & 0x7FFF) * 0.000030518509f;
    if ( 1.0f / (float)(BotNumTeamMates(bs) - 1) < v5 )
      return 0;
  }
  return 1;
}
//----- (10026E40) --------------------------------------------------------
// BotGPSToPosition — the Q3 cognate; this source form compiles to the ref DLL
// at 0x10026E40 here.
// DEAD in Gladiator — /INCREMENTAL.
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
//----- (10026F10) --------------------------------------------------------
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
  /* The full 240-byte bot_match_t (chat_state.h), including the variables[]
   * capture array, so the BotFindMatch / StringsMatch / BotMatchVariable
   * interfaces type-check. */
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
        bs->teamgoal.areanum = bs->areanum;
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
      if ( libvar_ctf->value == 0.0f || !ctf_redflag.areanum || !ctf_blueflag.areanum || !BotAddressedToBot(bs, &match) )
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
      if ( libvar_ctf->value == 0.0f || !ctf_redflag.areanum || !ctf_blueflag.areanum || !BotAddressedToBot(bs, &match) )
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
      origin[2] = 0.0;
      origin[1] = 0.0;
      origin[0] = 0.0;
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
//----- (10028650) --------------------------------------------------------
void __cdecl BotCheckConsoleMessages(bot_state_t *bs)
{
  bot_chatstate_t *v2; // ebp
  bot_consolemessage_t *v3;
  char *v4; // eax
  ptrdiff_t v5; // length in message
  int context; // ecx
  float v7; // st7
  char *v8; // eax
  char *botname; // [esp+10h] [ebp-14h]
  float v11; // [esp+14h] [ebp-10h]
  float v12; // [esp+18h] [ebp-Ch]
  double v13; // [esp+1Ch] [ebp-8h]
  float v14; // [esp+28h] [ebp+4h]

  botname = ClientName(bs->client);
  v2 = &bs->chatstate;
  while ( (v3 = BotNextConsoleMessage(v2)) != NULL )
  {
    if ( BotNumConsoleMessages(v2) < 10 && v3->type == 1 )
    {
      v12 = AAS_Time();
      if ( v12 - ((float)(rand() & 0x7FFF) * 0.000030518509f + 1.0f) < v3->time )
        return;
    }
    if ( v3->type == 1 )
    {
      v4 = strstr(v3->message, ":");
      /* POSITIVE guard, so the body stays the warm fall-through.  The negative
       * `if (!v4) { remove; continue; }` form makes the remove block the
       * fall-through, which also lets cl.exe cross-jump it into the strncmp
       * path's identical block; the original keeps the two separate. */
      if ( v4 )
      {
        v5 = v4 - (char *)v3;
        if ( !strncmp(v3->message, botname, v5 - 8) || !strncmp(v3->message + 1, botname, v5 - 10) )
        {
          BotRemoveConsoleMessage(v2, v3);
          continue;
        }
      }
      else
      {
        BotRemoveConsoleMessage(v2, v3);
        continue;
      }
    }
    UnifyWhiteSpaces(v3->message);
    context = 3;
    if ( libvar_ctf->value != 0.0f )
      context = BotCTFTeam(bs) != 1 ? 11 : 7;
    BotReplaceSynonyms(v3->message, context);
    if ( !BotMatchMessage(bs, v3->message) && v3->type == 1 )
    {
      v7 = libvar_nochat->value;
      if ( v7 == 0.0f && BotAINode(bs) != AINode_Stand )
      {
        if ( BotValidChatPosition(bs) )
        {
          v11 = Characteristic_BFloat(BotCharacter(bs), 22, 0.0, 1.0);
          v13 = (float)(rand() & 0x7FFF) * 0.000030518509f;
          if ( 1.5 / (float)(NumBots() + 1) > v13 )
          {
            if ( (float)(rand() & 0x7FFF) * 0.000030518509f < v11 )
            {
              v8 = strstr(v3->message, ":");
              if ( v8 )
              {
                memmove(v3->message, v8 + 1, strlen(v8 + 1) + 1);
                UnifyWhiteSpaces(v3->message);
                if ( BotReplyChat(v2, v3->message) )
                {
                  BotRemoveConsoleMessage(v2, v3);
                  v14 = BotChatTime(bs);
                  bs->stand_time = AAS_Time() + v14;
                  AIEnter_Stand(bs);
                  return;
                }
              }
            }
          }
       }
     }
   }
   BotRemoveConsoleMessage(v2, v3);
 }
}
//----- (100289A0) --------------------------------------------------------
float *__cdecl sub_100289A0(bot_state_t *bs, float a2)
{
  int v6; // edx

  bs->ltime += a2;
  bs->thinktime = a2;
  bs->origin[0] = bs->snapshot.origin[0];
  bs->origin[1] = bs->snapshot.origin[1];
  bs->eye[0] = bs->snapshot.viewoffset[0] + bs->snapshot.origin[0];
  v6 = (*(int *)&bs->snapshot.origin[2]);
  memcpy(bs->inventory, bs->inventory_src, 0x400u);
  bs->eye[1] = bs->snapshot.viewoffset[1] + bs->snapshot.origin[1];
  bs->eye[2] = bs->snapshot.viewoffset[2] + *(float *)&(*(int *)&bs->snapshot.origin[2]);
  *(int *)&bs->origin[2] = v6;
  return (float *)bs;
}
//----- (10028A40) --------------------------------------------------------
int __cdecl sub_10028A40(bot_state_t *bs, float a2)
{
  return ((int (__cdecl *)(int, float))EA_EndRegular)(bs->client, a2);
}
//----- (10028A70) --------------------------------------------------------
int BotDeathmatchAI(bot_state_t *bs, float thinktime)
{
  int i; // edi
  int result; // eax
  float v6; // [esp+10h] [ebp+8h]

  sub_100289A0(bs, thinktime);
  if ( dword_1006446C && AAS_Initialized() )
    dword_1006446C = 0;
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
    v6 = BotChatTime(bs);
    bs->stand_time = AAS_Time() + v6;
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
  result = *(_DWORD *)bs;
  if ( *(_DWORD *)bs )
    return sub_10028A40(bs, thinktime);
  return result;
}
// 10028B9E: conditional instruction was optimized away because edi.4<32
//----- (10028C30) --------------------------------------------------------
void BotSetupDeathmatchAI()
{
  libvar_dmflags = LibVar("dmflags", (char *)"0");
  libvar_ctf = LibVar("ctf", (char *)"0");
  libvar_ch = LibVar("ch", (char *)"0");
  libvar_ra = LibVar("ra", (char *)"0");
  libvar_fastchat = LibVar("fastchat", (char *)"0");
  libvar_nochat = LibVar("nochat", (char *)"0");
  libvar_teamplay = LibVar("teamplay", (char *)"0");
  libvar_usehook = LibVar("usehook", (char *)"0");
  libvar_rocketjump = LibVar("rocketjump", (char *)"1");
  libvar_runes = LibVar("runes", (char *)"0");
  libvar_teamplay_shell = LibVar("teamplay_shell", (char *)"0");
  libvar_assimilation = LibVar("assimilation", (char *)"0");
  if ( libvar_ctf->value != 0.0f )
  {
    if ( BotGetLevelItemGoal(-1, "Red Flag", &ctf_redflag) < 0 )
      botimport.Print(PRT_WARNING, "CTF without Red Flag\n");
    if ( BotGetLevelItemGoal(-1, "Blue Flag", &ctf_blueflag) < 0 )
      botimport.Print(PRT_WARNING, "CTF without Blue Flag\n");
    dword_10064484 = IndexFromModel("players/male/flag1.md2");
    dword_1006448C = IndexFromModel("players/male/flag2.md2");
    dword_1006449C = IndexFromModel("models/ctf/resistance/tris.md2");
    dword_10064498 = IndexFromModel("models/ctf/strength/tris.md2");
    dword_10064494 = IndexFromModel("models/ctf/haste/tris.md2");
    dword_10064490 = IndexFromModel("models/ctf/regeneration/tris.md2");
  }
  dword_1006446C = 1;
}
