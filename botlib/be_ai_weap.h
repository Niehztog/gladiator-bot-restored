/*
 * be_ai_weap.h — interface of be_ai_weap.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AI_WEAP_H
#define BOTLIB_BE_AI_WEAP_H

#include "botlib_local.h"

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
