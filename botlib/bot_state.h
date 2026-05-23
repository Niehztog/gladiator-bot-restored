/*
 * bot_state.h — Per-bot AI state (4560 bytes).
 *
 * Allocated as `GetClearedMemory(4560 * maxclients)` in BotSetupClient
 * (sub_10026EB0).
 *
 * The struct is wrapped in a union with `_raw[]` so that any unconverted
 * byte-offset access in legacy code still works against the same memory.
 *
 * Field naming convention:
 *   Verified-purpose fields use a semantic name (origin, eye, ltgtype, …).
 *   Offsets where only the type is verified (from access patterns) but the
 *   semantic role is not yet pinned down get a placeholder `_fN` (float),
 *   `_iN` (int) or `_bN` (byte) name, where N is the decimal byte offset.
 *   These placeholders match the original C struct *style* (named fields
 *   instead of raw byte arithmetic) without inventing semantics that might
 *   be wrong.
 *
 * Total size is locked at 4560 bytes by the compile-time assertion below.
 */

#ifndef BOT_STATE_H
#define BOT_STATE_H

#include "../game/botlib.h"    /* bot_updateclient_t */
#include "botlib_structs.h"   /* bot_goal_t */
#include "chat_state.h"       /* bot_chatstate_t */

#define BOT_STATE_SIZE       4560

/* bot_movestate_t — inline movement-state struct embedded in bot_state_t at
 * +2880 (covers +2880..+3007 = 128 bytes).  Reverse-engineered from
 * BotMoveToGoal (sub_100343A0) disasm offsets, which match Q3
 * bot_movestate_t (be_ai_move.c:59-88) field-for-field with MAX_AVOIDREACH=1.
 *
 * The struct aliases the legacy `movestate[16]` + `areanum` + `_pad_B84h[60]`
 * triple in bot_state_t via an anonymous union, so existing callers that
 * pass `bs->movestate` (decayed int*) still work unchanged while new code
 * can write `bs->ms.areanum` for typed access. */
typedef struct bot_movestate_s {
    vec3_t  origin;                 /* +0   bot world position (written at top of BotMoveToGoal via sub_10001550) */
    vec3_t  velocity;               /* +12  bot velocity */
    vec3_t  viewoffset;             /* +24  add to origin for eye coords */
    int     entitynum;              /* +36  bot entity number; 3rd arg to sub_10001EE2 swim/jump trigger */
    int     client;                 /* +40  bot client number */
    float   thinktime;              /* +44  frame-time delta; scaled by *10.0 in reachability_time decrement */
    int     presencetype;           /* +48  bbox presence type (normal/crouched/spectator) — 2nd arg to sub_10001EE2 */
    vec3_t  viewangles;             /* +52  ms-local view angles (note: bs->viewangles at +4224 is a separate snapshot field) */
    int     areanum;                /* +64  ALIASES bs->areanum (+2944 = +2880+64).  Written: BotReachabilityArea result. */
    int     lastareanum;            /* +68 */
    int     lastgoalareanum;        /* +72  compared to goal->areanum to detect re-pathing need */
    int     lastreachnum;           /* +76  current reachability number */
    vec3_t  lastorigin;             /* +80  copied from origin[] each BotMoveToGoal call */
    int     reachareanum;           /* +92 */
    int     moveflags;              /* +96  MFL_SWIMMING (2) | MFL_TELEPORTED (4) | MFL_WATERJUMP (8); cleared via &0xFFFFFFF3 */
    int     jumpreach;              /* +100 zeroed at LABEL_25 of BotMoveToGoal */
    float   grapplevisible_time;    /* +104 */
    float   lastgrappledist;        /* +108 */
    float   reachability_time;      /* +112 deadline vs AAS_Time(); decremented by thinktime*10.0 */
    int     avoidreach[1];          /* +116 reachability number to avoid (MAX_AVOIDREACH=1 inferred from BotMoveToGoal +116/+120/+124 out-pointer triple) */
    float   avoidreachtimes[1];     /* +120 expiry timestamp */
    int     avoidreachtries[1];     /* +124 retry count before adding to avoid list */
} bot_movestate_t;                  /* sizeof == 128 */

/* Named team-waypoint linked-list node, heap-allocated by BotCreateWayPoint.
 * On 32-bit the original DLL allocates `strlen(name)+1+68` bytes and stores
 * the head/tail pointers inline in bot_state_t's +4544/+4548/+4552 int slots
 * plus the node-internal +60/+64 dword slots.  On 64-bit Linux the host
 * pointers are 8 bytes wide, so we use a proper C struct here and side-band
 * the three bot_state_t head slots through parallel maxclients-sized arrays
 * (botcheckpoints/botpatrolpoints/botcurpatrolpoint in botlib.c).  Allocation
 * size becomes `sizeof(bot_waypoint_t) + strlen(name) + 1`; the trailing
 * inline name buffer lives at `(char *)(node + 1)`. */
typedef struct bot_waypoint_s {
    char                   *name;   /* points to inline name buffer right after struct */
    bot_goal_t              goal;   /* origin/areanum/mins/maxs/...; passed as bot_goal_t* */
    struct bot_waypoint_s  *next;
    struct bot_waypoint_s  *prev;
} bot_waypoint_t;

typedef struct bot_state_s {
    union {
        int _raw[BOT_STATE_SIZE / sizeof(int)];
        struct {
            int    inuse;                 /* +0    */
            int    client;                /* +4    */
            int    entitynum;             /* +8    */
            /* +12..+1239 (1228 B = 0x4CC): bot_updateclient_t snapshot.
             * Written wholesale every frame by the engine via the
             * BotUpdateClient import (slot 13).  The anonymous struct is only
             * a byte-exact alias for older restored field names. */
            union {
                bot_updateclient_t snapshot;
                struct {
                    int    pm_type;               /* +12   movement type (pmtype_t) */
                    vec3_t ps_origin;             /* +16..+27   alias of snapshot.origin */
                    vec3_t ps_velocity;           /* +28..+39   alias of snapshot.velocity */
                    unsigned char pm_flags;       /* +40   PMF_DUCKED/PMF_ON_GROUND/etc. */
                    unsigned char pm_time;        /* +41   each unit = 8 ms */
                    char   _pad_2Ah[2];           /* +42..+43 struct alignment pad before float */
                    float  gravity;               /* +44   current gravity */
                    vec3_t delta_angles;          /* +48..+59   add to cmd angles for view dir */
                    vec3_t ps_viewangles;         /* +60..+71   fixed-view angles from playerstate */
                    vec3_t viewoffset;            /* +72..+83   add to origin for eye coordinates */
                    vec3_t kick_angles;           /* +84..+95   weapon-kick + pain effect angles */
                    vec3_t gunangles;             /* +96..+107  gun model angles */
                    vec3_t gunoffset;             /* +108..+119 gun model offset relative to origin */
                    int    gunindex;              /* +120  gun model number (-> AAS_ModelFromIndex) */
                    int    gunframe;              /* +124  gun model frame number */
                    float  blend[4];              /* +128..+143 RGBA full-screen effect */
                    float  fov;                   /* +144  horizontal field of view */
                    int    rdflags;               /* +148  refdef flags */
                    short  stats[32];             /* +152..+215 MAX_STATS player stats */
                    int    inventory_src[256];    /* +216..+1239 alias of snapshot.inventory */
                };
            };
            char   settings[432];         /* +1240..+1671 bot_clientsettings_t */
            int    character;             /* +1672 */
            int    ainode;                /* +1676 current AINode_* dispatcher; called via (int(*)(int))bs->ainode */
            float  thinktime;             /* +1680 per-frame think delta; written `bs->thinktime = a2` at top of sub_100289A0 (bot frame entry) */
            vec3_t origin;                /* +1684..+1695 */
            char   _pad_6B0h[16];         /* +1696..+1711 */
            vec3_t eye;                   /* +1712..+1723 */
            char   _pad_6BCh[4];          /* +1724..+1727 */
            /* +1728..+2751: structured battle-snapshot region populated by
             * `BotUpdateBattleInventory(bs, enemy)` (enemy-side fields) and
             * `sub_10021020(bs)` (own-inventory & powerup fields).  Only the
             * verified offsets get named fields; the remainder stays padding.
             * Wrapped in an inner union so legacy `bs->chat_lines` (the
             * 1024-byte qmemcpy buffer at +1728) and the typed view share
             * storage. */
            /* +1728..+2751: 1024-byte inventory mirror.  qmemcpy'd each
             * frame from `inventory_src` (+216).  BotUpdateInventory and
             * BotUpdateBattleInventory overlay derived powerup/battle
             * snapshot fields onto this region at offsets that don't
             * collide with item slots actually read by the AI (powershield
             * at inventory[20] / +1808 is read via `bs->inventory[20]`,
             * while +1892 / +2528 / +2544.. are written for derived state). */
            union {
                int  inventory[256];                  /* +1728..+2751 working copy of inventory_src */
                struct {
                    char   _pad_6C0h[164];        /* +1728..+1891 */
                    int    inventory_health;       /* +1892 ps.stats[STAT_HEALTH] snapshot; Q3: inventory[INVENTORY_HEALTH] */
                    char   _pad_764h[632];        /* +1896..+2527 */
                    int    enemy_horizontal_dist;  /* +2528 Q3: inventory[ENEMY_HORIZONTAL_DIST] */
                    int    enemy_height;           /* +2532 Q3: inventory[ENEMY_HEIGHT] */
                    char   _pad_9E8h[8];          /* +2536..+2543 */
                    int    quad_seconds;           /* +2544 quad-damage seconds remaining (clamped >=0) */
                    int    invuln_seconds;         /* +2548 invulnerability seconds remaining */
                    char   _pad_9F4h[4];          /* +2552 — unused/reserved inventory slot */
                    int    rebreather_seconds;     /* +2556 rebreather seconds remaining */
                    int    enviro_seconds;         /* +2560 envirosuit seconds remaining */
                    char   _pad_A00h[4];          /* +2564 */
                    int    power_screen_active_cells; /* +2568 active power armor cells gate for Power Screen */
                    int    power_shield_active_cells; /* +2572 active power armor cells gate for Power Shield */
                    char   _pad_A0Ch[72];         /* +2576..+2647 */
                    /* +2648..+2692: one-hot enemy current-weapon flags from
                     * high byte of entity skinnum (Quake II WEAP_* numbers). */
                    int    enemy_weapon_blaster;        /* +2648 WEAP_BLASTER */
                    int    enemy_weapon_shotgun;        /* +2652 WEAP_SHOTGUN */
                    int    enemy_weapon_supershotgun;   /* +2656 WEAP_SUPERSHOTGUN */
                    int    enemy_weapon_machinegun;     /* +2660 WEAP_MACHINEGUN */
                    int    enemy_weapon_chaingun;       /* +2664 WEAP_CHAINGUN */
                    int    enemy_weapon_grenadelauncher;/* +2668 WEAP_GRENADELAUNCHER */
                    int    enemy_weapon_rocketlauncher; /* +2672 WEAP_ROCKETLAUNCHER */
                    int    enemy_weapon_hyperblaster;   /* +2676 WEAP_HYPERBLASTER */
                    int    enemy_weapon_railgun;        /* +2680 WEAP_RAILGUN */
                    int    enemy_weapon_bfg;            /* +2684 WEAP_BFG */
                    int    enemy_weapon_grenades;       /* +2688 WEAP_GRENADES */
                    int    enemy_weapon_phalanx;        /* +2692 WEAP_PHALANX / WEAP_12 */
                    char   _pad_A88h[12];         /* +2696..+2707 */
                    int    enemy_quad;             /* +2708 enemy effects & EF_QUAD */
                    int    enemy_invulnerability;  /* +2712 enemy effects & EF_PENT */
                    int    enemy_powerscreen;      /* +2716 enemy effects & EF_POWERSCREEN */
                    char   _pad_A9Ch[32];         /* +2720..+2751 */
                };
            };
            int    flags;                 /* +2752 several flags — bit 0x02 toggled in BotEntityVisible area,
                                           * bit 0x10 XOR-toggled in BotCheckActivateGoal direction flip.
                                           * Q3 ancestor: bs->flags (ai_main.h:143). Byte- and dword-accessed. */
            int    _i2756;                /* +2756 no readers AND no writers in the original DLL disasm
                                           * (zero hits for `[reg+0xac4]`).  Truly vestigial slot — likely a
                                           * planned field (the Q3 ancestor block at this offset holds e.g.
                                           * lastkilledplayer / lastkilledby / botsuicide) that gladiator
                                           * never implemented. */
            int    respawn_wait;          /* +2760 AIEnter_Respawn sets =0; AINode_Respawn sets =1 after EA_Respawn fires.
                                           * Q3 ancestor: bs->respawn_wait (ai_main.h:144). */
            int    lasthealth;            /* +2764 health value previous frame; compared against inventory_health
                                           * in BotFindEnemy to detect "I just took damage".
                                           * Q3 ancestor: bs->lasthealth (ai_main.h:145). */
            int    enemydeathtype;        /* +2768 bot_match_t.subtype recorded when a death-message names the
                                           * bot's enemy as victim (chat-driven death-type tracking).
                                           * Q3 ancestor: bs->enemydeathtype (ai_main.h:149). */
            int    botdeathtype;          /* +2772 bot_match_t.subtype recorded when a death-message names the
                                           * bot itself as victim.
                                           * Q3 ancestor: bs->botdeathtype (ai_main.h:148). */
            int    inuse_marker;          /* +2776 */
            int    _i2780;                /* +2780 no readers AND no writers in the original DLL disasm
                                           * (zero hits for `[reg+0xadc]`).  Truly vestigial — likely a
                                           * planned field (Q3's enemysuicide / setupcount / num_deaths
                                           * fall in this vicinity) that gladiator never wired up. */
            float  ltime;                 /* +2784 wall-clock accumulator; `bs->ltime += thinktime` per frame */
            float  setup_time;            /* +2788 */
            float  ltg_time;              /* +2792 long-term-goal re-pick throttle (set to AAS_Time()+20.0 after BotChooseLTGItem) */
            float  nbg_time;              /* +2796 nearby-goal timeout (set to AAS_Time()+5.0 after BotChooseNBGItem) */
            float  respawnchat_time;      /* +2800 time the bot started a chat during respawn; set in AIEnter_Respawn
                                           * to AAS_Time()+BotChatTime() if a chat fires; tested in AINode_Respawn.
                                           * Q3 ancestor: bs->respawnchat_time (ai_main.h:168). */
            float  chase_time;            /* +2804 time the bot will chase the enemy; set to AAS_Time()+10 in
                                           * AIEnter_Battle_Chase; reset to 0 when reaching enemy area or goal.
                                           * Q3 ancestor: bs->chase_time (ai_main.h:169). */
            float  check_time;            /* +2808 0.5–1.0 s throttle for BotChooseNBGItem nearby-item scans.
                                           * Q3 ancestor: bs->check_time (ai_main.h:171). */
            float  stand_time;            /* +2812 time the bot is standing still during random/kill/etc chats;
                                           * set in BotChat_Random / BotChat_Kill paths via AAS_Time()+BotChatTime().
                                           * Q3 ancestor: bs->stand_time (ai_main.h:172). */
            float  attackstrafe_drift;    /* +2816 strafe-direction confidence counter in BotAttackMove;
                                           * accumulates +0.1 per frame, reset to 0 when bs->flags bit 1 flips
                                           * (XOR 1) on direction reversal triggered by a random threshold.
                                           * Q3 ancestor: bs->attackstrafe_time (ai_main.h:177) but semantics
                                           * differ (Q3 stores AAS_Time(), we store a per-frame accumulator). */
            float  _f2820;                /* +2820 dual-use wait/crouch countdown in accompany (LTG=?) and
                                           * camp (LTG=?) branches: 5 s base + char24*15 randomized extension.
                                           * Bot crouches while it remains > AAS_Time() in camp; periodic
                                           * wave/say in accompany.  No clean Q3 ancestor — closest is
                                           * bs->attackcrouch_time (ai_main.h:178) but semantics differ. */
            float  attackchase_time;      /* +2824 chase-using-lastenemyorigin window.  In the original DLL
                                           * the WRITER IS COMMENTED OUT (Q3 ai_dmq3.c:2759 still preserves
                                           * `// bs->attackchase_time = AAS_Time() + 6;`), so this field
                                           * stays 0.0f from calloc and the `AAS_Time() < attackchase_time`
                                           * branch at BotAttackMove entry is dead code in shipped builds.
                                           * Restored under its Q3 ancestor name (ai_main.h:179) for
                                           * fidelity — the dead branch matches gladiator.dll.disasm.txt
                                           * exactly (only `fcomp [ebp+0xb08]` at 0x10022e3e, no store). */
            float  powerscreen_seen_time; /* +2828 last time the bot saw the powershield icon in stats[4];
                                           * 0.9 s grace before clearing power_screen/shield_active_cells.
                                           * Set to AAS_Time() in BotUpdatePowerupSeconds. */
            float  quad_endtime;          /* +2832 absolute deadline for quad damage; set on pickup to
                                           * AAS_Time()+stats[10].  Drives quad_seconds countdown. */
            float  invulnerability_endtime; /* +2836 deadline for Pent of Invulnerability; drives invuln_seconds. */
            float  rebreather_endtime;    /* +2840 deadline for Rebreather; drives rebreather_seconds. */
            float  enviro_endtime;        /* +2844 deadline for Environment Suit; drives enviro_seconds. */
            float  enemysight_time;       /* +2848 time the bot first saw the current enemy this engagement;
                                           * set to AAS_Time() at the end of BotFindEnemy. Tested as
                                           * `AAS_Time() - reaction >= enemysight_time` to gate firing.
                                           * Q3 ancestor: bs->enemysight_time (ai_main.h:181). */
            float  activategoal_time;     /* +2852 time the activategoal expires; set to AAS_Time()+10 when the
                                           * activategoal is established in BotCheckActivateGoal; reset to 0 on
                                           * touching goal. No direct Q3 ancestor (Q3 uses a stack of
                                           * bot_activategoal_t instead of a scalar timer). */
            char   _pad_B1Ch[4];          /* +2856..+2859 */
            float  defendaway_time;       /* +2860 time away while defending; randomized extension when bot
                                           * is at the defend teamgoal (ltgtype==3 / DEFEND).
                                           * Q3 ancestor: bs->defendaway_time (ai_main.h:184). */
            float  rushbaseaway_time;     /* +2864 time away from rushing to the base; randomized extension
                                           * when CTF flag-carrier is touching the home flag goal (ltgtype==5).
                                           * Q3 ancestor: bs->rushbaseaway_time (ai_main.h:186). */
            float  ctfroam_time;          /* +2868 CTF roam-then-pick-new-LTG cooldown; set to AAS_Time()+60
                                           * after picking nothing this cycle.
                                           * Q3 ancestor: bs->ctfroam_time (ai_main.h:189). */
            float  killedenemy_time;      /* +2872 time the bot killed the enemy; set to AAS_Time() in the
                                           * chat-death-message handler when match.victim == bot's enemy.
                                           * Drives 5-second post-kill EA_Wave celebration.
                                           * Q3 ancestor: bs->killedenemy_time (ai_main.h:190). */
            float  arrive_time;           /* +2876 time the bot arrived at the companion (accompany teammate);
                                           * set to AAS_Time() when bot first reaches the teammate area;
                                           * tested as `AAS_Time() - 2.0 > arrive_time` to throttle waves.
                                           * Q3 ancestor: bs->arrive_time (ai_main.h:191). */
            /* +2880..+3007 inline bot_movestate_t (128 B).  An anonymous union
             * provides both the legacy decay-as-int* view (`bs->movestate`,
             * used by ~30 opaque calls to BotMoveToGoal / BotResetAvoidReach /
             * BotMoveInDirection / BotEntityInfo / etc.) and the typed view
             * (`bs->ms`).  `bs->areanum` at +2944 lives inside this region —
             * it's the same memory as `bs->ms.areanum`. */
            union {
                struct {
                    int    movestate[16];        /* +2880..+2943 legacy int[]: first 16 ints of bot_movestate_t */
                    int    areanum;              /* +2944 alias of bs->ms.areanum */
                    char   _pad_B84h[60];        /* +2948..+3007 remainder of bot_movestate_t (lastareanum..avoidreachtries) */
                };
                bot_movestate_t ms;              /* +2880..+3007 typed view */
            };
            /* Note: a few stray byte-offset accesses outside the chat
             * region still go through the `_raw[]` union (e.g. +4248 in
             * the patrol-state loop).  Splitting more pads is fine; the
             * union keeps both spellings live against the same memory. */
            int    goalstate[243];        /* +3008..+3979 */
            bot_chatstate_t chatstate;    /* +3980..+4167 (188 bytes; see chat_state.h) */
            int    weaponweights[7];      /* +4168..+4195 */
            int    enemy;                 /* +4196 */
            int    lastenemyareanum;      /* +4200 areanum of enemy's last-confirmed position */
            vec3_t lastenemyorigin;       /* +4204..+4215 enemy origin at last sighting (written from AAS_EntityInfo origin) */
            char   _pad_1058h[8];         /* +4216..+4223 */
            vec3_t viewangles;            /* +4224..+4235 bot's current view angles, updated each frame
                                            * in BotUpdateClient via AngleMod(delta_angles + viewangles).
                                            * Passed as 3rd arg to BotEntityVisible/InFieldOfVision (the
                                            * "viewer angles" parameter) and overwritten with
                                            * ideal_viewangles before EA_View calls.  IDA decompilations
                                            * named this `enemyorigin` by mistake — fov=360.0 traces
                                            * masked the misnomer. */
            vec3_t ideal_viewangles;      /* +4236..+4247  vectoangles dst; copied into EA_View vec3 arg */
            vec3_t viewanglespeed;        /* +4248..+4259 per-axis view-angle smoothing speed (pitch/yaw/roll
                                           * change rate); evolved each frame in BotChangeViewAngles.  The
                                           * pattern at 0x100291da is `lea esi,[edi+0x1098]` followed by an
                                           * ebx=2..0 loop that fld/fcom/fadd/fsub against `[esi+ebx*4]`
                                           * — classic per-component speed clamp toward ideal_viewangles.
                                           * Q3 ancestor: bs->viewanglespeed (ai_main.h:223). */
            int    ltgtype;               /* +4260 */
            int    teammate;              /* +4264 */
            bot_goal_t teamgoal;          /* +4268..+4323 (56 B; origin/areanum/mins/maxs/entitynum/number/flags/iteminfo) */
            float  teammessage_time;      /* +4324  schedule LTG-start chat (Accompany/GetFlag/...) */
            float  teammatevisible_time;  /* +4328 NOTE: holds a deadline timestamp (AAS_Time()+120/240) not
                                           * a last-seen marker — semantics differ from Q3's same-named field.
                                           * Likely Q3 ancestor: bs->teamgoal_time (ai_main.h:236).  Kept under
                                           * the historical name pending a swap-pass with teammatelastseen_time. */
            float  teammatelastseen_time; /* +4332 set to AAS_Time() the moment the teammate becomes invisible
                                           * (BotEntityVisible() == false path); tested as
                                           * `AAS_Time() - 10.0 > teammatelastseen_time` to drop the LTG, and
                                           * `AAS_Time() - 60.0 > teammatelastseen_time` to chat "cannot find you".
                                           * Q3 ancestor: bs->teammatevisible_time (ai_main.h:237). */
            char   formation_teammate[16];/* +4336..+4351 netname of teammate used for formation positioning (Q3 backport: bs->formation_teammate[16]) */
            char   teamleader[32];        /* +4352..+4383 netname of team leader (Q3 backport: bs->teamleader[32]) */
            float  formation_dist;        /* +4384 formation-spacing distance to maintain from formation_teammate;
                                           * defaulted to 100.0f (encoded 0x42E80000); set by chat-match type 16
                                           * to atof(arg)*32.0 (units) or atof(arg)*9.7536 (feet→units), clamped
                                           * to [48,500].  Q3 ancestor: bs->formation_dist (ai_main.h:266). */
            char   _pad_1124h[100];       /* +4388..+4487 */
            bot_goal_t activategoal;      /* +4488..+4543 (56 B) embedded activate-goal: origin/areanum/mins/maxs/entitynum/number/flags/iteminfo.
                                           * Pointer to this is passed as `bot_goal_t *` to BotTouchingGoal/BotMoveToGoal.
                                           * Mins/maxs set as origin ± 5 with z-offsets; areanum filled from AAS_PointAreaNum. */
            int    checkpoints;           /* +4544 LEGACY 4-byte slot for bot_waypoint_t * head; on 64-bit Linux the
                                           * real pointer is side-banded via BotCheckpoints(bs); do not access directly. */
            int    patrolpoints;          /* +4548 LEGACY slot; real ptr in BotPatrolpoints(bs). */
            int    curpatrolpoint;        /* +4552 LEGACY slot; real ptr in BotCurPatrolPoint(bs). */
            int    patrolflags;           /* +4556 patrol direction/reverse flags (Q3: patrolflags) */
        };
    };
} bot_state_t;

typedef int _bot_state_t_size_check[sizeof(bot_state_t) == BOT_STATE_SIZE ? 1 : -1];

#endif /* BOT_STATE_H */
