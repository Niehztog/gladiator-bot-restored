/*
 * be_ai_def.h — the bot-AI internal definitions: the structures the be_ai_*,
 * be_ai2_* and be_ea translation units are built on, plus a roll-up of their
 * interfaces.  The exact counterpart of be_aas_def.h, built on the same two
 * conventions Q3 botlib uses:
 *
 *   - a `*_def.h` holds one subsystem's shared types and then #includes that
 *     subsystem's per-TU prototype headers (Q3's be_aas_def.h does precisely
 *     this for its twelve be_aas_*.h);
 *   - types shared by several botlib TUs but never seen by the game module
 *     stay inside botlib/ (Q3 keeps weightconfig_t in botlib/be_ai_weight.h
 *     for exactly that reason).
 *
 * Q3 has no be_ai_def.h because its bot AI lives in the GAME module, so
 * bot_goal_t / bot_match_t / bot_moveresult_t are public API in code/game/.
 * Gladiator's bot AI is inside botlib -- be_ai2_dmq2.c and friends are lcc.mak
 * objects -- and game.dll sees only game/botlib.h, so the same types are
 * internal here and belong on this side of the line.
 *
 * The three sections below are the reconstruction-era botlib_structs.h,
 * chat_state.h and bot_state.h, verbatim and in the order the preprocessor
 * already resolved them (bot_state.h included the other two).  Splitting them
 * further by subsystem is a separate step: their types cross-reference, so it
 * is a type-graph reordering, not a move.
 */
#ifndef BOTLIB_BE_AI_DEF_H
#define BOTLIB_BE_AI_DEF_H

/* ---- from botlib_structs.h ---- */
#define MAX_INFOSTRING   80     /* "name" / "model" field width (stride 0x50) */
#define MAX_SHORTSTRING  80

/* soundinfo_t — descriptor soundinfo_struct, field table at .data 0x1005C070. */
typedef struct soundinfo_s {
    char  name[80];          /* +0x00 — sound symbolic name (MAX_SHORTSTRING) */
    float volume;            /* +0x50 — playback volume (default 80.0)        */
    float duration;          /* +0x54 — duration in seconds (default 10.0)    */
    int   type;              /* +0x58 — sound type tag                        */
    float recognition;       /* +0x5C — recognition weight (default 1.0)      */
    char  string[80];        /* +0x60 — sound file path                       */
} soundinfo_t;               /* sizeof = 0xB0 = 176 */

/* iteminfo_t — descriptor iteminfo_struct. */
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

/* weaponinfo_t — descriptor weaponinfo_struct.  Same field names as Q3, larger
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

/* projectileinfo_t — descriptor projectileinfo_struct, field table at .data 0x1005DE30. */
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

/* ---- from chat_state.h ---- */
#define MAX_MESSAGE_SIZE       0x96   /* 150 */
#define MAX_MATCHVARIABLES     10

/* Match piece types (bot_matchpiece_t.type) */
#define MT_STRING              2
#define MT_VARIABLE            1

/* ---- Pattern templates (loaded from match.c) ----------------------------
 *
 * bot_matchstring_t — alternative string in a MT_STRING piece; list head at
 * bot_matchpiece_t.firststring. */
typedef struct bot_matchstring_s {
    char *string;                          /* offset  0 */
    struct bot_matchstring_s *next;        /* offset  4 */
} bot_matchstring_t;                       /* sizeof = 8 */

/* bot_matchpiece_t — one piece of a chat match pattern.  type=2 (MT_STRING)
 * means "match against any of firststring"; type=1 (MT_VARIABLE) means
 * "capture the substring up to the next MT_STRING into variables[variable]". */
typedef struct bot_matchpiece_s {
    int  type;                              /* offset  0 — MT_STRING or MT_VARIABLE */
    bot_matchstring_t       *firststring;   /* offset  4 — type==MT_STRING */
    int  variable;                          /* offset  8 — type==MT_VARIABLE: index 0..MAX_MATCHVARIABLES-1 */
    struct bot_matchpiece_s *next;          /* offset 12 — next piece in pattern */
} bot_matchpiece_t;                         /* sizeof = 16 */

/* bot_matchtemplate_t — one entry in the match-template list (head at
 * dword_10064378). */
typedef struct bot_matchtemplate_s {
    int  context;                                /* offset  0 — bitmask of contexts in which this pattern is active */
    int  type;                                   /* offset  4 — match-type returned to caller */
    int  subtype;                                /* offset  8 — match-subtype returned to caller */
    bot_matchpiece_t           *first;           /* offset 12 — first piece of the pattern */
    struct bot_matchtemplate_s *next;            /* offset 16 — next template */
} bot_matchtemplate_t;                           /* sizeof = 20 */

/* ---- Match-result structure (filled by BotFindMatch, read by AI) -------- */

/* bot_matchvariable_t — one captured variable.  Q2 stores a real char*
 * (pointer into match->string), unlike Q3 which stores a 1-byte offset. */
typedef struct bot_matchvariable_s {
    char *ptr;                                   /* offset  0 — start of capture inside match.string */
    int   length;                                /* offset  4 — capture length in bytes */
} bot_matchvariable_t;                           /* sizeof = 8 */

/* bot_match_t — result struct passed back to the AI when BotFindMatch hits. */
typedef struct bot_match_s {
    char string[MAX_MESSAGE_SIZE + 2];           /* offset   0 — chat message (150 used + 2 padding to 4-byte align) */
    int  type;                                   /* offset 152 — match type */
    int  subtype;                                /* offset 156 — match subtype */
    bot_matchvariable_t variables[MAX_MATCHVARIABLES]; /* offset 160 — 10 captures × 8 bytes = 80 bytes */
} bot_match_t;                                   /* sizeof = 240 */

/* ---- Reply-chat structures (rchat.c) ----------------------------------- */

/* bot_chatstate_t — per-client chat state embedded in bot_state_t at +3980,
 * 47 ints = 188 bytes.  Slots [43..46] hold pointers in the 32-bit original
 * and are routed through the side-band arrays (`botchatdumps`,
 * `botchatmsglinks`); they stay 4-byte ints to preserve the 188-byte size. */
typedef struct bot_chatstate_s {
    union {
        int  _slots[47];                         /* opaque payload — preserves the 188-byte size */
        struct {
            int  gender;                         /* +0   0=neutral, 1=female, 2=male; set by
                                                  * BotSetupClient from Characteristic_String(…, 3) */
            char name[16];                       /* +4   chat name; BotSetChatName memsets 15 and
                                                  * strncpy-limits to 15, so name[15] terminates */
            /* +20..+171 is ONE buffer holding the outgoing chat message:
             * BotChatLength strlen()s it, BotEnterChat passes it to
             * EA_Say/EA_SayTeam then empties it, BotConstructChatMessage
             * writes into it. */
            char chatmessage[152];               /* +20..+171 */
            int  _slot_43;                       /* +172 side-banded first-msg ptr */
            int  _slot_44;                       /* +176 side-banded last-msg ptr */
            int  _slot_45;                       /* +180 side-banded count (mirror only) */
            int  _slot_46;                       /* +184 side-banded chatdump ptr */
        };
    };
} bot_chatstate_t;                               /* sizeof = 188 */

/* bot_chatmessage_t — one message body inside a reply-chat entry. */
typedef struct bot_chatmessage_s {
    char *chatmessage;                          /* +0  message text (points just past header) */
    float time;                                 /* +4(32)/+8(64)  last-used timestamp; init to -40.0f */
    struct bot_chatmessage_s *next;             /* +8(32)/+16(64) chain link */
} bot_chatmessage_t;                            /* sizeof = 12 / 24 */

/* bot_replychatkey_t — one trigger keyword in a reply-chat entry.
 * `flags` is a bitfield: 1=AND, 2=OR, 4=NAME, 8=STRING, 0x10=MATCH,
 * 0x20=FEMALE, 0x40=MALE, 0x80=IT. */
typedef struct bot_replychatkey_s {
    int   flags;                                /* +0 */
    char *string;                               /* +4(32)/+8(64) literal keyword (STRING flag) */
    struct bot_matchpiece_s *match;             /* +8(32)/+16(64) match-piece chain (MATCH flag) */
    struct bot_replychatkey_s *next;            /* +12(32)/+24(64) next key */
} bot_replychatkey_t;                           /* sizeof = 16 / 32 */

/* bot_replychat_t — one entry in the reply-chat list (head at
 * dword_10064380).  No `cmd` field and no name, unlike Q3. */
typedef struct bot_replychat_s {
    bot_replychatkey_t       *keys;             /* +0  trigger key chain */
    float                     priority;         /* +4 */
    int                       numchatmessages;  /* +8 */
    bot_chatmessage_t        *firstchatmessage; /* +12(32)/+16(64) */
    struct bot_replychat_s   *next;             /* +16(32)/+24(64) */
} bot_replychat_t;                              /* sizeof = 20 / 32 */

/* bot_stringlist_t — generic string node used by the missing-random-key
 * integrity checker (BotCheckChatMessageIntegrety) and BotFindStringInList.
 * Header allocated inline with the string immediately following. */
typedef struct bot_stringlist_s {
    char                       *string;         /* +0 (points to data after header) */
    struct bot_stringlist_s    *next;           /* +4(32)/+8(64) */
} bot_stringlist_t;                             /* sizeof = 8 / 16 */

/* bot_consolemessage_t — entry in the free-listed console-message pool
 * (sub_1002A880 allocates the array, sub_1002A9A0 pops, sub_1002A9E0
 * returns nodes).  168-byte node on 32-bit with prev/next at +160/+164;
 * accessed via struct fields so sizeof() adapts to 64-bit pointers. */
typedef struct bot_consolemessage_s {
    float                          time;         /* +0  AAS_Time stamp */
    int                            type;         /* +4  message type/level */
    char                           message[152]; /* +8  message text (152 bytes -> +160) */
    struct bot_consolemessage_s   *prev;         /* +160 */
    struct bot_consolemessage_s   *next;         /* +164 / 64-bit: +168 */
} bot_consolemessage_t;                          /* sizeof = 168 / 176 */

/* ---- from bot_state.h ---- */
/* ../game/botlib.h has no include guards, so it cannot be re-included here:
 * includers must pull it in (after q_shared.h) before this header to make
 * bot_updateclient_t visible. */

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

/* ---- from the reconstruction-era botlib_local.h ---- */
/* bot_character (botlib-private):
 *   { int numcharacteristics; bot_characteristic_t pairs[N]; char strings[]; }
 * Each pair is an int[2]: a byte-sized type at +0 (padded) and the value at +4.
 * The byte field stays explicit so MSVC6 emits the same byte accesses, while
 * intptr_t keeps the value slot pointer-sized for the 64-bit port. */
typedef struct bot_characteristic_s {
    unsigned char type; /* 0=unset, 1=int, 2=float, 3=string */
    intptr_t      value; /* int, float (via *(float*)&value), or char * */
} bot_characteristic_t;

typedef struct bot_character_s {
    int numcharacteristics;
} bot_character_t;

/* bs->weaponweights is a flattened inline `int[7]`, five slots of which hold
 * pointers.  Mirrored into this typed struct (one per bot) on 64-bit; the field
 * offsets match the original inline layout (+0 … +24). */
typedef struct bot_weaponstate_s {
    int               client;       /* +0   = bs->client copy */
    int              *inventory;    /* +4   pointer into bs->inventory (item-count array; cast as int* for FuzzyWeight) */
    weightconfig_t   *weightconfig; /* +8   = loaded weights */
    int              *itemweights;  /* +12  = WeaponWeightIndex mapping table */
    char             *modelname;    /* +16  = AAS_ModelFromIndex(bs->snapshot.gunindex) */
    int               weaponindex;  /* +20  = chosen weapon index */
    float             nextthink;    /* +24  = AAS_Time gate */
} bot_weaponstate_t;

/* Side-band for the per-client console-message FIFO held inline in chatstate
 * slots [43..45] (firstmessage, lastmessage, count). */
typedef struct chatmsg_links_s {
    bot_consolemessage_t *first;   /* chatstate[43] @ +172 */
    bot_consolemessage_t *last;    /* chatstate[44] @ +176 */
    int                   count;   /* chatstate[45] @ +180 */
} chatmsg_links_t;

/* Initial-chat dump structures.  BotLoadInitialChat packs them into one
 * contiguous heap buffer:
 *   chatlist_t   @ +0        -> types
 *   chattype_t   @ +0..+43   (name[32], numlines, firstline, next)
 *   chatline_t   @ +0..+11   (string, ltime, next)
 * followed by the inline chat strings.  The 64-bit side-band path instead
 * allocates one node per chat-type / chat-line, string inline behind it. */
typedef struct chatline_s {
    char              *string;       /* +0  pointer to inline string buffer */
    float              ltime;        /* +4  last-time gate (AAS_Time) */
    struct chatline_s *next;         /* +8  next chat-line in this type */
#if BOTLIB_NEED_SIDEBAND
    char               buf[1];       /* +12 inline string follows (allocated) */
#endif
} chatline_t;

typedef struct chattype_s {
    char               name[32];     /* +0  type tag */
    int                numlines;     /* +32 number of chat-lines */
    chatline_t        *firstline;    /* +36 head of chat-line list */
    struct chattype_s *next;         /* +40 next chat-type */
} chattype_t;

typedef struct chatlist_s {
    chattype_t        *types;        /* head of chat-type list */
} chatlist_t;

/* BotInitialChat records up to 10 (string,len) pairs for BotConstructChatMessage
 * to substitute as %v0..%v9.  The original entry is 8 bytes; widened on 64-bit
 * so the pointer survives. */
typedef struct bot_chatvar_s { char *str; int len; } bot_chatvar_t;

/* The subsystem's interfaces, rolled up as be_aas_def.h does for the AAS
 * side.  be_ai_weight.h first: the goal and weapon interfaces declare against
 * its fuzzy-weight types. */
#include "be_ai_weight.h"
#include "be_ai_char.h"
#include "be_ai_chat.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"
#include "be_ai_weap.h"
#include "be_ai2_dmnet.h"
#include "be_ai2_dmq2.h"
#include "be_ai2_main.h"
#include "be_ea.h"

#endif /* BOTLIB_BE_AI_DEF_H */
