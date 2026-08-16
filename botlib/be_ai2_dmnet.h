/* be_ai2_dmnet.h — interface of be_ai2_dmnet.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AI2_DMNET_H
#define BOTLIB_BE_AI2_DMNET_H

/* Declarations for what this TU defines — last, so the types above are in scope. */
extern int numnodeswitches;
extern char nodeswitch[7344];

void __cdecl AIEnter_Battle_Chase(bot_state_t *bs);
void __cdecl AIEnter_Battle_Fight(bot_state_t *bs);
void __cdecl AIEnter_Battle_NBG(bot_state_t *bs);
void __cdecl AIEnter_Battle_Retreat(bot_state_t *bs);
void __cdecl AIEnter_Intermission(bot_state_t *bs);
void __cdecl AIEnter_Observer(bot_state_t *bs);
void __cdecl AIEnter_Respawn(bot_state_t *bs);
void __cdecl AIEnter_Seek_ActivateEntity(bot_state_t *bs);
void __cdecl AIEnter_Seek_LTG(bot_state_t *bs);
void __cdecl AIEnter_Seek_NBG(bot_state_t *bs);
void __cdecl AIEnter_Stand(bot_state_t *bs);
int __cdecl AINode_Battle_Chase(bot_state_t *bs);
int __cdecl AINode_Battle_Fight(bot_state_t *bs);
int __cdecl AINode_Battle_NBG(bot_state_t *bs);
int __cdecl AINode_Battle_Retreat(bot_state_t *bs);
int __cdecl AINode_Intermission(bot_state_t *bs);
int __cdecl AINode_Observer(bot_state_t *bs);
int __cdecl AINode_Respawn(bot_state_t *bs);
int __cdecl AINode_Seek_ActivateEntity(bot_state_t *bs);
int __cdecl AINode_Seek_LTG(bot_state_t *bs);
int __cdecl AINode_Seek_NBG(bot_state_t *bs);
int __cdecl AINode_Stand(bot_state_t *bs);
int __cdecl BotDumpNodeSwitches(bot_state_t *bs);
int BotGetFormationGoal(bot_state_t *bs);
float *__cdecl BotLongTermGoal(bot_state_t *bs, int tfl, int retreat);
void __cdecl BotRecordNodeSwitch(bot_state_t *bs, const char *node, const char *str);
void BotResetNodeSwitches();

#endif /* BOTLIB_BE_AI2_DMNET_H */
