/* be_aas_file.h — interface of be_aas_file.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
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
