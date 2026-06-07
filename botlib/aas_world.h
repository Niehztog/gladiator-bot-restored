/*
 * aas_world.h — AAS (Area Awareness System) world state structure
 *
 * Reconstructed from Gladiator Bot v0.96 decompilation.
 * In the original source, all fields below were a single global `aasworld`
 * of type `aas_t` (as in Q3's be_aas_def.h). The DLL was compiled with the
 * struct fields as individually-linked globals (`aasworld_*`), which is how
 * they appear in the decompiled code.
 *
 * Field mapping: Q2 global → Q3 aas_t field (be_aas_def.h)
 * Nearly 1:1 with Q3; differences noted with [Q2-ONLY] or [Q3-ONLY].
 */

#ifndef AAS_WORLD_H
#define AAS_WORLD_H

/* -----------------------------------------------------------------------
 * AAS element type sizes (from Q3 AAS file format, same in Q2)
 * --------------------------------------------------------------------- */
typedef float  aas_vertex_t[3];

typedef struct aas_plane_s { float normal[3]; float dist; int type; }  aas_plane_t;
typedef struct aas_edge_s  { int v[2]; }                               aas_edge_t;
typedef int                                                aas_edgeindex_t;
/* aas_face_t — 24-byte face record (verified by stride `24 * facenum` in
 * 10+ usages).  Field naming matches Q3 botlib (be_aas_def.h aas_face_t):
 *   +0  planenum  +4 faceflags  +8 numedges  +12 firstedge
 *   +16 frontarea +20 backarea
 * Earlier reconstruction had `{planenum; areas[2]; edges[2]}` which yielded
 * 20 bytes and reversed the area-vs-edge order; nothing in the codebase
 * referenced those member names so the rename is safe. */
typedef struct aas_face_s {
    int planenum;
    int faceflags;
    int numedges;
    int firstedge;
    int frontarea;
    int backarea;
} aas_face_t;
typedef int                                                aas_faceindex_t;

/* aas_area_t — 48-byte area record (stride verified as 48 == 12 dwords in
 * 40+ usages, e.g. `(char*)aasworld.areas + 48*areanum`).  Layout matches Q3
 * botlib aas_area_t exactly (be_aas_def.h): dword 1 = numfaces (loop count),
 * dword 2 = firstface (faceindex base), dwords 9-11 = center/origin.  The
 * earlier reconstruction here had a 32-byte {mins;maxs;firstface;numfaces}
 * which was wrong on every count; nothing referenced those member names. */
typedef struct aas_area_s {
    int   areanum;       /* +0   dword 0 */
    int   numfaces;      /* +4   dword 1 */
    int   firstface;     /* +8   dword 2 */
    float mins[3];       /* +12  dwords 3-5 */
    float maxs[3];       /* +24  dwords 6-8 */
    float center[3];     /* +36  dwords 9-11 (the area 'origin') */
} aas_area_t;                           /* 48 bytes  (offset stride = 48) */

typedef struct aas_areasettings_s {
    int contents;
    int areaflags;
    int presencetype;
    int cluster;
    int clusterareanum;
    int numreachableareas;
    int firstreachablearea;
} aas_areasettings_t;                   /* 28 bytes  (stride = 28) */

typedef struct aas_reachability_s {
    int   areanum;       /* +0  destination area number              */
    int   facenum;       /* +4  face crossed by this reachability    */
    int   edgenum;       /* +8  edge crossed by this reachability    */
    float start[3];      /* +12 start origin                         */
    float end[3];        /* +24 end origin                           */
    int   traveltype;    /* +36                                      */
    short traveltime;    /* +40                                      */
} aas_reachability_t;    /* 44 bytes (stride = 44)                   */

/* Free-list node used by AAS_SetupReachabilityHeap / AAS_AllocReachability.
 * 32-bit: 44 reach bytes + 4-byte next ptr = 48-byte stride.  64-bit
 * widens next to 8 bytes; the allocator uses sizeof() so the pool sizes
 * correctly for either word width and reach offsets within the node
 * remain at +0 (consumers cast the popped void* straight to int* / reach). */
typedef struct aas_reachabilitynode_s {
    aas_reachability_t              reach;     /* +0   payload */
    struct aas_reachabilitynode_s  *next;      /* +44 on 32-bit, +48 on 64-bit (padding) */
} aas_reachabilitynode_t;

typedef struct aas_node_s { int planenum; int children[2]; } aas_node_t;
typedef struct aas_portal_s {
    int areanum;             /* +0  area on the front side                  */
    int frontcluster;        /* +4  cluster on the front side               */
    int backcluster;         /* +8  cluster on the back side                */
    int clusterareanum[2];   /* +12 area number per side (front=0/back=1)   */
} aas_portal_t;              /* 20 bytes (stride = 20)                      */
typedef int                                                aas_portalindex_t;

/* BSP traversal stack frame used by AAS_TraceClientBBox / AAS_TraceAreas.
 * Matches Q3 botlib aas_tracestack_t exactly (be_aas_sample.c:56). */
typedef struct aas_tracestack_s {
    float start[3];     /* start point of the piece of line to trace */
    float end[3];       /* end point of the piece of line to trace   */
    int   planenum;     /* last plane used as splitter               */
    int   nodenum;      /* node found after splitting with planenum  */
} aas_tracestack_t;

/* Trace result returned by AAS_TraceClientBBox.  9 dwords (36 bytes).
 * Matches Q3 botlib aas_trace_t exactly (be_aas.h:84). */
typedef struct aas_trace_s {
    int   startsolid;   /* if true, the initial point was in solid */
    float fraction;     /* time completed, 1.0 = didn't hit anything */
    float endpos[3];    /* final position */
    int   ent;          /* entity blocking the trace */
    int   lastarea;     /* last area the trace was in (zero if none) */
    int   area;         /* area blocking the trace (zero if none) */
    int   planenum;     /* number of the plane that was hit */
} aas_trace_t;

typedef struct aas_cluster_s {
    int   numareas;              /* +0  stride was 12 (3 ints) in 32-bit binary */
    int   numreachabilityareas;  /* +4  */
    int   firstportal;           /* +8  */
} aas_cluster_t;

/* Working struct used by AAS_Optimize / AAS_OptimizeArea / AAS_OptimizeFace /
 * AAS_OptimizeEdge / AAS_OptimizeAlloc / AAS_OptimizeStore.  In the IDA
 * decompilation this was an anonymous _DWORD[15] local; on 64-bit pointer
 * fields are 8 bytes so a flat _DWORD[] truncates them. */
typedef struct optimized_s {
    int      numvertexes;        /*  0 */
    void    *vertexes;           /*  1  12-byte vec3 per vertex */
    int      numedges;           /*  2 */
    void    *edges;              /*  3  8-byte aas_edge_t */
    int      edgeindexsize;      /*  4 */
    int     *edgeindex;          /*  5 */
    int      numfaces;           /*  6 */
    void    *faces;              /*  7  24-byte aas_face_t */
    int      faceindexsize;      /*  8 */
    int     *faceindex;          /*  9 */
    int      numareas;           /* 10 */
    void    *areas;              /* 11  48-byte aas_area_t */
    int     *vertexremap;        /* 12  4*numvertexes */
    int     *edgeremap;          /* 13  4*numedges */
    int     *faceremap;          /* 14  4*numfaces */
} optimized_t;

typedef struct aas_link_s {
    int                entnum;     /* +0  */
    int                areanum;    /* +4  */
    struct aas_link_s *next_ent;   /* +8  in-area entity chain (forward) */
    struct aas_link_s *prev_ent;   /* +12 in-area entity chain (back)    */
    struct aas_link_s *next_area;  /* +16 per-entity area chain (forward)*/
    struct aas_link_s *prev_area;  /* +20 per-entity area chain (back)   */
} aas_link_t;

/* Forward declaration for the BSP-leaf link node; its full definition lives
 * in botlib_structs.h (included after this header).  Only a pointer to it is
 * needed below, so the struct tag alone is enough. */
struct bsp_link_s;

/* aas_entityinfo_t — per-frame snapshot of one engine entity, the "info" half
 * of aas_entity_t.  AAS_UpdateEntity() fills it from the game side's
 * bot_updateentity_t (game/botlib.h), and AAS_EntityInfo() hands back a
 * 124-byte (0x7C) copy of exactly this block.
 *
 * This is the Q2/Gladiator flavour of Q3's aas_entityinfo_t: it predates the
 * Q3 player-model fields (type/flags/groundent/weapon/legsAnim/torsoAnim) and
 * instead mirrors the Q2 entity_state_t members (modelindex2..4, skinnum,
 * effects, renderfx).  Field names follow Mr. Elusive's own bot_updateentity_t
 * and Q3's aas_entityinfo_t where they coincide; the header fields (valid,
 * ltime, update_time, number, lastvisorigin) use the Q3 names.
 *
 * Offsets verified instruction-by-instruction against AAS_UpdateEntity
 * @0x1000A920 (esi = entity base, edi = bot_updateentity_t) and the readers
 * AAS_EntityModelindex/RenderFX/ModelNum/BSPData/Size. */
typedef struct aas_entityinfo_s {
    int    valid;          /* +0    true if updated this frame             */
    float  ltime;          /* +4    local time of last update              */
    float  update_time;    /* +8    time between last and current update   */
    int    number;         /* +12   number of the entity                   */
    vec3_t origin;         /* +16   origin of the entity                   */
    vec3_t angles;         /* +28   angles of the model                    */
    vec3_t old_origin;     /* +40   for lerping                            */
    vec3_t lastvisorigin;  /* +52   last visible origin (prev-frame origin)*/
    vec3_t mins;           /* +64   bounding box minimums                  */
    vec3_t maxs;           /* +76   bounding box maximums                  */
    int    solid;          /* +88   solid type (SOLID_BBOX=2 / SOLID_BSP=3)*/
    int    modelindex;     /* +92   model used                            */
    int    modelindex2;    /* +96   weapons, CTF flags, etc               */
    int    modelindex3;    /* +100                                        */
    int    modelindex4;    /* +104                                        */
    int    frame;          /* +108  model frame number                    */
    int    skinnum;        /* +112  skin number (carried, not set by Update)*/
    int    effects;        /* +116  special effects                       */
    int    renderfx;       /* +120  render fx flags                       */
} aas_entityinfo_t;        /* 124 bytes (0x7C) */

/* aas_entity_t — element type of aasworld.entities[], 132 bytes.  Mirrors
 * Q3's  { aas_entityinfo_t i; aas_link_t *areas; bsp_link_t *leaves; }.
 *
 * The two link heads are inline 4-byte pointer slots in the original 32-bit
 * DLL (area chain @+124, BSP-leaf chain @+128).  An 8-byte pointer cannot fit
 * a 4-byte slot, so — exactly like the bot_state_t / bot_chatstate_t
 * side-bands (see BOTLIB_NEED_SIDEBAND in botlib.c) — on 64-bit the real heads
 * are mirrored into the parallel arrays aasentity_arealinks[] /
 * aasentity_bsplinks[] and these slots degrade to inert 4-byte placeholders.
 * Keeping them 4 bytes pins sizeof(aas_entity_t) at 132 on every target, so
 * aasworld.entities[entnum] indexing and the heap stride stay ABI-correct.
 *
 * Always read/write the link heads through AAS_EntAreaLink()/AAS_EntBspLink()
 * (botlib.c), never these fields directly — on 64-bit the fields are dead. */
typedef struct aas_entity_s {
    aas_entityinfo_t   i;          /* +0    entity info snapshot           */
#if __SIZEOF_POINTER__ == 4
    aas_link_t        *areas;      /* +124  links into the AAS areas       */
    struct bsp_link_s *leaves;     /* +128  links into the BSP leaves      */
#else
    int                areas;      /* +124  inert; real head in side-band  */
    int                leaves;     /* +128  inert; real head in side-band  */
#endif
} aas_entity_t;                    /* 132 bytes */

_Static_assert(sizeof(aas_entityinfo_t)                  == 124, "aas_entityinfo_t size");
_Static_assert(sizeof(aas_entity_t)                      == 132, "aas_entity_t size");
_Static_assert(offsetof(aas_entityinfo_t, number)        == 12,  "ent.number");
_Static_assert(offsetof(aas_entityinfo_t, origin)        == 16,  "ent.origin");
_Static_assert(offsetof(aas_entityinfo_t, lastvisorigin) == 52,  "ent.lastvisorigin");
_Static_assert(offsetof(aas_entityinfo_t, mins)          == 64,  "ent.mins");
_Static_assert(offsetof(aas_entityinfo_t, solid)         == 88,  "ent.solid");
_Static_assert(offsetof(aas_entityinfo_t, modelindex)    == 92,  "ent.modelindex");
_Static_assert(offsetof(aas_entityinfo_t, renderfx)      == 120, "ent.renderfx");

typedef struct aas_routingcache_s {
    /* Faithful Gladiator 32-bit layout (44-byte header + trailing
     * unsigned short traveltimes[numareas]).  Field offsets on 32-bit
     * match the original disassembly:
     *   +0  time, +4 cluster, +8 areanum, +12..+20 origin,
     *   +24 starttraveltime, +28 travelflags, +32 prev, +36 next.
     * On 64-bit the pointer fields grow, header becomes 56 bytes; the
     * trailing traveltimes are accessed via (cache + 1) instead of
     * (cache + 0x28), and AAS_AllocRoutingCache uses sizeof() so the
     * trailing array is correctly placed for either word size. */
    float time;                          /* +0    last-used timestamp */
    int   cluster;                       /* +4    source cluster      */
    int   areanum;                       /* +8    source area in cluster */
    float origin[3];                     /* +12   area->center        */
    float starttraveltime;               /* +24   typically 1.0f      */
    int   travelflags;                   /* +28   chain selector key  */
    struct aas_routingcache_s *prev;     /* +32   per-area chain link */
    struct aas_routingcache_s *next;     /* +36   per-area chain link */
    /* unsigned short traveltimes[numareas]  follows immediately after */
} aas_routingcache_t;

/* bsp_pointlight_t — Gladiator's Q2-specific dynamic point-light cache
 * entry used by BotAddPointLight / AAS_BSPTraceLight.  Q3 stubs the whole
 * subsystem (be_aas_bspq3.c returns 0 from AAS_BSPTraceLight); the live
 * list head is aasworld.newestcache, the free pool is aasworld.oldestcache.
 *
 * Verified offsets in BotAddPointLight (0x1000D550) and AAS_BSPTraceLight
 * (0x1000D5F0) on the 32-bit binary:
 *   +0..+8   origin xyz       (BotAddPointLight writes from vec3 param)
 *   +12      ent              (entity number that owns the light)
 *   +16..+24 color rgb        (intensity per channel)
 *   +28      radius           (used as r² in distance test in TraceLight)
 *   +32      endtime          (sub_1000D4E0 prunes when endtime < now)
 *   +36      starttime        (AAS_Time() at insert; record-keeping)
 *   +40      decay            (set but not read in the 5 consumers;
 *                              presumably consulted by engine-side code)
 *   +44      next             (live-list / free-list forward link)
 *   +48      prev             (live-list back link; head's prev == NULL)
 * Size = 52 B on 32-bit; on 64-bit the two trailing pointers grow.
 *
 * NB: in stock Gladiator the engine never calls Export_BotAddPointLight
 * in deathmatch — the free pool is never seeded, so sub_1000D450 emits
 * "Warning: empty list" on the first allocation attempt.  The subsystem
 * is effectively dormant at runtime; restoring it is fidelity work. */
typedef struct bsp_pointlight_s {
    float origin[3];                     /* +0   */
    int   ent;                           /* +12  */
    float color[3];                      /* +16  */
    float radius;                        /* +28  */
    float endtime;                       /* +32  */
    float starttime;                     /* +36  */
    float decay;                         /* +40  */
    struct bsp_pointlight_s *next;       /* +44  */
    struct bsp_pointlight_s *prev;       /* +48  */
} bsp_pointlight_t;

/* aas_routingupdate_t and aas_reversedreach_t are defined in
 * gladiator.dll.h with their full pointer-typed restored layouts. */

typedef struct {
    int   numreachabilityareas;
    int   firstcluster;
    int  *clusters;
} aas_reachabilityareas_t;

/* -----------------------------------------------------------------------
 * Historical note: the original Q3 source kept a single global `aasworld`
 * of type `aas_t` (be_aas_def.h).  In Gladiator-Q2 the same fields were
 * linked as individually-named globals (aasworld_numareas, aasworld_planes,
 * ...).  The active reconstruction collects them back into one struct
 * `aas_world_t` defined in gladiator.dll.h (with a _Static_assert gate on
 * the original 32-bit binary layout).  No separate `aas_t` typedef is
 * defined here — the per-field mapping comments scattered above (e.g.
 * "aasworld_numareas") double as documentation.
 * --------------------------------------------------------------------- */

#endif /* AAS_WORLD_H */
