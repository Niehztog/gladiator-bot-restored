/*
 * be_ai_weap.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10034BB0..0x100356D0; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * botlib_local.h carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs and every prototype), so this file compiles in
 * exactly the environment these functions had before the split.
 */

#include "botlib_local.h"

//----- (10034BB0) --------------------------------------------------------
weaponconfig_t * LoadWeaponConfig(char *filename)
{
  int max_weaponinfo, max_projectileinfo;
  token_t token;
  char path[144];                 /* MAX_PATH (144); literal kept — MAX_PATH macro not visible in this TU */
  int i, j;
  source_t *source;
  weaponconfig_t *wc;
  /* Gladiator-specific: Q2 locates the config inside a pak, so the file is
   * described by a (path, offset, length) triple rather than a bare name. */
  bot_fileref_t file_ref;

  /* Thunk 0x10001AAF -> LibVarValue, which returns the float value in ST(0) —
   * not LibVar, whose libvar_t* would be used as a count by the allocation
   * below. */
  max_weaponinfo = (int)LibVarValue("max_weaponinfo", "32");
  if ( max_weaponinfo < 0 )
  {
    botimport.Print(PRT_ERROR, "max_weaponinfo = %d\n", max_weaponinfo);
    max_weaponinfo = 32;
    LibVarSet("max_weaponinfo", "32");
  }
  max_projectileinfo = (int)LibVarValue("max_projectileinfo", "32");
  if ( max_projectileinfo < 0 )
  {
    botimport.Print(PRT_ERROR, "max_projectileinfo = %d\n", max_projectileinfo);
    max_projectileinfo = 32;
    LibVarSet("max_projectileinfo", "32");
  }
  memset(&file_ref, 0, sizeof(file_ref));
  strncpy(path, filename, 144u);
  if ( !sub_10041F60(path, &file_ref) )
  {
    botimport.Print(PRT_ERROR, "couldn't find %s\n", path);
    return 0;
  }
  source = LoadSourceFile(file_ref.path, file_ref.fileofs, file_ref.filelen);
  if ( !source )
  {
    botimport.Print(PRT_ERROR, "counldn't load %s\n", path);
    return 0;
  }
  //initialize weapon config
  wc = (weaponconfig_t *)GetClearedMemory(sizeof(weaponconfig_t) +
                                          max_weaponinfo * sizeof(weaponinfo_t) +
                                          max_projectileinfo * sizeof(projectileinfo_t));
  wc->numweapons      = 0;
  wc->weaponinfo      = (weaponinfo_t *)(wc + 1);
  wc->projectileinfo  = (projectileinfo_t *)(wc->weaponinfo + max_weaponinfo);
  wc->numprojectiles  = 0;
  //parse the source file
  while ( PC_ReadTokenHandle(source, token.string) )
  {
    if ( !strcmp(token.string, "weaponinfo") )
    {
      if ( wc->numweapons >= max_weaponinfo )
      {
        botimport.Print(PRT_ERROR, "more than %d weapons defined in %s\n", max_weaponinfo, path);
        FreeMemory(wc);
        FreeSource(source);
        return 0;
      }
      memset(&wc->weaponinfo[wc->numweapons], 0, sizeof(weaponinfo_t));
      if ( !ReadStructure(source, &weaponinfo_struct, &wc->weaponinfo[wc->numweapons]) )
      {
        FreeMemory(wc);
        FreeSource(source);
        return 0;
      }
      ++wc->numweapons;
    }
    else if ( !strcmp(token.string, "projectileinfo") )
    {
      if ( wc->numprojectiles >= max_projectileinfo )
      {
        botimport.Print(PRT_ERROR, "more than %d projectiles defined in %s\n", max_projectileinfo, path);
        FreeMemory(wc);
        FreeSource(source);
        return 0;
      }
      memset(&wc->projectileinfo[wc->numprojectiles], 0, sizeof(projectileinfo_t));
      if ( !ReadStructure(source, &projectileinfo_struct, &wc->projectileinfo[wc->numprojectiles]) )
      {
        FreeMemory(wc);
        FreeSource(source);
        return 0;
      }
      ++wc->numprojectiles;
    }
    else
    {
      botimport.Print(PRT_ERROR, "unknown definition %s in %s\n", token.string, path);
      FreeMemory(wc);
      FreeSource(source);
      return 0;
    }
  }
  FreeSource(source);
  //fix up weapons
  for ( i = 0; i < wc->numweapons; ++i )
  {
    if ( !wc->weaponinfo[i].name[0] )
    {
      botimport.Print(PRT_ERROR, "weapon %d has no name in %s\n", i, path);
      FreeMemory(wc);
      return 0;
    }
    if ( !wc->weaponinfo[i].projectile[0] )
    {
      botimport.Print(PRT_ERROR, "weapon %s has no projectile in %s\n", wc->weaponinfo[i].name, path);
      FreeMemory(wc);
      return 0;
    }
    //find the projectile info and store a pointer to it in the weapon info
    for ( j = 0; j < wc->numprojectiles; ++j )
    {
      if ( !strcmp(wc->projectileinfo[j].name, wc->weaponinfo[i].projectile) )
      {
        wc->weaponinfo[i].proj = &wc->projectileinfo[j];
        break;
      }
    }
    if ( j == wc->numprojectiles )
    {
      botimport.Print(PRT_ERROR, "weapon %s uses undefined projectile in %s\n", wc->weaponinfo[i].name, path);
      FreeMemory(wc);
      return 0;
    }
    wc->weaponinfo[i].number = i;
  }
  if ( !wc->numweapons )
    botimport.Print(PRT_WARNING, "no weapon info loaded\n");
  if ( file_ref.filelen )
    botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", file_ref.path, filename);
  else
    botimport.Print(PRT_MESSAGE, "loaded %s\n", path);
  return wc;
}
//----- (10035280) --------------------------------------------------------
_DWORD *__cdecl WeaponWeightIndex(weightconfig_t *wwc, weaponconfig_t *wc)
{
  _DWORD *result;
  int i;

  result = (_DWORD *)GetClearedMemory(4 * wc->numweapons);
  for ( i = 0; i < wc->numweapons; ++i )
    result[i] = FindFuzzyWeight(wwc, wc->weaponinfo[i].name);
  return result;
}
//----- (10035300) --------------------------------------------------------
void __cdecl BotFreeWeaponWeights(bot_weaponstate_t *weaponstate)
{
  if ( weaponstate->weightconfig )
    FreeWeightConfig2((int)(intptr_t)weaponstate->weightconfig);
  if ( weaponstate->itemweights )
    FreeMemory(weaponstate->itemweights);
}
//----- (10035340) --------------------------------------------------------
int __cdecl BotLoadWeaponWeights(bot_weaponstate_t *weaponstate, const char *filename)
{
  weightconfig_t *v2; // eax

  BotFreeWeaponWeights(weaponstate);
  v2 = ReadWeightConfig((char *)filename);
  weaponstate->weightconfig = v2;
  if ( !v2 )
  {
    botimport.Print(PRT_FATAL, "couldn't load weapon config %s\n", filename);
    return BLERR_CANNOTLOADWEAPONWEIGHTS;
  }
  if ( !weaponconfig )
    return BLERR_CANNOTLOADWEAPONCONFIG;
  weaponstate->itemweights = WeaponWeightIndex(v2, weaponconfig);
  return 0;
}
//----- (100353C0) --------------------------------------------------------
/* Case-insensitive lookup of a weapon by model name, returning its `number`,
 * or -1 if the config is unloaded, empty or has no match.  Sibling of
 * sub_10035430.  DEAD in Gladiator. */
int __cdecl sub_100353C0(const char *modelname)
{
  weaponconfig_t *cfg;
  weaponinfo_t   *w;
  int             i;

  cfg = weaponconfig;
  if ( !cfg )
    return -1;
  for ( i = 0; i < cfg->numweapons; i++ )
  {
    w = &cfg->weaponinfo[i];
    if ( !Q_stricmp(w->model, (char *)modelname) )
      return w->number;
  }
  return -1;
}
//----- (10035430) --------------------------------------------------------
/* As sub_100353C0, but returns the weapon's name, or "unknown weapon" on a
 * miss.  DEAD in Gladiator. */
const char *__cdecl sub_10035430(const char *modelname)
{
  static const char default_name[] = "unknown weapon"; /* @0x1005E3FC */
  weaponconfig_t *cfg;
  weaponinfo_t   *w;
  int             i;

  cfg = weaponconfig;
  if ( !cfg )
    return default_name;
  for ( i = 0; i < cfg->numweapons; i++ )
  {
    w = &cfg->weaponinfo[i];
    if ( !Q_stricmp(w->model, (char *)modelname) )
      return w->name;
  }
  return default_name;
}
//----- (100354B0) --------------------------------------------------------
weaponinfo_t *__cdecl sub_100354B0(bot_weaponstate_t *ws)
{
  int v1; // ecx

  v1 = ws->weaponindex;
  if ( v1 < 0 )
    return 0;
  if ( !weaponconfig )
    return 0;
  return &weaponconfig->weaponinfo[v1];
}
//----- (10035500) --------------------------------------------------------
void __cdecl BotChooseBestFightWeapon(bot_weaponstate_t *ws)
{
  weaponconfig_t *wc; // ebp
  weaponinfo_t *bestweaponinfo; // edi
  int i; // ebx
  int index; // eax
  float weight; // st7
  float bestweight; // [esp+10h] [ebp-4h]

  wc = weaponconfig;
  bestweight = 0.0f;
  bestweaponinfo = 0;
  if ( weaponconfig )
  {
    if ( AAS_Time() >= ws->nextthink )
    {
      if ( ws->weightconfig )
      {
        i = 0;
        if ( wc->numweapons > 0 )
        {
          do
          {
            index = ws->itemweights[i];
            if ( index >= 0 )
            {
              weight = FuzzyWeight(ws->inventory, &ws->weightconfig->weights[index]);
              if ( weight > bestweight )
              {
                bestweight = weight;
                bestweaponinfo = &wc->weaponinfo[i];
              }
            }
            ++i;
          }
          while ( i < wc->numweapons );
          if ( bestweaponinfo )
          {
            if ( Q_stricmp(bestweaponinfo->model, ws->modelname) )
            {
              EA_UseItem(ws->client, bestweaponinfo->name);
              ws->nextthink = AAS_Time() + wc->weaponinfo[bestweaponinfo->number].activate + 3.0f;
            }
            ws->modelname = bestweaponinfo->model;
            ws->weaponindex = bestweaponinfo->number;
          }
        }
      }
    }
  }
}
//----- (10035640) --------------------------------------------------------
int __cdecl BotResetWeaponState(bot_weaponstate_t *weaponstate)
{
  weightconfig_t *weightconfig; // esi
  int *itemweights; // ebx

#if defined(__x86_64__) || defined(__aarch64__)
  /* 64-bit only: the side-band slot is NULL until BotSetupClient allocates it. */
  if ( !weaponstate ) return 0;
#endif
  weightconfig = weaponstate->weightconfig;
  itemweights = weaponstate->itemweights;
  memset(weaponstate, 0, sizeof(*weaponstate));
  weaponstate->weightconfig = weightconfig;
  weaponstate->itemweights = itemweights;
  return 0;
}
//----- (10035680) --------------------------------------------------------
int BotSetupWeaponAI()
{

  char *file; // eax

  file = LibVarString("weaponconfig", "weapons.c");
  weaponconfig = LoadWeaponConfig(file);
  if ( !weaponconfig )
  {
    botimport.Print(PRT_FATAL, "couldn't load the weapon config\n");
    return BLERR_CANNOTLOADWEAPONCONFIG;
  }
  return BLERR_NOERROR;
}
//----- (100356D0) --------------------------------------------------------
int BotShutdownWeaponAI()
{
  int result; // eax

  result = weaponconfig;
  if ( weaponconfig )
    result = FreeMemory(weaponconfig);
  weaponconfig = 0;
  return result;
}
