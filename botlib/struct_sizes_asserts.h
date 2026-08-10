/*
 * struct_sizes_asserts.h — compile-time guard that every reconstructed struct
 * still matches the original 1999 binary's layout.  Adding, removing,
 * retyping or reordering a field fails the build.
 *
 * Included exactly once, from botlib.c.  Only add asserts for structs whose
 * binary layout is confirmed, never for speculative ones.
 */
#ifndef BOTLIB_STRUCT_SIZES_ASSERTS_H
#define BOTLIB_STRUCT_SIZES_ASSERTS_H

#include <stddef.h>

#include "be_ea.h"

/* Pointer-bearing structs grow on 64-bit hosts, so those asserts are gated to
 * 32-bit targets; the MinGW 32-bit path is what enforces DLL compatibility. */
#include <stdint.h>

/* MSVC 6.0 oracle build: _Static_assert is C11.  Skip the guards there — the
 * native MinGW/gcc build path validates them. */
#if defined(_MSC_VER) && _MSC_VER < 1900
#define _Static_assert(cond, msg) /* skipped under MSVC < 14.0 */
#endif

_Static_assert(sizeof(bot_updateclient_t) == 0x4CC, "bot_updateclient_t must remain 0x4CC bytes");
/* The next three are pointer-free, so they hold on 64-bit too. */
_Static_assert(sizeof(bot_moveresult_t)    == 48,   "bot_moveresult_t size (0x30, Q3 minus weapon field)");
_Static_assert(sizeof(bot_movestate_t)     == 128,  "bot_movestate_t size (bs+2880..+3007 inline block)");
_Static_assert(sizeof(bot_chatstate_t)     == 188,  "bot_chatstate_t size (47 ints, bs+3980 inline block)");

#if INTPTR_MAX == INT32_MAX
_Static_assert(sizeof(dBspHeader_t)        == 160,  "dBspHeader_t size (0xA0)");
_Static_assert(sizeof(aas_header_t)        == 120,  "aas_header_t size (0x78)");
_Static_assert(sizeof(bot_fileref_t)       == 152,  "bot_fileref_t size (38 ints)");
/* token_t has TWO right sizes on 32-bit, and both are measured.  `long double
 * floatvalue` is 8 B/8-aligned under MSVC (pad after intvalue, pad at the tail
 * -> 0x430, as gladiator.dll uses) and 12 B/4-aligned under gcc i386 (neither
 * pad -> 0x42C, as gladi386.so uses).  The field offsets are the invariant. */
_Static_assert(offsetof(token_t, whitespace_p) == 0x418, "token_t.whitespace_p offset");
_Static_assert(offsetof(token_t, next)         == 0x428, "token_t.next offset");
_Static_assert(sizeof(token_t) == (sizeof(long double) == 8 ? 0x430 : 0x42C),
               "token_t size follows the compiler's long double");
_Static_assert(sizeof(aas_world_t)         == 676,  "aas_world_t size (0x2A4)");

/* -- Soundinfo / iteminfo / weaponinfo ------------------------------- */
_Static_assert(sizeof(soundinfo_t)         == 176,  "soundinfo_t size (0xB0)");
_Static_assert(sizeof(iteminfo_t)          == 284,  "iteminfo_t size (0x11C)");
_Static_assert(sizeof(itemconfig_t)        == 8,    "itemconfig_t size (header only)");
_Static_assert(sizeof(weaponinfo_t)        == 344,  "weaponinfo_t size (0x158)");
_Static_assert(sizeof(weaponconfig_t)      == 16,   "weaponconfig_t size (header only)");

/* -- Parser primitives ---------------------------------------------- */
_Static_assert(sizeof(punctuation_t)       == 12,   "punctuation_t size");
_Static_assert(sizeof(indent_t)            == 16,   "indent_t size");
_Static_assert(sizeof(define_t)            == 32,   "define_t size (Q3 has same layout)");
/* script_t/source_t are sized by MAX_PATH and by token_t, both of which differ
 * between the two 1999 builds (1392/1624 in gladiator.dll, 1268/1384 in
 * gladi386.so).  Assert the structure rather than a number, so the guard still
 * bites on a reordered or retyped field under either compiler.  MSVC's own tail
 * padding is what makes the DLL 4 bytes larger than these expressions. */
_Static_assert(offsetof(script_t, token) == MAX_PATH + 52, "script_t.token offset");
_Static_assert(offsetof(source_t, cachedtoken)
               == 2 * MAX_PATH + 28 + (sizeof(long double) == 8 ? 4 : 0),
               "source_t.cachedtoken offset");
#if !defined(_MSC_VER)
_Static_assert(sizeof(script_t) == MAX_PATH + 52 + sizeof(token_t) + 4, "script_t size");
_Static_assert(sizeof(source_t) == 2 * MAX_PATH + 28 + sizeof(token_t), "source_t size");
#endif

/* -- Fuzzy logic / weights ------------------------------------------ */
_Static_assert(sizeof(fuzzyseperator_t)    == 32,   "fuzzyseperator_t size");
_Static_assert(sizeof(weight_t)            == 8,    "weight_t size");
_Static_assert(sizeof(weightconfig_t)      == 1028, "weightconfig_t size (4 + 128*8)");

/* -- Goal / level item ---------------------------------------------- */
_Static_assert(sizeof(bot_goal_t)          == 56,   "bot_goal_t size");
_Static_assert(sizeof(levelitem_t)         == 52,   "levelitem_t size");

/* -- Chat / match --------------------------------------------------- */
_Static_assert(sizeof(bot_matchstring_t)   == 8,    "bot_matchstring_t size");
_Static_assert(sizeof(bot_matchpiece_t)    == 16,   "bot_matchpiece_t size");
_Static_assert(sizeof(bot_matchtemplate_t) == 20,   "bot_matchtemplate_t size");
_Static_assert(sizeof(bot_matchvariable_t) == 8,    "bot_matchvariable_t size");
_Static_assert(sizeof(bot_match_t)         == 240,  "bot_match_t size");
_Static_assert(sizeof(bot_chatmessage_t)   == 12,   "bot_chatmessage_t size (12 on 32-bit)");
_Static_assert(sizeof(bot_replychatkey_t)  == 16,   "bot_replychatkey_t size (16 on 32-bit)");
_Static_assert(sizeof(bot_replychat_t)     == 20,   "bot_replychat_t size (20 on 32-bit)");
_Static_assert(sizeof(bot_stringlist_t)    == 8,    "bot_stringlist_t size (8 on 32-bit)");
_Static_assert(sizeof(bot_consolemessage_t) == 168, "bot_consolemessage_t size (168 on 32-bit)");

/* -- Synonyms / random strings --------------------------------------- */
_Static_assert(sizeof(bot_synonym_t)       == 12,   "bot_synonym_t size (12 on 32-bit)");
_Static_assert(sizeof(bot_synonymlist_t)   == 16,   "bot_synonymlist_t size (16 on 32-bit)");
_Static_assert(sizeof(bot_randomstring_t)  == 8,    "bot_randomstring_t size (8 on 32-bit)");
_Static_assert(sizeof(bot_randomlist_t)    == 16,   "bot_randomlist_t size (16 on 32-bit)");

/* -- Script CRC cache ------------------------------------------------ */
_Static_assert(sizeof(scriptcrc_t)         == 152,  "scriptcrc_t size (152 on 32-bit)");

/* -- Bot state ------------------------------------------------------- */
_Static_assert(sizeof(ea_state_t)          == 36,   "ea_state_t size");
_Static_assert(sizeof(bot_state_t)         == 4560, "bot_state_t size (BOT_STATE_SIZE)");
#endif  /* INTPTR_MAX == INT32_MAX */

#endif /* BOTLIB_STRUCT_SIZES_ASSERTS_H */
