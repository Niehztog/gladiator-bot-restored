/*
 * chat_state.h — Bot chat-system types.
 *
 * The chat AI parses incoming messages against patterns loaded from
 * gladiator/match.c.  Each pattern is a chain of bot_matchpiece_t nodes
 * (alternating MT_STRING and MT_VARIABLE); on a hit the AI gets a
 * bot_match_t with the type/subtype plus one capture per MT_VARIABLE.
 *
 * Same data flow as Q3 botlib (be_ai_chat.h), different layout:
 *   MAX_MESSAGE_SIZE 150 (Q3: 256), MAX_MATCHVARIABLES 10 (Q3: 8), and
 *   bot_matchvariable_s.ptr is a real char* (Q3: a signed-char offset).
 */

#ifndef CHAT_STATE_H
#define CHAT_STATE_H

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


#endif /* CHAT_STATE_H */
