/*
 * bot_state.h — Per-bot AI state (4560 bytes), allocated as
 * `GetClearedMemory(4560 * maxclients)` in BotSetupClient (sub_10026EB0).
 *
 * Wrapped in a union with `_raw[]` so unconverted byte-offset accesses in
 * legacy code still hit the same memory.  Fields whose type is known but
 * whose role is not are named `_fN`/`_iN`/`_bN` after their decimal byte
 * offset.  Size is locked at 4560 by the assertion at the bottom.
 */

#ifndef BOT_STATE_H
#define BOT_STATE_H

/* ../game/botlib.h has no include guards, so it cannot be re-included here:
 * includers must pull it in (after q_shared.h) before this header to make
 * bot_updateclient_t visible. */
#include "botlib_structs.h"   /* bot_goal_t */
#include "chat_state.h"       /* bot_chatstate_t */

#define BOT_STATE_SIZE       4560

/* bot_movestate_t — inline movement state at bot_state_t +2880 (128 bytes).
 * Matches Q3 bot_movestate_t field-for-field with MAX_AVOIDREACH=1.
 * Unioned with the legacy `movestate[16]`/`areanum`/pad triple, so callers
 * passing `bs->movestate` as int* keep working alongside `bs->ms.areanum`. */
typedef struct bot_movestate_s {
    vec3_t  origin;                 /* +0   bot world position */
    vec3_t  velocity;               /* +12  bot velocity */
    vec3_t  viewoffset;             /* +24  add to origin for eye coords */
    int     entitynum;              /* +36  bot entity number */
    int     client;                 /* +40  bot client number */
    float   thinktime;              /* +44  frame-time delta; scaled by *10.0 in reachability_time decrement */
    int     presencetype;           /* +48  bbox presence type (normal/crouched/spectator) */
    vec3_t  viewangles;             /* +52  ms-local view angles (bs->viewangles at +4224 is a separate field) */
    int     areanum;                /* +64  ALIASES bs->areanum (+2944); holds the BotReachabilityArea result */
    int     lastareanum;            /* +68 */
    int     lastgoalareanum;        /* +72  compared to goal->areanum to detect re-pathing need */
    int     lastreachnum;           /* +76  current reachability number */
    vec3_t  lastorigin;             /* +80  copied from origin[] each BotMoveToGoal call */
    int     reachareanum;           /* +92 */
    int     moveflags;              /* +96  MFL_SWIMMING (2) | MFL_TELEPORTED (4) | MFL_WATERJUMP (8); cleared via &0xFFFFFFF3 */
    int     jumpreach;              /* +100 */
    float   grapplevisible_time;    /* +104 */
    float   lastgrappledist;        /* +108 */
    float   reachability_time;      /* +112 deadline vs AAS_Time(); decremented by thinktime*10.0 */
    int     avoidreach[1];          /* +116 reachability number to avoid (MAX_AVOIDREACH=1) */
    float   avoidreachtimes[1];     /* +120 expiry timestamp */
    int     avoidreachtries[1];     /* +124 retry count before adding to avoid list */
} bot_movestate_t;                  /* sizeof == 128 */

/* bot_goalstate_t — inline goal management at bot_state_t +3008 (972 bytes).
 * Matches Q3 bot_goalstate_s minus the `client`/`lastreachabilityarea`
 * fields Q3 added later (either would push past the 972-byte lock), with
 * MAX_AVOIDGOALS=64 (Q3 later raised it to 256).
 *
 * itemweightconfig/itemweightindex hold 32-bit pointer BIT-PATTERNS and stay
 * `int` — a real 8-byte pointer would shift every following offset on
 * 64-bit.  Access them through the BotGoalP0/BotGoalP1/BotGoalHandleP0/
 * BotGoalHandleP1 side-band macros. */
typedef struct bot_goalstate_s {
    int         itemweightconfig;   /* +0   weightconfig_t* bit-pattern — see BotGoalP0 */
    int         itemweightindex;    /* +4   int* bit-pattern — see BotGoalP1 */
    bot_goal_t  goalstack[8];       /* +8   1-indexed goal stack (slot 0 unused), MAX_GOALSTACK=8 */
    int         goalstacktop;       /* +456 current stack depth, 0..7 (overflow logs + drops the push) */
    int         avoidgoals[64];     /* +460 item numbers currently being avoided */
    float       avoidgoaltimes[64]; /* +716 AAS_Time() expiry per avoidgoals[] slot */
} bot_goalstate_t;                  /* sizeof == 972 */

/* Named team-waypoint list node, heap-allocated by BotCreateWayPoint as
 * `sizeof(bot_waypoint_t) + strlen(name) + 1` (the original allocated
 * strlen(name)+1+68); the inline name buffer follows at `(char *)(node + 1)`.
 * The three bot_state_t head slots at +4544/+4548/+4552 are side-banded
 * through botcheckpoints/botpatrolpoints/botcurpatrolpoint in botlib.c. */
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
            /* +12..+1239 (1228 B): bot_updateclient_t snapshot, written
             * wholesale every frame via the BotUpdateClient import (slot 13).
             * The anonymous struct is a byte-exact alias of it. */
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
            float  thinktime;             /* +1680 per-frame think delta */
            vec3_t origin;                /* +1684..+1695 */
            char   _pad_6B0h[16];         /* +1696..+1711 */
            vec3_t eye;                   /* +1712..+1723 */
            char   _pad_6BCh[4];          /* +1724..+1727 */
            /* +1728..+2751: 1024-byte inventory mirror, memcpy'd each frame
             * from `inventory_src` (+216).  BotUpdateInventory and
             * BotUpdateBattleInventory overlay derived powerup/battle fields
             * onto offsets the AI does not read as item slots. */
            union {
                int  inventory[256];                  /* +1728..+2751 working copy of inventory_src */
                struct {
                    char   _pad_6C0h[164];        /* +1728..+1891 */
                    int    inventory_health;       /* +1892 ps.stats[STAT_HEALTH] snapshot */
                    char   _pad_764h[632];        /* +1896..+2527 */
                    int    enemy_horizontal_dist;  /* +2528 */
                    int    enemy_height;           /* +2532 */
                    char   _pad_9E8h[8];          /* +2536..+2543 */
                    int    quad_seconds;           /* +2544 quad-damage seconds remaining (clamped >=0) */
                    int    invuln_seconds;         /* +2548 invulnerability seconds remaining */
                    char   _pad_9F4h[4];          /* +2552 unused/reserved inventory slot */
                    int    rebreather_seconds;     /* +2556 rebreather seconds remaining */
                    int    enviro_seconds;         /* +2560 envirosuit seconds remaining */
                    char   _pad_A00h[4];          /* +2564 */
                    int    power_screen_active_cells; /* +2568 active power armor cells gate for Power Screen */
                    int    power_shield_active_cells; /* +2572 active power armor cells gate for Power Shield */
                    char   _pad_A0Ch[72];         /* +2576..+2647 */
                    /* +2648..+2692: one-hot enemy current-weapon flags, from
                     * the high byte of entity skinnum (Q2 WEAP_* numbers). */
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
            int    flags;                 /* +2752 bit 0x02 toggled in the BotEntityVisible area, bit 0x10
                                           * XOR-toggled on the BotAIBlocked direction flip.  Byte- and
                                           * dword-accessed. */
            int    _i2756;                /* +2756 vestigial: no readers and no writers in the original */
            int    respawn_wait;          /* +2760 AIEnter_Respawn sets 0; AINode_Respawn sets 1 after EA_Respawn */
            int    lasthealth;            /* +2764 previous-frame health; vs inventory_health in BotFindEnemy
                                           * to detect "I just took damage" */
            int    enemydeathtype;        /* +2768 bot_match_t.subtype from a death message naming the enemy */
            int    botdeathtype;          /* +2772 bot_match_t.subtype from a death message naming the bot */
            int    inuse_marker;          /* +2776 */
            int    _i2780;                /* +2780 vestigial: no readers and no writers in the original */
            float  ltime;                 /* +2784 wall-clock accumulator; `bs->ltime += thinktime` per frame */
            float  setup_time;            /* +2788 */
            float  ltg_time;              /* +2792 LTG re-pick throttle; AAS_Time()+20 after BotChooseLTGItem */
            float  nbg_time;              /* +2796 nearby-goal timeout; AAS_Time()+5 after BotChooseNBGItem */
            float  respawnchat_time;      /* +2800 AAS_Time()+BotChatTime() if a chat fires in AIEnter_Respawn */
            float  chase_time;            /* +2804 AAS_Time()+10 in AIEnter_Battle_Chase; 0 on reaching the
                                           * enemy area or goal */
            float  check_time;            /* +2808 0.5–1.0 s throttle for BotChooseNBGItem scans */
            float  stand_time;            /* +2812 stand-still window during random/kill chats */
            float  attackstrafe_drift;    /* +2816 strafe-direction confidence counter in BotAttackMove:
                                           * +0.1 per frame, reset to 0 when bs->flags bit 1 flips.
                                           * (Q3's attackstrafe_time stores a timestamp instead.) */
            float  attackcrouch_time;     /* +2820 dual-use crouch/wait deadline, as Q3:
                                           * if (now-5 > t) t = now + croucher*15 + 5; if Swimming t = now-1.
                                           * Drives crouching in camp and periodic waves in accompany. */
            float  attackchase_time;      /* +2824 chase-using-lastenemyorigin window.  The writer is
                                           * commented out in the original, so this stays 0.0f and the
                                           * `AAS_Time() < attackchase_time` branch at BotAttackMove entry
                                           * is dead code — faithful, do not "fix". */
            float  powerscreen_seen_time; /* +2828 last sighting of the powershield icon in stats[4]; 0.9 s
                                           * grace before clearing power_screen/shield_active_cells */
            float  quad_endtime;          /* +2832 AAS_Time()+stats[10] on pickup; drives quad_seconds */
            float  invulnerability_endtime; /* +2836 drives invuln_seconds */
            float  rebreather_endtime;    /* +2840 drives rebreather_seconds */
            float  enviro_endtime;        /* +2844 drives enviro_seconds */
            float  enemysight_time;       /* +2848 first sighting of the current enemy; firing is gated on
                                           * `AAS_Time() - reaction >= enemysight_time` */
            float  activategoal_time;     /* +2852 activategoal expiry, AAS_Time()+10 when established in
                                           * BotAIBlocked; 0 on touching the goal.  (Q3 uses a stack of
                                           * bot_activategoal_t rather than a scalar timer.) */
            char   _pad_B1Ch[4];          /* +2856..+2859 */
            float  defendaway_time;       /* +2860 time away while defending (ltgtype==3) */
            float  rushbaseaway_time;     /* +2864 time away from rushing the base (ltgtype==5) */
            float  ctfroam_time;          /* +2868 CTF roam cooldown; AAS_Time()+60 when nothing was picked */
            float  killedenemy_time;      /* +2872 AAS_Time() when a death message names the bot's enemy;
                                           * drives the 5-second post-kill celebration */
            float  arrive_time;           /* +2876 arrival at the accompanied teammate; waves are throttled
                                           * on `AAS_Time() - 2.0 > arrive_time` */
            /* +2880..+3007 inline bot_movestate_t (128 B).  The union gives
             * both the legacy decay-as-int* view (`bs->movestate`, used by the
             * opaque BotMoveToGoal / BotResetAvoidReach / … calls) and the
             * typed `bs->ms`.  `bs->areanum` at +2944 is the same memory as
             * `bs->ms.areanum`. */
            union {
                struct {
                    int    movestate[16];        /* +2880..+2943 legacy int[]: first 16 ints of bot_movestate_t */
                    int    areanum;              /* +2944 alias of bs->ms.areanum */
                    char   _pad_B84h[60];        /* +2948..+3007 remainder of bot_movestate_t (lastareanum..avoidreachtries) */
                };
                bot_movestate_t ms;              /* +2880..+3007 typed view */
            };
            bot_goalstate_t goalstate;    /* +3008..+3979 (972 B) */
            bot_chatstate_t chatstate;    /* +3980..+4167 (188 bytes; see chat_state.h) */
            int    weaponweights[7];      /* +4168..+4195 */
            int    enemy;                 /* +4196 */
            int    lastenemyareanum;      /* +4200 areanum of enemy's last-confirmed position */
            vec3_t lastenemyorigin;       /* +4204..+4215 enemy origin at last sighting */
            char   _pad_1058h[8];         /* +4216..+4223 */
            vec3_t viewangles;            /* +4224..+4235 current view angles, updated each frame in
                                           * BotUpdateClient via AngleMod(delta_angles + viewangles);
                                           * the "viewer angles" argument to BotEntityVisible /
                                           * InFieldOfVision, overwritten with ideal_viewangles before
                                           * EA_View. */
            vec3_t ideal_viewangles;      /* +4236..+4247 vectoangles dst; copied into the EA_View arg */
            vec3_t viewanglespeed;        /* +4248..+4259 per-axis view-angle change rate, clamped toward
                                           * ideal_viewangles each frame in BotChangeViewAngles */
            int    ltgtype;               /* +4260 */
            int    teammate;              /* +4264 */
            bot_goal_t teamgoal;          /* +4268..+4323 (56 B) */
            float  teammessage_time;      /* +4324 schedule LTG-start chat (Accompany/GetFlag/...) */
            float  teamgoal_time;         /* +4328 deadline for the current team LTG; AAS_Time() +
                                           * {60,120,180,240,300} when established or extended, dropped
                                           * when `AAS_Time() > teamgoal_time` */
            float  teammatevisible_time;  /* +4332 last time the teammate was NOT visible: AAS_Time() the
                                           * moment BotEntityVisible() returns false.  >10 s drops the LTG,
                                           * >60 s triggers the "cannot find you" chat. */
            char   formation_teammate[16];/* +4336..+4351 netname of the formation-position teammate */
            char   teamleader[32];        /* +4352..+4383 netname of the team leader */
            float  formation_dist;        /* +4384 spacing to hold from formation_teammate; default 100.0f,
                                           * set by chat-match type 16 to atof(arg)*32.0 (units) or
                                           * *9.7536 (feet), clamped to [48,500] */
            /* +4388..+4487 — BotGetFormationGoal's (0x1001D420) working set,
             * its only consumer; decomposes as 16+4+12+12+56 = 100. */
            char       formationgoal_name[16]; /* +4388 target netname; ClientFromName() input   */
            float      formationgoal_yawbias;  /* +4404 yaw bias added to the target's direction */
            vec3_t     formationgoal_dir;      /* +4408 target movement delta, then AngleVectors fwd */
            vec3_t     formationgoal_origin;   /* +4420 target origin, saved as the predict start */
            bot_goal_t formationgoal;          /* +4432..+4487 filled and returned by address    */
            bot_goal_t activategoal;      /* +4488..+4543 (56 B) passed by address to BotTouchingGoal /
                                           * BotMoveToGoal.  Mins/maxs are origin ± 5 with z-offsets;
                                           * areanum from AAS_PointAreaNum. */
            int    checkpoints;           /* +4544 LEGACY 4-byte slot for the bot_waypoint_t * head; the real
                                           * pointer is side-banded via BotCheckpoints(bs) — never read directly */
            int    patrolpoints;          /* +4548 LEGACY slot; real ptr in BotPatrolpoints(bs) */
            int    curpatrolpoint;        /* +4552 LEGACY slot; real ptr in BotCurPatrolPoint(bs) */
            int    patrolflags;           /* +4556 patrol direction/reverse flags */
        };
    };
} bot_state_t;

typedef int _bot_state_t_size_check[sizeof(bot_state_t) == BOT_STATE_SIZE ? 1 : -1];

#endif /* BOT_STATE_H */
