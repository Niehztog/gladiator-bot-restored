/* l_struct.h — interface of l_struct.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_L_STRUCT_H
#define BOTLIB_L_STRUCT_H

/* Q3 l_struct.h's string length and field types, verbatim.  The FE() tables and
 * ReadStructure/WriteStructure use every one. */
#define MAX_STRINGFIELD				80
//field types
#define FT_CHAR						1			// char
#define FT_INT							2			// int
#define FT_FLOAT						3			// float
#define FT_STRING						4			// char [MAX_STRINGFIELD]
#define FT_STRUCT						6			// struct (sub structure)
//type only mask
#define FT_TYPE						0x00FF	// only type, clear subtype
//sub types
#define FT_ARRAY						0x0100	// array of type
#define FT_BOUNDED					0x0200	// bounded value
#define FT_UNSIGNED					0x0400

/* structdef_t: the struct descriptor ReadStructure/FindField take. */
/* structdef_t — struct descriptor passed to ReadStructure / FindField.  A
 * plain int[2] { size, fields_ptr } in the 32-bit original; typed here so it
 * is pointer-size-agnostic. */
typedef struct { int size; char **fields; } structdef_t;

/* fielddef_t — Q3's l_struct.h field descriptor, overlaid on the char *[7] entries
 * the FE() tables build.  Every member spans one pointer-sized slot so the overlay
 * holds on either width: the float bounds are real `float`s (what ReadNumber's
 * compares need on both oracles) in the low half of their slot, with an explicit
 * pad on LP64 where a slot is 8 bytes. */
typedef struct fielddef_s {
    const char *name;            /* slot 0 */
    intptr_t    offset;          /* slot 1 */
    intptr_t    type;            /* slot 2 — FT_* type and sub types */
    intptr_t    maxarray;        /* slot 3 */
    float       floatmin;        /* slot 4 — Q3's float members; each is the low */
#if BOTLIB_NEED_SIDEBAND
    int         _floatmin_hi;    /*          half of a pointer-sized slot on LP64 */
#endif
    float       floatmax;        /* slot 5 */
#if BOTLIB_NEED_SIDEBAND
    int         _floatmax_hi;
#endif
    structdef_t *substruct;      /* slot 6 */
} fielddef_t;

fielddef_t *__cdecl FindField(fielddef_t *defs, const char *name);
int __cdecl ReadChar(source_t *source, fielddef_t *fd, float *p);
int __cdecl ReadNumber(source_t *source, fielddef_t *fd, float *p);
int __cdecl ReadString(source_t * source, char ** fd, char *p);
int __cdecl ReadStructure(source_t *source, structdef_t *def, char *structure);
int __cdecl WriteFloat(FILE *fp, float value);
int __cdecl WriteIndent(FILE *fp, int indent);
int __cdecl WriteStructWithIndent(FILE *fp, structdef_t *def, int structure, int indent);
int __cdecl WriteStructure(FILE *fp, int def, int structure);

#endif /* BOTLIB_L_STRUCT_H */
