/*
 * be_ai_goal.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x1002ED20..0x10030A20; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * Its own interface is in the matching .h; botlib_local.h, which that header
 * pulls in, carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs, the side-band scheme and the externs for
 * botlib.c's remaining globals).
 */

#include "be_ai_goal.h"
#include "be_aas_bspq2.h"
#include "be_aas_entity.h"
#include "be_aas_main.h"
#include "be_aas_move.h"
#include "be_aas_reach.h"
#include "be_aas_route.h"
#include "be_ai_move.h"
#include "be_ai_weight.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_precomp.h"
#include "l_script.h"
#include "l_struct.h"
#include "l_utils.h"

levelitem_t *freelevelitems; // 0x10064344 free-list head    (be_ai_goal.c; was dword_10064344)
int numlevelitems;           // 0x10064354 active item count  (be_ai_goal.c; was dword_10064354)
levelitem_t *levelitemheap;  // 0x10064358 pool base          (be_ai_goal.c; was dword_10064358)
itemconfig_t *itemconfig; /* current item config (was dword_1006435C) */
levelitem_t *levelitems;     // 0x10064360 active-list head   (be_ai_goal.c; was dword_10064360)

//----- (1002ED20) --------------------------------------------------------
itemconfig_t * LoadItemConfig(char *filename)
{
  int max_iteminfo;
  source_t *src;
  itemconfig_t *cfg;
  iteminfo_t *item;
  bot_fileref_t file_ref; /* original bot_fileref_t local */
  char Destination[144]; // [esp+ACh] [ebp-4C0h] BYREF
  char ArgList[sizeof(token_t)] __attribute__((aligned(8))); // [esp+13Ch] [ebp-430h] BYREF

  max_iteminfo = (int)LibVarValue("max_iteminfo", (char *)"256");
  if ( max_iteminfo < 0 )
  {
    botimport.Print(PRT_ERROR, "max_iteminfo = %d\n", max_iteminfo);
    max_iteminfo = 256;
    LibVarSet("max_iteminfo", (char *)"256");
  }
  memset(&file_ref, 0, sizeof(file_ref));
  strncpy(Destination, filename, 0x90u);
  if ( !sub_10041F60(Destination, &file_ref) )
  {
    botimport.Print(PRT_ERROR, "couldn't find %s\n", Destination);
    return 0;
  }
  src = LoadSourceFile(file_ref.path, file_ref.fileofs, file_ref.filelen);
  if ( !src )
  {
    botimport.Print(PRT_ERROR, "counldn't load %s\n", Destination);
    return 0;
  }
  cfg = (itemconfig_t *)GetClearedMemory(sizeof(itemconfig_t) + sizeof(iteminfo_t) * max_iteminfo);
  cfg->numitems = 0;
  cfg->items    = (iteminfo_t *)(cfg + 1);
  /* Same shape as the sibling LoadWeaponConfig: a plain top-tested
   * `while (PC_ReadTokenHandle(...))`, every error path written out INLINE with
   * no shared label, and the unknown-definition case as the trailing `else`.
   * An `if(read){ do{…}while(read); }` form makes MSVC6 peel the strcmp. */
  while ( PC_ReadTokenHandle(src, ArgList) )
  {
    if ( !strcmp(ArgList, "iteminfo") )
    {
      if ( cfg->numitems >= max_iteminfo )
      {
        SourceError(src, "more than %d item info defined\n", max_iteminfo);
        FreeMemory(cfg);
        FreeSource(src);
        return 0;
      }
      item = &cfg->items[cfg->numitems];
      memset(item, 0, sizeof(iteminfo_t));
      if ( !PC_ExpectTokenType(src, 1, 0, ArgList) )
      {
        FreeMemory(cfg);
        FreeMemory(src);
        return 0;
      }
      StripDoubleQuotes(ArgList);
      strncpy(item->dispname, ArgList, 80);
      if ( !ReadStructure(src, &iteminfo_struct, item) )
      {
        FreeMemory(cfg);
        FreeSource(src);
        return 0;
      }
      item->number = cfg->numitems++;
    }
    else
    {
      SourceError(src, "unknown definition %s\n", ArgList);
      FreeMemory(cfg);
      FreeSource(src);
      return 0;
    }
  }
  FreeSource(src);
  if ( !cfg->numitems )
    botimport.Print(PRT_WARNING, "no item info loaded\n");
  if ( file_ref.filelen )
    botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", file_ref.path, Destination);
  else
    botimport.Print(PRT_MESSAGE, "loaded %s\n", Destination);
  return cfg;
}
//----- (1002F100) --------------------------------------------------------
int *__cdecl ItemWeightIndex(weightconfig_t *iwc, itemconfig_t *ic)
{
  int *index; // eax / [esp+10h] [ebp+8h]
  int i; // ebx

  index = (int *)GetClearedMemory(4 * ic->numitems);
  for ( i = 0; i < ic->numitems; ++i )
  {
    index[i] = FindFuzzyWeight(iwc, ic->items[i].dispname);
    if ( index[i] < 0 )
      Log_Write("item info %d \"%s\" has no fuzzy weight", i,
                ic->items[i].dispname);
  }
  return index;
}
//----- (1002F1A0) --------------------------------------------------------
void InitLevelItemHeap()
{
  int max_levelitems;
  int i;

  if ( levelitemheap )
    FreeMemory(levelitemheap);
  max_levelitems = (int)LibVarValue("max_levelitems", (char *)"512");
  levelitemheap = (levelitem_t *)GetMemory(sizeof(levelitem_t) * max_levelitems);
  /* ONE textual tail: MSVC6 /O2 duplicates it so the loop-skipped path can reuse
   * the still-live GetMemory result while the post-loop path reloads the global.
   * Writing both tails out explicitly makes them identical and they get
   * tail-merged back into one. */
  for ( i = 0; i < max_levelitems - 2; ++i )
    levelitemheap[i].next = &levelitemheap[i + 1];
  levelitemheap[max_levelitems - 1].next = NULL;
  freelevelitems = levelitemheap;
}
//----- (1002F270) --------------------------------------------------------
_DWORD *__cdecl AllocLevelItem(void)
{
  levelitem_t *result;

  result = freelevelitems;
  if ( !freelevelitems )
  {
    botimport.Print(PRT_FATAL, "out of level items\n");
    return 0;
  }
  freelevelitems = result->next;
  return (_DWORD *)result;
}
//----- (1002F2B0) --------------------------------------------------------
void __cdecl FreeLevelItem(levelitem_t *li)
{
  li->next = freelevelitems;
  freelevelitems = li;
}
//----- (1002F2E0) --------------------------------------------------------
levelitem_t *__cdecl AddLevelItemToList(levelitem_t *li)
{
  if ( levelitems )
    levelitems->prev = li;
  li->prev = 0;
  li->next = levelitems;
  levelitems = li;
  return li;
}
//----- (1002F320) --------------------------------------------------------
levelitem_t *__cdecl RemoveLevelItemFromList(levelitem_t *li)
{
  levelitem_t *prev;
  levelitem_t *next;

  prev = li->prev;
  if ( prev )
    prev->next = li->next;
  else
    levelitems = li->next;
  next = li->next;
  if ( next )
    next->prev = li->prev;
  return li;
}
//----- (1002F360) --------------------------------------------------------
void BotInitLevelItems()
{
  itemconfig_t *ic; // ebx
  bsp_entity_t *v4; // ebp
  int i; // edi
  int v7; // ebp
  const char *classname; // [esp+28h] [ebp-18h]
  bsp_entity_t *ent; // [esp+2Ch] [ebp-14h]
  /* The item origin must be a real vec3_t — it is passed by address to
   * AAS_VectorForBSPEpairKey, AAS_DropToFloor and AAS_BestReachableArea, and
   * split into separate locals nearly every item ends up with areanum 0. */
  int notspawnflags_mask; // [esp+18h] (LibVar("notspawnflags","2048") return value)
  vec3_t origin; // [esp+34h] [ebp-Ch] BYREF (was v12+v13+v14)

  InitLevelItemHeap();
  ic = itemconfig;
  levelitems = 0;
  numlevelitems = 0;
  if ( ic )
  {
    v4 = AAS_ParseBSPEntities();
    notspawnflags_mask = (int)LibVarValue("notspawnflags", (char *)"2048");
    for ( i = 0; i < ic->numitems; ++i )
    {
      ic->items[i].modelindex = IndexFromModel(ic->items[i].model);
      if ( !ic->items[i].modelindex )
        Log_Write("item %s has modelindex 0", ic->items[i].dispname);
    }
    ent = v4;
    if ( v4 )
    {
      do
      {
        classname = (const char *)AAS_ValueForBSPEpairKey(ent, "classname");
        if ( classname && (AAS_IntForBSPEpairKey(ent, "spawnflags") & notspawnflags_mask) == 0 )
        {
          for ( v7 = 0; v7 < ic->numitems; ++v7 )
          {
            if ( !strcmp(classname, ic->items[v7].dispname) )
            {
              if ( AAS_VectorForBSPEpairKey(ent, "origin", origin) )
              {
                levelitem_t *li = (levelitem_t *)AllocLevelItem();
                if ( !li )
                  goto done;
                li->number = ++numlevelitems;
                li->timeout = 0.0f;
                li->entitynum = 0;
                if ( !AAS_DropToFloor(origin, ic->items[v7].mins, ic->items[v7].maxs) )
                  botimport.Print(PRT_MESSAGE, "%s in solid at (%1.1f %1.1f %1.1f)\n", classname, origin[0], origin[1], origin[2]);
                li->iteminfo = v7;
                VectorCopy(origin, li->origin);
                li->areanum = AAS_BestReachableArea(
                                         (int *)origin,
                                         ic->items[v7].mins,
                                         ic->items[v7].maxs,
                                         li->goalorigin);
                AddLevelItemToList(li);
              }
              else
              {
                botimport.Print(PRT_ERROR, "item %s without origin\n", classname);
              }
              break;
            }
          }
          if ( v7 >= ic->numitems )
            Log_Write("entity %s unkown item", classname);
        }
        ent = ent->next;
      }
      while ( ent );
    }
    botimport.Print(PRT_MESSAGE, "found %d level items\n", numlevelitems);
  }
  // Single shared exit: the function is void, so the !itemconfig, alloc-fail and
  // normal paths all fall into ONE epilogue.  A return value would give each
  // `return` its own epilogue.
done:
  ;
}
//----- (1002F6A0) --------------------------------------------------------
char *__cdecl BotGoalName(int number)
{
  levelitem_t *v1;

  if ( !itemconfig )
    return &byte_1006294C;
  for ( v1 = levelitems; v1; v1 = v1->next )
  {
    if ( v1->number == number )
      return itemconfig->items[v1->iteminfo].name;
  }
  return &byte_1006294C;
}
//----- (1002F6F0) --------------------------------------------------------
int __cdecl BotResetAvoidGoals(bot_goalstate_t *goalstate)
{
  int result; // eax

  result = 0;
  memset(goalstate->avoidgoals, 0, sizeof(goalstate->avoidgoals));
  memset(goalstate->avoidgoaltimes, 0, sizeof(goalstate->avoidgoaltimes));
  return result;
}
//----- (1002F730) --------------------------------------------------------
void __cdecl BotDumpAvoidGoals(bot_goalstate_t *goalstate)
{
  int i;
  int n;

  i = 0;
  n = 64;
  do
  {
    if ( goalstate->avoidgoaltimes[i] >= AAS_Time() )
    {
      Log_Write("avoid goal %s, number %d for %f seconds", BotGoalName(goalstate->avoidgoals[i]),
                goalstate->avoidgoals[i], goalstate->avoidgoaltimes[i] - AAS_Time());
    }
    i++;
  }
  while ( --n );
}
//----- (1002F7B0) --------------------------------------------------------
void __cdecl BotAddToAvoidGoals(bot_goalstate_t *gs, int number, float avoidtime)
{
  int v3; // esi

  for ( v3 = 0; v3 < 64; ++v3 )
  {
    if ( AAS_Time() > gs->avoidgoaltimes[v3] )
    {
      gs->avoidgoals[v3] = number;
      gs->avoidgoaltimes[v3] = AAS_Time() + avoidtime;
      return;
    }
  }
}
//----- (1002F820) --------------------------------------------------------
float __cdecl BotAvoidGoalTime(bot_goalstate_t *goalstate, int number)
{
  int v2; // esi

  v2 = 0;
  while ( 1 )
  {
    if ( goalstate->avoidgoals[v2] == number && AAS_Time() <= goalstate->avoidgoaltimes[v2] )
      break;
    ++v2;
    if ( v2 >= 64 )
      return 0.0f;
  }
  return goalstate->avoidgoaltimes[v2] - AAS_Time();
}
//----- (1002F890) --------------------------------------------------------
int __cdecl BotGetLevelItemGoal(int index, char *name, bot_goal_t *goal)
{
  levelitem_t *li;

  if ( !itemconfig )
    goto notfound;
  li = levelitems;
  if ( !li )
    goto notfound;
  while ( 1 )
  {
    if ( li->number > index && !_strcmpi(name, itemconfig->items[li->iteminfo].name) )
      break;
    li = li->next;
    if ( !li )
      goto notfound;
  }
  goal->areanum   = li->areanum;
  VectorCopy(li->goalorigin, goal->origin);
  goal->entitynum = li->entitynum;
  VectorCopy(itemconfig->items[li->iteminfo].mins, goal->mins);
  VectorCopy(itemconfig->items[li->iteminfo].maxs, goal->maxs);
  goal->number    = li->number;
  return li->number;
notfound:
  return -1;
}
//----- (1002FA20) --------------------------------------------------------
void BotUpdateEntityItems(void)
{
  levelitem_t *v0;
  levelitem_t *nextli;
  int ent; // ebp
  int v4; // ebx
  levelitem_t *li;
  int v7; // eax
  levelitem_t *v13;
  itemconfig_t *ic; // [esp+Ch] [ebp-10Ch]
  int v19; // [esp+10h] [ebp-108h]
  int modelindex; // [esp+10h] [ebp-108h]
  vec3_t dir; // [esp+14h] [ebp-104h] BYREF
  float entinfo[31]; // [esp+20h] [ebp-F8h] BYREF

  v0 = levelitems;
  if ( v0 )
  {
    do
    {
      nextli = v0->next;
      if ( v0->timeout != 0.0f && AAS_Time() > v0->timeout )
      {
        RemoveLevelItemFromList(v0);
        FreeLevelItem(v0);
      }
      v0 = nextli;
    }
    while ( nextli );
  }
  ic = itemconfig;
  if ( !itemconfig )
    return;
  ent = AAS_NextBSPEntity(0);
  if ( !ent )
    return;
  v4 = v19;
  do
  {
    modelindex = AAS_EntityModelindex(ent);
    if ( !modelindex )
      goto LABEL_31;
    *(aas_entityinfo_t *)entinfo = AAS_EntityInfo(ent);
    if ( entinfo[4] != entinfo[13] || entinfo[5] != entinfo[14] || entinfo[6] != entinfo[15] )
      goto LABEL_31;
    li = levelitems;
    if ( !li )
      goto LABEL_24;
    while ( 1 )
    {
      if ( ic->items[li->iteminfo].modelindex != modelindex )
        goto LABEL_19;
      v7 = li->entitynum;
      if ( !v7 )
        break;
      if ( v7 == ent )
      {
        li->origin[0] = entinfo[4];
        li->origin[1] = entinfo[5];
        li->origin[2] = entinfo[6];
        goto LABEL_23;
      }
LABEL_19:
      li = li->next;
      if ( !li )
        goto LABEL_25;
    }
    dir[0] = li->origin[0] - entinfo[4];
    dir[1] = li->origin[1] - entinfo[5];
    dir[2] = li->origin[2] - entinfo[6];
    if ( VectorLength(dir) >= 20.0f )
      goto LABEL_19;
    li->origin[1] = entinfo[5];
    li->entitynum = ent;
    li->origin[0] = entinfo[4];
    li->origin[2] = entinfo[6];
    li->areanum = AAS_BestReachableArea(
              (int *)li->origin,
              ic->items[v4].mins,
              ic->items[v4].maxs,
              li->goalorigin);
LABEL_23:
    if ( li )
      goto LABEL_31;
LABEL_24:
LABEL_25:
    for ( v4 = 0; v4 < ic->numitems; ++v4 )
    {
      if ( ic->items[v4].modelindex == modelindex )
        break;
    }
    if ( v4 >= ic->numitems )
      goto LABEL_31;
    v13 = (levelitem_t *)AllocLevelItem();
    v13->entitynum = ent;
    v13->number = numlevelitems + ent;
    v13->origin[0] = entinfo[4];
    v13->origin[1] = entinfo[5];
    v13->origin[2] = entinfo[6];
    v13->iteminfo = v4;
    v13->areanum = AAS_BestReachableArea(
                              (int *)v13->origin,
                              ic->items[v4].mins,
                              ic->items[v4].maxs,
                              v13->goalorigin);
    v13->timeout = AAS_Time() + 30.0f;
    AddLevelItemToList(v13);
LABEL_31:
    ent = AAS_NextBSPEntity(ent);
  }
  while ( ent );
}
//----- (1002FD40) --------------------------------------------------------
void __cdecl BotDumpGoalStack(bot_goalstate_t *goalstate)
{
  int i; // esi

  i = 1;
  if ( goalstate->goalstacktop >= 1 )
  {
    do
    {
      Log_Write("%d: %s", i, BotGoalName(goalstate->goalstack[i].number));
      ++i;
    }
    while ( i <= goalstate->goalstacktop );
  }
}
//----- (1002FD90) --------------------------------------------------------
int __cdecl BotPushGoal(bot_goalstate_t *goalstate, const void *goal)
{
  int v2; // eax
  int result; // eax

  v2 = goalstate->goalstacktop;
  if ( v2 >= 7 )
  {
    botimport.Print(PRT_ERROR, "goal heap overflow\n");
    return ((int (__cdecl *)(bot_goalstate_t *))BotDumpGoalStack)(goalstate);
  }
  result = v2 + 1;
  goalstate->goalstacktop = result;
  memcpy(&goalstate->goalstack[result], goal, sizeof(bot_goal_t));
  return result;
}
//----- (1002FE00) --------------------------------------------------------
int __cdecl BotPopGoal(bot_goalstate_t *goalstate)
{
  int result; // eax

  result = goalstate->goalstacktop;
  if ( result > 0 )
    goalstate->goalstacktop = --result;
  return result;
}
//----- (1002FE30) --------------------------------------------------------
void __cdecl BotEmptyGoalStack(bot_goalstate_t *goalstate)
{
  goalstate->goalstacktop = 0;
}
//----- (1002FE50) --------------------------------------------------------
void *__cdecl BotGetTopGoal(bot_goalstate_t *goalstate)
{
  int result; // eax

  result = goalstate->goalstacktop;
  if ( !result )
    return (void *)(intptr_t)result;
  return &goalstate->goalstack[result];
}
//----- (1002FE80) --------------------------------------------------------
void *__cdecl BotGetSecondGoal(bot_goalstate_t *goalstate)
{
  int v1; // eax

  v1 = goalstate->goalstacktop;
  if ( v1 <= 1 )
    return 0;
  return &goalstate->goalstack[v1 - 1];
}
//----- (1002FEB0) --------------------------------------------------------
int __cdecl BotChooseLTGItem(bot_goalstate_t *goalstate, vec3_t origin, char *inventory, int travelflags)
{
  levelitem_t *li; // esi (was v8)
  int v9; // ebx
  iteminfo_t *iteminfo; // edi (was _DWORD *) - 64-bit fix
  int weightnum; // eax
  float weight; // st7 (was double)
  float v13; // st7 (was double)
  levelitem_t *bestitem; // [esp+10h] [ebp-48h]
  int areanum; // [esp+14h] [ebp-44h]
  itemconfig_t *ic; // [esp+18h] [ebp-40h]  - 64-bit fix (was int)
  int t; // [esp+1Ch] [ebp-3Ch]
  bot_goal_t goal; // [esp+20h] [ebp-38h] BYREF
  float v19; // [esp+5Ch] [ebp+4h]
  float bestweight; // [esp+60h] [ebp+8h]
  float avoidtime; // [esp+68h] [ebp+10h]
  /* 64-bit fix: see BotChooseNBGItem — itemweightconfig/itemweightindex live in sideband. */
#if BOTLIB_NEED_SIDEBAND
  bot_state_t *bs = (bot_state_t *)((char *)goalstate - offsetof(bot_state_t, goalstate));
  weightconfig_t *p0 = (weightconfig_t *)BotGoalP0(bs);
  int *p1 = (int *)BotGoalP1(bs);
#define LTG_IWC p0
#define LTG_IWI p1
#else
#define LTG_IWC (*(weightconfig_t **)&goalstate->itemweightconfig)
#define LTG_IWI (*(int **)&goalstate->itemweightindex)
#endif

  if ( !LTG_IWC )
    return 0;
  areanum = BotReachabilityArea(origin, !AAS_Swimming(origin));
  if ( !areanum )
    return 0;
  if ( !AAS_AreaReachability(areanum) )
    return 0;
  ic = itemconfig;
  if ( !itemconfig )
    return 0;
  li = levelitems;
  bestweight = 0.0;
  bestitem = 0;
  memset(&goal, 0, sizeof(goal));
  if ( li )
  {
    do
    {
      if ( BotAvoidGoalTime(goalstate, li->number) <= 0.0f )
      {
        v9 = li->areanum;
        if ( v9 )
        {
          iteminfo = &ic->items[li->iteminfo];
          weightnum = LTG_IWI[iteminfo->number];
          if ( weightnum >= 0 )
          {
            weight = FuzzyWeightUndecided(inventory, &LTG_IWC->weights[weightnum]);
            if ( weight > 0.0f )
            {
              t = (unsigned __int16)AAS_AreaTravelTimeToGoalArea(areanum, v9, travelflags);
              if ( t > 0 )
              {
                v19 = weight;
                v13 = v19 / ((float)t * 0.01);
                if ( li->timeout != 0.0f )
                  v13 = v13 + 20.0f;
                if ( v13 > bestweight )
                {
                  bestitem = li;
                  VectorCopy(li->goalorigin, goal.origin);
                  VectorCopy(iteminfo->mins, goal.mins);
                  VectorCopy(iteminfo->maxs, goal.maxs);
                  goal.areanum = v9;
                  bestweight = v13;
                  goal.entitynum = li->entitynum;
                  goal.number = li->number;
                  goal.flags = 1;
                  goal.iteminfo = li->iteminfo;
                }
              }
            }
          }
        }
      }
      li = li->next;
    }
    while ( li );
  }
  if ( !bestitem )
  {
    /* Thunk 0x10001A64 goes to the 4-arg AAS_RandomGoalArea, NOT to
     * BotFindWayPoint. */
    if ( AAS_RandomGoalArea(areanum, travelflags, (_DWORD *)&goal.areanum, goal.origin) )
    {
      goal.mins[0] = -15;
      goal.mins[1] = -15;
      goal.mins[2] = -15;
      goal.maxs[0] = 15;
      goal.maxs[1] = 15;
      goal.maxs[2] = 15;
      goal.entitynum = 0;
      goal.number = 0;
      goal.flags = 2;
      goal.iteminfo = 0;
      BotPushGoal(goalstate, &goal);
      return 1;
    }
    return 0;
  }
  avoidtime = ic->items[bestitem->iteminfo].respawntime;
  if ( avoidtime == 0.0f )
    avoidtime = 30.0f;
  BotAddToAvoidGoals(goalstate, bestitem->number, avoidtime);
  BotPushGoal(goalstate, &goal);
  return 1;
}
#undef LTG_IWC
#undef LTG_IWI
//----- (10030260) --------------------------------------------------------
int __cdecl BotChooseNBGItem(bot_goalstate_t *goalstate, vec3_t origin, char *inventory, int travelflags, bot_goal_t *ltg, float maxtime)
{
  int v9; // esi
  levelitem_t *li; // esi (was v10)
  int v11; // ebx
  iteminfo_t *iteminfo; // edi (was _DWORD *) - 64-bit fix
  int weightnum; // eax
  float weight; // st7
  float v15; // st7
  int v16; // eax
  float bestweight; // [esp+Ch] [ebp-4Ch]
  levelitem_t *bestitem; // [esp+10h] [ebp-48h]
  itemconfig_t *ic; // 64-bit fix - typed copy of itemconfig
  int areanum; // [esp+18h] [ebp-40h]
  int t; // [esp+1Ch] [ebp-3Ch]
  bot_goal_t goal; // [esp+20h] [ebp-38h] BYREF
  int ltg_time; // [esp+60h] [ebp+8h]
  float avoidtime; // [esp+68h] [ebp+10h]
  /* 64-bit fix: see BotChooseLTGItem — itemweightconfig/itemweightindex live in sideband. */
#if BOTLIB_NEED_SIDEBAND
  bot_state_t *bs = (bot_state_t *)((char *)goalstate - offsetof(bot_state_t, goalstate));
  weightconfig_t *p0 = (weightconfig_t *)BotGoalP0(bs);
  int *p1 = (int *)BotGoalP1(bs);
#define NBG_IWC p0
#define NBG_IWI p1
#else
#define NBG_IWC (*(weightconfig_t **)&goalstate->itemweightconfig)
#define NBG_IWI (*(int **)&goalstate->itemweightindex)
#endif

  if ( !NBG_IWC )
    return 0;
  areanum = BotReachabilityArea(origin, !AAS_Swimming(origin));
  v9 = areanum;
  if ( !areanum )
    return 0;
  if ( !AAS_AreaReachability(areanum) )
    return 0;
  ltg_time = ltg ? (unsigned __int16)AAS_AreaTravelTimeToGoalArea(v9, ltg->areanum, travelflags) : 99999;
  ic = itemconfig;
  if ( !ic )
    return 0;
  li = levelitems;
  bestweight = 0.0;
  bestitem = 0;
  memset(&goal, 0, sizeof(goal));
  for ( ; li; li = li->next )
  {
          if ( BotAvoidGoalTime(goalstate, li->number) <= 0.0f )
          {
            v11 = li->areanum;
            if ( v11 )
            {
              iteminfo = &ic->items[li->iteminfo];
              weightnum = NBG_IWI[iteminfo->number];
              if ( weightnum >= 0 )
              {
                weight = FuzzyWeightUndecided(inventory, &NBG_IWC->weights[weightnum]);
                if ( weight > 0.0f )
                {
                  t = (unsigned __int16)AAS_AreaTravelTimeToGoalArea(areanum, v11, travelflags);
                  if ( t > 0 )
                  {
                    v15 = (float)t;
                    if ( v15 < maxtime )
                    {
                      weight = weight / (v15 * 0.01);
                      if ( li->timeout != 0.0f )
                        weight = weight + 20.0f;
                      if ( weight > bestweight )
                      {
                        v16 = 0;
                        if ( ltg )
                          v16 = (unsigned __int16)AAS_AreaTravelTimeToGoalArea(v11, ltg->areanum, travelflags);
                        if ( v16 <= ltg_time )
                        {
                          /* The two scalar stores are NOT interleaved into the vec copy in
                           * the source — the original's interleaved emission is
                           * MSVC6 filling load-ahead bubbles, since both read the
                           * same `li`/`weight` the copy uses. */
                          bestweight = weight;
                          bestitem = li;
                          VectorCopy(li->goalorigin, goal.origin);
                          VectorCopy(iteminfo->mins, goal.mins);
                          VectorCopy(iteminfo->maxs, goal.maxs);
                          goal.areanum = v11;
                          goal.entitynum = li->entitynum;
                          goal.number = li->number;
                          goal.flags = 1;
                          goal.iteminfo = li->iteminfo;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
  }
  if ( !bestitem )
    return 0;
  avoidtime = ic->items[bestitem->iteminfo].respawntime;
  if ( avoidtime == 0.0f )
    avoidtime = 30.0f;
  BotAddToAvoidGoals(goalstate, bestitem->number, avoidtime);
  BotPushGoal(goalstate, &goal);
  return 1;
}
#undef NBG_IWC
#undef NBG_IWI
//----- (10030600) --------------------------------------------------------
int __cdecl BotTouchingGoal(vec3_t origin, float *goal)
{
  int i; // edx
  vec3_t boxmins;     // [esp+14h] [ebp-24h] BYREF
  vec3_t boxmaxs;     // [esp+20h] [ebp-18h] BYREF
  vec3_t absmins;     // [esp+2Ch] [ebp-Ch] BYREF
  vec3_t absmaxs;     // merged by MSVC6 with the same stack home pattern as Q3
  vec3_t safety_maxs; // keep the Gladiator binary's safety shrink values, not Q3's later zeroes
  vec3_t safety_mins;

  safety_maxs[0] = 4.0f;
  safety_maxs[1] = 4.0f;
  safety_maxs[2] = 10.0f;
  safety_mins[0] = -4.0f;
  safety_mins[1] = -4.0f;
  safety_mins[2] = 0.0f;
  AAS_PresenceTypeBoundingBox(4, boxmins, boxmaxs);
  VectorSubtract((goal + 4), boxmaxs, absmins);
  VectorSubtract((goal + 7), boxmins, absmaxs);
  VectorAdd(absmins, goal, absmins);
  VectorAdd(absmaxs, goal, absmaxs);
  VectorSubtract(absmaxs, safety_maxs, absmaxs);
  VectorSubtract(absmins, safety_mins, absmins);
  for ( i = 0; i < 3; i++ )
  {
    if ( origin[i] < absmins[i] || origin[i] > absmaxs[i] )
      return 0;
  }
  return 1;
}
//----- (10030770) --------------------------------------------------------
/* BotItemGoalInVisButNotVisible — true when the goal item's bbox centre is
 * visible from the bot's eye but the goal entity is not currently being updated
 * by the engine.  `trace` and `entinfo` have non-overlapping lifetimes, so
 * MSVC6 coalesces them into one stack slot, reused after the fraction test.
 *
 * Three divergences from Q3, all faithful to the binary:
 *   - `entitynum <= 0` returns 1, where Q3 returns qfalse;
 *   - the "not updated" test is the older `if (!entinfo.valid)`, which survives
 *     commented out in Q3, whose live code uses `ltime < AAS_Time() - 0.5`;
 *   - the second VectorAdd is `middle + goal->origin` (Q3 has the operands the
 *     other way round), and contentmask is the literal 3.
 * `viewangles` is unused, present only to match the engine call signature. */
BOOL __cdecl BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, bot_goal_t *goal)
{
  aas_entityinfo_t entinfo; // [ebp-7Ch] BYREF — coalesced with `trace`
  bsp_trace_t trace;        // [ebp-7Ch] BYREF
  vec3_t middle;            // [ebp-88h] world-space goal centre

  if ( !(goal->flags & 1) )
    return 0;
  VectorAdd(goal->mins, goal->mins, middle);
  VectorScale(middle, 0.5, middle);
  VectorAdd(middle, goal->origin, middle);
  trace = AAS_Trace(eye, NULL, NULL, middle, viewer, 3);
  if ( trace.fraction >= 1.0f )
  {
    if ( goal->entitynum <= 0 )
      return 1;
    entinfo = AAS_EntityInfo(goal->entitynum);
    if ( !entinfo.valid )
      return 1;
  }
  return 0;
}
//----- (100308D0) --------------------------------------------------------
int __cdecl BotLoadItemWeights(bot_goalstate_t *goalstate, char *filename)
{
  weightconfig_t *v2;

  v2 = ReadWeightConfig(filename);
  BotGoalHandleP0(goalstate) = v2;
  if ( !v2 )
  {
    botimport.Print(PRT_FATAL, "couldn't load weights\n");
    return BLERR_CANNOTLOADITEMWEIGHTS;
  }
  if ( !itemconfig )
    return BLERR_CANNOTLOADITEMWEIGHTS;
  BotGoalHandleP1(goalstate) = ItemWeightIndex(v2, itemconfig);
  return BLERR_NOERROR;
}
//----- (10030950) --------------------------------------------------------
void __cdecl BotFreeItemWeights(bot_goalstate_t *goalstate)
{
  if ( BotGoalHandleP0(goalstate) )
    FreeWeightConfig2((weightconfig_t *)BotGoalHandleP0(goalstate));
  if ( BotGoalHandleP1(goalstate) )
    FreeMemory(BotGoalHandleP1(goalstate));
}
//----- (10030990) --------------------------------------------------------
int __cdecl BotResetGoalState(bot_goalstate_t *goalstate)
{
  memset(goalstate->goalstack, 0, sizeof(goalstate->goalstack));
  goalstate->goalstacktop = 0;
  return BotResetAvoidGoals(goalstate);
}
//----- (100309D0) --------------------------------------------------------
int BotSetupGoalAI()
{
  char *filename; // eax

  filename = LibVarString("itemconfig", (char *)"items.c");
  itemconfig = LoadItemConfig(filename);
  if ( !itemconfig )
  {
    botimport.Print(PRT_FATAL, "couldn't load item config\n");
    return BLERR_CANNOTLOADITEMCONFIG;
  }
  return BLERR_NOERROR;
}
//----- (10030A20) --------------------------------------------------------
int BotShutdownGoalAI()
{
  int result; // eax

  result = itemconfig;
  if ( itemconfig )
    result = FreeMemory(itemconfig);
  itemconfig = 0;
  return result;
}
