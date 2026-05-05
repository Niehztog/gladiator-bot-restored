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

#endif /* CHAT_STATE_H */
