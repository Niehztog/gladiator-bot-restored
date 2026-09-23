/*
 * be_aas_route.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10018D00..0x1001A610.
 */

#include "botlib_port.h"
#include "l_libvar.h"
#undef VectorNegate
#include "be_ea.h"
#include "q2files.h"
#include "aasfile.h"
#include "be_aas_def.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "be_ai_def.h"
#include "be_interface.h"
#include "struct_sizes_asserts.h"
#include "be_aas_route.h"
#include "be_aas_bspq2.h"
#include "be_aas_main.h"
#include "be_aas_reach.h"
#include "be_aas_sample.h"
#include "be_interface.h"
#include "l_log.h"
#include "l_memory.h"

int numportalcacheupdates; // weak
int numareacacheupdates; // weak

/* ------------------------------------------------------------------------
 * Present in gladi386.so, ABSENT from gladiator.dll.
 *
 * Two different reasons produce that.  Where Q3 declares the helper `__inline`
 * (AAS_ClusterAreaNum, AAS_GetAreaContentsTravelFlags) the source is the same in
 * both originals: cl.exe /O2 (/Ob1) expands every call and /OPT:REF discards the
 * unreferenced COMDAT, while gcc 2.7.2.3 expands the calls AND emits the external
 * definition.  Those are written as Q3 has them and called normally -- one text
 * reproduces both images.  The rest are gated below as .so-only (the .so is the
 * Aug 2 1999 build, the DLL Jul 18) until they pass the same test.
 *
 * Each sits at the position gladi386.so's address order records for it, because
 * gcc 2.7 auto-inlines only a same-TU callee it has already parsed.
 * ------------------------------------------------------------------------ */
/* F508 @ 0x0002639c, 100 bytes: Q3's `__inline int AAS_ClusterAreaNum`, verbatim.
 * The disassembly indexes areasettings by 28 (its sizeof) and portals by 20, takes
 * `clusterareanum` at +0x10 when cluster > 0, and otherwise selects
 * clusterareanum[frontcluster != cluster] out of the portal at -cluster.
 *
 * FIRST in the TU, which is where gladi386.so's address order puts it (0x2639c,
 * ahead of AAS_InitTravelFlagFromType at 0x26400): gcc inlines only a same-TU
 * callee it has already parsed, and every in-TU caller follows it. */
// gladiator.dll: absent
// gladi386.so:   0002639C..00026400
__inline int __cdecl AAS_ClusterAreaNum(int cluster, int areanum)
{
  int side, areacluster;

  areacluster = aasworld.areasettings[areanum].cluster;
  if ( areacluster > 0 )
    return aasworld.areasettings[areanum].clusterareanum;
  side = aasworld.portals[-areacluster].frontcluster != cluster;
  return aasworld.portals[-areacluster].clusterareanum[side];
} //end of the function AAS_ClusterAreaNum

// gladiator.dll: 10018D00..10018D8D
// gladi386.so:   00026400..000264EF
/* Fill the travel-type -> travel-flag table (aasworld.travelflagfortype, 32
 * entries).  Entries [1..14] map TRAVEL_* to TFL_* one bit per type, in order,
 * EXCEPT that index 7 (WALKOFFLEDGE) is 0x80, skipping bit 6.  Indices [0] and
 * [15..31] stay zero.  Same as Q3's be_aas_main.c. */
void AAS_InitTravelFlagFromType(void)
{
  aasworld.travelflagfortype[1]  = 0x0001;     /* TFL_INVALID         */
  aasworld.travelflagfortype[2]  = 0x0002;     /* TFL_WALK            */
  aasworld.travelflagfortype[3]  = 0x0004;     /* TFL_CROUCH          */
  aasworld.travelflagfortype[4]  = 0x0008;     /* TFL_BARRIERJUMP     */
  aasworld.travelflagfortype[5]  = 0x0010;     /* TFL_JUMP            */
  aasworld.travelflagfortype[6]  = 0x0020;     /* TFL_LADDER          */
  aasworld.travelflagfortype[7]  = 0x0080;     /* TFL_WALKOFFLEDGE    */
  aasworld.travelflagfortype[8]  = 0x0100;     /* TFL_SWIM            */
  aasworld.travelflagfortype[9]  = 0x0200;     /* TFL_WATERJUMP       */
  aasworld.travelflagfortype[10] = 0x0400;     /* TFL_TELEPORT        */
  aasworld.travelflagfortype[11] = 0x0800;     /* TFL_ELEVATOR        */
  aasworld.travelflagfortype[12] = 0x1000;     /* TFL_ROCKETJUMP      */
  aasworld.travelflagfortype[13] = 0x2000;     /* TFL_BFGJUMP         */
  aasworld.travelflagfortype[14] = 0x4000;     /* TFL_GRAPPLEHOOK     */
}

// gladiator.dll: 10018DC0..10018DD8
// gladi386.so:   000264F0..0002651C
int __cdecl AAS_TravelFlagForType(int traveltype)
{
  if ( traveltype < 0 || traveltype >= 32 )
    return 0;
  else
    return aasworld.travelflagfortype[traveltype];
}

#ifndef _WIN32
/* F511 @ 0x0002651c, 20 bytes — a bare tail call to AAS_Time().  Q3 has it as
 * `__inline float AAS_RoutingTime(void) { return AAS_Time(); }`; here it is out of
 * line, which is what `__inline` compiles to under a compiler that ignores the
 * hint. */
// gladiator.dll: absent
// gladi386.so:   0002651C..00026530
float __cdecl AAS_RoutingTime(void)
{
  return AAS_Time();
} //end of the function AAS_RoutingTime

#endif /* !_WIN32 -- gladi386.so-only */

// gladiator.dll: 10018DF0..10018EFC
// gladi386.so:   00026530..000266B3
/* One contiguous blob of `numareas` reversed-reach heads followed by
 * `reachabilitysize` link nodes, as in the original, but typed as
 * aas_reversedreach_t[] + aas_reversedlink_t[] so the inline `next` slots are the
 * right width on both ABIs. */
void AAS_CreateReversedReachability(void)
{
  int i, n;
  aas_reversedlink_t *revlink;
  aas_reachability_t *reach;
  aas_areasettings_t *settings;
  char *ptr;

  if ( aasworld.reversedreachability )
    FreeMemory(aasworld.reversedreachability);
  ptr = (char *)GetClearedMemory(
      sizeof(aas_reversedreach_t) * aasworld.numareas
      + sizeof(aas_reversedlink_t) * aasworld.reachabilitysize);
  aasworld.reversedreachability = (aas_reversedreach_t *)ptr;
  ptr += sizeof(aas_reversedreach_t) * aasworld.numareas;
  for ( i = 1; i < aasworld.numareas; ++i )
  {
    settings = &((aas_areasettings_t *)aasworld.areasettings)[i];
    for ( n = 0; n < settings->numreachableareas; ++n )
    {
      reach = &((aas_reachability_t *)aasworld.reachability)[settings->firstreachablearea + n];
      revlink = (aas_reversedlink_t *)ptr;
      ptr += sizeof(aas_reversedlink_t);
      revlink->areanum = i;
      revlink->linknum = settings->firstreachablearea + n;
      revlink->next = aasworld.reversedreachability[reach->areanum].first;
      aasworld.reversedreachability[reach->areanum].first = revlink;
      ++aasworld.reversedreachability[reach->areanum].numlinks;
    }
  }
}

// gladiator.dll: 10018F50..10018FD3
// gladi386.so:   000266B4..00026760
unsigned short __cdecl AAS_AreaTravelTime(int areanum, float *start, float *end)
{
  int intdist;
  float dist;
  vec3_t dir;

  VectorSubtract(start, end, dir);
  dist = VectorLength(dir);
  if ( AAS_AreaCrouch(areanum) )
    dist = dist * 1.3;
  else if ( AAS_AreaSwim(areanum) )
    dist = dist * 1;
  else
    dist = dist * 0.33;
  intdist = (int)dist;
  if ( intdist <= 0 )
    intdist = 1;
  return intdist;
}

// gladiator.dll: 10019010..100191BF
// gladi386.so:   00026760..00026AC3
/* One contiguous blob, areatraveltimes[area][reachidx][linkidx] -> unsigned short,
 * built with a single advancing pointer as in the original — keep that shape. */
void AAS_CalculateAreaTravelTimes(void)
{
  int i, l, n, size;
  char *ptr;
  vec3_t end;
  aas_reversedreach_t *revreach;
  aas_reversedlink_t *revlink;
  aas_reachability_t *reach;
  aas_areasettings_t *settings;

  Sys_MilliSeconds();
  //if there are still area travel times, free the memory
  if ( aasworld.areatraveltimes ) FreeMemory(aasworld.areatraveltimes);
  //get the total size of all the area travel times
  size = aasworld.numareas * sizeof(unsigned short **);
  for ( i = 0; i < aasworld.numareas; i++ )
  {
    revreach = &aasworld.reversedreachability[i];
    //settings of the area
    settings = &aasworld.areasettings[i];
    //
    size += settings->numreachableareas * sizeof(unsigned short *);
    /* An inner loop, not Q3's single multiply: gcc 2.7 unrolls it 4x in the .so,
     * and cl.exe collapses it into the guarded `n * numlinks * 2` the DLL has. */
    for ( l = 0; l < settings->numreachableareas; l++ )
      size += revreach->numlinks * sizeof(unsigned short);
  }
  //allocate memory for the area travel times
  ptr = (char *) GetClearedMemory(size);
  aasworld.areatraveltimes = (unsigned short ***) ptr;
  ptr += aasworld.numareas * sizeof(unsigned short **);
  //calcluate the travel times for all the areas
  for ( i = 0; i < aasworld.numareas; i++ )
  {
    //reversed reachabilities of this area
    revreach = &aasworld.reversedreachability[i];
    //settings of the area
    settings = &aasworld.areasettings[i];
    //
    aasworld.areatraveltimes[i] = (unsigned short **) ptr;
    ptr += settings->numreachableareas * sizeof(unsigned short *);
    //
    for ( l = 0; l < settings->numreachableareas; l++ )
    {
      aasworld.areatraveltimes[i][l] = (unsigned short *) ptr;
      ptr += revreach->numlinks * sizeof(unsigned short);
      //reachability link
      reach = &aasworld.reachability[settings->firstreachablearea + l];
      //
      for ( n = 0, revlink = revreach->first; revlink; revlink = revlink->next, n++ )
      {
        VectorCopy(aasworld.reachability[revlink->linknum].end, end);
        //
        aasworld.areatraveltimes[i][l][n] = AAS_AreaTravelTime(i, reach->start, end);
      }
    }
  }
}

// gladiator.dll: 10019230..10019242
// gladi386.so:   00026AC4..00026AE7
aas_routingcache_t *__cdecl AAS_AllocRoutingCache(int numtraveltimes)
{
  /* The original allocates `2 * numareas + 44`, where 44 is the 32-bit
   * sizeof(aas_routingcache_t); using sizeof() keeps the trailing traveltimes[]
   * array (reached via &cache[1]) correct on both ABIs. */
  return (aas_routingcache_t *)GetClearedMemory(2 * numtraveltimes + (int)sizeof(aas_routingcache_t) + 4);
}

// gladiator.dll: 10019260..1001926C
// gladi386.so:   00026AE8..00026B04
void __cdecl AAS_FreeRoutingCache(void *cache)
{
  FreeMemory(cache);
}

// gladiator.dll: 10019280..10019320
// gladi386.so:   00026B04..00026BDC
void AAS_FreeAllClusterAreaCache(void)
{
  int i, j;
  aas_routingcache_t *cache, *nextcache;
  aas_cluster_t *cluster;

  if ( !aasworld.clusterareacache )
    return;
  for ( i = 0; i < aasworld.numclusters; i++ )
  {
    cluster = &((aas_cluster_t *)aasworld.clusters)[i];
    for ( j = 0; j < cluster->numareas; j++ )
    {
      for ( cache = aasworld.clusterareacache[i][j]; cache; cache = nextcache )
      {
        nextcache = cache->next;
        AAS_FreeRoutingCache(cache);
      }
      aasworld.clusterareacache[i][j] = NULL;
    }
  }
  FreeMemory(aasworld.clusterareacache);
  aasworld.clusterareacache = NULL;
}

// gladiator.dll: 10019350..100193BC
// gladi386.so:   00026BDC..00026C81
/* One head row of `aas_routingcache_t **` per cluster, followed by
 * per-cluster arrays of `aas_routingcache_t *`, one slot per area. */
void AAS_InitClusterAreaCache(void)
{
  int i, size;
  char *ptr;

  /* The original's shape: one advancing pointer over the contiguous
   * [cluster heads][per-area rows] blob, every step scaled by sizeof. */
  for ( size = 0, i = 0; i < aasworld.numclusters; ++i )
    size += aasworld.clusters[i].numareas;
  ptr = (char *)GetClearedMemory(
      aasworld.numclusters * sizeof(aas_routingcache_t **)
      + size * sizeof(aas_routingcache_t *));
  aasworld.clusterareacache = (aas_routingcache_t ***)ptr;
  ptr += aasworld.numclusters * sizeof(aas_routingcache_t **);
  for ( i = 0; i < aasworld.numclusters; ++i )
  {
    aasworld.clusterareacache[i] = (aas_routingcache_t **)ptr;
    ptr += aasworld.clusters[i].numareas * sizeof(aas_routingcache_t *);
  }
}

// gladiator.dll: 100193E0..10019448
// gladi386.so:   00026C84..00026D19
/* AAS_FreeAllPortalCache — release every per-area portal cache chain, then the
 * top-level array. */
int AAS_FreeAllPortalCache(void)
{
  /* Typed array indexing and a typed `next` field, so the chain walk is
   * pointer-width-correct. */
  int i;
  aas_routingcache_t *entry, *next;

  /* `if (portalcache) { ... }` with no return statement — the original is
   * effectively void, both paths falling into one shared `ret`.  An early
   * `if (!portalcache) return 0;` would invert the branch and add an
   * `xor eax,eax`.  Same idiom as FreeMemory. */
  if ( aasworld.portalcache )
  {
    for ( i = 0; i < aasworld.numareas; ++i )
    {
      for ( entry = aasworld.portalcache[i]; entry; entry = next )
      {
        next = entry->next;
        AAS_FreeRoutingCache(entry);
      }
      aasworld.portalcache[i] = NULL;
    }
    FreeMemory(aasworld.portalcache);
    aasworld.portalcache = NULL;
  }
}

// gladiator.dll: 10019470..1001948B
// gladi386.so:   00026D1C..00026D51
void AAS_InitPortalCache(void)
{
  /* One pointer slot per area; sized with sizeof(), not the original's 4. */
  aasworld.portalcache = (aas_routingcache_t **)GetClearedMemory(
      aasworld.numareas * (int)sizeof(aas_routingcache_t *));
}

// gladiator.dll: 100194A0..100194F7
// gladi386.so:   00026D54..00026DE3
void AAS_InitRoutingUpdate(void)
{
  /* sizeof(), not the original's 40, so each slot grows with the typed
   * struct. */
  if ( aasworld.areaupdate )
    FreeMemory(aasworld.areaupdate);
  aasworld.areaupdate = (aas_routingupdate_t *)GetClearedMemory(sizeof(aas_routingupdate_t) * aasworld.numareas);
  if ( aasworld.portalupdate )
    FreeMemory(aasworld.portalupdate);
  aasworld.portalupdate = (aas_routingupdate_t *)GetClearedMemory(sizeof(aas_routingupdate_t) * aasworld.numareas);
}

// gladiator.dll: 10019520..1001953E
// gladi386.so:   00026DE4..00026F86
void AAS_InitRouting(void)
{
  AAS_InitTravelFlagFromType();
  AAS_InitRoutingUpdate();
  AAS_CreateReversedReachability();
  AAS_InitClusterAreaCache();
  AAS_InitPortalCache();
  AAS_CalculateAreaTravelTimes();
}

// 1000105F: thunk → 0x10018D00 = AAS_InitTravelFlagFromType
// gladiator.dll: 10019550..1001955A
// gladi386.so:   00026F88..000270E5
/* Drop both routing caches: the cluster-area cache, then the portal cache.  Neither
 * call site uses the return value and AAS_FreeAllPortalCache never returns anything
 * meaningful, so this is void, not int; threading a fake "result" through costs an
 * extra spill/reload the .so does not have. */
void AAS_FreeRoutingCaches(void)
{
  AAS_FreeAllClusterAreaCache();
  AAS_FreeAllPortalCache();
}

// gladiator.dll: 10019570..100196AF
// gladi386.so:   absent
/* Walk both routing caches and free every aas_routingcache_t whose .time is older
 * than AAS_Time() - 15.0 s.  Nearly the same instruction stream as Q3's equivalent.
 *
 * numclusters/numareas are re-fetched inside the loop tail (the compiler cannot
 * prove FreeMemory does not alias them) — preserved.
 *
 * DEAD in Gladiator. */
static void sub_10019570(void)
{
  int i, j, numareas_in_cluster;
  aas_routingcache_t *cache, *nextcache, *prev;

  for (i = 0; i < aasworld.numclusters; i++) {
    numareas_in_cluster = aasworld.clusters[i].numareas;
    for (j = 0; j < numareas_in_cluster; j++) {
      cache = aasworld.clusterareacache[i][j];
      while (cache) {
        nextcache = cache->next;
        if (AAS_Time() - 15.0 > cache->time) {
          prev = cache->prev;
          if (prev)
            prev->next = nextcache;
          else
            aasworld.clusterareacache[i][j] = nextcache;
          if (nextcache)
            nextcache->prev = prev;
          AAS_FreeRoutingCache(cache);
        }
        cache = nextcache;
      }
    }
  }

  for (i = 0; i < aasworld.numareas; i++) {
    cache = aasworld.portalcache[i];
    while (cache) {
      nextcache = cache->next;
      if (AAS_Time() - 15.0 > cache->time) {
        prev = cache->prev;
        if (prev)
          prev->next = nextcache;
        else
          aasworld.portalcache[i] = nextcache;
        if (nextcache)
          nextcache->prev = prev;
        AAS_FreeRoutingCache(cache);
      }
      cache = nextcache;
    }
  }
}

#ifndef _WIN32
/* F524 @ 0x000270e8, 403 bytes.  Sweeps every routing cache older than 15 seconds
 * and frees it — cluster-area caches first, then the per-area portal caches.  Q3 has
 * no counterpart (it evicts one cache at a time under memory pressure), so there is
 * no name to take; the identifier is the symbol gladi386.so ships.
 *
 * From the disassembly: cluster stride 12 (aas_cluster_t) with numareas at +0, cache
 * prev/next at +0x20/+0x24, the threshold a QWORD 15.0 at .rodata 0x571e0, and the
 * second loop bounded by numareas because portalcache is indexed by area, not by
 * portal.  It frees through FreeMemory directly, not through a wrapper. */
// gladiator.dll: absent
// gladi386.so:   000270E8..0002727B
void __cdecl F524(void)
{
  int i, j;
  aas_routingcache_t *cache, *nextcache;

  for ( i = 0; i < aasworld.numclusters; i++ )
  {
    for ( j = 0; j < aasworld.clusters[i].numareas; j++ )
    {
      for ( cache = aasworld.clusterareacache[i][j]; cache; cache = nextcache )
      {
        nextcache = cache->next;
        if ( cache->time < AAS_Time() - 15.0 )
        {
          if ( cache->prev )
            cache->prev->next = cache->next;
          else
            aasworld.clusterareacache[i][j] = cache->next;
          if ( cache->next )
            cache->next->prev = cache->prev;
          FreeMemory(cache);
        }
      }
    }
  }
  for ( i = 0; i < aasworld.numareas; i++ )
  {
    for ( cache = aasworld.portalcache[i]; cache; cache = nextcache )
    {
      nextcache = cache->next;
      if ( cache->time < AAS_Time() - 15.0 )
      {
        if ( cache->prev )
          cache->prev->next = cache->next;
        else
          aasworld.portalcache[i] = cache->next;
        if ( cache->next )
          cache->next->prev = cache->prev;
        FreeMemory(cache);
      }
    }
  }
} //end of the function F524

/* F525 @ 0x0002727c, 57 bytes.  Appends a routing update to the FIFO that
 * AAS_UpdateAreaRoutingCache / AAS_UpdatePortalRoutingCache drain.  Q3 writes this
 * same sequence INLINE at each enqueue site, so there is no Q3 name to take; the
 * identifier is the symbol gladi386.so ships.
 *
 * The field offsets settle which struct it is: it guards on +0x1c and writes +0x20
 * then +0x24, which is aas_routingupdate_t's inlist / next / prev — NOT
 * aas_routingcache_t, whose +0x1c is travelflags.
 *
 *   arg1 = &updateliststart (set only when the list was empty)
 *   arg2 = &updatelistend
 *   arg3 = the update to append
 */
// gladiator.dll: absent
// gladi386.so:   0002727C..000272B5
void __cdecl F525(aas_routingupdate_t **updateliststart,
                  aas_routingupdate_t **updatelistend,
                  aas_routingupdate_t *update)
{
  if ( update->inlist )
    return;
  if ( *updatelistend )
    (*updatelistend)->next = update;
  else
    *updateliststart = update;
  update->prev = *updatelistend;
  update->next = NULL;
  *updatelistend = update;
  update->inlist = 1;
} //end of the function F525

#endif /* !_WIN32 -- gladi386.so-only */

/* F526 @ 0x000272b8, 83 bytes.  Q3's AAS_GetAreaContentsTravelFlags, minus the
 * DONOTENTER / NOTTEAM / BRIDGE clauses Q3 added later.  The four TFL_ values and
 * three AREACONTENTS_ bits come straight out of this function: `test al,1 ->
 * 0x10000`, `test al,4 -> 0x20000`, `test al,2 -> 0x40000`, else `0x8000`.
 *
 * Q3 1.32 calls it once, to fill a per-area table; Gladiator calls it straight from
 * AAS_UpdateAreaRoutingCache's inner loop.  It has to be `__inline` here: the DLL has
 * no body for it and expands it in place, and cl.exe /O2 (/Ob1) expands nothing that
 * is not marked inline -- with the keyword both routing loops byte-match. */
// gladiator.dll: absent
// gladi386.so:   000272B8..0002730B
__inline int __cdecl AAS_GetAreaContentsTravelFlags(int areanum)
{
  int contents;

  contents = aasworld.areasettings[areanum].contents;
  if ( contents & AREACONTENTS_WATER )
    return TFL_WATER;
  if ( contents & AREACONTENTS_LAVA )
    return TFL_LAVA;
  if ( contents & AREACONTENTS_SLIME )
    return TFL_SLIME;
  return TFL_AIR;
} //end of the function AAS_GetAreaContentsTravelFlags

// gladiator.dll: 10019700..100199C3
// gladi386.so:   0002730C..000276C3
/* The routing-update FIFO is walked through typed aas_routingupdate_t fields
 * rather than the original's byte arithmetic on int-typed globals. */
void __cdecl AAS_UpdateAreaRoutingCache(aas_routingcache_t *areacache)
{
  int i, nextareanum, cluster, badtravelflags, clusterareanum, linknum;
  unsigned short int t;
  aas_routingupdate_t *updateliststart, *updatelistend, *curupdate, *nextupdate;
  aas_reachability_t *reach;
  aas_reversedreach_t *revreach;
  aas_reversedlink_t *revlink;

  numareacacheupdates++;
  //
  aasworld.frameroutingupdates++;
  //clear the routing update fields
  memset(aasworld.areaupdate, 0, aasworld.numareas * sizeof(aas_routingupdate_t));
  //
  badtravelflags = ~areacache->travelflags;
  //
  curupdate = &aasworld.areaupdate[areacache->areanum];
  curupdate->areanum = areacache->areanum;
  /* areatraveltimes[a][r][l] is "time to cross area a from the end of reachability r
   * to the start of its l'th reversed link".  For the START area there is no entering
   * reachability — the bot is already at areacache->origin — so the row is meaningless;
   * the original picks row 0 and does not check that row 0 exists.  An area with
   * numreachableareas == 0 gets a zero-length row array out of
   * AAS_CalculateAreaTravelTimes, so `[0]` reads one word past the blob, and if that
   * word happens to be 0 the loop below dereferences NULL.
   *
   * That is not hypothetical: it is the 2026-09-19 xatrix SIGSEGV (crash/README.md).
   * On marics102_bspk1 area 2391 is the last area, is the only one with no
   * reachabilities but incoming links, and areatraveltimes[2391] lands exactly on the
   * blob end — so `[0]` read the 0 past it and the first of its 145 reversed links
   * faulted at `movzx eax,WORD PTR [eax]`.
   *
   * Q3 fixed this in be_aas_route.c by giving the start node a zeroed
   * `unsigned short startareatraveltimes[128]` instead, which is also the right
   * answer numerically: the start area costs nothing to cross.  We take the zero but
   * not the array — Q3's fixed 128 is itself indexed by the reversed-link counter, and
   * this very map would over-read it by 17 — so the start row is NULL here and the sum
   * below folds the zero in.  GLAD_SERVERFIX(route-start-areatraveltimes). */
#if GLAD_SERVERFIX /* GLAD_SERVERFIX(route-start-areatraveltimes) */
  curupdate->areatraveltimes = NULL;   /* Q3's zeroed startareatraveltimes[], without the array */
#else
  curupdate->areatraveltimes = aasworld.areatraveltimes[areacache->areanum][0];
#endif
  curupdate->tmptraveltime = areacache->starttraveltime;
  //
  clusterareanum = AAS_ClusterAreaNum(areacache->cluster, areacache->areanum);
  ((unsigned short *)(areacache + 1))[clusterareanum] = areacache->starttraveltime;
  //put the area to start with in the current read list
  curupdate->next = NULL;
  curupdate->prev = NULL;
  updateliststart = curupdate;
  updatelistend = curupdate;
  //while there are updates in the current list
  while ( updateliststart )
  {
    curupdate = updateliststart;
    //
    if ( curupdate->next ) curupdate->next->prev = NULL;
    else updatelistend = NULL;
    updateliststart = curupdate->next;
    //
    curupdate->inlist = 0;
    //check all reversed reachability links
    revreach = &aasworld.reversedreachability[curupdate->areanum];
    //
    for ( i = 0, revlink = revreach->first; revlink; revlink = revlink->next, i++ )
    {
      linknum = revlink->linknum;
      reach = &aasworld.reachability[linknum];
      //if there is used an undesired travel type
      if ( aasworld.travelflagfortype[reach->traveltype] & badtravelflags ) continue;
      //if the next area has a not allowed travel flag
      if ( AAS_GetAreaContentsTravelFlags(reach->areanum) & badtravelflags ) continue;
      //number of the area the reversed reachability leads to
      nextareanum = revlink->areanum;
      //get the cluster number of the area
      cluster = aasworld.areasettings[nextareanum].cluster;
      //don't leave the cluster
      if ( cluster > 0 && cluster != areacache->cluster ) continue;
      //time already travelled plus the traveltime through
      //the current area plus the travel time from the reachability
#if GLAD_SERVERFIX /* GLAD_SERVERFIX(route-start-areatraveltimes) */
      /* NULL is the start node's zero row — see the assignment above. */
      t = curupdate->tmptraveltime +
            (curupdate->areatraveltimes ? curupdate->areatraveltimes[i] : 0) +
              reach->traveltime;
#else
      t = curupdate->tmptraveltime +
            curupdate->areatraveltimes[i] +
              reach->traveltime;
#endif
      //get the number of the area in the cluster
      clusterareanum = AAS_ClusterAreaNum(areacache->cluster, nextareanum);
      //
      if ( !((unsigned short *)(areacache + 1))[clusterareanum] ||
            ((unsigned short *)(areacache + 1))[clusterareanum] > t )
      {
        ((unsigned short *)(areacache + 1))[clusterareanum] = t;
        nextupdate = &aasworld.areaupdate[nextareanum];
        nextupdate->areanum = nextareanum;
        nextupdate->tmptraveltime = t;
        nextupdate->areatraveltimes = aasworld.areatraveltimes[nextareanum][linknum -
                          aasworld.areasettings[nextareanum].firstreachablearea];
        if ( !nextupdate->inlist )
        {
          nextupdate->next = NULL;
          nextupdate->prev = updatelistend;
          if ( updatelistend ) updatelistend->next = nextupdate;
          else updateliststart = nextupdate;
          updatelistend = nextupdate;
          nextupdate->inlist = 1;
        }
      }
    }
  }
}

// gladiator.dll: 10019A90..10019BA7
// gladi386.so:   000276C4..0002781C
aas_routingcache_t *__cdecl AAS_GetAreaRoutingCache(int clusternum, int areanum, int travelflags)
{
  /* The per-area chain head is aasworld.clusterareacache[cluster][areaInCluster];
   * prev/next go through typed fields, not the original's +0x20/+0x24 byte accesses.
   *
   * AAS_ClusterAreaNum is Q3's `__inline` helper in BOTH originals, not a Jul-to-Aug
   * source drift: MSVC6 /O2 (/Ob1) expands `__inline` and /OPT:REF drops the unused
   * COMDAT, so the DLL has no body for it; gcc 2.7.2.3 expands it too but still emits
   * the external copy, which is the .so's F508.  One call reproduces both images. */
  aas_routingcache_t *clustercache, *cache;
  int clusterareanum; // [esp+18h] [ebp+8h]

  clusterareanum = AAS_ClusterAreaNum(clusternum, areanum);
  clustercache = aasworld.clusterareacache[clusternum][clusterareanum];
  cache  = clustercache;
  while ( cache && cache->travelflags != travelflags )
    cache = cache->next;
  if ( !cache )
  {
    cache = AAS_AllocRoutingCache(
        aasworld.clusters[clusternum].numareas);
    cache->cluster        = clusternum;
    cache->areanum        = areanum;
    VectorCopy(aasworld.areas[areanum].center, cache->origin);
    cache->starttraveltime = 1.0f;
    cache->travelflags    = travelflags;
    cache->prev           = NULL;
    cache->next           = clustercache;
    if ( clustercache )
      clustercache->prev = cache;
    aasworld.clusterareacache[clusternum][clusterareanum] = cache;
    AAS_UpdateAreaRoutingCache(cache);
  }
  cache->time = AAS_Time();
  return cache;
}

// gladiator.dll: 10019C00..10019E13
// gladi386.so:   0002781C..00027B15
void __cdecl AAS_UpdatePortalRoutingCache(aas_routingcache_t *portalcache)
{
  aas_routingcache_t *entry;
  int i;
  int clusternum, v7, portalnum, v11, v14, clusterareanum, v20;
  aas_cluster_t *clust;
  unsigned short t, v17, v18;
  /* Walks aasworld.portalupdate through the typed aas_routingupdate_t FIFO
   * rather than the original's 40-byte byte arithmetic. */
  aas_routingupdate_t *cur, *head, *tail, *upd;

  ++numportalcacheupdates;
  memset((void *)aasworld.portalupdate, 0,
         sizeof(aas_routingupdate_t) * aasworld.numareas);

  cur = &aasworld.portalupdate[portalcache->areanum];
  cur->cluster       = portalcache->cluster;
  cur->areanum       = portalcache->areanum;
  cur->tmptraveltime = (unsigned short)(__int64)portalcache->starttraveltime;
  clusternum = ((aas_areasettings_t *)aasworld.areasettings)[portalcache->areanum].cluster;
  if ( clusternum < 0 )
    ((unsigned short *)(portalcache + 1))[-clusternum] = (unsigned short)(__int64)portalcache->starttraveltime;
  cur->next   = NULL;
  cur->prev   = NULL;
  head = cur;
  tail = cur;

  while ( tail )
  {
    cur = tail;
    upd = cur->next;
    if ( upd )
      upd->prev = NULL;
    else
      head = NULL;
    v7 = cur->cluster;
    tail = cur->next;
    cur->inlist = 0;
    clust = &((aas_cluster_t *)aasworld.clusters)[v7];
    entry = AAS_GetAreaRoutingCache(v7, cur->areanum, portalcache->travelflags);

    for ( i = 0; i < clust->numreachabilityareas; ++i )
    {
      portalnum = aasworld.portalindex[clust->firstportal + i];
      v11 = ((aas_portal_t *)aasworld.portals)[portalnum].areanum;
      if ( v11 != cur->areanum )
      {
        v14 = ((aas_areasettings_t *)aasworld.areasettings)[v11].cluster;
        if ( v14 > 0 )
        {
          clusterareanum = ((aas_areasettings_t *)aasworld.areasettings)[v11].clusterareanum;
        }
        else
        {
          clusterareanum = ((aas_portal_t *)aasworld.portals)[-v14].clusterareanum[((aas_portal_t *)aasworld.portals)[-v14].frontcluster != cur->cluster];
        }
        t = ((unsigned short *)(entry + 1))[clusterareanum];
        if ( t )
        {
          v17 = cur->tmptraveltime + t;
          v18 = ((unsigned short *)(portalcache + 1))[portalnum];
          if ( !v18 || v18 > v17 )
          {
            ((unsigned short *)(portalcache + 1))[portalnum] = v17;
            upd = &aasworld.portalupdate[((aas_portal_t *)aasworld.portals)[portalnum].areanum];
            v20 = ((aas_portal_t *)aasworld.portals)[portalnum].frontcluster;
            if ( v20 == cur->cluster )
              v20 = ((aas_portal_t *)aasworld.portals)[portalnum].backcluster;
            upd->cluster = v20;
            upd->areanum = ((aas_portal_t *)aasworld.portals)[portalnum].areanum;
            upd->tmptraveltime = v17;
            if ( !upd->inlist )
            {
              upd->next = NULL;
              upd->prev = head;
              if ( head )
                head->next = upd;
              else
                tail = upd;
              head = upd;
              upd->inlist = 1;
            }
          }
        }
      }
    }
  }
  { (void)(0); return; }
}

// gladiator.dll: 10019EB0..10019F6F
// gladi386.so:   00027B18..00027C09
aas_routingcache_t *__cdecl AAS_GetPortalRoutingCache(int clusternum, int areanum, int travelflags)
{
  /* Per-area portal chain head is aasworld.portalcache[area]. */
  aas_routingcache_t *cache;

  cache = aasworld.portalcache[areanum];
  while ( cache )
  {
    if ( cache->travelflags == travelflags )
      break;
    cache = cache->next;
  }
  if ( !cache )
  {
    cache = AAS_AllocRoutingCache(aasworld.numportals);
    cache->cluster        = clusternum;
    cache->areanum        = areanum;
    VectorCopy(aasworld.areas[areanum].center, cache->origin);
    cache->starttraveltime = 1.0f;
    cache->travelflags    = travelflags;
    cache->prev           = NULL;
    cache->next           = aasworld.portalcache[areanum];
    if ( aasworld.portalcache[areanum] )
      aasworld.portalcache[areanum]->prev = cache;
    aasworld.portalcache[areanum] = cache;
    AAS_UpdatePortalRoutingCache(cache);
  }
  cache->time = AAS_Time();
  return cache;
}

// gladiator.dll: 10019FA0..1001A229
// gladi386.so:   00027C0C..00027F8E
__int16 __cdecl AAS_AreaTravelTimeToGoalArea(int areanum, int a2, int goalareanum)
{
  __int16 result; // ax
  int clusternum; // edi
  int v7; // esi
  int v9; // eax
  aas_portal_t *v10; // ecx
  aas_portal_t *v11; // ecx
  aas_routingcache_t *v12; // 64-bit fix (was int)
  int v13; // edx
  int v14; // ebp
  aas_routingcache_t *portalcache; // 64-bit fix (was int)
  int v16; // edx
  aas_routingcache_t *v17; // 64-bit fix (was int)
  aas_cluster_t *cluster; // ecx
  aas_portal_t *v19; // eax
  int portalnum; // esi
  unsigned __int16 besttime; // [esp+20h] [ebp+8h]
  aas_routingcache_t *v21; // 64-bit fix (was int)
  int v22; // ecx
  int v23; // edx
  __int16 t; // cx
  unsigned __int16 v25; // si
  int v27; // [esp+14h] [ebp-4h]
  aas_routingcache_t *v28; // 64-bit fix (was int)
  aas_cluster_t *v26; // [esp+10h] [ebp-8h]

  if ( !aasworld.initialized )
    return 0;
  if ( areanum == a2 )
    return 1;
  if ( areanum <= 0 || areanum >= aasworld.numareas )
  {
    botimport.Print(PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: areanum %d out of range\n", areanum);
    return 0;
  }
  if ( a2 <= 0 || a2 >= aasworld.numareas )
  {
    botimport.Print(PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: goalareanum %d out of range\n", a2);
    return 0;
  }
  if ( aasworld.frameroutingupdates > 10 )
    return 0;
  clusternum = aasworld.areasettings[areanum].cluster;
  v9 = aasworld.areasettings[a2].cluster;
  v7 = clusternum;
  if ( clusternum < 0 && v9 > 0 )
  {
    v10 = &aasworld.portals[-clusternum];
    if ( v10->frontcluster == v9 || v10->backcluster == v9 )
      v7 = v9;
  }
  else
  {
    if ( clusternum <= 0 )
      goto portalpath;
    if ( v9 < 0 )
    {
      v11 = &aasworld.portals[-v9];
      if ( v11->frontcluster == clusternum || v11->backcluster == clusternum )
        v9 = clusternum;
    }
  }
  if ( v7 > 0 && v9 > 0 && v7 == v9 )
  {
    v12 = AAS_GetAreaRoutingCache(v7, a2, goalareanum);
    clusternum = aasworld.areasettings[areanum].cluster;
    if ( clusternum <= 0 )
    {
      v13 = aasworld.portals[-clusternum].clusterareanum[aasworld.portals[-clusternum].frontcluster != v7];
    }
    else
    {
      v13 = aasworld.areasettings[areanum].clusterareanum;
    }
    /* `(cache + 1)`, not the original's hard-coded `+ 40`: the header grows on
     * 64-bit, where that offset lands inside the `next` pointer. */
    result = ((unsigned short *)(v12 + 1))[v13];
    if ( result )
      return result;
  }
portalpath:
  v14 = aasworld.areasettings[a2].cluster;
  if ( v14 < 0 )
    v14 = aasworld.portals[-v14].frontcluster;
  portalcache = AAS_GetPortalRoutingCache(v14, a2, goalareanum);
  v16 = 0;
  v17 = portalcache;
  v28 = portalcache;
  if ( clusternum < 0 )
    return ((unsigned short *)(portalcache + 1))[-clusternum];   /* 64-bit fix: was `+ 2*(20-v6)` */
  besttime = 0;
  v27 = 0;
  cluster = &aasworld.clusters[clusternum];
  v26 = cluster;
  if ( cluster->numreachabilityareas > 0 )
  {
    v19 = aasworld.portals;
    while ( 1 )
    {
      portalnum = aasworld.portalindex[v16 + cluster->firstportal];
      if ( ((unsigned short *)(v17 + 1))[portalnum] )   /* 64-bit fix: was `+ 2*v20 + 40` */
      {
        v21 = AAS_GetAreaRoutingCache(clusternum, v19[portalnum].areanum, goalareanum);
        v19 = aasworld.portals;
        v22 = aasworld.areasettings[areanum].cluster;
        if ( v22 <= 0 )
        {
          v19 = aasworld.portals;
          v23 = aasworld.portals[-v22].clusterareanum[aasworld.portals[-v22].frontcluster != clusternum];
        }
        else
        {
          v23 = aasworld.areasettings[areanum].clusterareanum;
        }
        t = ((unsigned short *)(v21 + 1))[v23];   /* 64-bit fix: was `+ 2*v23 + 40` */
        if ( t )
        {
          v25 = t + ((unsigned short *)(v28 + 1))[portalnum];   /* 64-bit fix: was `+ 2*v20 + 40` */
          if ( !besttime || v25 < besttime )
            besttime = v25;
        }
        cluster = v26;
      }
      v16 = ++v27;
      if ( v27 >= cluster->numreachabilityareas )
        break;
      v17 = v28;
    }
  }
  return besttime;
}

// gladiator.dll: 1001A2E0..1001A343
// gladi386.so:   00027F90..00028005
/* Reachability record `num`, returned BY VALUE through MSVC's hidden-retbuf ABI (as
 * AAS_EntityInfo does): the record itself on the valid path, a zeroed local
 * otherwise.  Q3 instead takes an output pointer. */
aas_reachability_t __cdecl AAS_ReachabilityFromNum(int num)
{
  aas_reachability_t reach;

  if ( !aasworld.initialized || num < 0 || num > aasworld.reachabilitysize )
  {
    memset(&reach, 0, sizeof(reach));
    return reach;
  }
  return aasworld.reachability[num];
}

// gladiator.dll: 1001A370..1001A3E6
// gladi386.so:   00028008..000280B1
int __cdecl AAS_NextAreaReachability(int areanum, int reachnum)
{
  aas_areasettings_t *settings;

  if ( !aasworld.initialized )
    return 0;
  if ( areanum <= 0 || areanum >= aasworld.numareas )
  {
    botimport.Print(PRT_ERROR, "AAS_NextAreaReachability: areanum %d out of range\n", areanum);
    return 0;
  }
  settings = (aas_areasettings_t *)(&aasworld.areasettings[areanum]);
  if ( !reachnum )
    return settings->firstreachablearea;
  if ( reachnum < settings->firstreachablearea )
  {
    botimport.Print(PRT_FATAL,
             "AAS_NextAreaReachability: reachnum < settings->firstreachableara");
    return 0;
  }
  reachnum++;
  if ( reachnum >= settings->firstreachablearea + settings->numreachableareas )
    return 0;
  return reachnum;
}

// gladiator.dll: 1001A410..1001A595
// gladi386.so:   000280B4..00028295
int __cdecl AAS_RandomGoalArea(int areanum, int travelflags, _DWORD *goalareanum, vec3_t goalorigin)
{
  int n; // ebx
  int v8; // eax
  unsigned __int16 t; // travel time, tested as unsigned -> jbe (was &&-folded -> je)
  int i; // [esp+24h] [ebp-64h]
  vec3_t center; // [esp+28h] [ebp-60h] BYREF — area center, passed to AAS_PointAreaNum/AAS_TraceClientBBox
  vec3_t end; // [esp+34h] [ebp-54h] BYREF
  aas_trace_t trace; // [esp+40h] [ebp-48h] (was int v17[9] + char v18[36] hidden return buffer)

  n = (int)(aasworld.numareas * random());
  /* A counted `for` with the success block INSIDE the loop, and the third clause
   * spelled `++n, ++i` — not `++i, ++n`.  The loop shape is what the ELF wants (the
   * continue block sits cold at the end, falling through into the success path); the
   * increment ORDER is what the PE wants (`inc ebx` for n before `inc ecx` for i). */
  for ( i = 0; i < aasworld.numareas; ++n, ++i )
  {
    if ( n <= 0 )
      n = 1;
    if ( n >= aasworld.numareas )
      n = 1;
    if ( AAS_AreaReachability(n) )
    {
      t = (unsigned __int16)AAS_AreaTravelTimeToGoalArea(areanum, n, travelflags);
      if ( t > 0 )
      {
        VectorCopy(aasworld.areas[n].center, center);
        if ( !AAS_PointAreaNum(center) )
          Log_Write("area %d center %f %f %f in solid?", n, center[0],
                    center[1], center[2]);
        VectorCopy(center, end);
        end[2] = end[2] - 300.0f;
        trace = AAS_TraceClientBBox(center, end, 4, -1);
        if ( !trace.startsolid )
        {
          v8 = AAS_PointAreaNum(trace.endpos);
          *goalareanum = v8;
          VectorCopy(trace.endpos, goalorigin);
          return 1;
        }
      }
    }
  }
  return 0;
}

// gladiator.dll: 1001A610..1001A63B
// gladi386.so:   00028298..000282E0
int AAS_RoutingInfo()
{
  botimport.Print(PRT_MESSAGE, "%d area cache updates\n", numareacacheupdates);
  return botimport.Print(PRT_MESSAGE, "%d portal cache updates\n", numportalcacheupdates);
}

