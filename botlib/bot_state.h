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

#include "botlib_structs.h"   /* vec3_t */

#define BOT_STATE_SIZE       4560

typedef struct bot_state_s {
    union {
        int _raw[BOT_STATE_SIZE / sizeof(int)];
        struct {
            int    inuse;                 /* +0    */
            int    client;                /* +4    */
            int    entitynum;             /* +8    */
            int    _i12;                  /* +12   */
            int    _i16;                  /* +16   */
            int    _i20;                  /* +20   */
            int    _i24;                  /* +24   */
            int    _i28;                  /* +28   */
            int    _i32;                  /* +32   */
            int    _i36;                  /* +36   */
            int    _i40;                  /* +40   flags (low byte at +40, second byte at +41) */
            char   _pad_2Ch[28];          /* +44..+71  */
            int    _i72;                  /* +72   */
            int    _i76;                  /* +76   */
            float  _f80;                  /* +80   */
            char   _pad_54h[36];          /* +84..+119 */
            int    _i120;                 /* +120  */
            char   _pad_7Ch[92];          /* +124..+215 */
            char   chat_lines_src[1024];  /* +216..+1239  qmemcpy source for chat_lines */
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
            union {
                char chat_lines[1024];                /* +1728..+2751 raw 1024-byte view */
                struct {
                    char   _pad_6C0h[164];        /* +1728..+1891 */
                    int    _i1892;                /* +1892 health snapshot copied from playerstate ps[154] */
                    char   _pad_764h[632];        /* +1896..+2527 */
                    int    enemy_horiz_dist;      /* +2528 (int)VectorLength(enemy.xy - bs.xy) */
                    int    enemy_height_diff;     /* +2532 (int)(enemy.z - bs.z) */
                    char   _pad_9E8h[8];          /* +2536..+2543 */
                    int    quad_seconds;          /* +2544 quad-damage seconds remaining (clamped >=0) */
                    int    invuln_seconds;        /* +2548 invulnerability seconds remaining */
                    char   _pad_9F4h[4];          /* +2552 — slot for a 5th powerup; never written in our codepaths */
                    int    rebreather_seconds;    /* +2556 rebreather seconds remaining */
                    int    enviro_seconds;        /* +2560 envirosuit seconds remaining */
                    char   _pad_A00h[4];          /* +2564 */
                    int    powershield_a;         /* +2568 powershield active-flag pair, set together */
                    int    powershield_b;         /* +2572 */
                    char   _pad_A0Ch[72];         /* +2576..+2647 */
                    /* +2648..+2692 — 12-entry per-weapon-class flag table set
                     * by BotUpdateBattleInventory from the enemy entity's
                     * current-weapon byte.  Switch->offset map is not a clean
                     * sequential array (case 5 → +2688, case 6..10 → +2668..+2684),
                     * so the slots are kept as individually-named ints. */
                    int    _i2648;                /* +2648 case 0  */
                    int    _i2652;                /* +2652 case 1  */
                    int    _i2656;                /* +2656 case 2  */
                    int    _i2660;                /* +2660 case 3  */
                    int    _i2664;                /* +2664 case 4  */
                    int    _i2668;                /* +2668 case 6  */
                    int    _i2672;                /* +2672 case 7  */
                    int    _i2676;                /* +2676 case 8  */
                    int    _i2680;                /* +2680 case 9  */
                    int    _i2684;                /* +2684 case 10 */
                    int    _i2688;                /* +2688 case 5  */
                    int    _i2692;                /* +2692 case 11 */
                    char   _pad_A88h[12];         /* +2696..+2707 */
                    int    _i2708;                /* +2708 enemy entity-flags high bit */
                    int    _i2712;                /* +2712 enemy entity-flags & 0x10000 */
                    int    _i2716;                /* +2716 enemy entity-flags bit 9 */
                    char   _pad_A9Ch[32];         /* +2720..+2751 */
                };
            };
            int    _i2752;                /* +2752 byte- and dword-accessed */
            int    _i2756;                /* +2756 */
            int    _i2760;                /* +2760 */
            int    _i2764;                /* +2764 */
            int    _i2768;                /* +2768 */
            int    _i2772;                /* +2772 */
            int    inuse_marker;          /* +2776 */
            int    _i2780;                /* +2780 */
            float  ltime;                 /* +2784 wall-clock accumulator; `bs->ltime += thinktime` per frame */
            float  setup_time;            /* +2788 */
            float  ltg_time;              /* +2792 long-term-goal re-pick throttle (set to AAS_Time()+20.0 after BotChooseLTGItem) */
            float  nbg_time;              /* +2796 nearby-goal timeout (set to AAS_Time()+5.0 after BotChooseNBGItem) */
            float  _f2800;                /* +2800 */
            float  _f2804;                /* +2804 */
            float  _f2808;                /* +2808 */
            float  _f2812;                /* +2812 */
            char   _pad_AFCh[4];          /* +2816..+2819 */
            float  _f2820;                /* +2820 */
            char   _pad_B04h[24];         /* +2824..+2847 */
            float  _f2848;                /* +2848 */
            float  _f2852;                /* +2852 */
            char   _pad_B1Ch[4];          /* +2856..+2859 */
            float  _f2860;                /* +2860 */
            float  _f2864;                /* +2864 */
            float  _f2868;                /* +2868 */
            float  _f2872;                /* +2872 */
            float  _f2876;                /* +2876 */
            int    movestate[16];         /* +2880..+2943 embedded movement/goal scratch (64 bytes) */
            int    _i2944;                /* +2944 */
            char   _pad_B84h[60];         /* +2948..+3007 */
            /* Note: a few stray byte-offset accesses outside the chat
             * region still go through the `_raw[]` union (e.g. +4248 in
             * the patrol-state loop).  Splitting more pads is fine; the
             * union keeps both spellings live against the same memory. */
            int    goalstate[243];        /* +3008..+3979 */
            int    chatstate[47];         /* +3980..+4167 */
            int    weaponweights[7];      /* +4168..+4195 */
            int    enemy;                 /* +4196 */
            int    lastenemyareanum;      /* +4200 areanum of enemy's last-confirmed position */
            vec3_t lastenemyorigin;       /* +4204..+4215 enemy origin at last sighting (written from AAS_EntityInfo origin) */
            char   _pad_1058h[8];         /* +4216..+4223 */
            vec3_t enemyorigin;           /* +4224..+4235 */
            vec3_t ideal_viewangles;      /* +4236..+4247  vectoangles dst; copied into EA_View vec3 arg */
            int    _i4248;                /* +4248 */
            char   _pad_1094h[8];         /* +4252..+4259 */
            int    ltgtype;               /* +4260 */
            int    teammate;              /* +4264 */
            bot_goal_t teamgoal;          /* +4268..+4323 (56 B; origin/areanum/mins/maxs/entitynum/number/flags/iteminfo) */
            float  teammessage_time;      /* +4324  schedule LTG-start chat (Accompany/GetFlag/...) */
            float  teammatevisible_time;  /* +4328 */
            int    _i4332;                /* +4332 */
            char   formation_teammate[16];/* +4336..+4351 netname of teammate used for formation positioning (Q3 backport: bs->formation_teammate[16]) */
            char   teamleader[32];        /* +4352..+4383 netname of team leader (Q3 backport: bs->teamleader[32]) */
            int    _i4384;                /* +4384 */
            char   _pad_1124h[100];       /* +4388..+4487 */
            bot_goal_t activategoal;      /* +4488..+4543 (56 B) embedded activate-goal: origin/areanum/mins/maxs/entitynum/number/flags/iteminfo.
                                           * Pointer to this is passed as `bot_goal_t *` to BotTouchingGoal/BotMoveToGoal.
                                           * Mins/maxs set as origin ± 5 with z-offsets; areanum filled from AAS_PointAreaNum. */
            int    checkpoints;           /* +4544 head of bot_waypoint_t linked list (chat /checkpoint cmd); walked via +60 (next) / +64 (prev). Q3 backport: bs->checkpoints. */
            int    patrolpoints;          /* +4548 head of patrol-checkpoints linked list */
            int    curpatrolpoint;        /* +4552 current waypoint the bot is going toward (Q3: curpatrolpoint) */
            int    patrolflags;           /* +4556 patrol direction/reverse flags (Q3: patrolflags) */
        };
    };
} bot_state_t;

typedef int _bot_state_t_size_check[sizeof(bot_state_t) == BOT_STATE_SIZE ? 1 : -1];

#endif /* BOT_STATE_H */
