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
/* Elementary Action flag bits — must match gladq2_src/botlib.h.
 * ACTION_JUMP/ACTION_MOVEUP share bit 0x008; ACTION_CROUCH/ACTION_MOVEDOWN
 * share 0x010. */
#define ACTION_ATTACK       0x001  /* primary fire                     */
#define ACTION_USE          0x002  /* "use" / activate                 */
#define ACTION_RESPAWN      0x004  /* request respawn after death      */
#define ACTION_JUMP         0x008  /* jump (gated by JUMPEDLASTFRAME)  */
#define ACTION_MOVEUP       0x008  /* move-up (alias of JUMP)          */
#define ACTION_CROUCH       0x010  /* crouch                           */
#define ACTION_MOVEDOWN     0x010  /* move-down (alias of CROUCH)      */
#define ACTION_MOVEFORWARD  0x020  /* +forward                         */
#define ACTION_MOVEBACK     0x040  /* +back                            */
#define ACTION_MOVELEFT     0x080  /* +moveleft                        */
#define ACTION_MOVERIGHT    0x100  /* +moveright                       */
#define ACTION_DELAYEDJUMP  0x200  /* deferred jump (Q3 carryover)     */

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
