/* be_ai_goal.h — interface of be_ai_goal.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AI_GOAL_H
#define BOTLIB_BE_AI_GOAL_H

/* Declarations for what this TU defines — last, so the types above are in scope. */
extern structdef_t iteminfo_struct; /* item/entity structdef — defined in botlib_structdefs.c */
extern levelitem_t *freelevelitems;
extern int numlevelitems;
extern levelitem_t *levelitemheap;
extern itemconfig_t *itemconfig;
extern levelitem_t *levelitems;

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
void __cdecl BotPushGoal(bot_goalstate_t *goalstate, const void *goal);
void __cdecl BotResetAvoidGoals(bot_goalstate_t *goalstate);
void __cdecl BotResetGoalState(bot_goalstate_t *goalstate);
int BotSetupGoalAI();
void BotShutdownGoalAI(void);
int __cdecl BotTouchingGoal(vec3_t origin, float *goal);
void BotUpdateEntityItems(void);
void __cdecl FreeLevelItem(levelitem_t *li);
void InitLevelItemHeap();
int *__cdecl ItemWeightIndex(weightconfig_t *iwc, itemconfig_t *ic);
itemconfig_t *LoadItemConfig(char *Source);
levelitem_t *__cdecl RemoveLevelItemFromList(levelitem_t *li);

#endif /* BOTLIB_BE_AI_GOAL_H */
