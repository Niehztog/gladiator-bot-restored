/*
 * chat_state.h — Bot chat-system types restored from disassembly
 *
 * The Gladiator Bot v0.96 chat AI parses incoming chat messages against a
 * list of patterns loaded from gladiator/match.c.  Each pattern is a chain
 * of bot_matchpiece_t nodes (alternating MT_STRING and MT_VARIABLE).  When
 * a pattern matches, the AI gets back a bot_match_t struct describing the
 * match: which type/subtype, plus capture info for each MT_VARIABLE piece.
 *
 * Q3 botlib has the same data flow with similar names (be_ai_chat.h), but
 * the Q2 layout differs:
 *   - MAX_MESSAGE_SIZE   = 0x96 (150) in Q2  vs 256 in Q3
 *   - MAX_MATCHVARIABLES = 10        in Q2  vs   8 in Q3
 *   - bot_matchvariable_s.ptr is a real `char*` in Q2 (Q3 stores a signed-char offset).
 *
 * Confirmed against gladiator.dll_ disassembly:
 *   - BotFindMatch  (sub_1002C930) does `strncpy(match->string, src, 0x96)`.
 *   - sub_1002C800 (sub_1002C800) reads/writes match->variables[i].ptr at
 *     offset +160 (= 152 + 4 + 4) within the match struct, and .length at +164.
 *   - BotMatchVariable (sub_1002CA20) checks `variable < 10`.
 *   - sub_10026F10's stack frame allocates the struct at ebp-5B4 (240
 *     bytes before ebp-374 = first non-match local), matching sizeof = 240.
 */

#ifndef CHAT_STATE_H
#define CHAT_STATE_H

#define MAX_MESSAGE_SIZE       0x96   /* 150 — chat message buffer per Q2 binary */
#define MAX_MATCHVARIABLES     10     /* 10 — capture slots per Q2 binary */

/* Match piece types (used in bot_matchpiece_t.type).
 * Confirmed by sub_1002C800's `cmp $2, %eax; jne …; cmp $1, %eax`. */
#define MT_STRING              2
#define MT_VARIABLE            1

/* ---- Pattern-template data structures (loaded from match.c) -------------
 *
 * bot_matchstring_t — alternative string in a MT_STRING piece.  Linked list
 * head lives at bot_matchpiece_t.firststring; chain via .next. */
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
 * dword_10064378).  Confirmed by BotFindMatch reading [v3+0]=context flags,
 * [v3+4]=type, [v3+8]=subtype, [v3+12]=first piece, [v3+16]=next template. */
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

/* bot_match_t — result struct passed back to AI when BotFindMatch hits.
 * Total size 240 bytes; layout matches the original Gladiator binary
 * (verified via sub_10026F10's stack allocation at ebp-5B4). */
typedef struct bot_match_s {
    char string[MAX_MESSAGE_SIZE + 2];           /* offset   0 — chat message (150 used + 2 padding to 4-byte align) */
    int  type;                                   /* offset 152 — match type */
    int  subtype;                                /* offset 156 — match subtype */
    bot_matchvariable_t variables[MAX_MATCHVARIABLES]; /* offset 160 — 10 captures × 8 bytes = 80 bytes */
} bot_match_t;                                   /* sizeof = 240 */

/* ---- Reply-chat structures (rchat.c) ----------------------------------- */

/* bot_chatstate_t — per-client chat state embedded inline in bot_state_t
 * at offset +3980.  Original 32-bit DLL size = 47 ints = 188 bytes.  Most
 * of the slot layout is opaque; the few fields confirmed from disasm /
 * existing code paths are aliased below.  All actual pointer-bearing
 * slots ([43..46]) cannot hold 64-bit pointers and are routed through
 * side-band arrays (`botchatdumps`, `botchatmsglinks`) — they remain
 * 4-byte ints here purely to preserve the 188-byte layout.
 *
 * Confirmed accessors:
 *   - chatstate[0]      @ +0    int  gender   — 1=female, 2=male, 0=neutral.
 *     Set by BotSetupClient from Characteristic_String(BotCharacter, 3).
 *     Read by BotReplyChat (sub_1002E7D0) via *(_DWORD *)cs comparisons.
 *   - chatstate[5..]    @ +20   char name[?]  — strlen()/strcpy() target
 *     in BotChatLength (sub_1002E760) and BotInitialChat.
 *   - chatstate[43]     @ +172  bot_consolemessage_t *first  (side-banded)
 *   - chatstate[44]     @ +176  bot_consolemessage_t *last   (side-banded)
 *   - chatstate[45]     @ +180  int                   count  (side-banded)
 *   - chatstate[46]     @ +184  chatlist_t *chatdump         (side-banded) */
typedef struct bot_chatstate_s {
    union {
        int  _slots[47];                         /* opaque 47-int payload — preserves 188-byte size */
        struct {
            int  gender;                         /* +0 — 0/1/2 */
            char name[16];                       /* +4..+19 — the bot's own chat name;
                                                  * set by BotSetChatName (0x1002EB30):
                                                  * memset 15 + strncpy limit 15, so name[15]
                                                  * is the terminator slot. */
            /* +20..+171 (152 B) is ONE buffer, not a 20-byte name plus padding:
             * every consumer treats +20 as the outgoing chat message —
             * BotChatLength does strlen(cs+20), BotEnterChat passes it to
             * EA_Say/EA_SayTeam and then stores the empty-string byte at +20,
             * and BotConstructChatMessage uses it as its output buffer.  Q3's
             * cognate field is bot_chatstate_t.chatmessage. */
            char chatmessage[152];               /* +20..+171 */
            int  _slot_43;                       /* +172 — side-banded first-msg ptr */
            int  _slot_44;                       /* +176 — side-banded last-msg ptr */
            int  _slot_45;                       /* +180 — side-banded count (mirror only) */
            int  _slot_46;                       /* +184 — side-banded chatdump ptr */
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
 * dword_10064380).  Q2 differs from Q3 (no `cmd` field, no name). */
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
 * (sub_1002A880 allocates a fixed array of these, sub_1002A9A0 pops, and
 * sub_1002A9E0 pushes back into the freelist).  The 32-bit binary used
 * a 168-byte node where prev/next live at offsets 160/164 — exact field
 * breakdown of the leading 160 bytes is not relevant here, we just keep
 * the opaque payload so prev/next stay correctly placed.  On 64-bit the
 * pointer fields grow from 4 to 8 bytes so we let the compiler compute
 * sizeof() and use struct field access instead of byte arithmetic. */
typedef struct bot_consolemessage_s {
    float                          time;         /* +0  AAS_Time stamp */
    int                            type;         /* +4  message type/level */
    char                           message[152]; /* +8  message text (152 bytes -> +160) */
    struct bot_consolemessage_s   *prev;         /* +160 */
    struct bot_consolemessage_s   *next;         /* +164 / 64-bit: +168 */
} bot_consolemessage_t;                          /* sizeof = 168 / 176 */


#endif /* CHAT_STATE_H */
