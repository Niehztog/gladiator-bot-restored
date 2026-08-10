/*
 * l_struct.h — interface of l_struct.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_L_STRUCT_H
#define BOTLIB_L_STRUCT_H

#include "botlib_local.h"

const char **__cdecl FindField(const char **defs, const char *name);
int __cdecl ReadChar(source_t *source, char **fd, float *p);
int __cdecl ReadNumber(source_t *source, char **fd, float *p);
int __cdecl ReadString(source_t * source, char ** fd, char *p);
int __cdecl ReadStructure(source_t *source, structdef_t *def, char *structure);
int __cdecl WriteFloat(FILE *fp, float value);
int __cdecl WriteIndent(FILE *fp, int indent);
int __cdecl WriteStructWithIndent(FILE *fp, structdef_t *def, int structure, int indent);
int __cdecl WriteStructure(FILE *fp, int def, int structure);
static inline float fielddef_float(char **f, int slot);

#endif /* BOTLIB_L_STRUCT_H */
