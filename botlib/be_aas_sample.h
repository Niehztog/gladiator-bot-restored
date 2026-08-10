/*
 * be_aas_sample.h — interface of be_aas_sample.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AAS_SAMPLE_H
#define BOTLIB_BE_AAS_SAMPLE_H

#include "botlib_local.h"

aas_link_t *__cdecl AAS_AASLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum);
aas_link_t *AAS_AllocAASLink(void);
int __cdecl AAS_AreaCluster(int areanum);
qboolean __cdecl AAS_AreaEntityCollision(int areanum, char *start, vec3_t end, int presencetype, int passent, aas_trace_t *trace);
void *__cdecl AAS_AreaGroundFace(int areanum, void *point);
int __cdecl AAS_AreaPresenceType(int areanum);
aas_link_t *__cdecl AAS_DeAllocAASLink(aas_link_t *link);
void __cdecl AAS_FacePlane(int facenum, float *normal, float *dist);
void AAS_FreeAASLinkHeap();
void AAS_FreeAASLinkedEntities();
void AAS_InitAASLinkHeap();
int AAS_InitAASLinkedEntities();
qboolean __cdecl AAS_InsideFace(aas_face_t *face, vec3_t pnormal, vec3_t point, float epsilon);
aas_link_t *__cdecl AAS_LinkEntityClientBBox(vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype);
char *__cdecl AAS_PlaneFromNum(int planenum);
int __cdecl AAS_PointAreaNum(vec3_t point);
int __cdecl AAS_PointContents(vec3_t point);
qboolean __cdecl AAS_PointInsideFace(int facenum, vec3_t point, float epsilon);
int __cdecl AAS_TraceAreas(float *start, float *end, int *areas, int maxareas);
aas_trace_t __cdecl AAS_TraceClientBBox(vec3_t start, vec3_t end, int presencetype, int passent);
aas_link_t *__cdecl AAS_UnlinkFromAreas(aas_link_t *areas);
double __cdecl sub_1001AFF0(float *normal, float *mins, float *maxs, int sign_select);
void *__cdecl sub_1001C210(int *gate);
int __cdecl sub_1001C2E0(float *a1, float *a2, float *a3);

#endif /* BOTLIB_BE_AAS_SAMPLE_H */
