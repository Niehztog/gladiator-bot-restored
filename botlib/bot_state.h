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
            float  _f1680;                /* +1680 random/skill factor */
            vec3_t origin;                /* +1684..+1695 */
            char   _pad_6B0h[16];         /* +1696..+1711 */
            vec3_t eye;                   /* +1712..+1723 */
            char   _pad_6BCh[4];          /* +1724..+1727 */
            /* +1728..+2751: structured per-target / enemy-observation region.
             * Verified offsets below come from `BotInitAttackTarget`
             * (BotUpdateBattleInventory) + sub_100204D0; remaining bytes stay padding.
             * Wrapped in an inner union so legacy `bs->chat_lines` (the
             * 1024-byte qmemcpy buffer) and the typed view share storage. */
            union {
                char chat_lines[1024];                /* +1728..+2751 raw 1024-byte view */
                struct {
                    char   _pad_6C0h[164];        /* +1728..+1891 */
                    int    _i1892;                /* +1892 read/written in sub_100204D0 */
                    char   _pad_764h[632];        /* +1896..+2527 */
                    int    _i2528;                /* +2528 target XY distance truncated to int (_ftol) */
                    int    _i2532;                /* +2532 target Z delta truncated to int (_ftol) */
                    char   _pad_9E4h[112];        /* +2536..+2647 */
                    int    _i2648;                /* +2648 \                            */
                    int    _i2652;                /* +2652 |                            */
                    int    _i2656;                /* +2656 |                            */
                    int    _i2660;                /* +2660 |                            */
                    int    _i2664;                /* +2664 |                            */
                    int    _i2668;                /* +2668 |  12-entry per-weapon       */
                    int    _i2672;                /* +2672 |  target-class flag table   */
                    int    _i2676;                /* +2676 |  (one int set per Q2 wid)  */
                    int    _i2680;                /* +2680 |                            */
                    int    _i2684;                /* +2684 |                            */
                    int    _i2688;                /* +2688 |                            */
                    int    _i2692;                /* +2692 /                            */
                    char   _pad_A88h[12];         /* +2696..+2707 */
                    int    _i2708;                /* +2708 entity event flag bit (& 0x10000) */
                    int    _i2712;                /* +2712 entity flags sign bit */
                    int    _i2716;                /* +2716 entity flags & 2 */
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
            float  _f2784;                /* +2784 */
            float  setup_time;            /* +2788 */
            float  _f2792;                /* +2792 */
            float  _f2796;                /* +2796 */
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
            int    _i4200;                /* +4200 */
            int    _i4204;                /* +4204 */
            int    _i4208;                /* +4208 */
            int    _i4212;                /* +4212 */
            char   _pad_1058h[8];         /* +4216..+4223 */
            vec3_t enemyorigin;           /* +4224..+4235 */
            int    _i4236;                /* +4236 */
            int    _i4240;                /* +4240 */
            int    _i4244;                /* +4244 */
            int    _i4248;                /* +4248 */
            char   _pad_1094h[8];         /* +4252..+4259 */
            int    ltgtype;               /* +4260 */
            int    teammate;              /* +4264 */
            vec3_t teamgoal;              /* +4268..+4279 */
            int    _i4280;                /* +4280 */
            int    _i4284;                /* +4284 */
            int    _i4288;                /* +4288 */
            int    _i4292;                /* +4292 */
            int    _i4296;                /* +4296 */
            int    _i4300;                /* +4300 */
            int    _i4304;                /* +4304 */
            int    _i4308;                /* +4308 */
            int    _i4312;                /* +4312 */
            char   _pad_10D8h[8];         /* +4316..+4323 */
            float  _f4324;                /* +4324 */
            float  teammatevisible_time;  /* +4328 */
            int    _i4332;                /* +4332 */
            int    _i4336;                /* +4336 */
            char   _pad_10F0h[12];        /* +4340..+4351 */
            int    _i4352;                /* +4352 byte-accessed at low + second byte */
            char   _pad_1104h[28];        /* +4356..+4383 */
            int    _i4384;                /* +4384 */
            char   _pad_1124h[100];       /* +4388..+4487 */
            float  _f4488;                /* +4488 */
            float  _f4492;                /* +4492 */
            float  _f4496;                /* +4496 */
            int    _i4500;                /* +4500 */
            float  _f4504;                /* +4504 */
            float  _f4508;                /* +4508 */
            float  _f4512;                /* +4512 */
            float  _f4516;                /* +4516 */
            float  _f4520;                /* +4520 */
            float  _f4524;                /* +4524 */
            int    _i4528;                /* +4528 */
            int    _i4532;                /* +4532 */
            int    _i4536;                /* +4536 */
            char   _pad_11BCh[4];         /* +4540..+4543 */
            int    _i4544;                /* +4544 */
            int    patrolpoints;          /* +4548 */
            int    _i4552;                /* +4552 */
            int    _i4556;                /* +4556 */
        };
    };
} bot_state_t;

typedef int _bot_state_t_size_check[sizeof(bot_state_t) == BOT_STATE_SIZE ? 1 : -1];

#endif /* BOT_STATE_H */
