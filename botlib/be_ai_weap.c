/*
 * be_ai_weap.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10034BB0..0x100356D0.
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
#include "be_ai_weap.h"
#include "be_aas_main.h"
#include "be_ai_weight.h"
#include "be_ea.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_memory.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"

/* weaponinfo_struct — descriptor at 0x1005DFD8 (344 B); field table at 0x1005DBC8.
 * Deliberately NOT `static`: gladi386.so exports it as a `D` symbol
 * (0005bdb0 D weaponinfo_fields), which a file-static array can never be. */
char *weaponinfo_fields[] = {
    FE("name",            0x004, 0x004, 0, 0x00000000),
    FE("level",           0x0A4, 0x002, 0, 0x00000000),
    FE("model",           0x054, 0x004, 0, 0x00000000),
    FE("weaponindex",     0x0A8, 0x002, 0, 0x00000000),
    FE("flags",           0x0AC, 0x002, 0, 0x00000000),
    FE("projectile",      0x0B0, 0x004, 0, 0x00000000),
    FE("numprojectiles",  0x100, 0x002, 0, 0x00000000),
    FE("hspread",         0x104, 0x003, 0, 0x00000000),
    FE("vspread",         0x108, 0x003, 0, 0x00000000),
    FE("speed",           0x10C, 0x003, 0, 0x00000000),
    FE("acceleration",    0x110, 0x003, 0, 0x00000000),
    FE("recoil",          0x114, 0x103, 3, 0x00000000),  /* vec3 */
    FE("offset",          0x120, 0x103, 3, 0x00000000),  /* vec3 */
    FE("angleoffset",     0x12C, 0x103, 3, 0x00000000),  /* vec3 */
    FE("extrazvelocity",  0x138, 0x003, 0, 0x00000000),
    FE("ammoamount",      0x13C, 0x002, 0, 0x00000000),
    FE("ammoindex",       0x140, 0x002, 0, 0x00000000),
    FE("activate",        0x144, 0x003, 0, 0x00000000),
    FE("reload",          0x148, 0x003, 0, 0x00000000),
    FE("spinup",          0x14C, 0x003, 0, 0x00000000),
    FE("spindown",        0x150, 0x003, 0, 0x00000000),
    FE_END
};
structdef_t weaponinfo_struct = { 344, weaponinfo_fields };

/* projectileinfo_struct — descriptor at 0x1005DFE0 (208 B); field table at 0x1005DE30.
 * Deliberately NOT `static`: gladi386.so exports it as a `D` symbol
 * (0005c018 D projectileinfo_fields), which a file-static array can never be. */
char *projectileinfo_fields[] = {
    FE("name",        0x000, 0x004, 0, 0x00000000),
    FE("model",       0x054, 0x004, 0, 0x00000000),
    FE("flags",       0x0A0, 0x002, 0, 0x00000000),
    FE("gravity",     0x0A4, 0x003, 0, 0x00000000),
    FE("damage",      0x0A8, 0x002, 0, 0x00000000),
    FE("radius",      0x0AC, 0x003, 0, 0x00000000),
    FE("visdamage",   0x0B0, 0x002, 0, 0x00000000),
    FE("damagetype",  0x0B4, 0x002, 0, 0x00000000),
    FE("healthinc",   0x0B8, 0x002, 0, 0x00000000),
    FE("push",        0x0BC, 0x003, 0, 0x00000000),
    FE("detonation",  0x0C0, 0x003, 0, 0x00000000),
    FE("bounce",      0x0C4, 0x003, 0, 0x00000000),
    FE("bouncefric",  0x0C8, 0x003, 0, 0x00000000),
    FE("bouncestop",  0x0CC, 0x003, 0, 0x00000000),
    FE_END
};
structdef_t projectileinfo_struct = { 208, projectileinfo_fields };

weaponconfig_t *weaponconfig; /* current weapon config (was dword_10064080) */

// gladiator.dll: 10034BB0..10035111
// gladi386.so:   00045AA8..00046020
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

  /* Thunk 0x10001AAF -> LibVarValue, which returns the float value in ST(0) — not
   * LibVar, whose libvar_t* would be used as a count by the allocation below. */
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
  wc->weaponinfo      = (weaponinfo_t *)(wc + 1);
  wc->projectileinfo  = (projectileinfo_t *)(wc->weaponinfo + max_weaponinfo);
  wc->numweapons      = 0;
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
// gladiator.dll: 10035280..100352D6
// gladi386.so:   00046020..0004609B
_DWORD *__cdecl WeaponWeightIndex(weightconfig_t *wwc, weaponconfig_t *wc)
{
  _DWORD *result;
  int i;

  result = (_DWORD *)GetClearedMemory(4 * wc->numweapons);
  for ( i = 0; i < wc->numweapons; ++i )
    result[i] = FindFuzzyWeight(wwc, wc->weaponinfo[i].name);
  return result;
}
// gladiator.dll: 10035300..10035325
// gladi386.so:   0004609C..000460D1
void __cdecl BotFreeWeaponWeights(bot_weaponstate_t *weaponstate)
{
  if ( weaponstate->weightconfig )
    FreeWeightConfig2((int)(intptr_t)weaponstate->weightconfig);
  if ( weaponstate->itemweights )
    FreeMemory(weaponstate->itemweights);
}
// gladiator.dll: 10035340..1003539D
// gladi386.so:   000460D4..000461E0
int __cdecl BotLoadWeaponWeights(bot_weaponstate_t *weaponstate, const char *filename)
{
  BotFreeWeaponWeights(weaponstate);
  weaponstate->weightconfig = ReadWeightConfig((char *)filename);
  if ( !weaponstate->weightconfig )
  {
    botimport.Print(PRT_FATAL, "couldn't load weapon config %s\n", filename);
    return BLERR_CANNOTLOADWEAPONWEIGHTS;
  }
  if ( !weaponconfig )
    return BLERR_CANNOTLOADWEAPONCONFIG;
  weaponstate->itemweights = WeaponWeightIndex(weaponstate->weightconfig, weaponconfig);
  return 0;
}
// gladiator.dll: 100353C0..1003540E
// gladi386.so:   000461E0..0004625A
/* Case-insensitive lookup of a weapon by model name, returning its `number`, or -1 if
 * the config is unloaded, empty or has no match.  Sibling of sub_10035430.  DEAD. */
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
// gladiator.dll: 10035430..10035481
// gladi386.so:   0004625C..000462DB
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
// gladiator.dll: 100354B0..100354E1
// gladi386.so:   000462DC..0004631C
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
// gladiator.dll: 10035500..100355FE
// gladi386.so:   0004631C..00046463
void __cdecl BotChooseBestFightWeapon(bot_weaponstate_t *ws)
{
  weaponconfig_t *wc; // ebp
  float bestweight;
  weaponinfo_t *bestweaponinfo; // edi
  int i; // ebx
  int index; // eax
  float weight; // st7

  bestweight = 0.0f;
  bestweaponinfo = 0;
  wc = weaponconfig;
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
              /* Both builds call FuzzyWeight here.  They differ only in whether the
               * compiler INLINED FuzzyWeight_r into it: gladiator.dll's FuzzyWeight is
               * the thin 36-byte wrapper, gladi386.so's is 231 bytes and 98% identical
               * to FuzzyWeight_r itself.  Same source, different inlining. */
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
// gladiator.dll: 10035640..10035662
// gladi386.so:   00046464..00046496
void __cdecl BotResetWeaponState(bot_weaponstate_t *weaponstate)
{
  weightconfig_t *weightconfig; // esi
  int *itemweights; // ebx

#if defined(__x86_64__) || defined(__aarch64__)
  /* 64-bit only: the side-band slot is NULL until BotSetupClient allocates it. */
  if ( !weaponstate ) return;
#endif
  weightconfig = weaponstate->weightconfig;
  itemweights = weaponstate->itemweights;
  memset(weaponstate, 0, sizeof(*weaponstate));
  weaponstate->weightconfig = weightconfig;
  weaponstate->itemweights = itemweights;
}
// gladiator.dll: 10035680..100356BA
// gladi386.so:   00046498..000464F4
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
// gladiator.dll: 100356D0..100356ED
// gladi386.so:   000464F4..00046524
void BotShutdownWeaponAI(void)
{
  if ( weaponconfig )
    FreeMemory(weaponconfig);
  weaponconfig = 0;
}
