/*
 * be_aas_def.h — the AAS internal definitions: the world structures the
 * be_aas_*.c files (and be_interface.c) are built on.  Mr. Elusive's own name
 * for this header — Q3 botlib ships be_aas_def.h holding exactly this subject
 * matter, included by its twelve be_aas_*.c plus be_interface.c.  Was
 * aas_world.h until 2026-08-10.
 */
/*
 * be_aas_def.h — AAS (Area Awareness System) world element types.
 *
 * The original DLL links the fields of Q3's single `aasworld` (aas_t,
 * be_aas_def.h) as individually-named globals (`aasworld_*`); the
 * reconstruction collects them into `aas_world_t` in gladiator.dll.h.
 */

#ifndef AAS_WORLD_H
#define AAS_WORLD_H

typedef float  aas_vertex_t[3];

/* 32-byte presence bbox; layout as Q3 aas_bbox_t (aasfile.h). */
typedef struct aas_bbox_s {
    int   presencetype;  /* +0  dword 0 */
    int   flags;         /* +4  dword 1 */
    float mins[3];       /* +8  dwords 2-4 */
    float maxs[3];       /* +20 dwords 5-7 */
} aas_bbox_t;                           /* 32 bytes  (offset stride = 32) */

typedef struct aas_plane_s { float normal[3]; float dist; int type; }  aas_plane_t;
typedef struct aas_edge_s  { int v[2]; }                               aas_edge_t;
typedef int                                                aas_edgeindex_t;
/* 24-byte face record (stride 24); layout as Q3 aas_face_t. */
typedef struct aas_face_s {
    int planenum;
    int faceflags;
    int numedges;
    int firstedge;
    int frontarea;
    int backarea;
} aas_face_t;
typedef int                                                aas_faceindex_t;

/* 48-byte area record (stride 48); layout as Q3 aas_area_t. */
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
 * 48-byte stride on 32-bit; the allocator uses sizeof() so a widened `next`
 * still sizes the pool correctly.  Payload stays at +0 — consumers cast the
 * popped void* straight to int* / aas_reachability_t*. */
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

/* BSP traversal stack frame used by AAS_TraceClientBBox / AAS_TraceAreas. */
typedef struct aas_tracestack_s {
    float start[3];     /* start point of the piece of line to trace */
    float end[3];       /* end point of the piece of line to trace   */
    int   planenum;     /* last plane used as splitter               */
    int   nodenum;      /* node found after splitting with planenum  */
} aas_tracestack_t;

/* Trace result returned by AAS_TraceClientBBox.  9 dwords (36 bytes). */
typedef struct aas_trace_s {
    int   startsolid;   /* if true, the initial point was in solid */
    float fraction;     /* time completed, 1.0 = didn't hit anything */
    float endpos[3];    /* final position */
    int   ent;          /* entity blocking the trace */
    int   lastarea;     /* last area the trace was in (zero if none) */
    int   area;         /* area blocking the trace (zero if none) */
    int   planenum;     /* number of the plane that was hit */
} aas_trace_t;

/* Movement-prediction result — Gladiator's older 80-byte layout: no
 * 'endarea', and a float at +0x44 where Q3 carries int endcontents.
 * AAS_ClientMovementPrediction returns it BY VALUE (hidden-pointer ABI). */
typedef struct aas_clientmove_s {
    float endpos[3];       /* +0x00 position at the end of movement prediction */
    float velocity[3];     /* +0x0C velocity at the end */
    aas_trace_t trace;     /* +0x18 last trace (36 B) */
    int   presencetype;    /* +0x3C presence type at the end */
    int   stopevent;       /* +0x40 SE_* event(s) that stopped the prediction */
    float endcontents;     /* +0x44 4.0f normally; (float)pointcontents on liquid stop */
    float time;            /* +0x48 time predicted ahead (frames * frametime) */
    int   frames;          /* +0x4C number of frames predicted ahead */
} aas_clientmove_t;        /* 80 bytes */

typedef struct aas_cluster_s {
    int   numareas;              /* +0  stride was 12 (3 ints) in 32-bit binary */
    int   numreachabilityareas;  /* +4  */
    int   firstportal;           /* +8  */
} aas_cluster_t;

/* Working struct for the AAS_Optimize* family.  Typed rather than the
 * decompiler's _DWORD[15] local, whose slots truncate 64-bit pointers. */
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

struct bsp_link_s;   /* full definition in botlib_structs.h */

/* aas_entityinfo_t — per-frame snapshot of one engine entity, the "info" half
 * of aas_entity_t.  Filled by AAS_UpdateEntity() from bot_updateentity_t
 * (game/botlib.h); AAS_EntityInfo() hands back a copy of this block.
 *
 * The Q2 flavour of Q3's aas_entityinfo_t: no player-model fields, instead
 * the Q2 entity_state_t members (modelindex2..4, skinnum, effects, renderfx). */
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

/* aas_entity_t — element type of aasworld.entities[], 132 bytes.
 *
 * The two link heads are 4-byte pointer slots in the 32-bit original.  On
 * 64-bit they degrade to inert placeholders (keeping sizeof at 132 so the
 * entities[] stride stays ABI-correct) and the real heads live in the
 * side-band arrays aasentity_arealinks[]/aasentity_bsplinks[].  ALWAYS go
 * through AAS_EntAreaLink()/AAS_EntBspLink(), never these fields. */
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
    /* 44-byte header on 32-bit + trailing unsigned short
     * traveltimes[numareas].  On 64-bit the header grows to 56; the
     * trailing array is reached via (cache + 1) and AAS_AllocRoutingCache
     * uses sizeof(), so it lands correctly for either word size. */
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

/* bsp_pointlight_t — Q2-specific dynamic point-light cache entry used by
 * BotAddPointLight / AAS_BSPTraceLight (Q3 stubs the whole subsystem).
 * Live list head is aasworld.newestcache, free pool aasworld.oldestcache.
 * 52 bytes on 32-bit.
 *
 * Dormant at runtime: the engine never calls Export_BotAddPointLight in
 * deathmatch, so the free pool is never seeded and the first allocation
 * logs "Warning: empty list". */
typedef struct bsp_pointlight_s {
    float origin[3];                     /* +0   */
    int   ent;                           /* +12  owning entity */
    float color[3];                      /* +16  intensity per channel */
    float radius;                        /* +28  used as r² in the distance test */
    float endtime;                       /* +32  pruned when endtime < now */
    float starttime;                     /* +36  AAS_Time() at insert */
    float decay;                         /* +40  written, never read here */
    struct bsp_pointlight_s *next;       /* +44  live/free list forward link */
    struct bsp_pointlight_s *prev;       /* +48  head's prev == NULL */
} bsp_pointlight_t;

/* aas_routingupdate_t and aas_reversedreach_t are defined in
 * gladiator.dll.h with their full pointer-typed restored layouts. */

typedef struct {
    int   numreachabilityareas;
    int   firstcluster;
    int  *clusters;
} aas_reachabilityareas_t;

#endif /* AAS_WORLD_H */
