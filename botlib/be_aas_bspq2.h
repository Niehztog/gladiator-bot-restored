/*
 * be_aas_bspq2.h — interface of be_aas_bspq2.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_BE_AAS_BSPQ2_H
#define BOTLIB_BE_AAS_BSPQ2_H

static void sub_10005640(
        void *out,
        float *start,
        int *boxmins,
        float *boxmaxs,
        int *end,
        int a5,
        int contentmask);

static void sub_10005640(
        void *out,
        float *start,
        int *boxmins,
        float *boxmaxs,
        int *end,
        int a5,
        int contentmask);

static void sub_10005640(
        void *out,
        float *start,
        int *boxmins,
        float *boxmaxs,
        int *end,
        int a5,
        int contentmask);

static void sub_10005640(
        void *out,
        float *start,
        int *boxmins,
        float *boxmaxs,
        int *end,
        int a5,
        int contentmask);

static void sub_10005640(
        void *out,
        float *start,
        int *boxmins,
        float *boxmaxs,
        int *end,
        int a5,
        int contentmask);

static void sub_10005640(
        void *out,
        float *start,
        int *boxmins,
        float *boxmaxs,
        int *end,
        int a5,
        int contentmask);

bsp_link_t *__cdecl AAS_BSPLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum, int modelnum);
void __cdecl AAS_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin);
int __cdecl AAS_BoxOnPlaneSide2(vec3_t absmins, vec3_t absmaxs, float *p);
void __cdecl AAS_DecompressVis(int a1, int a2);
int AAS_DumpBSPData();
qboolean __cdecl AAS_EntityCollision(int entnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int contentmask, bsp_trace_t *trace);
void __cdecl AAS_FreeBSPEntities(bsp_entity_t *a1);
BOOL __cdecl AAS_InPVS(float *a1, float *a2, int a3);
int __cdecl AAS_IntForBSPEpairKey(bsp_entity_t *ent, const char *key);
int AAS_LoadBSPFile(char *FileName, int Offset, int Length);
bsp_entity_t *AAS_ParseBSPEntities(void);
bsp_trace_t __cdecl AAS_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask);
bsp_trace_t __cdecl AAS_TraceBSPModel(int modelnum, const vec3_t modelorigin, vec3_t angles, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int passent, int contentmask);
bsp_link_t *__cdecl AAS_UnlinkFromBSPLeaves(bsp_link_t *leaves);
char *__cdecl AAS_ValueForBSPEpairKey(bsp_entity_t *ent, const char *key);
int __cdecl AAS_VectorForBSPEpairKey(bsp_entity_t *ent, const char *key, vec3_t v);
qboolean __cdecl AAS_inPVS(vec3_t p1, vec3_t p2);
void __cdecl AnglesToAxis(const vec3_t angles, float axis[3][3]);
int __cdecl CM_PointLeafnum(const vec3_t point, int modelnum);
int __cdecl CM_TraceThroughBrush(dbrush_t *a1, float *a2, float *a3, float *a4, float *a5, float *a6, float *a7, float *a8, _DWORD *a9, float *a10, float *a11);
int __cdecl CM_TraceThroughLeaf(int leafnum, vec3_t origin, vec3_t angles, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int contentmask, bsp_trace_t *trace);
void CalcSurfaceExtents();
float __cdecl FloatForKey(bsp_entity_t *ent, const char *key);
int Q2_SwapBSPFile(void);
int __cdecl RecursiveLightPoint(int nodenum, float *start, float *end, float *lightspot, int *pointcolor);
int __cdecl sub_10003080(vec3_t point);
void sub_100030A0();
void __cdecl sub_100031B0(char *name);
bsp_link_t *sub_100031F0(void);
void __cdecl sub_10003240(bsp_link_t *a1);
void sub_10003280();
void sub_100032D0();
dleaf_t *__cdecl sub_10003420(const vec3_t point, int modelnum);
void __cdecl sub_10003460(vec3_t v, float m[3][3]);
int __cdecl sub_10003BF0(int leafnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int passent, int contentmask, bsp_trace_t *trace);
static void sub_10005640(
        void *out,
        float *start,
        int *boxmins,
        float *boxmaxs,
        int *end,
        int a5,
        int contentmask);
int __cdecl sub_100056D0(dbrush_t *a1, float *a2);
int __cdecl sub_100057A0(float *a1, int a2, float *a3, float *a4);
int __cdecl sub_10005A10(float *origin);
BOOL __cdecl sub_10005C90(float *a1, float *a2);
int __cdecl sub_10005CC0(int a, int b);
void __cdecl sub_10005CF0(int row_index, int value);
int __cdecl sub_100063D0(vec3_t mins, vec3_t maxs, int *list, int maxcount);
void __cdecl sub_10006600(bsp_epair_t **head, char *key, char *value);
int __cdecl sub_10007150(intptr_t start, intptr_t end, intptr_t endpos, _DWORD *red, _DWORD *green, _DWORD *blue);
void *__cdecl sub_10007C40(FILE *Stream, int Offset, size_t ElementSize, int a4, char *ArgList);
int sub_100085F0();

#endif /* BOTLIB_BE_AAS_BSPQ2_H */
