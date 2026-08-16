/* l_libvar.h — interface of l_libvar.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_L_LIBVAR_H
#define BOTLIB_L_LIBVAR_H

/* libvar_t lives here with the LibVar* prototypes it belongs to -- the shape of Q3 botlib's own l_libvar.h. */
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



/* Declarations for what this TU defines — last, so the types above are in scope. */
libvar_t *__cdecl LibVar(char *var_name, char *value);            /* register/lookup libvar */
float     __cdecl LibVarValue(char *var_name, char *value);    /* register, return value */
void __cdecl LibVarSet(char *var_name, char *value);  /* body at ~30304 */
char     *__cdecl LibVarString(char *var_name, char *value);   /* returns libvar->string */

libvar_t *__cdecl LibVar(char *var_name, char *value);
libvar_t *__cdecl LibVarAlloc(const char *var_name);
int __cdecl LibVarChanged(const char *var_name);
void      __cdecl LibVarDeAlloc(libvar_t *v);
void __cdecl LibVarDeAllocAll(void);
libvar_t *__cdecl LibVarGet(const char *var_name);
char     *__cdecl LibVarGetString(const char *var_name);
float     __cdecl LibVarGetValue(const char *var_name);
void __cdecl LibVarSet(char *var_name, char *value);
void __cdecl LibVarSetNotModified(const char *var_name);
char *__cdecl LibVarString(char *var_name, char *value);
float __cdecl LibVarStringValue(char *string);
float __cdecl LibVarValue(char *var_name, char *value);

#endif /* BOTLIB_L_LIBVAR_H */
