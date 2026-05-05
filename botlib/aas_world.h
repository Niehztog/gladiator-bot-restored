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

typedef struct { float normal[3]; float dist; int type; }  aas_plane_t;
typedef struct { int v[2]; }                               aas_edge_t;
typedef int                                                aas_edgeindex_t;
typedef struct { int planenum; int areas[2]; int edges[2]; } aas_face_t;
typedef int                                                aas_faceindex_t;

typedef struct {
    float mins[3];
    float maxs[3];
    int   firstface;
    int   numfaces;
} aas_area_t;                           /* 40 bytes  (offset stride = 40) */

typedef struct {
    int contents;
    int areaflags;
    int presencetype;
    int cluster;
    int clusterareanum;
    int firstreachablearea;
    int numreachableareas;
} aas_areasettings_t;                   /* 28 bytes  (stride = 28) */

typedef struct {
    int   areanum;
    float start[3];
    float end[3];
    int   traveltype;
    short traveltime;
} aas_reachability_t;

typedef struct { int planenum; int children[2]; } aas_node_t;
typedef struct { int areanum; int clusterareanum; }        aas_portal_t;
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

typedef struct {
    int   numareas;
    int   numreachabilityareas;
    int   firstportal;
    int   numportals;
} aas_cluster_t;

typedef struct aas_link_s {
    int                entnum;
    int                areanum;
    struct aas_link_s *prev_ent;
    struct aas_link_s *next_ent;
    struct aas_link_s *prev_area;
    struct aas_link_s *next_area;
} aas_link_t;

typedef struct {
    float   origin[3];
    float   angles[3];
    float   mins[3];
    float   maxs[3];
    int     presencetype;
    int     modelindex;
} aas_entity_t;

typedef struct aas_routingcache_s {
    int   type;
    float time;
    int   areanum;
    int   cluster;
    int  *startareaslist;
    int   startareaslistsize;
    unsigned char *reachabilities;
    unsigned short *traveltimes;
    struct aas_routingcache_s *prev;
    struct aas_routingcache_s *next;
} aas_routingcache_t;

typedef struct {
    int   areanum;
    short delay;
} aas_routingupdate_t;

typedef struct {
    int   numreachabilities;
    int  *reachabilities;
} aas_reversedreach_t;

typedef struct {
    int   numreachabilityareas;
    int   firstcluster;
    int  *clusters;
} aas_reachabilityareas_t;

/* -----------------------------------------------------------------------
 * The AAS world state — mirrors the aasworld_* globals in the decompiled
 * file. This header documents the intended layout; the actual code still
 * uses the individual globals.
 * --------------------------------------------------------------------- */
typedef struct {
    /* --- status --- */
    int   loaded;               /* aasworld_loaded              */
    int   initialized;          /* aasworld_initialized         */
    int   savefile;             /* aasworld_savefile            */
    float time;                 /* aastime                      */
    char  filename[144];        /* [Q2-ONLY, larger than Q3]    */
    char  mapname[144];         /* [Q2-ONLY, larger than Q3]    */

    /* --- geometry --- */
    int            numbboxes;           /* aasworld_numbboxes         */
    /* bboxes not present as separate global in Q2 decompilation */
    int            numvertexes;         /* aasworld_numvertexes       */
    aas_vertex_t  *vertexes;            /* aasworld_vertexes          */
    int            numplanes;           /* aasworld_numplanes         */
    aas_plane_t   *planes;              /* aasworld_planes            */
    int            numedges;            /* aasworld_numedges          */
    aas_edge_t    *edges;               /* aasworld_edges             */
    int            edgeindexsize;       /* aasworld_edgeindexsize     */
    aas_edgeindex_t *edgeindex;         /* aasworld_edgeindex         */
    int            numfaces;            /* aasworld_numfaces          */
    aas_face_t    *faces;               /* aasworld_faces             */
    int            faceindexsize;       /* aasworld_faceindexsize     */
    aas_faceindex_t *faceindex;         /* aasworld_faceindex         */
    int            numareas;            /* aasworld_numareas          */
    aas_area_t    *areas;               /* aasworld_areas             */
    int            numareasettings;     /* aasworld_numareasettings   */
    aas_areasettings_t *areasettings;   /* aasworld_areasettings      */
    int            reachabilitysize;    /* aasworld_reachabilitysize  */
    aas_reachability_t *reachability;   /* aasworld_reachability      */
    int            numnodes;            /* aasworld_numnodes          */
    aas_node_t    *nodes;               /* aasworld_nodes             */
    aas_portal_t  *portals;             /* aasworld_portals           */
    int            portalindexsize;     /* aasworld_portalindexsize   */
    aas_portalindex_t *portalindex;     /* aasworld_portalindex       */
    aas_cluster_t *clusters;            /* aasworld_clusters          */

    /* --- entity tracking --- */
    int            numentities;         /* aasworld_numentities       */
    int            maxclients;          /* aasworld_maxclients        */
    aas_entity_t  *entities;            /* aasworld_entities          */
    aas_link_t   **arealinkedentities;  /* aasworld_arealinkedentities*/

    /* --- link heap --- */
    aas_link_t    *linkheap;            /* aasworld_linkheap          */
    int            linkheapsize;        /* aasworld_linkheapsize      */
    aas_link_t    *freelinks;           /* aasworld_freelinks         */

    /* --- routing cache --- */
    aas_routingupdate_t *areaupdate;    /* aasworld_areaupdate        */
    aas_routingupdate_t *portalupdate;  /* aasworld_portalupdate      */
    int            frameroutingupdates; /* aasworld_frameroutingupdates*/
    aas_reversedreach_t *reversedreachability; /* aasworld_reversedreachability */
    unsigned short ***areatraveltimes;  /* aasworld_areatraveltimes   */
    aas_routingcache_t ***clusterareacache; /* aasworld_clusterareacache */
    aas_routingcache_t **portalcache;   /* aasworld_portalcache       */
    aas_routingcache_t *oldestcache;    /* aasworld_oldestcache       */
    aas_routingcache_t *newestcache;    /* aasworld_newestcache       */

    /* --- misc --- */
    int  *travelflagfortype;            /* aasworld_travelflagfortype (array) */
    int   areaupdate_count;             /* aasworld_areaupdate (count variant) */
    int   numframes;                    /* [Q3-ONLY; not present in Q2 globals] */
} aas_t;

/*
 * NOTE: The decompiled code uses the individual aasworld_* globals directly.
 * This header documents what those globals represent. To fully consolidate
 * them into a single aasworld struct, each occurrence of e.g.
 * `aasworld_numareas` would need to become `aasworld.numareas`.
 */

#endif /* AAS_WORLD_H */
