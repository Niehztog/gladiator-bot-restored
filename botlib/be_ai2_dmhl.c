/*
 * be_ai2_dmhl.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999).
 *
 * An EMPTY translation unit: the Half-Life deathmatch AI.  linux-i386.mak links
 * be_ai2_dmq2 -> be_ai2_dmhl -> be_ai2_dmnet, and gladi386.so has no code or data
 * between the dmq2 and dmnet runs; lcc.mak has a single `be_ai2_dm` instead, so
 * the DLL may never have had this object at all.
 *
 * Kept for the same reason as be_aas_bsphl.c: the object's `.comment` / `.note`
 * entries are part of gladi386.so (41 of each), and nothing of its text survives.
 */
