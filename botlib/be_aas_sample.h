/* be_aas_sample.h — interface of be_aas_sample.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AAS_SAMPLE_H
#define BOTLIB_BE_AAS_SAMPLE_H

aas_link_t *__cdecl AAS_AASLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum);
aas_link_t *AAS_AllocAASLink(void);
int __cdecl AAS_AreaCluster(int areanum);
qboolean __cdecl AAS_AreaEntityCollision(int areanum, char *start, vec3_t end, int presencetype, int passent, aas_trace_t *trace);
void *__cdecl AAS_AreaGroundFace(int areanum, void *point);
int __cdecl AAS_AreaPresenceType(int areanum);
void __cdecl AAS_DeAllocAASLink(aas_link_t *link);
void __cdecl AAS_FacePlane(int facenum, vec3_t normal, float *dist);
void AAS_FreeAASLinkHeap();
void AAS_FreeAASLinkedEntities();
void AAS_InitAASLinkHeap();
void AAS_InitAASLinkedEntities(void);
qboolean __cdecl AAS_InsideFace(aas_face_t *face, vec3_t pnormal, vec3_t point, float epsilon);
aas_link_t *__cdecl AAS_LinkEntityClientBBox(vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype);
char *__cdecl AAS_PlaneFromNum(int planenum);
int __cdecl AAS_PointAreaNum(vec3_t point);
int __cdecl AAS_PointPresenceType(vec3_t point);
qboolean __cdecl AAS_PointInsideFace(int facenum, vec3_t point, float epsilon);
int __cdecl AAS_TraceAreas(float *start, float *end, int *areas, int maxareas);
aas_trace_t __cdecl AAS_TraceClientBBox(vec3_t start, vec3_t end, int presencetype, int passent);
void __cdecl AAS_UnlinkFromAreas(aas_link_t *areas);
float __cdecl AAS_BoxOriginDistanceFromPlane(vec3_t normal, vec3_t mins, vec3_t maxs, int side);
aas_face_t *__cdecl AAS_TraceEndFace(aas_trace_t *trace);
int __cdecl AAS_BoxOnPlaneSide2(float *a1, float *a2, float *a3);

#endif /* BOTLIB_BE_AAS_SAMPLE_H */
