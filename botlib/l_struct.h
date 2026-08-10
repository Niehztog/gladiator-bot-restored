/*
 * l_struct.h — interface of l_struct.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_L_STRUCT_H
#define BOTLIB_L_STRUCT_H

/* structdef_t: the struct descriptor ReadStructure/FindField take. */
/* structdef_t — struct descriptor passed to ReadStructure / FindField.  A
 * plain int[2] { size, fields_ptr } in the 32-bit original; typed here so it
 * is pointer-size-agnostic. */
typedef struct { int size; char **fields; } structdef_t;

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
