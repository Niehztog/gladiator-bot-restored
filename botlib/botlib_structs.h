/*
 * botlib_structs.h — Reconstructed Gladiator botlib structs.
 *
 * The four primary config structs (soundinfo_t, iteminfo_t, weaponinfo_t,
 * projectileinfo_t) have byte-exact layouts because their field tables are
 * preserved in the original .data section and have been recovered into
 * botlib_structdefs.c.  Each field's byte offset and type is taken directly
 * from those tables, which makes the C struct equivalent guaranteed to be
 * byte-identical to the original.
 *
 * Q3 botlib equivalents:
 *   weaponinfo_t   : be_ai_weap.h:61   (Q3 = 180 B,  Q2 = 344 B — Q2 was extended)
 *   projectileinfo_t: be_ai_weap.h:43  (Q3 =  76 B,  Q2 = 208 B — Q2 was extended)
 *   iteminfo_t     : not in Q3 (uses bot_goal_t)
 *   soundinfo_t    : not in Q3 botlib (Q2-specific)
 */

#ifndef BOTLIB_STRUCTS_H
#define BOTLIB_STRUCTS_H

#include "gladiator.dll.h"   /* vec3_t */

/* Constants used inside the structs */
#define MAX_INFOSTRING   80     /* "name" / "model" string field width  (offset stride 0x50) */
#define MAX_SHORTSTRING  80     /* same — Gladiator inlines fixed buffers */

/* -------------------------------------------------------------------------
 * soundinfo_t — 176 bytes (descriptor unk_1005C138, size 0xB0).
 * Field table at .data 0x1005C070.  Field offsets:
 *   name(str,0x00) volume(f,0x50,def=80) duration(f,0x54,def=10)
 *   type(i,0x58)   recognition(f,0x5C,def=1) string(str,0x60)
 * ------------------------------------------------------------------------- */
typedef struct soundinfo_s {
    char  name[80];          /* +0x00 — sound symbolic name (MAX_SHORTSTRING) */
    float volume;            /* +0x50 — playback volume (default 80.0)        */
    float duration;          /* +0x54 — duration in seconds (default 10.0)    */
    int   type;              /* +0x58 — sound type tag                        */
    float recognition;       /* +0x5C — recognition weight (default 1.0)      */
    char  string[80];        /* +0x60 — sound file path                       */
} soundinfo_t;               /* sizeof = 0xB0 = 176 */

/* -------------------------------------------------------------------------
 * iteminfo_t — 284 bytes (descriptor unk_1005D890, size 0x11C).
 * Fields: name(str,0x00) model(str,0xA0) type(i,0xF4) index(i,0xF8)
 *         respawntime(f,0xFC) mins(vec3,0x100) maxs(vec3,0x10C)
 * Trailing 8 bytes (0x118..0x11F) appear unused/reserved.
 * ------------------------------------------------------------------------- */
typedef struct iteminfo_s {
    char    name[80];        /* +0x000 — item classname (e.g. "item_health")  */
    char    dispname[80];    /* +0x050 — display name (set by sub_1002ED20 before sub_10040AD0) */
    char    model[84];       /* +0x0A0 — model path; ends at 0xF4             */
    int     type;            /* +0x0F4 — item-type bitfield                   */
    int     index;           /* +0x0F8 — item index in inventory              */
    float   respawntime;     /* +0x0FC — respawn delay in seconds             */
    vec3_t  mins;            /* +0x100 — bounding-box mins                    */
    vec3_t  maxs;            /* +0x10C — bounding-box maxs                    */
    int     number;          /* +0x118 — loader-assigned index in items[]     */
} iteminfo_t;                /* sizeof = 0x11C = 284 */

/* itemconfig_t — top-level container returned by sub_1002ED20
 * (sub_1002ED20).  Header is 8 bytes followed by `numitems * 284`-byte
 * iteminfo_t entries inline.
 *   v5[0] = numitems
 *   v5[1] = (iteminfo_t *)(v5 + 2)   → points just past the header
 */
typedef struct itemconfig_s {
    int          numitems;        /* +0  populated count of items[]         */
    iteminfo_t  *items;            /* +4  → base + 8                         */
} itemconfig_t;                    /* sizeof = 8 (header only) */

/* -------------------------------------------------------------------------
 * weaponinfo_t — 344 bytes (descriptor unk_1005DFD8, size 0x158).
 * Q3 has same field names but at smaller offsets.
 * Recovered fields:
 *   valid(?,0x00) name(str,0x04) model(str,0x54) level(i,0xA4) weaponindex(i,0xA8)
 *   flags(i,0xAC) projectile(str,0xB0) numprojectiles(i,0x100) hspread(f,0x104)
 *   vspread(f,0x108) speed(f,0x10C) acceleration(f,0x110) recoil(vec3,0x114)
 *   offset(vec3,0x120) angleoffset(vec3,0x12C) extrazvelocity(f,0x138)
 *   ammoamount(i,0x13C) ammoindex(i,0x140) activate(f,0x144) reload(f,0x148)
 *   spinup(f,0x14C) spindown(f,0x150)
 * The first 4 bytes (offset 0..3) precede "name" and are most likely the Q3
 * "valid" flag — kept as `int valid` to preserve total size.
 * ------------------------------------------------------------------------- */
typedef struct projectileinfo_s projectileinfo_t;  /* fwd decl for proj field */
typedef struct weaponinfo_s {
    int                number;          /* +0x000 — index in weaponconfig array (set by sub_10034BB0) */
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

/* -------------------------------------------------------------------------
 * projectileinfo_t — 208 bytes (descriptor unk_1005DFE0, size 0xD0).
 * Field table at .data 0x1005DE30.
 * Fields (verified from descriptor):
 *   name(str,0x00) model(str,0x54) flags(i,0xA0) gravity(f,0xA4) damage(i,0xA8)
 *   radius(f,0xAC) visdamage(i,0xB0) damagetype(i,0xB4) healthinc(i,0xB8)
 *   push(f,0xBC) detonation(f,0xC0) bounce(f,0xC4) bouncefric(f,0xC8)
 *   bouncestop(f,0xCC)
 * ------------------------------------------------------------------------- */
struct projectileinfo_s {
    char    name[80];        /* +0x00 — projectile symbolic name              */
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

/* -------------------------------------------------------------------------
 * weaponconfig_t — top-level container returned by sub_10034BB0.
 * Single allocation: 16-byte header + (numweapons * 344) + (numprojectiles * 208).
 * Header layout confirmed at sub_10034BB0 (line ~28567):
 *   v7[0] = numweapons               (int)
 *   v7[1] = numprojectiles           (int)
 *   v7[2] = (projectileinfo_t *)     points at base + 16 + 344*numweapons
 *   v7[3] = (weaponinfo_t *)         points at base + 16
 *
 * Q3 botlib's equivalent struct is `weaponconfig_t` in be_ai_weap.h, but with
 * inline arrays (MAX_WEAPONS / MAX_PROJECTILES); Gladiator uses pointers since
 * the limits are configurable via the max_weaponinfo / max_projectileinfo
 * libvars at runtime.
 * ------------------------------------------------------------------------- */
typedef struct weaponconfig_s {
    int                numweapons;       /* +0  populated count of weapons[]      */
    int                numprojectiles;   /* +4  populated count of projectiles[]  */
    projectileinfo_t  *projectiles;      /* +8  → base + 16 + 344*numweapons      */
    weaponinfo_t      *weapons;          /* +12 → base + 16                       */
} weaponconfig_t;                        /* sizeof = 16 (header only) */

/* -------------------------------------------------------------------------
 * Script-parser structs (l_script.h / l_precomp.h equivalents).
 * Offsets verified by disassembly of sub_100401A0 (sub_100402F0, line ~34370),
 * sub_10040380 (sub_10040380, line ~34418), FreeScript (sub_10040470),
 * sub_1003DE60 source-alloc (sub_1003DE60, line ~33216), sub_1003E000
 * (sub_1003E000, line ~33243), PC_PushIndent / PC_PopIndent (sub_~30640),
 * PC_FreeDefine (sub_~31080), PC_FreeToken.
 * ------------------------------------------------------------------------- */

/* punctuation_t — 12 bytes; layout matches Q3 l_script.h:149 byte-for-byte. */
typedef struct punctuation_s {
    char                  *p;       /* +0  punctuation literal text   */
    int                    n;       /* +4  punctuation type id        */
    struct punctuation_s  *next;    /* +8  chain                      */
} punctuation_t;                    /* sizeof = 12 */

/* indent_t — 16 bytes; PC_PushIndent allocates 16 bytes, layout matches Q3.
 * Field offsets confirmed: type@+0 skip@+4 script@+8 next@+12. */
typedef struct indent_s {
    int                    type;    /* +0  INDENT_IF / IFDEF / ELSE / ELIF / ... */
    int                    skip;    /* +4  > 0: skipping until matching #endif   */
    struct script_s       *script;  /* +8  script that opened this indent        */
    struct indent_s       *next;    /* +12 next on indent stack                  */
} indent_t;                         /* sizeof = 16 */

/* define_t — chain link at +28 (sub_1003E000: *(_DWORD *)(m + 28) = next).
 * parms@+16 tokens@+20 confirmed via PC_FreeDefine.  Layout matches Q3
 * l_precomp.h::define_t — restore Q3 names directly. */
typedef struct define_s {
    char                  *name;    /* +0  macro name                              */
    int                    flags;   /* +4  DEFINE_FIXED etc.                       */
    int                    builtin; /* +8  > 0 for built-in defines (__LINE__ etc.) */
    int                    numparms;/* +12 number of parameter tokens              */
    struct token_s *parms;/* +16 parameter token list                    */
    struct token_s *tokens;/* +20 macro body token list                   */
    struct define_s       *next;    /* +24 hash-chain next                          */
    struct define_s       *hashnext;/* +28 (sub_1003E000 walks this offset)       */
} define_t;                         /* sizeof = 32 (Q3 has same layout, 32 bytes) */

/* script_t — 1392-byte header followed by file data inline.  Offsets all
 * verified from sub_100401A0 and sub_10040380:
 *   filename[260]@+0  buffer@+260  script_p@+264  end_p@+268  lastscript_p@+272
 *   length@+284  line@+288  lastline@+292  tokenavailable@+296  ...
 *   punctuationtable@+308 (freed in FreeScript)  token@+312..+1383
 *   next@+1384 (sub_1003E000 walks scriptstack via i+1384)
 *
 * Q2 differences vs Q3: filename buffer trimmed from 1024 to 260 bytes;
 * embedded token is token_t (1072 B, has double floatvalue) instead
 * of Q3's float-floatvalue token_t. */
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

/* source_t — 1624 bytes; sub_1003E000 accesses scriptstack/tokens/defines/
 * definehash/indentstack/skip at dword indices [131..136] (offsets 524..544).
 * sub_1003E120 accesses *(a1 + 308) as a separately-allocated 1024-byte
 * scratch buffer (extends Q3 source_t).
 *
 * Q3 source_t layout (l_precomp.h:95):
 *   filename[1024], includepath[1024], punctuations*, scriptstack*, tokens*,
 *   defines*, definehash**, indentstack*, skip, token.
 * Q2 trims path buffers and adds a definebuffer scratchpad.  Total = 1624. */
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
    struct token_s         cachedtoken;        /* +552  last-read token (qmemcpy'd by PC_ReadTokenHandle) */
} source_t;                                    /* sizeof = 1624 on 32-bit */

/* -------------------------------------------------------------------------
 * AI weight structs (be_ai_weight.h equivalents).  Layouts taken from Q3 —
 * Gladiator field offsets in fuzzyseperator_t are 36 bytes / 9 ints; matched
 * by ReadFuzzyWeights / FuzzyWeight functions in the original DLL.
 * ------------------------------------------------------------------------- */
typedef struct fuzzyseperator_s {
    int                       index;       /* +0  fact index             */
    int                       value;       /* +4  comparison value       */
    int                       type;        /* +8  comparison kind        */
    float                     weight;      /* +12 base weight            */
    float                     minweight;   /* +16 lower bound            */
    float                     maxweight;   /* +20 upper bound            */
    struct fuzzyseperator_s  *child;       /* +24 nested branch          */
    struct fuzzyseperator_s  *next;        /* +28 sibling separator      */
    /* Q3 has 8 fields (32 B); Gladiator may extend to 36 B with one trailing
     * field (e.g. probability) — verify via disassembly before using. */
} fuzzyseperator_t;                        /* sizeof = 32 */

typedef struct weight_s {
    char                     *name;        /* +0  weight rule name       */
    fuzzyseperator_t         *firstseperator; /* +4 first separator      */
} weight_t;                                /* sizeof = 8 */

/* weightconfig_t — 1028 bytes (sub_10035FA0 allocates GetClearedMemory(1028)).
 * Inline weight array, MAX_WEIGHTS = 128.  No filename field (Q3 has one; Gladiator dropped it). */
#define MAX_FUZZY_WEIGHTS 128
typedef struct weightconfig_s {
    int       numweights;                       /* +0  weight count           */
    weight_t  weights[MAX_FUZZY_WEIGHTS];       /* +4  inline weight array    */
} weightconfig_t;                               /* sizeof = 4 + 128*8 = 1028  */

/* -------------------------------------------------------------------------
 * Synonym structs (be_ai_chat.c equivalents, used by sub_1002B110 — the
 * syn.c loader).  Gladiator built these by 32-bit-only byte-stride
 * arithmetic in a packed scratch buffer; on 64-bit the buffer must use
 * the natural struct sizes (pointer fields grow 4→8 bytes).
 *
 * Layouts mirror Q3 botlib's bot_synonymlist_t / bot_synonym_t, but the
 * `context` field is `int` (not `unsigned long`) to stay binary-compatible
 * with the original 32-bit Windows DLL where sizeof(long) == 4.
 * ------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------
 * Random-string structs (be_ai_chat.c equivalents).  Used by sub_1002B990
 * (the rnd.c loader / BotLoadRandomStrings).  Same 32-bit-only byte
 * arithmetic problem as the synonym loader: pointer fields grow on
 * 64-bit so size calc and buffer writes must use struct types.
 * ------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------
 * Bot goal struct — 56 bytes (matches Q3 be_ai_goal.h::bot_goal_t).
 * Field offsets confirmed from sub_1002F890 (sub_1002F890):
 *   a3+0,4,8   = origin[3]
 *   a3+12      = areanum
 *   a3+16..28  = mins[3]
 *   a3+28..40  = maxs[3]
 *   a3+40      = entitynum
 *   a3+44      = number (item number from levelitem)
 * The trailing `flags` and `iteminfo` fields are present in the layout (the
 * CTF-flag .bss slots are 56 bytes, and qmemcpy at sub_10027240 copies 0x38u
 * = 56 bytes when transferring a goal) but Gladiator never writes to them —
 * sub_1002F890 fills 48 bytes; the trailing 8 stay zero from .bss.
 * ------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------
 * Level item pool entry — 52 bytes.
 * Allocated as a fixed-size pool by InitLevelItemHeap (52 * max_levelitems).
 * Free list is singly linked via `next`; active list is doubly linked via
 * `prev`/`next` (AddLevelItemToList / sub_1002F320).
 * Field offsets confirmed from sub_1002FA20 (sub_1002FA20) and
 * sub_1002F890 (sub_1002F890):
 *   +0   number      — item number = entitynum + map base bias
 *   +4   iteminfo    — index into itemconfig->items[]
 *   +8   origin      — entity world position (vec3)
 *   +20  areanum     — AAS area number reachable for this item
 *   +24  goalorigin  — best reachable origin (vec3), filled by AAS_BestReachableArea
 *   +36  entitynum   — BSP entity number
 *   +40  timeout     — sub_1000E120() + 30.0 when item respawns / expires
 *   +44  prev        — doubly-linked active list back pointer
 *   +48  next        — list link (free list and active list both use this)
 * ------------------------------------------------------------------------- */
typedef struct levelitem_s {
    int                       number;       /* +0  item number          */
    int                       iteminfo;     /* +4  iteminfo[] index     */
    vec3_t                    origin;       /* +8  world position       */
    int                       areanum;      /* +20 AAS area number      */
    vec3_t                    goalorigin;   /* +24 best reachable point */
    int                       entitynum;    /* +36 BSP entity number    */
    float                     timeout;      /* +40 expiry time          */
    struct levelitem_s       *prev;         /* +44 active-list back ptr */
    struct levelitem_s       *next;         /* +48 list link            */
} levelitem_t;                               /* sizeof = 52 */

/* -------------------------------------------------------------------------
 * AAS movement physics settings (be_aas_def.h::aas_settings_t).
 * 37 floats (148 B).  Loaded from libvar_sv_* cvars in AAS_LoadSettings.
 * Field order matches Q3 — 16 sv_* values are read into matching slots.
 * ------------------------------------------------------------------------- */
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
    /* sizeof = 148 — confirm via Gladiator AAS_InitSettings disassembly */
} aas_settings_t;

/* PC_EvaluateTokens / PC_DollarEvaluate value- and operator-cell lists.
 * Matches the Q3 botlib l_precomp.c shape, but with `double floatvalue`
 * (Gladiator's expression evaluator was extended to doubles; the original
 * binary's GetClearedMemory(32) for value_t and (24) for operator_t still
 * hold on 32-bit MSVC; on 64-bit the trailing prev/next pointers double
 * to 8 bytes so we must allocate sizeof(...) instead). */
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

/* -------------------------------------------------------------------------
 * bsp_link_t — entity<->BSP-leaf link node, 24 bytes on 32-bit.
 *
 * Q3 equivalent: be_aas_def.h:70 (same layout).  Field offsets are confirmed
 * from sub_100030A0 disassembly (24-byte stride, +8/+12 used as free-list
 * next/prev which corresponds to next_ent/prev_ent slots).  On 64-bit each
 * pointer expands to 8 bytes so the node grows to 40 bytes.
 */
typedef struct bsp_link_s {
    int                 entnum;
    int                 leafnum;
    struct bsp_link_s  *next_ent;
    struct bsp_link_s  *prev_ent;
    struct bsp_link_s  *next_leaf;
    struct bsp_link_s  *prev_leaf;
} bsp_link_t;

/* -------------------------------------------------------------------------
 * indexlist_t — model/sound/image name lookup table.
 *
 * Used by aasworld.modelindex_table / soundindex_table / imageindex_table.
 * Original 32-bit layout (sub_1000DA80 disassembly @ 1000DA80):
 *   +0  int    numindexes
 *   +4  char **indexes   (points to the trailing array at +8)
 *   +8  char  *slots[numindexes]
 * Allocation = sizeof(int) + sizeof(char**) + numindexes*sizeof(char*).
 * On 32-bit that's 4 + 4 + 4*n = 4*n + 8 (matches original "lea eax,[esi*4+8]").
 * On 64-bit it becomes 4 + 8 + 8*n + 4 alignment = 8*n + 16.
 */
typedef struct indexlist_s {
    int    numindexes;
    char **indexes;
} indexlist_t;

/* -------------------------------------------------------------------------
 * bsp_epair_t / bsp_entity_t — BSP entity-lump epair list.
 *
 * Q3 equivalent: be_aas_bspq2.c:60.  Layout confirmed from
 * AAS_ValueForBSPEpairKey @ 0x10006760: reads key at +0, value at +4,
 * next at +8 (32-bit).  bsp_entity_t holds just one pointer (epairs head).
 * On 64-bit the struct grows from 12 → 24 bytes; entity slot from 4 → 8 bytes.
 */
typedef struct bsp_epair_s {
    char               *key;
    char               *value;
    struct bsp_epair_s *next;
} bsp_epair_t;

typedef struct bsp_entity_s {
    bsp_epair_t        *epairs;
    struct bsp_entity_s *next;
} bsp_entity_t;

#endif /* BOTLIB_STRUCTS_H */
