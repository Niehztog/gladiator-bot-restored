/*
 * be_aas_sound.h — interface of be_aas_sound.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AAS_SOUND_H
#define BOTLIB_BE_AAS_SOUND_H

/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
extern structdef_t soundinfo_struct; /* sound info structdef — defined in botlib_structdefs.c */

void __cdecl sub_1001C6F0(void);
int __cdecl sub_1001C760(char *Source);
void sub_1001CAB0();
aas_soundpool_t *sub_1001CBE0();
aas_soundpool_t *sub_1001CC10(aas_soundpool_t *a1);
void sub_1001CC50(aas_soundpool_t *a1);
aas_soundpool_t *sub_1001CCC0(aas_soundpool_t *a1);
void sub_1001CD10(aas_soundpool_t *a1);
aas_soundpool_t *sub_1001CD80(aas_soundpool_t *a1);
void __cdecl sub_1001CDD0(int a1, int a2);
int __cdecl sub_1001CE20(float *, int, int, int, int, int, float);
void __cdecl sub_1001CFA0(float a1);
int __cdecl sub_1001D040(aas_soundpool_t *p);
int __cdecl sub_1001D070(aas_soundpool_t *p);
float __cdecl sub_1001D0A0(float *listener, aas_soundpool_t *emitter);
void sub_1001D140();
int sub_1001D260();
void sub_1001D290(void);  

#endif /* BOTLIB_BE_AAS_SOUND_H */
