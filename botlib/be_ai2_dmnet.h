/*
 * be_ai2_dmnet.h — interface of be_ai2_dmnet.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AI2_DMNET_H
#define BOTLIB_BE_AI2_DMNET_H

#include "botlib_local.h"

void __cdecl AIEnter_Battle_Chase(bot_state_t *bs);
int __cdecl AIEnter_Battle_Fight(bot_state_t *bs);
int __cdecl AIEnter_Battle_NBG(bot_state_t *bs);
int __cdecl AIEnter_Battle_Retreat(bot_state_t *bs);
void __cdecl AIEnter_Intermission(bot_state_t *bs);
int __cdecl AIEnter_Observer(bot_state_t *bs);
void __cdecl AIEnter_Respawn(bot_state_t *bs);
int __cdecl AIEnter_Seek_ActivateEntity(bot_state_t *bs);
int __cdecl AIEnter_Seek_LTG(bot_state_t *bs);
int __cdecl AIEnter_Seek_NBG(bot_state_t *bs);
int __cdecl AIEnter_Stand(bot_state_t *bs);
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
int __cdecl BotRecordNodeSwitch(bot_state_t *bs, const char *node, const char *str);
void BotResetNodeSwitches();

#endif /* BOTLIB_BE_AI2_DMNET_H */
