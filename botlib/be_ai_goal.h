/*
 * be_ai_goal.h — interface of be_ai_goal.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AI_GOAL_H
#define BOTLIB_BE_AI_GOAL_H

levelitem_t *__cdecl AddLevelItemToList(levelitem_t *li);
_DWORD *__cdecl AllocLevelItem(void);
void __cdecl BotAddToAvoidGoals(bot_goalstate_t *gs, int number, float avoidtime);
float __cdecl BotAvoidGoalTime(bot_goalstate_t *goalstate, int number);
int __cdecl BotChooseLTGItem(bot_goalstate_t *goalstate, vec3_t origin, char *inventory, int travelflags);
int __cdecl BotChooseNBGItem(bot_goalstate_t *goalstate, vec3_t origin, char *inventory, int travelflags, bot_goal_t *ltg, float maxtime);
void __cdecl BotDumpAvoidGoals(bot_goalstate_t *goalstate);
void __cdecl BotDumpGoalStack(bot_goalstate_t *goalstate);
void __cdecl BotEmptyGoalStack(bot_goalstate_t *goalstate);
void __cdecl BotFreeItemWeights(bot_goalstate_t *goalstate);
int __cdecl BotGetLevelItemGoal(int index, char *name, bot_goal_t *goal);
void *__cdecl BotGetSecondGoal(bot_goalstate_t *goalstate);
void *__cdecl BotGetTopGoal(bot_goalstate_t *goalstate);
char *__cdecl BotGoalName(int number);
void BotInitLevelItems(void);
BOOL __cdecl BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, bot_goal_t *goal);
int __cdecl BotLoadItemWeights(bot_goalstate_t *goalstate, char *filename);
int __cdecl BotPopGoal(bot_goalstate_t *goalstate);
int __cdecl BotPushGoal(bot_goalstate_t *goalstate, const void *goal);
int __cdecl BotResetAvoidGoals(bot_goalstate_t *goalstate);
int __cdecl BotResetGoalState(bot_goalstate_t *goalstate);
int BotSetupGoalAI();
int BotShutdownGoalAI();
int __cdecl BotTouchingGoal(vec3_t origin, float *goal);
void BotUpdateEntityItems(void);
void __cdecl FreeLevelItem(levelitem_t *li);
void InitLevelItemHeap();
int *__cdecl ItemWeightIndex(weightconfig_t *iwc, itemconfig_t *ic);
itemconfig_t *LoadItemConfig(char *Source);
levelitem_t *__cdecl RemoveLevelItemFromList(levelitem_t *li);

#endif /* BOTLIB_BE_AI_GOAL_H */
