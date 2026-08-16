/* be_ai_weap.h — interface of be_ai_weap.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AI_WEAP_H
#define BOTLIB_BE_AI_WEAP_H

/* Declarations for what this TU defines — last, so the types above are in scope. */
extern structdef_t weaponinfo_struct; /* weapon config structdef — defined in botlib_structdefs.c */
extern structdef_t projectileinfo_struct; /* projectile config structdef — defined in botlib_structdefs.c */
extern weaponconfig_t *weaponconfig;

void __cdecl BotChooseBestFightWeapon(bot_weaponstate_t *ws);
void __cdecl BotFreeWeaponWeights(bot_weaponstate_t *weaponstate);
int __cdecl BotLoadWeaponWeights(bot_weaponstate_t *weaponstate, const char *filename);
void __cdecl BotResetWeaponState(bot_weaponstate_t *weaponstate);
int BotSetupWeaponAI();
void BotShutdownWeaponAI(void);
weaponconfig_t *LoadWeaponConfig(char *filename);
_DWORD *__cdecl WeaponWeightIndex(weightconfig_t *wwc, weaponconfig_t *wc);
int __cdecl sub_100353C0(const char *modelname);
const char *__cdecl sub_10035430(const char *modelname);
weaponinfo_t *__cdecl sub_100354B0(bot_weaponstate_t *ws);

#endif /* BOTLIB_BE_AI_WEAP_H */
