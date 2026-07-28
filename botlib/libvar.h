/*
 * libvar.h — Bot library variable (Q3's l_libvar.h equivalent).
 *
 * Same layout as Q3's libvar_t, except the name buffer is appended inline to
 * the allocation and `name` points at it (Q3 strdups it separately), so the
 * alloc is 24 + strlen(name) + 1 bytes.
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

/* Head of the libvar list (dword_10063F20). */
extern libvar_t *libvarlist;

#endif /* LIBVAR_H */
