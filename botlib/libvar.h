/*
 * libvar.h — Bot library variable structure (l_libvar.h equivalent).
 *
 * Restored from disassembly of LibVarAlloc / LibVarGet / LibVarString /
 * LibVarValue at sub_10038810..sub_10038A90.  Layout is byte-identical to
 * Q3's libvar_t (q3a_bot_backport_for_q2/botlib/l_libvar.h:33), with the
 * Gladiator-specific tweak that the `name` buffer is appended inline to the
 * end of the alloc and `name` is set to point at it (Q3 keeps `name` as a
 * separate strdup).  Field offsets confirmed:
 *   +0  name          — LibVarGet:  *(const char **)v1   (struct base)
 *   +4  string        — LibVarGetString: *(char **)(v1 + 4)
 *   +8  flags         — written by LibVar via *((_DWORD *)v3 + 2) (was 0 by memset)
 *   +12 modified      — LibVar sets *((_DWORD *)v3 + 3) = 1  (offset 12)
 *   +16 value         — LibVarGetValue: *(float *)(v1 + 16)
 *   +20 next          — LibVarAlloc: *((_DWORD *)v1 + 5) = head; LibVarGet: *(_DWORD *)(v1 + 20)
 * sizeof = 24 + strlen(name) + 1   (name buffer trails the struct)
 *
 * Verified via disassembly of gladiator.dll_:
 *   LibVarAlloc memsets 0x18 bytes (= 24) → struct is 24 bytes.
 *   LibVarAlloc writes (v1 + 24) into *(v1+0) → name is the inline tail.
 */

#ifndef LIBVAR_H
#define LIBVAR_H

typedef struct libvar_s {
    char            *name;       /* +0  inline pointer to name buffer (tail of alloc) */
    char            *string;     /* +4  separately allocated string value             */
    int              flags;      /* +8  (zero in Gladiator; reserved for Q3 compat)   */
    int              modified;   /* +12 1 if value has been set since last poll       */
    float            value;      /* +16 numeric value (atof of string)                */
    struct libvar_s *next;       /* +20 next in singly-linked global list             */
    /* +24: inline name buffer (variable length, NUL-terminated) */
} libvar_t;

/* The global head of the libvar list (was dword_10063F20). */
extern libvar_t *libvarlist;

#endif /* LIBVAR_H */
