/*
 * be_ea.h — interface of be_ea.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_EA_H
#define BOTLIB_BE_EA_H

#include "botlib_local.h"

int __cdecl EA_Attack(int client);
int __cdecl EA_Command(int client, char *command, ...);
int __cdecl EA_Crouch(int client);
int __cdecl EA_DelayedJump(int client);
void __cdecl EA_DropInv(int client, char *inv);
void __cdecl EA_DropItem(int client, char *it);
void __cdecl EA_EndRegular(int client, float thinktime);
char __cdecl EA_Jump(int client);
void __cdecl EA_Move(int client, vec3_t dir, float speed);
void __cdecl EA_MoveBack(int client);
void __cdecl EA_MoveDown(int client);
int __cdecl EA_MoveForward(int client);
void __cdecl EA_MoveLeft(int client);
void __cdecl EA_MoveRight(int client);
int __cdecl EA_MoveUp(int client);
int __cdecl EA_Respawn(int client);
void __cdecl EA_Say(int client, char *str);
void __cdecl EA_SayTeam(int client, char *str);
int EA_Setup();
void EA_Shutdown();
void __cdecl EA_Use(int client);
void __cdecl EA_UseInv(int client, char *inv);
void __cdecl EA_UseItem(int client, char *it);
void __cdecl EA_View(int client, vec3_t viewangles);
void __cdecl sub_100371B0(int client, int sequence);

#endif /* BOTLIB_BE_EA_H */
