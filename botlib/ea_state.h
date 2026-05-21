/*
 * ea_state.h — Elementary Action subsystem per-client state (36 bytes).
 *
 * EA_Setup() allocates `ea_controls = GetClearedMemory(36 * maxclients)`,
 * indexed with stride 36 = sizeof(ea_state_t) per client.
 *
 * Field layout confirmed from the EA_* function bodies (EA_Move writes dir +
 * speed at +4..+19; EA_View writes angles at +20..+31; EA_Jump/Attack
 * toggle bits in flags at +32; thinktime at +0 set just before BotInput
 * fires, then cleared after).
 *
 * The flags field holds bitmasks defined as ACTION_* in gladq2_src/botlib.h.
 * Reproduced here so callers don't have to pull in the whole botlib.h.
 *
 * Layout matches the engine's bot_input_t exactly (thinktime/dir/speed/
 * viewangles/actionflags).  EA_EndRegular passes a pointer to this struct
 * to game.dll's BotInput() which memcpys all 36 bytes into
 * botglobals.botinputs[client] — so the field types and offsets here are
 * what the engine sees.
 */

#ifndef EA_STATE_H
#define EA_STATE_H

/* Elementary Action flag bits — must match gladq2_src/botlib.h.
 * ACTION_JUMP and ACTION_MOVEUP are aliased to the same bit (0x008);
 * ACTION_CROUCH and ACTION_MOVEDOWN likewise (0x010). */
#ifndef ACTION_ATTACK
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
#endif

/* Internal-only bit used by EA_Jump / EA_DelayedJump to gate the
 * jump-press to one frame at a time.  Set by EA_EndRegular when the
 * frame's input is consumed; checked at the top of EA_Jump.  This bit
 * does not have an ACTION_* name in botlib.h since it's an
 * implementation detail of the jump latch. */
#define EA_JUMPEDLASTFRAME  0x080

typedef struct ea_state_s {
    float thinktime;  /* +0  frame thinktime in seconds (used by BotInput→ucmd.msec) */
    float dir[3];     /* +4  movement direction vector              */
    float speed;      /* +16 movement speed, clamped to ±565        */
    float angles[3];  /* +20 view angles: pitch, yaw, roll          */
    int   flags;      /* +32 ACTION_* bitmask                       */
} ea_state_t;         /* sizeof = 36 */

#endif /* EA_STATE_H */
