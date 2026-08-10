/*
 * be_ai_weap.h — interface of be_ai_weap.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AI_WEAP_H
#define BOTLIB_BE_AI_WEAP_H

void __cdecl BotChooseBestFightWeapon(bot_weaponstate_t *ws);
void __cdecl BotFreeWeaponWeights(bot_weaponstate_t *weaponstate);
int __cdecl BotLoadWeaponWeights(bot_weaponstate_t *weaponstate, const char *filename);
int __cdecl BotResetWeaponState(bot_weaponstate_t *weaponstate);
int BotSetupWeaponAI();
int BotShutdownWeaponAI();
weaponconfig_t *LoadWeaponConfig(char *filename);
_DWORD *__cdecl WeaponWeightIndex(weightconfig_t *wwc, weaponconfig_t *wc);
int __cdecl sub_100353C0(const char *modelname);
const char *__cdecl sub_10035430(const char *modelname);
weaponinfo_t *__cdecl sub_100354B0(bot_weaponstate_t *ws);

#endif /* BOTLIB_BE_AI_WEAP_H */
