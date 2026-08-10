/*
 * be_aas_file.h — interface of be_aas_file.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AAS_FILE_H
#define BOTLIB_BE_AAS_FILE_H

#include "botlib_local.h"

void *AAS_DumpAASData();
int __cdecl AAS_LoadAASFile(char *FileName, int Offset, int Length);
void *__cdecl AAS_LoadAASLump(FILE *Stream, int Offset, size_t ElementCount);
void AAS_SwapAASData();
qboolean __cdecl AAS_WriteAASFile(char *filename);
int __cdecl AAS_WriteAASLump(FILE *fp, int *h, int lumpnum, void *data, size_t length);
static void sub_1000D340(void);

#endif /* BOTLIB_BE_AAS_FILE_H */
