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
 * straight out of the image.  The TYPE is only half constrained: `botai` has NO
 * reference anywhere in either 1999 image and no struct in this tree is 556 bytes,
 * but its ALIGNMENT is fixed by the link.  binutils 2.9.1's ld treats a common's
 * declared alignment of 1 as "unknown" and falls back to min(log2(size), 4), so
 * every `char[]` common of 16 bytes or more lands 16-aligned (nodeswitch and
 * com_token do, in both images) -- while the real `botai` sits at a 4-aligned,
 * not 16-aligned, offset.  So it was not a char array: its element type had
 * alignment 2 or 4.  `int[139]` is the neutral 556-byte choice.  As `char[556]`
 * the oracle padded 12 bytes in front of it, shifting every .bss object up to
 * l_precomp's; with it, gladi386_oracle.so's .bss layout is identical to the real
 * one, object for object.  If a use is ever found, retype it then.
 */
#include "botlib_port.h"

int botai[139];   /* 556 bytes; unreferenced in BOTH images -- see the note above */
