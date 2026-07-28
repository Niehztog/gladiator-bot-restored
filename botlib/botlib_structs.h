/*
 * botlib_structs.h — Reconstructed Gladiator botlib structs.
 *
 * soundinfo_t, iteminfo_t, weaponinfo_t and projectileinfo_t are byte-exact:
 * their field tables survive in the original .data section and are recovered
 * in botlib_structdefs.c, so every offset and type comes straight from there.
 *
 * weaponinfo_t and projectileinfo_t are Q2-extended versions of the Q3
 * structs (344 vs 180 B, 208 vs 76 B); iteminfo_t and soundinfo_t have no
 * Q3 counterpart.
 */

#ifndef BOTLIB_STRUCTS_H
#define BOTLIB_STRUCTS_H

#include "gladiator.dll.h"   /* vec3_t */

#define MAX_INFOSTRING   80     /* "name" / "model" field width (stride 0x50) */
#define MAX_SHORTSTRING  80

/* soundinfo_t — descriptor unk_1005C138, field table at .data 0x1005C070. */
typedef struct soundinfo_s {
    char  name[80];          /* +0x00 — sound symbolic name (MAX_SHORTSTRING) */
    float volume;            /* +0x50 — playback volume (default 80.0)        */
    float duration;          /* +0x54 — duration in seconds (default 10.0)    */
    int   type;              /* +0x58 — sound type tag                        */
    float recognition;       /* +0x5C — recognition weight (default 1.0)      */
    char  string[80];        /* +0x60 — sound file path                       */
} soundinfo_t;               /* sizeof = 0xB0 = 176 */

/* iteminfo_t — descriptor unk_1005D890. */
typedef struct iteminfo_s {
    char    name[80];        /* +0x000 — item config name (e.g. "Shotgun")    */
    char    dispname[80];    /* +0x050 — spawn classname for BSP matching (set by LoadItemConfig) */
    char    model[80];       /* +0x0A0 — model path                           */
    int     modelindex;      /* +0x0F0 — engine model index, = IndexFromModel(model); filled by BotInitLevelItems */
    int     type;            /* +0x0F4 — item-type bitfield                   */
    int     index;           /* +0x0F8 — item index in inventory              */
    float   respawntime;     /* +0x0FC — respawn delay in seconds             */
    vec3_t  mins;            /* +0x100 — bounding-box mins                    */
    vec3_t  maxs;            /* +0x10C — bounding-box maxs                    */
    int     number;          /* +0x118 — loader-assigned index in items[]     */
} iteminfo_t;                /* sizeof = 0x11C = 284 */

/* itemconfig_t — container returned by LoadItemConfig: 8-byte header with the
 * iteminfo_t entries inline right after it. */
typedef struct itemconfig_s {
    int          numitems;        /* +0  populated count of items[]         */
    iteminfo_t  *items;            /* +4  → base + 8                         */
} itemconfig_t;                    /* sizeof = 8 (header only) */

/* weaponinfo_t — descriptor unk_1005DFD8.  Same field names as Q3, larger
 * offsets.  The leading dword before `name` is Q3's `valid` flag. */
typedef struct projectileinfo_s projectileinfo_t;  /* fwd decl for proj field */
typedef struct weaponinfo_s {
    int                number;          /* +0x000 — index in weaponconfig array (set by LoadWeaponConfig) */
    char               name[80];        /* +0x004 — weapon symbolic name                 */
    char               model[80];       /* +0x054 — weapon model path                    */
    int                level;           /* +0x0A4 — weapon level                         */
    int                weaponindex;     /* +0x0A8 — index in inventory                   */
    int                flags;           /* +0x0AC — weapon flags                         */
    char               projectile[80];  /* +0x0B0 — projectile name (looked up by name)  */
    int                numprojectiles;  /* +0x100 — projectiles per shot                 */
    float              hspread;         /* +0x104 — horizontal spread                    */
    float              vspread;         /* +0x108 — vertical spread                      */
    float              speed;           /* +0x10C — projectile speed                     */
    float              acceleration;    /* +0x110 — projectile acceleration              */
    vec3_t             recoil;          /* +0x114 — recoil impulse                       */
    vec3_t             offset;          /* +0x120 — muzzle offset from view origin       */
    vec3_t             angleoffset;     /* +0x12C — angular offset from view             */
    float              extrazvelocity;  /* +0x138 — extra Z velocity for projectiles     */
    int                ammoamount;      /* +0x13C — ammo per shot                        */
    int                ammoindex;       /* +0x140 — ammo type index                      */
    float              activate;        /* +0x144 — activation/lift time                 */
    float              reload;          /* +0x148 — reload time                          */
    float              spinup;          /* +0x14C — spinup time                          */
    float              spindown;        /* +0x150 — spindown time                        */
    projectileinfo_t  *proj;            /* +0x154 — resolved projectileinfo pointer (Gladiator-specific; Q3 embeds the projectileinfo) */
} weaponinfo_t;                         /* sizeof = 0x158 = 344 */

/* projectileinfo_t — descriptor unk_1005DFE0, field table at .data 0x1005DE30. */
struct projectileinfo_s {
    char    name[84];        /* +0x00 — 84, not 80: the descriptor puts model
                                at +0x54                                       */
    char    model[76];       /* +0x54 — projectile model path; ends at 0xA0   */
    int     flags;           /* +0xA0 — projectile flags                      */
    float   gravity;         /* +0xA4 — gravity multiplier                    */
    int     damage;          /* +0xA8 — direct hit damage                     */
    float   radius;          /* +0xAC — explosion radius                      */
    int     visdamage;       /* +0xB0 — visual / radius damage                */
    int     damagetype;      /* +0xB4 — damage-type bitmask                   */
    int     healthinc;       /* +0xB8 — health gained on hit (negative=hurt)  */
    float   push;            /* +0xBC — knockback push amount                 */
    float   detonation;      /* +0xC0 — detonation timer                      */
    float   bounce;          /* +0xC4 — bounce factor                         */
    float   bouncefric;      /* +0xC8 — bounce friction                       */
    float   bouncestop;      /* +0xCC — bounce stop velocity                  */
};                           /* sizeof = 0xD0 = 208 */

/* weaponconfig_t — container returned by LoadWeaponConfig as one allocation:
 * 16-byte header + numweapons*344 + numprojectiles*208.  Pointers rather than
 * Q3's inline arrays, because the limits come from the max_weaponinfo /
 * max_projectileinfo libvars at runtime. */
typedef struct weaponconfig_s {
    int                numweapons;       /* +0  populated count of weaponinfo[]     */
    int                numprojectiles;   /* +4  populated count of projectileinfo[] */
    projectileinfo_t  *projectileinfo;   /* +8  → base + 16 + 344*numweapons        */
    weaponinfo_t      *weaponinfo;       /* +12 → base + 16                         */
} weaponconfig_t;                        /* sizeof = 16 (header only) */

/* ---- Script-parser structs (l_script.h / l_precomp.h equivalents) ------- */

/* punctuation_t — layout as Q3. */
typedef struct punctuation_s {
    char                  *p;       /* +0  punctuation literal text   */
    int                    n;       /* +4  punctuation type id        */
    struct punctuation_s  *next;    /* +8  chain                      */
} punctuation_t;                    /* sizeof = 12 */

/* indent_t — layout as Q3. */
typedef struct indent_s {
    int                    type;    /* +0  INDENT_IF / IFDEF / ELSE / ELIF / ... */
    int                    skip;    /* +4  > 0: skipping until matching #endif   */
    struct script_s       *script;  /* +8  script that opened this indent        */
    struct indent_s       *next;    /* +12 next on indent stack                  */
} indent_t;                         /* sizeof = 16 */

/* define_t — layout as Q3 l_precomp.h::define_t. */
typedef struct define_s {
    char                  *name;    /* +0  macro name                              */
    int                    flags;   /* +4  DEFINE_FIXED etc.                       */
    int                    builtin; /* +8  > 0 for built-in defines (__LINE__ etc.) */
    int                    numparms;/* +12 number of parameter tokens              */
    struct token_s *parms;/* +16 parameter token list                    */
    struct token_s *tokens;/* +20 macro body token list                   */
    struct define_s       *next;    /* +24 hash-chain next                          */
    struct define_s       *hashnext;/* +28                                        */
} define_t;                         /* sizeof = 32 */

/* script_t — 1392-byte header followed by the file data inline.  Differs from
 * Q3: filename buffer trimmed from 1024 to 260 bytes, and the embedded token
 * is token_t (1072 B, double floatvalue) rather than Q3's float variant. */
typedef struct script_s {
    char                   filename[260];      /* +0    file path (strcpy at sub_100401A0) */
    char                  *buffer;             /* +260  start of file data buffer           */
    char                  *script_p;           /* +264  current parse pointer               */
    char                  *end_p;              /* +268  one-past-end of buffer              */
    char                  *lastscript_p;       /* +272  start of last token                 */
    char                  *whitespace_p;       /* +276  start of preceding whitespace       */
    char                  *endwhitespace_p;    /* +280  end of preceding whitespace         */
    int                    length;             /* +284  buffer length in bytes              */
    int                    line;               /* +288  current source line (1-based)       */
    int                    lastline;           /* +292  line of last token                  */
    int                    tokenavailable;     /* +296  pushed-back token flag              */
    int                    flags;              /* +300  script flags                        */
    punctuation_t         *punctuations;       /* +304  per-script punctuation list head    */
    punctuation_t        **punctuationtable;   /* +308  perfect-hash table (FreeScript frees)*/
    struct token_s token;            /* +312..+1383  embedded last token          */
    struct script_s       *next;               /* +1384 next in scriptstack chain           */
    int                    _trail;             /* +1388 trailing padding (memset clears 1392)*/
    /* +1392 onwards: file data lives inline after the header */
} script_t;                                    /* sizeof = 1392 (header only) */

/* source_t — 1624 bytes.  Q2 trims Q3's 1024-byte path buffers and adds the
 * separately-allocated 1024-byte definebuffer scratchpad at +308. */
typedef struct source_s {
    char                   filename[260];      /* +0    source filename                     */
    char                   includepath[48];    /* +260  base path for #include resolution   */
    char                  *definebuffer;       /* +308  scratch buffer (sub_1003E120)  */
    char                   _pad_1[212];        /* +312..+523 reserved/unknown               */
    script_t              *scriptstack;        /* +524  current script-include stack head   */
    struct token_s *tokens;          /* +528  pushed-back token list              */
    define_t              *defines;            /* +532  define list head (linear)           */
    define_t             **definehash;         /* +536  define hash table (4096 B)          */
    indent_t              *indentstack;        /* +540  conditional-compile indent stack    */
    int                    skip;               /* +544  > 0 skipping #if/#else block        */
    int                    _pad_after_skip;    /* +548  unused / padding                    */
    struct token_s         cachedtoken;        /* +552  last-read token (memcpy'd by PC_ReadTokenHandle) */
} source_t;                                    /* sizeof = 1624 on 32-bit */

/* ---- AI weight structs (be_ai_weight.h equivalents) -------------------- */
typedef struct fuzzyseperator_s {
    int                       index;       /* +0  fact index             */
    int                       value;       /* +4  comparison value       */
    int                       type;        /* +8  comparison kind        */
    float                     weight;      /* +12 base weight            */
    float                     minweight;   /* +16 lower bound            */
    float                     maxweight;   /* +20 upper bound            */
    struct fuzzyseperator_s  *child;       /* +24 nested branch          */
    struct fuzzyseperator_s  *next;        /* +28 sibling separator      */
} fuzzyseperator_t;                        /* sizeof = 32 */

typedef struct weight_s {
    char                     *name;        /* +0  weight rule name       */
    fuzzyseperator_t         *firstseperator; /* +4 first separator      */
} weight_t;                                /* sizeof = 8 */

/* weightconfig_t — 1028 bytes, inline weight array.  No filename field
 * (Q3 has one). */
#define MAX_FUZZY_WEIGHTS 128
typedef struct weightconfig_s {
    int       numweights;                       /* +0  weight count           */
    weight_t  weights[MAX_FUZZY_WEIGHTS];       /* +4  inline weight array    */
} weightconfig_t;                               /* sizeof = 4 + 128*8 = 1028  */

/* Synonym structs (be_ai_chat.c equivalents, used by the syn.c loader
 * sub_1002B110).  The original packed these with byte-stride arithmetic;
 * here the scratch buffer must be sized with sizeof() so 8-byte pointers
 * fit.  `context` is int, not Q3's unsigned long, to keep the 32-bit size. */
typedef struct bot_synonym_s {
    char                     *string;       /* +0   inline string ptr            */
    float                     weight;       /* +4(32) / +8(64) probability       */
    struct bot_synonym_s     *next;         /* +8(32) / +16(64) chain link       */
} bot_synonym_t;                            /* sizeof = 12 (32-bit) / 24 (64-bit) */

typedef struct bot_synonymlist_s {
    int                       context;       /* +0  context bitmask              */
    float                     totalweight;   /* +4  sum of synonym weights       */
    struct bot_synonym_s     *firstsynonym;  /* +8(32) / +16(64) head of synonyms*/
    struct bot_synonymlist_s *next;          /* +12(32)/+24(64) chain link       */
} bot_synonymlist_t;                         /* sizeof = 16 (32-bit) / 32 (64-bit)*/

/* Random-string structs, used by the rnd.c loader (sub_1002B990).  Same
 * sizeof()-not-byte-arithmetic requirement as the synonym loader above. */
typedef struct bot_randomstring_s {
    char                       *string;     /* +0  inline string ptr           */
    struct bot_randomstring_s  *next;       /* +4(32) / +8(64) chain link       */
} bot_randomstring_t;                       /* sizeof = 8 (32-bit) / 16 (64-bit) */

typedef struct bot_randomlist_s {
    char                       *string;             /* +0  name token */
    int                         numstrings;         /* +4 */
    struct bot_randomstring_s  *firstrandomstring;  /* +8(32) / +16(64) */
    struct bot_randomlist_s    *next;               /* +12(32) / +24(64) */
} bot_randomlist_t;                                 /* sizeof = 16 / 32 */

/* bot_goal_t — 56 bytes, as Q3 be_ai_goal.h.  The trailing flags/iteminfo
 * fields are part of the layout (goal memcpys move 56 bytes) but nothing
 * writes them: BotGetLevelItemGoal fills the leading 48. */
typedef struct bot_goal_s {
    vec3_t                    origin;        /* +0   goal world position    */
    int                       areanum;       /* +12  AAS area number        */
    vec3_t                    mins;          /* +16  bounding-box mins      */
    vec3_t                    maxs;          /* +28  bounding-box maxs      */
    int                       entitynum;     /* +40  entity number (-1 none)*/
    int                       number;        /* +44  iteminfo number        */
    int                       flags;         /* +48  Q3 field; unused in Gladiator */
    int                       iteminfo;      /* +52  Q3 field; unused in Gladiator */
} bot_goal_t;                                /* sizeof = 56 */

/* levelitem_t — fixed-size pool allocated by InitLevelItemHeap
 * (52 * max_levelitems).  The free list is singly linked via `next`, the
 * active list doubly linked via `prev`/`next`. */
typedef struct levelitem_s {
    int                       number;       /* +0  entitynum + map base bias */
    int                       iteminfo;     /* +4  itemconfig->items[] index */
    vec3_t                    origin;       /* +8  world position       */
    int                       areanum;      /* +20 AAS area number      */
    vec3_t                    goalorigin;   /* +24 from AAS_BestReachableArea */
    int                       entitynum;    /* +36 BSP entity number    */
    float                     timeout;      /* +40 respawn/expiry time  */
    struct levelitem_s       *prev;         /* +44 active-list back ptr */
    struct levelitem_s       *next;         /* +48 list link            */
} levelitem_t;                               /* sizeof = 52 */

/* aas_settings_t — 37 movement-physics floats, loaded from the libvar_sv_*
 * cvars in AAS_LoadSettings.  Field order as Q3. */
typedef struct aas_settings_s {
    float phys_gravitydirection[3];  /* +0   normalised gravity direction */
    float phys_friction;             /* +12                                */
    float phys_stopspeed;            /* +16                                */
    float phys_gravity;              /* +20                                */
    float phys_waterfriction;        /* +24                                */
    float phys_watergravity;         /* +28                                */
    float phys_maxvelocity;          /* +32                                */
    float phys_maxwalkvelocity;      /* +36                                */
    float phys_maxcrouchvelocity;    /* +40                                */
    float phys_maxswimvelocity;      /* +44                                */
    float phys_walkaccelerate;       /* +48                                */
    float phys_airaccelerate;        /* +52                                */
    float phys_swimaccelerate;       /* +56                                */
    float phys_maxstep;              /* +60                                */
    float phys_maxsteepness;         /* +64                                */
    float phys_maxwaterjump;         /* +68                                */
    float phys_maxbarrier;           /* +72                                */
    float phys_jumpvel;              /* +76                                */
    float phys_falldelta5;           /* +80                                */
    float phys_falldelta10;          /* +84                                */
    float rs_waterjump;              /* +88                                */
    float rs_teleport;               /* +92                                */
    float rs_barrierjump;            /* +96                                */
    float rs_startcrouch;            /* +100                               */
    float rs_startgrapple;           /* +104                               */
    float rs_startwalkoffledge;      /* +108                               */
    float rs_startjump;              /* +112                               */
    float rs_rocketjump;             /* +116                               */
    float rs_bfgjump;                /* +120                               */
    float rs_jumppad;                /* +124                               */
    float rs_aircontrolledjumppad;   /* +128                               */
    float rs_funcbob;                /* +132                               */
    float rs_startelevator;          /* +136                               */
    float rs_falldamage5;            /* +140                               */
    float rs_falldamage10;           /* +144                               */
} aas_settings_t;                    /* sizeof = 148 */

/* PC_EvaluateTokens / PC_DollarEvaluate value- and operator-cell lists.
 * Q3's l_precomp.c shape but with `double floatvalue`.  Allocate with
 * sizeof() — the original's fixed 32/24 bytes only hold on 32-bit. */
typedef struct value_s {
    int intvalue;                /* +0  */
    int _pad0;                   /* +4  alignment for double */
    double floatvalue;           /* +8  */
    int parentheses;             /* +16 */
    /* Compiler auto-pads 4 bytes here on 64-bit ABIs to align prev/next. */
    struct value_s *prev;        /* +20 on 32-bit, +24 on 64-bit */
    struct value_s *next;        /* +24 on 32-bit, +32 on 64-bit */
} value_t;

typedef struct operator_s {
    int op;
    int priority;
    int parentheses;
    struct operator_s *prev;
    struct operator_s *next;
} operator_t;

/* bsp_link_t — entity<->BSP-leaf link node, 24 bytes on 32-bit (40 on
 * 64-bit).  Same layout as Q3.  AAS_InitAASLinkHeap free-lists nodes through
 * the next_ent/prev_ent slots. */
typedef struct bsp_link_s {
    int                 entnum;
    int                 leafnum;
    struct bsp_link_s  *next_ent;
    struct bsp_link_s  *prev_ent;
    struct bsp_link_s  *next_leaf;
    struct bsp_link_s  *prev_leaf;
} bsp_link_t;

/* bsp_entdata_t — entity bbox/solid snapshot filled by AAS_EntityBSPData;
 * 56 bytes, pointer-free.  Same layout as Q3. */
typedef struct bsp_entdata_s {
    vec3_t  origin;
    vec3_t  angles;
    vec3_t  absmins;
    vec3_t  absmaxs;
    int     solid;
    int     modelnum;   /* entity modelindex - 1 */
} bsp_entdata_t;

/* indexlist_t — model/sound/image name lookup table used by
 * aasworld.modelindex_table / soundindex_table / imageindex_table.  The
 * char* slot array trails the header, so `indexes` points at (this + 1) and
 * the allocation must be computed with sizeof(), not 4*n + 8. */
typedef struct indexlist_s {
    int    numindexes;
    char **indexes;
} indexlist_t;

/* bsp_epair_t / bsp_entity_t — BSP entity-lump epair list; same layout as Q3
 * be_aas_bspq2.c.  12 bytes on 32-bit, 24 on 64-bit. */
typedef struct bsp_epair_s {
    char               *key;
    char               *value;
    struct bsp_epair_s *next;
} bsp_epair_t;

typedef struct bsp_entity_s {
    bsp_epair_t        *epairs;
    struct bsp_entity_s *next;
} bsp_entity_t;

/* bot_moveresult_t — result of one movement tick, filled by BotMoveToGoal /
 * BotMoveInGoalArea / the BotTravel_* builders.  The Q3 layout MINUS the
 * `weapon` field Q3 later inserted at +0x18 for grapple-as-movement-weapon,
 * so movedir sits at +0x18 and the struct is 48 bytes, not Q3's 52.
 *
 * flags: 1=uses view for movement, 2=uses view for swimming,
 *        4=waiting for something, 8=view set by movement code.
 * type:  1=elevator up / wait-for-mover.
 * ------------------------------------------------------------------------- */
typedef struct bot_moveresult_s {
    int    failure;             /* +0x00 movement failed all together        */
    int    type;                /* +0x04 failure or blocked type             */
    int    blocked;             /* +0x08 blocked by an entity                */
    int    blockentity;         /* +0x0C entity number blocking the bot      */
    int    traveltype;          /* +0x10 last executed AAS travel type       */
    int    flags;               /* +0x14 result flags (see above)            */
    vec3_t movedir;             /* +0x18 movement direction                  */
    vec3_t ideal_viewangles;    /* +0x24 ideal view angles for the movement  */
} bot_moveresult_t;             /* sizeof = 0x30 = 48 */

#endif /* BOTLIB_STRUCTS_H */
