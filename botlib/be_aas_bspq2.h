/* be_aas_bspq2.h — interface of be_aas_bspq2.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AAS_BSPQ2_H
#define BOTLIB_BE_AAS_BSPQ2_H


/* Declarations for what this TU defines — last, so the types above are in scope. */
int __cdecl AAS_BoxOnPlaneSide2(vec3_t absmins, vec3_t absmaxs, float *p);  /* Q3 canonical name */
void __cdecl AnglesToAxis(const vec3_t angles, float axis[3][3]);  // 0x100034D0; was sub_100034D0 (originally also mislabeled sub_100423B0)
int  AAS_LoadBSPFile(char *FileName, int Offset, int Length); /* be_aas_bspq2.c 0x10007D30 */

/* bspworld_t — the Q2 BSP/CM (collision-model) file-format data used by this TU's
 * runtime BSP loader/queries (AAS_LoadBSPFile, CM_PointLeafnum, CM_TraceThroughLeaf,
 * AAS_InPVS, …): ONE contiguous 8392-byte (0x20C8) BSS struct at
 * VA 0x100674C0..0x10069588, confirmed against gladi386.so's exported data symbol
 * (`0005dd8c 8392 OBJECT GLOBAL DEFAULT 18 bspworld`), which also gives the name.
 *
 * It must stay ONE struct: splitting it into separate globals changes codegen for any
 * function that computes a member offset off one base pointer — the original emits a
 * single-base offset access where separate globals emit independent absolute
 * addresses.
 *
 * Field names/order for the 37 standard Q2 BSP lumps (nummodels..dareaportals) match
 * Mr. Elusive's own Q2 BSP loader bspc/l_bsp_q2.c 1:1.  Lumps with a real record type
 * use their typed q2files.h pointer and are indexed as arrays; only the genuinely
 * untyped blobs (dvisdata, dlightdata, dentdata) stay byte pointers.  The one remaining
 * byte-view is the dmodels walk in Q2_SwapBSPFile.  The other 14 fields have no
 * l_bsp_q2.c cognate — Gladiator-specific runtime precompute/cache state — and keep
 * their decompiled dword_/byte_/flt_ names. */
typedef struct bspworld_s {
    int            dword_100674C0;  /* +0x000 (VA 0x100674C0) "BSP loaded" guard flag; no l_bsp_q2.c cognate */
    int            nummodels;       /* +0x004 (VA 0x100674C4) */
    dmodel_t      *dmodels;         /* +0x008 (VA 0x100674C8) */
    int            visdatasize;     /* +0x00C (VA 0x100674CC) */
    char          *dvisdata;        /* +0x010 (VA 0x100674D0) */
    dvis_t        *dvis;            /* +0x014 (VA 0x100674D4) dvis_t* alias of dvisdata */
    int            lightdatasize;   /* +0x018 (VA 0x100674D8) */
    char          *dlightdata;      /* +0x01C (VA 0x100674DC) */
    int            entdatasize;     /* +0x020 (VA 0x100674E0) */
    unsigned char *dentdata;        /* +0x024 (VA 0x100674E4; was int in 32-bit binary) */
    int            numleafs;        /* +0x028 (VA 0x100674E8) */
    dleaf_t       *dleafs;          /* +0x02C (VA 0x100674EC) */
    int            numplanes;       /* +0x030 (VA 0x100674F0) */
    dplane_t      *dplanes;         /* +0x034 (VA 0x100674F4) */
    int            numvertexes;     /* +0x038 (VA 0x100674F8) */
    dvertex_t     *dvertexes;       /* +0x03C (VA 0x100674FC) */
    int            numnodes;        /* +0x040 (VA 0x10067500) */
    dnode_t       *dnodes;          /* +0x044 (VA 0x10067504) */
    int            numtexinfo;      /* +0x048 (VA 0x10067508) */
    texinfo_t     *texinfo;         /* +0x04C (VA 0x1006750C) */
    int            numfaces;        /* +0x050 (VA 0x10067510) */
    dface_t       *dfaces;          /* +0x054 (VA 0x10067514) */
    int            numedges;        /* +0x058 (VA 0x10067518) */
    dedge_t       *dedges;          /* +0x05C (VA 0x1006751C) */
    int            numleaffaces;    /* +0x060 (VA 0x10067520) */
    unsigned short *dleaffaces;     /* +0x064 (VA 0x10067524) */
    int            numleafbrushes;  /* +0x068 (VA 0x10067528) */
    unsigned short *dleafbrushes;   /* +0x06C (VA 0x1006752C) */
    int            numsurfedges;    /* +0x070 (VA 0x10067530) */
    int           *dsurfedges;      /* +0x074 (VA 0x10067534) */
    int            numbrushes;      /* +0x078 (VA 0x10067538) */
    dbrush_t      *dbrushes;        /* +0x07C (VA 0x1006753C) */
    int            numbrushsides;   /* +0x080 (VA 0x10067540) */
    dbrushside_t  *dbrushsides;     /* +0x084 (VA 0x10067544) */
    int            numareas;        /* +0x088 (VA 0x10067548) */
    darea_t       *dareas;          /* +0x08C (VA 0x1006754C) */
    int            numareaportals;  /* +0x090 (VA 0x10067550) */
    dareaportal_t *dareaportals;    /* +0x094 (VA 0x10067554) */
    /* 0x098..0x0A0 -- three Gladiator-specific AAS precompute pointers after
     * the standard Q2 lumps; no cognate in l_bsp_q2.c, so left unnamed. */
    char *dword_10067558;  /* +0x098 (VA 0x10067558) per-face {short texturemins[2];
                             * short extents[2]} table, 8*numfaces, built by
                             * CalcSurfaceExtents, read by RecursiveLightPoint.
                             * NOT a PVS table. */
    char *dword_1006755C; /* +0x09C (VA 0x1006755C) pointer */
    char *dword_10067560; /* +0x0A0 (VA 0x10067560) pointer */
    char  byte_10067564[8192]; /* +0x0A4 (VA 0x10067564) AAS_DecompressVis's output row
                                 * buffer (Q2 CM_DecompressVis's static `decompressed[]`
                                 * cognate) */
    int   dword_10069564;  /* +0x20A4 (VA 0x10069564) last-decompressed cluster
                              * (AAS_DecompressVis early-out cache) */
    int   dword_10069568;  /* +0x20A8 (VA 0x10069568) weak; written only, from
                              * dleaf_t.area in AAS_InPVS's point-cache path */
    float flt_1006956C;    /* +0x20AC (VA 0x1006956C) AAS_InPVS point cache x */
    float flt_10069570;    /* +0x20B0 (VA 0x10069570) AAS_InPVS point cache y */
    float flt_10069574;    /* +0x20B4 (VA 0x10069574) AAS_InPVS point cache z */
    bsp_link_t  *dword_10069578; /* +0x20B8 (VA 0x10069578) bsp_linkheap (pool base) */
    int          dword_1006957C; /* +0x20BC (VA 0x1006957C) bsp_linkheapsize (count) */
    bsp_link_t  *dword_10069580; /* +0x20C0 (VA 0x10069580) bsp_freelinks (head of free list) */
    bsp_link_t **dword_10069584; /* +0x20C4 (VA 0x10069584) bsp_leaflinks (per-leaf list-heads array) */
} bspworld_t;                    /* sizeof == 0x20C8 == 8392 */

/* Offset checks vs the original binary's VA layout.  32-bit only -- every
 * offset past the first pointer field shifts on 64-bit. */
#include <stddef.h>
#if __SIZEOF_POINTER__ == 4
_Static_assert(sizeof(bspworld_t) == 0x20C8,                        "bspworld_t size");
_Static_assert(offsetof(bspworld_t, dword_100674C0) == 0x000,       "dword_100674C0");
_Static_assert(offsetof(bspworld_t, nummodels)      == 0x004,       "nummodels");
_Static_assert(offsetof(bspworld_t, numleafs)       == 0x028,       "numleafs");
_Static_assert(offsetof(bspworld_t, numplanes)      == 0x030,       "numplanes");
_Static_assert(offsetof(bspworld_t, numtexinfo)     == 0x048,       "numtexinfo");
_Static_assert(offsetof(bspworld_t, numfaces)       == 0x050,       "numfaces");
_Static_assert(offsetof(bspworld_t, numbrushes)     == 0x078,       "numbrushes");
_Static_assert(offsetof(bspworld_t, numareas)       == 0x088,       "numareas");
_Static_assert(offsetof(bspworld_t, dareaportals)   == 0x094,       "dareaportals");
_Static_assert(offsetof(bspworld_t, dword_10067558) == 0x098,       "dword_10067558");
_Static_assert(offsetof(bspworld_t, byte_10067564)  == 0x0A4,       "byte_10067564");
_Static_assert(offsetof(bspworld_t, dword_10069564) == 0x20A4,      "dword_10069564");
_Static_assert(offsetof(bspworld_t, dword_10069578) == 0x20B8,      "dword_10069578");
_Static_assert(offsetof(bspworld_t, dword_10069584) == 0x20C4,      "dword_10069584");
#endif /* __SIZEOF_POINTER__ == 4 */

/* Single instance defined in be_aas_bspq2.c; name matches the real Linux
 * symbol exactly. */
extern bspworld_t bspworld;


/* Declared but never defined -- a dead declaration from the decompilation. */

bsp_link_t *__cdecl AAS_BSPLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum, int modelnum);
void __cdecl AAS_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin);
int __cdecl AAS_BoxOnPlaneSide2(vec3_t absmins, vec3_t absmaxs, float *p);
void __cdecl AAS_DecompressVis(int a1, int a2);
void AAS_DumpBSPData();
qboolean __cdecl AAS_EntityCollision(int entnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int contentmask, bsp_trace_t *trace);
void __cdecl AAS_FreeBSPEntities(bsp_entity_t *a1);
BOOL __cdecl AAS_InPVS(float *a1, float *a2, int a3);
int __cdecl AAS_IntForBSPEpairKey(bsp_entity_t *ent, const char *key);
int AAS_LoadBSPFile(char *FileName, int Offset, int Length);
bsp_entity_t *AAS_ParseBSPEntities(void);
bsp_trace_t __cdecl AAS_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask);
bsp_trace_t __cdecl AAS_TraceBSPModel(int modelnum, const vec3_t modelorigin, vec3_t angles, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int passent, int contentmask);
void __cdecl AAS_UnlinkFromBSPLeaves(bsp_link_t *leaves);
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
void __cdecl RotatePoint(vec3_t point, float matrix[3][3]);
int __cdecl sub_10003BF0(int leafnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int passent, int contentmask, bsp_trace_t *trace);
/* NOT static: the real gladi386.so exports it as F680 (0xd0cc, 106 B) and
 * the DLL keeps it at 0x10005640 through its /INCREMENTAL thunk.  It has no
 * caller in either image, so `static` would let both compilers strip it. */
bsp_trace_t __cdecl sub_10005640(vec3_t start, vec3_t boxmins, vec3_t boxmaxs,
                                 vec3_t end, int passent, int contentmask);
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
