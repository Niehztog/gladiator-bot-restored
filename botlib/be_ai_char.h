/*
 * be_ai_char.h — interface of be_ai_char.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AI_CHAR_H
#define BOTLIB_BE_AI_CHAR_H

#include "botlib_local.h"

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
