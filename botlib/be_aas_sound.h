/* be_aas_sound.h — interface of be_aas_sound.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AAS_SOUND_H
#define BOTLIB_BE_AAS_SOUND_H

/* Declarations for what this TU defines — last, so the types above are in scope. */
extern structdef_t soundinfo_struct; /* sound info structdef — defined in botlib_structdefs.c */

void __cdecl sub_1001C6F0(void);
int __cdecl sub_1001C760(char *Source);
void sub_1001CAB0();
aas_soundpool_t *sub_1001CBE0();
void sub_1001CC10(aas_soundpool_t *a1);
void sub_1001CC50(aas_soundpool_t *a1);
void sub_1001CCC0(aas_soundpool_t *a1);
void sub_1001CD10(aas_soundpool_t *a1);
void sub_1001CD80(aas_soundpool_t *a1);
void __cdecl sub_1001CDD0(int a1, int a2);
int __cdecl sub_1001CE20(float *, int, int, int, float, float, float);
void __cdecl sub_1001CFA0(float a1);
int __cdecl sub_1001D040(aas_soundpool_t *p);
int __cdecl sub_1001D070(aas_soundpool_t *p);
float __cdecl sub_1001D0A0(float *listener, aas_soundpool_t *emitter);
void sub_1001D140();
int sub_1001D260();
void sub_1001D290(void);  

#endif /* BOTLIB_BE_AAS_SOUND_H */
