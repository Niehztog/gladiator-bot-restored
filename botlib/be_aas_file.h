/*
 * be_aas_file.h — interface of be_aas_file.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AAS_FILE_H
#define BOTLIB_BE_AAS_FILE_H

void *AAS_DumpAASData();
int __cdecl AAS_LoadAASFile(char *FileName, int Offset, int Length);
void *__cdecl AAS_LoadAASLump(FILE *Stream, int Offset, size_t ElementCount);
void AAS_SwapAASData();
qboolean __cdecl AAS_WriteAASFile(char *filename);
int __cdecl AAS_WriteAASLump(FILE *fp, int *h, int lumpnum, void *data, size_t length);
void sub_1000D340(void);

#endif /* BOTLIB_BE_AAS_FILE_H */
