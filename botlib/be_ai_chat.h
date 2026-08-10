/*
 * be_ai_chat.h — interface of be_ai_chat.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AI_CHAT_H
#define BOTLIB_BE_AI_CHAT_H

#include "botlib_local.h"

bot_consolemessage_t *AllocConsoleMessage();
void __cdecl BotDumpSynonymList(int *synlist);
bot_synonymlist_t *__cdecl BotLoadSynonyms(char *filename);
bot_consolemessage_t *__cdecl BotNextConsoleMessage(bot_chatstate_t *cs);
int __cdecl BotNumConsoleMessages(bot_chatstate_t *chatstate);
int __cdecl BotQueueConsoleMessage(bot_chatstate_t *chatstate, int type, char *message);
int __cdecl BotRemoveConsoleMessage(bot_chatstate_t *chatstate, bot_consolemessage_t *msg);
int __cdecl FreeConsoleMessage(bot_consolemessage_t *message);
int InitConsoleMessageHeap();
BOOL __cdecl IsWhiteSpace(char c);
const char *__cdecl StringContains(const char *str1, const char *str2, int casesensitive);
const char *__cdecl StringContainsWord(const char *str1, const char *str2, int casesensitive);
void __cdecl StringReplaceWords(const char *string, const char *synonym, const char *replacement);
void __cdecl UnifyWhiteSpaces(void *string);

#endif /* BOTLIB_BE_AI_CHAT_H */
