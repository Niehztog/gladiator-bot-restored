/*
 * be_ea.h — interface of be_ea.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_EA_H
#define BOTLIB_BE_EA_H

/* ea_state_t: the per-client elementary-action state this TU owns. Was ea_state.h until 2026-08-10. */

/* The ACTION_* bits stored in ea_state_t.flags are deliberately NOT defined
 * here: they are part of the botlib<->game interface and belong to
 * ../game/botlib.h (verbatim from the shipped gladq2_src/botlib.h), which
 * botlib_port.h pulls into every botlib TU ahead of this header.  Q3's
 * be_ea.c takes them from ../game/botlib.h the same way and defines only its
 * private jump latch locally.  Values worth knowing while reading below:
 * ACTION_JUMP/ACTION_MOVEUP share bit 8, ACTION_CROUCH/ACTION_MOVEDOWN
 * share bit 16. */

/* Internal jump latch (no ACTION_* name in botlib.h): set by EA_EndRegular
 * when the frame's input is consumed, checked at the top of EA_Jump to gate
 * the jump-press to one frame at a time. */
#define EA_JUMPEDLASTFRAME  0x080

typedef struct ea_state_s {
    float thinktime;  /* +0  frame thinktime in seconds (used by BotInput→ucmd.msec) */
    float dir[3];     /* +4  movement direction vector              */
    float speed;      /* +16 movement speed, clamped to ±565        */
    float angles[3];  /* +20 view angles: pitch, yaw, roll          */
    int   flags;      /* +32 ACTION_* bitmask                       */
} ea_state_t;         /* sizeof = 36 */



/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
void __cdecl EA_Move(int client, vec3_t dir, float speed); /* EA_Move impl */
void __cdecl EA_View(int client, vec3_t viewangles); /* EA_View impl */
extern ea_state_t *ea_controls;

void __cdecl EA_Attack(int client);
int __cdecl EA_Command(int client, char *command, ...);
void __cdecl EA_Crouch(int client);
int __cdecl EA_DelayedJump(int client);
void __cdecl EA_DropInv(int client, char *inv);
void __cdecl EA_DropItem(int client, char *it);
void __cdecl EA_EndRegular(int client, float thinktime);
char __cdecl EA_Jump(int client);
void __cdecl EA_Move(int client, vec3_t dir, float speed);
void __cdecl EA_MoveBack(int client);
void __cdecl EA_MoveDown(int client);
void __cdecl EA_MoveForward(int client);
void __cdecl EA_MoveLeft(int client);
void __cdecl EA_MoveRight(int client);
void __cdecl EA_MoveUp(int client);
void __cdecl EA_Respawn(int client);
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
