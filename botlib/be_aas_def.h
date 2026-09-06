/*
 * be_aas_def.h — AAS (Area Awareness System) internal definitions: the world
 * structures the be_aas_*.c files and be_interface.c are built on.  Mr. Elusive's
 * own name for this header; Q3 botlib ships be_aas_def.h holding exactly this
 * subject matter.
 *
 * The original DLL links the fields of Q3's single `aasworld` as individually-named
 * globals (`aasworld_*`); the reconstruction collects them into `aas_world_t`.
 */

#ifndef AAS_WORLD_H
#define AAS_WORLD_H

/* The AAS internal structures and the aasworld instance, from the
 * IDA-emitted gladiator.dll.h.  This is what be_aas_def.h is for. */
/* aas_soundpool_t — node of the AAS active-sound pool, a packed array of 52-byte
 * nodes (`52 * MAX_AAS_SOUNDS`) with prev/next at +44/+48.
 *
 * The subsystem is DEAD: nothing reaches the writer (sub_1001CE20), so entnum/channel
 * are named from field position and game.h's positioned_sound() argument order, not
 * from a live call site.  attenuation (+40) is confirmed float by the disasm.
 *
 * Six aas_world fields reference these nodes:
 *   d_100669C4 = pool base (FreeMemory'd in sub_1001CAB0)
 *   d_100669C8 = free-list head, chained via .next
 *   d_100669CC/D0 = head/tail of the endtime-sorted active list
 *   d_100669D4/D8 = head/tail of the starttime-sorted active list */
typedef struct aas_soundpool_s {
    /* The 1999 Linux build was gcc 2.7.2.3, which has no anonymous struct/union
     * members, so the original cannot have had a `union { char data[44]; struct
     * {...}; }` overlay.  The fields are the record. */
    float  starttime;   /* +0  AAS_Time() + requested delay */
    float  endtime;     /* +4  starttime + sound duration; sort key */
    vec3_t origin;      /* +8  sound emission origin */
    int    _reserved20; /* +20 always written 0 */
    int    entnum;      /* +24 */
    int    channel;     /* +28 */
    int    soundindex;  /* +32 indexes aasworld.d_100669C0[] */
    float  volume;      /* +36 read as a float multiplier by
                          * sub_1001D0A0's audibility formula */
    float  attenuation; /* +40 matches game.h's positioned_sound(origin, ent, channel,
                          * soundindex, volume, attenuation, timeofs) argument order;
                          * never read back (dead field) */
    struct aas_soundpool_s    *prev;     /* +44 on 32-bit, +48 on 64-bit */
    struct aas_soundpool_s    *next;
} aas_soundpool_t;

/* bot_import_t, bot_export_t — defined in game/botlib.h (properly typed).
 * Include game/botlib.h before this header to get these definitions. */

/* aas_world_t — Area Awareness System global state: one contiguous 676-byte (0x2A4)
 * BSS struct at VA 0x100667E0.  It must stay one aggregate — as separate globals,
 * AAS_Shutdown's `memset(&aasworld, 0, 0x2A4u)` would corrupt whatever the linker
 * placed next.  Each field carries its binary VA; the legacy `aasworld_*` names are
 * #defines in the .c file. */
/* Tag only -- the typedef lives at the definition further down.  C89 has no
 * repeatable typedef, and gcc 2.7.2.3 (the 1999 compiler) rejects one. */
struct aas_entity_s;

/* Routing structures whose pointer slots the decompiler typed as plain `int`.  Real
 * pointers here, with the stride taken from sizeof() at the use sites rather than the
 * 32-bit binary's hard-coded 40 / 8 / 12. */
typedef struct aas_reversedlink_s {
    int                         linknum;     /* +0  reachability index        */
    int                         areanum;     /* +4  source area for the link  */
    struct aas_reversedlink_s  *next;        /* +8  next link for this area   */
} aas_reversedlink_t;

typedef struct {
    int                         numlinks;    /* +0  links into this area      */
    aas_reversedlink_t         *first;       /* +4  head of link chain        */
} aas_reversedreach_t;

typedef struct aas_routingupdate_s {
    int                            cluster;          /* +0  origin cluster (portal updates) */
    int                            areanum;          /* +4  area being updated     */
    float                          start[3];         /* +8  origin                 */
    unsigned short                 tmptraveltime;    /* +20 best traveltime so far */
    unsigned short                 pad22;            /* +22 alignment              */
    unsigned short                *areatraveltimes;  /* +24 inner traveltime row   */
    int                            inlist;           /* +28 1 = queued             */
    struct aas_routingupdate_s    *next;             /* +32 fifo next              */
    struct aas_routingupdate_s    *prev;             /* +36 fifo prev              */
} aas_routingupdate_t;
/* ---- AAS file-format element types -------------------------------------
 * Ahead of aas_world_t because its arrays are typed as element pointers and C89 cannot
 * forward-typedef.  Q3 botlib reaches the same layout by putting them in aasfile.h and
 * including it first. */
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

/* Free-list node used by AAS_SetupReachabilityHeap / AAS_AllocReachability.  48-byte
 * stride on 32-bit; the allocator uses sizeof() so a widened `next` still sizes the
 * pool correctly.  Payload stays at +0 — consumers cast the popped void* straight to
 * int* / aas_reachability_t*. */
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

/* Movement-prediction result — Gladiator's older 80-byte layout: no 'endarea', and a
 * float at +0x44 where Q3 carries int endcontents.  AAS_ClientMovementPrediction
 * returns it BY VALUE (hidden-pointer ABI). */
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

typedef struct aas_world_s {
    int   loaded;                   /* +0x000  (VA 0x100667E0) */
    int   initialized;              /* +0x004  (VA 0x100667E4) */
    int   savefile;                 /* +0x008  (VA 0x100667E8) */
    float time;                     /* +0x00C  (VA 0x100667EC, was global "aastime")    */
    char  filename[144];            /* +0x010  (VA 0x100667F0) */
    char  mapname[144];             /* +0x0A0  (VA 0x10066880) */
    int   numbboxes;                /* +0x130  (VA 0x10066910) */
    aas_bbox_t *bboxes;              /* +0x134  (VA 0x10066914, was "Buffer")            */
    int   numvertexes;              /* +0x138 */
    vec3_t *vertexes;               /* +0x13C  (float[3] per vertex) */
    int   numplanes;                /* +0x140 */
    aas_plane_t *planes;            /* +0x144 */
    int   numedges;                 /* +0x148 */
    aas_edge_t *edges;              /* +0x14C */
    int   edgeindexsize;            /* +0x150 */
    int  *edgeindex;                /* +0x154  flat int[] (edge numbers per face) */
    int   numfaces;                 /* +0x158 */
    aas_face_t *faces;              /* +0x15C */
    int   faceindexsize;            /* +0x160 */
    int  *faceindex;                /* +0x164  flat int[] (face numbers per area) */
    int   numareas;                 /* +0x168 */
    aas_area_t *areas;              /* +0x16C */
    int   numareasettings;          /* +0x170 */
    aas_areasettings_t *areasettings; /* +0x174 */
    int   reachabilitysize;         /* +0x178 */
    aas_reachability_t *reachability; /* +0x17C */
    int   numnodes;                 /* +0x180 */
    aas_node_t *nodes;              /* +0x184 */
    int   numportals;               /* +0x188  (VA 0x10066968, was "dword_10066968")    */
    aas_portal_t *portals;          /* +0x18C */
    int   portalindexsize;          /* +0x190 */
    int  *portalindex;              /* +0x194  flat int[] (portal numbers per cluster) */
    int   numclusters;              /* +0x198  (VA 0x10066978, was "ArgList")           */
    aas_cluster_t *clusters;        /* +0x19C */
    int   numreachabilityareas;     /* +0x1A0  (VA 0x10066980) */
    float reachabilitytime;         /* +0x1A4  (Q3-equivalent slot; binary leaves 4 B)  */
    struct aas_link_s  *linkheap;   /* +0x1A8  (VA 0x10066988) */
    int   linkheapsize;             /* +0x1AC */
    struct aas_link_s  *freelinks;  /* +0x1B0 */
    struct aas_link_s **arealinkedentities; /* +0x1B4 */
    int   numentities;              /* +0x1B8 */
    int   aas_maxclients;           /* +0x1BC */
    struct aas_entity_s *entities;  /* +0x1C0  (pointer; 32-bit binary stored as int) */
    struct indexlist_s *modelindex_table;  /* +0x1C4  (VA 0x100669A4) */
    struct indexlist_s *soundindex_table;  /* +0x1C8 */
    struct indexlist_s *imageindex_table;  /* +0x1CC */
    int   indexes_loaded;           /* +0x1D0 */
    int   numsoundinfo;             /* +0x1D4  (VA 0x100669B4) sound entry count        */
    struct soundinfo_s *soundinfo;  /* +0x1D8  (VA 0x100669B8) soundinfo_t array (fwd-decl; full def in botlib_structs.h) */
    int    d_100669BC;              /* +0x1DC */
    void **d_100669C0;              /* +0x1E0  per-soundindex pointers into soundinfo[] */
    /* Six pointers maintaining the aas_soundpool_t node pool (see above).  `int` in the
     * original; must be real pointers or 64-bit list walks truncate. */
    struct aas_soundpool_s *d_100669C4;  /* +0x1E4 */
    struct aas_soundpool_s *d_100669C8;  /* +0x1E8 */
    struct aas_soundpool_s *d_100669CC;  /* +0x1EC */
    struct aas_soundpool_s *d_100669D0;  /* +0x1F0 */
    struct aas_soundpool_s *d_100669D4;  /* +0x1F4 */
    struct aas_soundpool_s *d_100669D8;  /* +0x1F8 */
    struct bsp_pointlight_s *pointlightheap; /* +0x1FC (VA 0x100669DC) base of the
                                                  bsp_pointlight_t pool, kept so the buffer
                                                  can be FreeMemory'd — unlike oldestcache,
                                                  this never advances.  Typed rather than the
                                                  decompiler's `int`: sub_1000D340 indexes it
                                                  directly and re-reads it per statement,
                                                  which is what the .so emits. */
    struct bsp_pointlight_s *oldestcache;  /* +0x200  (VA 0x100669E0) — point-light free pool head */
    struct bsp_pointlight_s *newestcache;  /* +0x204                  — point-light live list head */
    int   travelflagfortype[32];    /* +0x208  (VA 0x100669E8, 128 bytes)               */
    aas_routingupdate_t *areaupdate;     /* +0x288  (VA 0x10066A68) */
    aas_routingupdate_t *portalupdate;   /* +0x28C */
    int   frameroutingupdates;      /* +0x290 */
    aas_reversedreach_t *reversedreachability; /* +0x294 */
    unsigned short ***areatraveltimes; /* +0x298 [area][reachidx][2*linkidx] */
    struct aas_routingcache_s ***clusterareacache; /* +0x29C [cluster][areaInCluster] */
    struct aas_routingcache_s **portalcache;       /* +0x2A0 [area]                   */
} aas_world_t;                      /* sizeof == 0x2A4 == 676                           */

/* Offset checks vs the original binary's VA layout.  32-bit only — every
 * offset past the first pointer field shifts on 64-bit. */
#include <stddef.h>
#if __SIZEOF_POINTER__ == 4
_Static_assert(sizeof(aas_world_t) == 0x2A4,                     "aas_world_t size");
_Static_assert(offsetof(aas_world_t, loaded)               == 0x000, "loaded");
_Static_assert(offsetof(aas_world_t, time)                 == 0x00C, "time");
_Static_assert(offsetof(aas_world_t, filename)             == 0x010, "filename");
_Static_assert(offsetof(aas_world_t, mapname)              == 0x0A0, "mapname");
_Static_assert(offsetof(aas_world_t, numbboxes)            == 0x130, "numbboxes");
_Static_assert(offsetof(aas_world_t, numportals)           == 0x188, "numportals");
_Static_assert(offsetof(aas_world_t, numclusters)          == 0x198, "numclusters");
_Static_assert(offsetof(aas_world_t, numreachabilityareas) == 0x1A0, "numreachabilityareas");
_Static_assert(offsetof(aas_world_t, linkheap)             == 0x1A8, "linkheap");
_Static_assert(offsetof(aas_world_t, modelindex_table)     == 0x1C4, "modelindex_table");
_Static_assert(offsetof(aas_world_t, pointlightheap)       == 0x1FC, "pointlightheap");
_Static_assert(offsetof(aas_world_t, oldestcache)          == 0x200, "oldestcache");
_Static_assert(offsetof(aas_world_t, travelflagfortype)    == 0x208, "travelflagfortype");
_Static_assert(offsetof(aas_world_t, areaupdate)           == 0x288, "areaupdate");
_Static_assert(offsetof(aas_world_t, portalcache)          == 0x2A0, "portalcache");
#endif /* __SIZEOF_POINTER__ == 4 */

/* Single instance defined in botlib.c. */
extern aas_world_t aasworld;

typedef float  aas_vertex_t[3];


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

/* aas_entityinfo_t — per-frame snapshot of one engine entity, the "info" half of
 * aas_entity_t.  Filled by AAS_UpdateEntity() from bot_updateentity_t; AAS_EntityInfo()
 * hands back a copy of this block.
 *
 * The Q2 flavour of Q3's aas_entityinfo_t: no player-model fields, instead the Q2
 * entity_state_t members (modelindex2..4, skinnum, effects, renderfx). */
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
} aas_entityinfo_t;        /* sizeof = 124 = 0x7C (same on 32- and 64-bit) */

/* aas_entity_t — element type of aasworld.entities[], 132 bytes.
 *
 * The two link heads are 4-byte pointer slots in the 32-bit original.  On 64-bit they
 * degrade to inert placeholders (keeping sizeof at 132 so the entities[] stride stays
 * ABI-correct) and the real heads live in the side-band arrays
 * aasentity_arealinks[]/aasentity_bsplinks[].  ALWAYS go through
 * AAS_EntAreaLink()/AAS_EntBspLink(), never these fields. */
typedef struct aas_entity_s {
    aas_entityinfo_t   i;          /* +0    entity info snapshot           */
#if __SIZEOF_POINTER__ == 4
    aas_link_t        *areas;      /* +124  links into the AAS areas       */
    struct bsp_link_s *leaves;     /* +128  links into the BSP leaves      */
#else
    int                areas;      /* +124  inert; real head in side-band  */
    int                leaves;     /* +128  inert; real head in side-band  */
#endif
} aas_entity_t;                    /* sizeof = 132 (same on 32- and 64-bit:
                                    * the two link heads are inert int
                                    * placeholders there, real heads in the
                                    * side-band -- see be_aas_def.h above) */

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
    /* 44-byte header on 32-bit + trailing unsigned short traveltimes[numareas].  On
     * 64-bit the header grows to 56; the trailing array is reached via (cache + 1) and
     * AAS_AllocRoutingCache uses sizeof(), so it lands correctly either way. */
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
 * BotAddPointLight / AAS_BSPTraceLight (Q3 stubs the whole subsystem).  Live list head
 * is aasworld.newestcache, free pool aasworld.oldestcache.  52 bytes on 32-bit.
 *
 * Dormant at runtime: the engine never calls Export_BotAddPointLight in deathmatch, so
 * the free pool is never seeded and the first allocation logs "Warning: empty list". */
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


/* 64-bit side-band accessors for aas_entity_t.  PORT-ONLY: no 1999 counterpart.  They
 * live here rather than in botlib_port.h because each wraps one specific structure
 * above.  This one covers the two link-list heads at +124 (areas) and +128 (BSP
 * leaves), keyed by entity index, allocated with aasworld.entities in sub_1000EDC0. */
#if BOTLIB_NEED_SIDEBAND
#define AAS_EntAreaLink(entnum) (aasentity_arealinks[(entnum)])
#define AAS_EntBspLink(entnum)  (aasentity_bsplinks[(entnum)])
#else
/* 32-bit: the link-list heads are real inline pointer members at +124
 * (area chain) and +128 (BSP-leaf chain) of the 132-byte aas_entity_t. */
#define AAS_EntAreaLink(entnum) (aasworld.entities[(entnum)].areas)
#define AAS_EntBspLink(entnum)  (aasworld.entities[(entnum)].leaves)
#endif

#endif /* AAS_WORLD_H */
