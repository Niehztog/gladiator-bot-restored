/*
 * be_ai_char.h — interface of be_ai_char.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AI_CHAR_H
#define BOTLIB_BE_AI_CHAR_H

void __cdecl BotDumpCharacter(bot_character_t *ch);
bot_character_t *__cdecl BotLoadCharacter(char *charfile, const char *a2);
float __cdecl Characteristic_BFloat(bot_character_t *character, int index, float min, float max);
int __cdecl Characteristic_BInteger(bot_character_t *character, int index, int min, int max);
float __cdecl Characteristic_Float(bot_character_t * character, int index);
int __cdecl Characteristic_Integer(bot_character_t * character, int index);
char *__cdecl Characteristic_String(bot_character_t *character, int index);
int __cdecl CheckCharacteristicIndex(bot_character_t *character, int index);
void __cdecl sub_1002A590(int a1);

#endif /* BOTLIB_BE_AI_CHAR_H */
