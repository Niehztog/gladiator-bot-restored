/*
 * be_ai2_dmnet.h — interface of be_ai2_dmnet.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AI2_DMNET_H
#define BOTLIB_BE_AI2_DMNET_H

/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
extern int numnodeswitches;
extern char nodeswitch[7344];

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
