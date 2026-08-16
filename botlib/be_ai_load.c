/*
 * be_ai_load.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999).
 *
 * A DATA-ONLY translation unit: both 1999 makefiles list `be_ai_load.o` in
 * their object lists, and neither shipped image contains a single byte of
 * `.text` attributable to it.  What it does contain is `botai`.
 *
 * `botai` is recovered from gladi386.so's .dynsym: a 556-byte `.bss` OBJECT at
 * 0x62724, sitting between `levelitemheap` (the last of be_ai_goal.o's data)
 * and `weaponconfig` (the first of be_ai_weap.o's).  The link order is
 * be_ai_goal, be_ai_load, be_ai_move, be_ai_weap, and be_ai_move contributes
 * no exported data at all -- so the gap belongs to this object.
 *
 * WHAT IS AND IS NOT EVIDENCE.  Name, size, section and position are read
 * straight out of the image.  The TYPE is not: `botai` has NO reference
 * anywhere in either 1999 image, so nothing constrains its layout, and no
 * struct in this tree is 556 bytes.  It is therefore declared as a size-exact
 * byte array rather than guessed at.  If a use is ever found, retype it then.
 */
#include "botlib_port.h"

char botai[556];   /* unreferenced in BOTH images -- see the note above */
