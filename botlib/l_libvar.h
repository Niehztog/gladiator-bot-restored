/*
 * l_libvar.h — interface of l_libvar.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_L_LIBVAR_H
#define BOTLIB_L_LIBVAR_H

#include "botlib_local.h"

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
