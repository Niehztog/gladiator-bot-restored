/*
 * be_aas_bsphl.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999).
 *
 * An EMPTY translation unit: the Half-Life BSP loader.  Both 1999 makefiles list
 * `be_aas_bsphl.o` (lcc.mak and linux-i386.mak, first in the AAS block), and
 * neither shipped image holds a byte of code or data from it -- no run in the
 * `.so`'s .text between the crt objects and be_aas_bspq2, no block in the DLL.
 * Whatever the file contained was compiled out.
 *
 * It is here because the object still is: gladi386.so's `.comment` and `.note`
 * carry one `GCC: (GNU) 2.7.2.3` / `01.01` entry per linked object, 41 in all, and
 * without this file and be_ai2_dmhl.c the gcc 2.7.2.3 oracle linked 39.  Nothing
 * of the original text is recoverable, so nothing is invented here.
 */
