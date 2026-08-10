/*
 * l_utils.h — interface of l_utils.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_L_UTILS_H
#define BOTLIB_L_UTILS_H

#include "botlib_local.h"

BOOL __cdecl sub_10041240(int a1, const char *a2, int a3);
int __stdcall sub_100415E0(int a1);
void sub_10041600(void);
static int sub_10041650(void);
LPSTR __stdcall sub_10041680(unsigned int a1, unsigned int a2, unsigned __int16 a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13);
int __stdcall sub_10041740(int a1, int a2, int a3, int a4);
int __stdcall sub_10041760(const char *a1, int a2);
char __cdecl sub_100418D0(_BYTE *a1);
void __cdecl sub_10041900(const char *a1, int a2);
int __cdecl sub_10041970(char *FileName, const char *, bot_fileref_t *);
int __cdecl sub_10041BA0(char *a1, char *Source, char *a3, bot_fileref_t *a4);
BOOL __cdecl sub_10041F60(char *a1, bot_fileref_t *a2);
int __cdecl sub_10041FF0(const char *zipfile, const char *file_to_archive);
void sub_10042380();
int __stdcall sub_100423B0(int a1, int a2, int a3, int a4);
int __cdecl sub_100423D0(int a1, int a2);
void __stdcall sub_100423F0(char *p);
int __cdecl vectoangles(float *value1, float *angles);

#endif /* BOTLIB_L_UTILS_H */
