/*
 * be_ai_chat.h — interface of be_ai_chat.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AI_CHAT_H
#define BOTLIB_BE_AI_CHAT_H

/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
const char *__cdecl StringContains(const char *str1, const char *str2, int casesensitive);  /* 0x1002ACF0 — substring search */
extern bot_consolemessage_t *freeconsolemessages;
extern bot_consolemessage_t *consolemessageheap;
extern bot_matchtemplate_t *matchtemplates;
extern bot_randomlist_t *randomstrings;
extern bot_replychat_t *replychats;
extern bot_synonymlist_t *synonyms;

bot_consolemessage_t *AllocConsoleMessage();
unsigned int __cdecl BotChatLength(bot_chatstate_t *chatstate);
bot_stringlist_t *__cdecl BotCheckChatMessageIntegrety(const char *message, bot_stringlist_t *stringlist);
void __cdecl BotCheckInitialChatIntegrety(struct chatlist_s *chat);
void __cdecl BotCheckReplyChatIntegrety(bot_replychat_t *replychat);
char *__cdecl BotChooseInitialChatMessage(chatlist_t *cs, char *type);
void __cdecl BotConstructChatMessage(bot_chatstate_t *cs, const char *message, int mcontext, bot_chatvar_t *vars, int vcontext);
void __cdecl BotDumpInitialChat(chatlist_t *chat);
void __cdecl BotDumpMatchTemplates(void *matches);
void __cdecl BotDumpRandomStringList(int *randomlist);
void __cdecl BotDumpReplyChat(bot_replychat_t *replychat);
void __cdecl BotDumpSynonymList(int *synlist);
void __cdecl BotEnterChat(bot_chatstate_t *chatstate, int clientto, int sendto);
int __cdecl BotFindMatch(char *str, bot_match_t *match, int context);
bot_stringlist_t *__cdecl BotFindStringInList(bot_stringlist_t *list, const char *string);
int __cdecl BotFreeChatFile(bot_chatstate_t *chatstate);
int __cdecl BotFreeChatState(bot_chatstate_t *cs);
static void BotFreeChatTree(chatlist_t *list);
void __cdecl BotFreeMatchPieces(bot_matchpiece_t *matchpieces);
void __cdecl BotFreeMatchTemplates(bot_matchtemplate_t *mt);
void __cdecl BotFreeReplyChat(bot_replychat_t *replychat);
void __cdecl BotInitialChat(bot_chatstate_t *cs, char *type, ...);
int __cdecl BotLoadChatFile(bot_chatstate_t *chatstate, char *chatfile, char *chatname);
int __cdecl BotLoadChatMessage(source_t *source, char *chatmessagestring);
void *__cdecl BotLoadInitialChat(char *chatfile, char *chatname);
bot_matchpiece_t *__cdecl BotLoadMatchPieces(source_t *source, const char *endtoken);
bot_matchtemplate_t *__cdecl BotLoadMatchTemplates(char *matchfile);
bot_randomlist_t *__cdecl BotLoadRandomStrings(char *filename);
bot_replychat_t *__cdecl BotLoadReplyChat(char *filename);
bot_synonymlist_t *__cdecl BotLoadSynonyms(char *filename);
char *__cdecl BotMatchVariable(bot_match_t *match, int variable, char *buf);
bot_consolemessage_t *__cdecl BotNextConsoleMessage(bot_chatstate_t *cs);
int __cdecl BotNumConsoleMessages(bot_chatstate_t *chatstate);
void __cdecl BotQueueConsoleMessage(bot_chatstate_t *chatstate, int type, char *message);
int __cdecl BotRemoveConsoleMessage(bot_chatstate_t *chatstate, bot_consolemessage_t *msg);
void __cdecl BotReplaceSynonyms(char *string, unsigned long int context);
void __cdecl BotReplaceWeightedSynonyms(const char *string, int context);
int __cdecl BotReplyChat(bot_chatstate_t *cs, const char *message);
void __cdecl BotResetChatAI(void);
void __cdecl BotSetChatGender(bot_chatstate_t *chatstate, int gender);
void __cdecl BotSetChatName(bot_chatstate_t *chatstate, const char *name);
int BotSetupChatAI();
void BotShutdownChatAI();
void __cdecl FreeConsoleMessage(bot_consolemessage_t *message);
void InitConsoleMessageHeap();
BOOL __cdecl IsWhiteSpace(char c);
char *__cdecl RandomString(const char *name);
const char *__cdecl StringContains(const char *str1, const char *str2, int casesensitive);
const char *__cdecl StringContainsWord(const char *str1, const char *str2, int casesensitive);
void __cdecl StringReplaceWords(const char *string, const char *synonym, const char *replacement);
BOOL __cdecl StringsMatch(bot_matchpiece_t *pieces, bot_match_t *match);
void __cdecl UnifyWhiteSpaces(void *string);
void __cdecl sub_1002E5D0(bot_replychat_t *arg);

#endif /* BOTLIB_BE_AI_CHAT_H */
