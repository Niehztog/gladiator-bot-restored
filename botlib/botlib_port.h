/*
 * botlib_port.h — the parts of the build environment that have NO counterpart
 * in Mr. Elusive's 1999 sources.
 *
 * Everything here exists only because we compile 32-bit MSVC6 source for a
 * 64-bit, POSIX toolchain.  It is kept apart from the reconstruction so the
 * line between "restored" and "scaffolding" is visible at a glance:
 *
 *   - the system #include set (the original .c files each included what they
 *     needed; we cannot know which, so one common set stands in)
 *   - shims for MSVC CRT names that POSIX spells differently
 *   - SHIDWORD, an IDA spelling for the high dword of a 64-bit value
 *   - the 64-bit side-band GATE.  Its per-struct accessor macros unavoidably
 *     live next to the structures they wrap, in botlib_local.h.
 *   - P()/FE()/FE_END, our spelling of the structdef field tables recovered
 *     from .data
 *
 * Nothing in this file corresponds to anything in the shipped DLL.
 */
#ifndef BOTLIB_PORT_H
#define BOTLIB_PORT_H

#include <math.h>
#include <stdarg.h>
#include <stddef.h>    /* offsetof, size_t */
#include <stdint.h>    /* intptr_t */
#include <stdio.h>     /* file I/O, sprintf, sscanf */
#include <string.h>    /* string and memory ops */
#include <stdlib.h>    /* malloc, atoi, rand, abs */
#include <ctype.h>     /* toupper */
#include <errno.h>     /* errno */
#include <time.h>      /* time(), ctime() */
#include <unistd.h>    /* access(), chdir(), getcwd() */


/* signed high-word/dword accessors */
#define SHIDWORD(x)  (*((int *)&(x) + 1))

/* Stubs for the statically-linked MSVC CRT helpers the bot code still calls. */
static size_t fread_locked(void *buf, size_t sz, size_t n, FILE *f) { return fread(buf, sz, n, f); }
#ifdef _WIN32
/* Windows-only winbspc/zip subsystem (sub_1000E140/sub_1000E430, sub_10041FF0);
 * the Linux botlib has neither a process-spawn nor a chdir-for-bspc path. */
static int    remove_file(const char *path) { return remove(path); }
static int    getcwd_locked(char *buf, int size) { return getcwd(buf, size) ? 0 : -1; }
static intptr_t SpawnProcess(int mode, char *file, char *args, char *cmd, char *addargs) { (void)mode; (void)file; (void)args; (void)cmd; (void)addargs; return -1; }
/* _access is provided by MinGW's <io.h>; _chdir by <direct.h> — use POSIX wrappers */
static int    _chdir(const char *path) { return chdir(path); }
#endif /* _WIN32 */

#ifndef _WIN32
/* POSIX equivalents for the Windows CRT functions used below. */
#include <strings.h>    /* strcasecmp */
#define _strcmpi   strcasecmp
#define _access    access
typedef int __time32_t;   /* Windows 32-bit time type; int is 32-bit on all targets */
#endif

/* AI node function pointer type (used for BotAINode side-band table) */
typedef int (*ai_node_fn_t)(struct bot_state_s *bs);

/* ------------------------------------------------------------------------
 * Side-band gate.
 *
 * Every "pointer slot" in bot_state_t / bot_chatstate_t / aas_entity_t is a
 * 4-byte int field holding a pointer bit-pattern.  On 64-bit those slots are
 * too narrow, so each is mirrored into a parallel heap array (the side-band)
 * reached through helper macros (BotCharacter, BotAINode, BotWS, …).
 *
 * On 32-bit the macros just reinterpret the inline int slot and the side-band
 * tables compile out entirely, reproducing the original memory image exactly.
 * Every gate in botlib.c follows this one predicate. */
#if defined(__x86_64__) || defined(__aarch64__)
#define BOTLIB_NEED_SIDEBAND 1
#else
#define BOTLIB_NEED_SIDEBAND 0
#endif


/* Field-table spelling for the structdef_t descriptors recovered from .data.
 * Shared by be_aas_sound / be_ai_goal / be_ai_weap, which own one table each.
 * A structdef is { int size; char **fields; }; each field entry is 7 (char *)
 * slots: name, byte offset, type flags, array count, 0, default float-bits, 0. */
/* Cast integer constant to char * for mixed pointer/int field table slots */
#define P(x) ((char *)(uintptr_t)(x))

/* One field table entry (7 slots): name, offset, flags, arrcount, 0, default, 0 */
#define FE(name, off, flags, arr, def) \
    (name), P(off), P(flags), P(arr), P(0), P(def), P(0)

/* Null terminator entry */
#define FE_END \
    P(0),  P(0),   P(0),   P(0),  P(0),  P(0),  P(0)


#endif /* BOTLIB_PORT_H */
