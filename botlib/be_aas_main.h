/* be_aas_main.h — interface of be_aas_main.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AAS_MAIN_H
#define BOTLIB_BE_AAS_MAIN_H

/* Declarations for what this TU defines — last, so the types above are in scope. */
extern aas_link_t **aasentity_arealinks;
extern bsp_link_t **aasentity_bsplinks;
extern aas_world_t aasworld;

int __cdecl AAS_ContinueInit(float time);
int AAS_Error(char *Format, ...);
char *__cdecl AAS_ImageFromIndex(int index);
int __cdecl AAS_IndexFromImage(char *String2);
int __cdecl AAS_IndexFromSound(char *String2);
int __cdecl AAS_IndexFromString(const char * indexname, indexlist_t * list, char *String2);
int AAS_Initialized();
char *__cdecl AAS_ModelFromIndex(int index);
int __cdecl AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
int __cdecl AAS_SetInitialized(void);
int AAS_Shutdown();
char *__cdecl AAS_SoundFromIndex(int index);
int __cdecl AAS_StartFrame(float time);
char *__cdecl AAS_StringFromIndex(const char *indexname, indexlist_t *list, int index);
float AAS_Time();
int __cdecl BotLibLoadMap(char *Source);
int __cdecl BotLoadMap(char *Source, int, char **, int, char **, int, char **);
int __cdecl IndexFromModel(char *String2);
indexlist_t *__cdecl sub_1000DA80(int numindexes, char **names);
void __cdecl sub_1000DB40(indexlist_t *list, int numindexes, char **names);
int __cdecl sub_1000DBD0(indexlist_t *list);
void __cdecl sub_1000DC20(int a1, char **a2, int a3, char **a4, int a5, char **a6);
void __cdecl sub_1000DCC0(int a1, char **a2, int a3, char **a4, int a5, char **a6);
intptr_t __cdecl sub_1000E140(char *Source);
int __cdecl sub_1000E430(char *Source);
int __cdecl sub_1000EDC0(int a1, int a2);

#endif /* BOTLIB_BE_AAS_MAIN_H */
