/*
 * struct_sizes_asserts.h — compile-time guard that every reconstructed
 * struct still matches the layout of the original Mr. Elusive 1999
 * binary.
 *
 * These _Static_assert lines are the strongest form of regression check
 * we have: if anyone adds, removes, retypes, or reorders a field, the
 * build itself fails with the assertion name pointing at the affected
 * struct.  Each size here was verified against the original DLL
 * disassembly (stack-frame `sub esp, N` analysis or struct-pointer
 * arithmetic) — see the per-struct headers for the trail.
 *
 * Include this file exactly once, from a single .c translation unit, to
 * avoid duplicate-assert warnings.  We include it from botlib.c.
 *
 * Add new asserts here whenever a new struct gets a confirmed binary
 * layout.  Do NOT add asserts for structs whose layout is still
 * speculative — wait until the binary cross-reference is complete.
 */
#ifndef BOTLIB_STRUCT_SIZES_ASSERTS_H
#define BOTLIB_STRUCT_SIZES_ASSERTS_H

#include <stddef.h>

#include "gladiator.dll.h"
#include "botlib_structs.h"
#include "bot_state.h"
#include "chat_state.h"
#include "ea_state.h"

/*
 * These assertions encode the original 32-bit Windows DLL struct layout.
 * On 64-bit hosts, pointer-containing structs naturally grow, so we only
 * enable the asserts when compiling for a 32-bit target.  Binary
 * compatibility with the original DLL is enforced through the MinGW
 * 32-bit build path; the 64-bit Linux build path is for development
 * smoke-testing only.
 */
#include <stdint.h>
#if INTPTR_MAX == INT32_MAX
_Static_assert(sizeof(dBspHeader_t)        == 160,  "dBspHeader_t size (0xA0)");
_Static_assert(sizeof(aas_header_t)        == 120,  "aas_header_t size (0x78)");
_Static_assert(sizeof(bot_fileref_t)       == 152,  "bot_fileref_t size (38 ints)");
_Static_assert(sizeof(token_t)             == 1072, "token_t size (0x430)");
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
_Static_assert(sizeof(script_t)            == 1392, "script_t size (header only)");
_Static_assert(sizeof(source_t)            == 1624, "source_t size");

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

/* -- Bot state ------------------------------------------------------- */
_Static_assert(sizeof(ea_state_t)          == 36,   "ea_state_t size");
_Static_assert(sizeof(bot_state_t)         == 4560, "bot_state_t size (BOT_STATE_SIZE)");
#endif  /* INTPTR_MAX == INT32_MAX */

#endif /* BOTLIB_STRUCT_SIZES_ASSERTS_H */
