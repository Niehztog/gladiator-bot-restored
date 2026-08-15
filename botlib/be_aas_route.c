/*
 * be_aas_route.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10018D00..0x1001A610; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * Its own interface is in the matching .h; botlib_local.h, which that header
 * pulls in, carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs, the side-band scheme and the externs for
 * botlib.c's remaining globals).
 */

#include "botlib_port.h"
#include "l_libvar.h"      /* libvar_t: 24-byte botlib cvar (reconstructed) */
#undef VectorNegate
#include "be_ea.h"    /* ea_state_t: 36-byte per-client EA struct (reconstructed) */
#include "q2files.h"       /* Q2 BSP file format (+ the BSP header) */
#include "aasfile.h"       /* AAS file format: aas_lump_t, aas_header_t */
#include "be_aas_def.h"   /* aas_t, aas_area_t etc. (reconstructed from aasworld_* globals) */
#include "l_script.h"      /* token_t, script_t, punctuation_t */
#include "l_precomp.h"     /* source_t, define_t */
#include "l_struct.h"      /* structdef_t */
#include "l_utils.h"       /* bot_fileref_t + the Win32 UnZip declarations */
#include "be_ai_def.h"     /* the bot-AI structures and interfaces */
#include "be_interface.h"   /* botimport / botstate / bot_exports + libvar aliases */
#include "struct_sizes_asserts.h" /* compile-time struct-layout guard */
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

//----- (10018D00) --------------------------------------------------------
/* AAS_InitTravelFlagFromType — fill the travel-type -> travel-flag table
 * (aasworld.travelflagfortype, 32 entries).  Entries [1..14] map TRAVEL_* to
 * TFL_* one bit per type, in order, EXCEPT that index 7 (WALKOFFLEDGE) is
 * 0x80, skipping bit 6.  Indices [0] and [15..31] stay zero.  Q3's
 * be_aas_main.c does the same. */
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
//----- (10018DC0) --------------------------------------------------------
int __cdecl AAS_TravelFlagForType(int traveltype)
{
  if ( traveltype < 0 || traveltype >= 32 )
    return 0;
  else
    return aasworld.travelflagfortype[traveltype];
}
//----- (10018DF0) --------------------------------------------------------
/* One contiguous blob of `numareas` reversed-reach heads followed by
 * `reachabilitysize` link nodes, as in the original, but typed as
 * aas_reversedreach_t[] + aas_reversedlink_t[] so the inline `next` slots are
 * the right width on both ABIs. */
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
//----- (10018F50) --------------------------------------------------------
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
//----- (10019010) --------------------------------------------------------
/* One contiguous blob, areatraveltimes[area][reachidx][linkidx] -> unsigned
 * short, built with a single advancing pointer as in the original — keep that
 * shape, while the allocation still scales with pointer width. */
void AAS_CalculateAreaTravelTimes(void)
{
  int i, l, n, size;
  int *numreachptr;
  char *ptr;
  vec3_t end;
  aas_reversedreach_t *revreach;
  aas_reversedlink_t *revlink;
  aas_reachability_t *reach;
  aas_areasettings_t *settings;

  Sys_MilliSeconds();
  if ( aasworld.areatraveltimes )
    FreeMemory(aasworld.areatraveltimes);

  size = aasworld.numareas * (int)sizeof(unsigned short **);
  if ( aasworld.numareas > 0 )
  {
    revreach = aasworld.reversedreachability;
    numreachptr = &((aas_areasettings_t *)aasworld.areasettings)->numreachableareas;
    i = aasworld.numareas;
    do
    {
      l = *numreachptr;
      size += l * (int)sizeof(unsigned short *);
      if ( l > 0 )
        size += l * revreach->numlinks * (int)sizeof(unsigned short);
      ++revreach;
      numreachptr += 7;
      --i;
    }
    while ( i );
  }

  ptr = (char *)GetClearedMemory(size);
  aasworld.areatraveltimes = (unsigned short ***)ptr;
  ptr += aasworld.numareas * sizeof(unsigned short **);
  for ( i = 0; i < aasworld.numareas; ++i )
  {
    revreach = &aasworld.reversedreachability[i];
    settings = &((aas_areasettings_t *)aasworld.areasettings)[i];
    aasworld.areatraveltimes[i] = (unsigned short **)ptr;
    ptr += settings->numreachableareas * sizeof(unsigned short *);
    for ( l = 0; l < settings->numreachableareas; ++l )
    {
      aasworld.areatraveltimes[i][l] = (unsigned short *)ptr;
      ptr += revreach->numlinks * sizeof(unsigned short);
      reach = &((aas_reachability_t *)aasworld.reachability)[settings->firstreachablearea + l];
      for ( n = 0, revlink = revreach->first; revlink; revlink = revlink->next, ++n )
      {
        VectorCopy(((aas_reachability_t *)aasworld.reachability)[revlink->linknum].end, end);
        aasworld.areatraveltimes[i][l][n] = AAS_AreaTravelTime(i, reach->start, end);
      }
    }
  }
}
//----- (10019230) --------------------------------------------------------
aas_routingcache_t *__cdecl AAS_AllocRoutingCache(int numtraveltimes)
{
  /* The original allocates `2 * numareas + 44`, where 44 is the 32-bit
   * sizeof(aas_routingcache_t); using sizeof() keeps the trailing
   * traveltimes[] array (reached via &cache[1]) correct on both ABIs. */
  return (aas_routingcache_t *)GetClearedMemory(2 * numtraveltimes + (int)sizeof(aas_routingcache_t) + 4);
}
//----- (10019260) --------------------------------------------------------
void __cdecl AAS_FreeRoutingCache(void *cache)
{
  FreeMemory(cache);
}
//----- (10019280) --------------------------------------------------------
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
//----- (10019350) --------------------------------------------------------
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
//----- (100193E0) --------------------------------------------------------
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
//----- (10019470) --------------------------------------------------------
void AAS_InitPortalCache(void)
{
  /* One pointer slot per area; sized with sizeof(), not the original's 4. */
  aasworld.portalcache = (aas_routingcache_t **)GetClearedMemory(
      aasworld.numareas * (int)sizeof(aas_routingcache_t *));
}
//----- (100194A0) --------------------------------------------------------
void AAS_InitRoutingUpdate(void)
{
  /* sizeof(), not the original's 40, so each slot grows with the typed
   * struct. */
  if ( aasworld.areaupdate )
    FreeMemory((int)(intptr_t)aasworld.areaupdate);
  aasworld.areaupdate = (aas_routingupdate_t *)GetClearedMemory(sizeof(aas_routingupdate_t) * aasworld.numareas);
  if ( aasworld.portalupdate )
    FreeMemory((int)(intptr_t)aasworld.portalupdate);
  aasworld.portalupdate = (aas_routingupdate_t *)GetClearedMemory(sizeof(aas_routingupdate_t) * aasworld.numareas);
}
//----- (10019520) --------------------------------------------------------
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
//----- (10019550) --------------------------------------------------------
/* AAS_FreeRoutingCaches — drop both routing caches: the cluster-area cache,
 * then the portal cache.  Neither call site (BotLibLoadMap's re-init path,
 * AAS_Shutdown) uses the return value, and AAS_FreeAllPortalCache itself
 * never returns anything meaningful (falls off the end — see its own
 * comment), so this is void, not int; threading a fake "result" through
 * costs an extra spill/reload the real .so never has. */
void AAS_FreeRoutingCaches(void)
{
  AAS_FreeAllClusterAreaCache();
  AAS_FreeAllPortalCache();
}
//----- (10019570) --------------------------------------------------------
/* sub_10019570 — walk both routing caches and free every aas_routingcache_t
 * whose .time is older than AAS_Time() - 15.0 s.  Nearly the same instruction
 * stream as Q3's equivalent in be_aas_route.c.
 *
 * numclusters/numareas are re-fetched inside the loop tail (the compiler
 * cannot prove FreeMemory does not alias them) — preserved.
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
//----- (10019700) --------------------------------------------------------
/* The routing-update FIFO is walked through typed aas_routingupdate_t fields
 * rather than the original's byte arithmetic on int-typed globals. */
void __cdecl AAS_UpdateAreaRoutingCache(aas_routingcache_t *areacache)
{
  /* 64-bit fix: was `int a1_` cast to pointer — truncated on aarch64. */
  int                  travelmask;       /* v26 */
  aas_routingupdate_t *cur;              /* v25 */
  aas_routingupdate_t *tail;             /* v21 */
  aas_routingupdate_t *head;             /* v24 */
  aas_routingupdate_t *upd;
  aas_reversedlink_t  *link;             /* v22 */
  int                  linkidx;          /* v23 / 2 */
  int                  linknum;
  aas_reachability_t  *reach;
  int                  destcluster;      /* v3, v16 */
  int                  destclusterareanum;
  int                  srcareanum;       /* v14 */
  int                  contents, presencemask;
  unsigned short       newtt;            /* v17 */
  unsigned short       oldtt;            /* v19 */

  ++numareacacheupdates;
  ++aasworld.frameroutingupdates;
  memset((void *)aasworld.areaupdate, 0, sizeof(aas_routingupdate_t) * aasworld.numareas);

  travelmask        = ~areacache->travelflags;

  cur              = &aasworld.areaupdate[areacache->areanum];
  cur->areanum         = areacache->areanum;
  cur->areatraveltimes = aasworld.areatraveltimes[areacache->areanum][0];
  cur->tmptraveltime   = (unsigned short)(__int64)areacache->starttraveltime;

  destcluster = aasworld.areasettings[areacache->areanum].cluster;
  if ( destcluster > 0 )
  {
    destclusterareanum = aasworld.areasettings[areacache->areanum].clusterareanum;
  }
  else
  {
    destclusterareanum = ((aas_portal_t *)aasworld.portals)[-destcluster].clusterareanum
                            [((aas_portal_t *)aasworld.portals)[-destcluster].frontcluster != areacache->cluster];
  }
  ((unsigned short *)(areacache + 1))[destclusterareanum] = (unsigned short)(__int64)areacache->starttraveltime;

  cur->next = NULL;
  cur->prev = NULL;
  head      = cur;
  tail      = cur;

  while ( head )
  {
    cur  = head;
    if ( cur->next )
      cur->next->prev = NULL;
    else
      tail = NULL;
    head = cur->next;
    cur->inlist = 0;

    link = aasworld.reversedreachability[cur->areanum].first;
    linkidx = 0;
    while ( link )
    {
      linknum = link->linknum;
      reach = &aasworld.reachability[linknum];
      if ( (travelmask & aasworld.travelflagfortype[reach->traveltype]) == 0 )
      {
        contents = aasworld.areasettings[reach->areanum].contents;
        if ( contents & 1 )      presencemask = 0x10000;
        else if ( contents & 4 ) presencemask = 0x20000;
        else if ( contents & 2 ) presencemask = 0x40000;
        else                     presencemask = 0x8000;
        if ( (presencemask & travelmask) == 0 )
        {
          srcareanum  = link->areanum;
          destcluster = aasworld.areasettings[srcareanum].cluster;
          if ( destcluster <= 0 || destcluster == areacache->cluster )
          {
            newtt = cur->tmptraveltime
                  + reach->traveltime
                  + cur->areatraveltimes[linkidx];
            if ( destcluster > 0 )
            {
              destclusterareanum = aasworld.areasettings[srcareanum].clusterareanum;
            }
            else
            {
              destclusterareanum = ((aas_portal_t *)aasworld.portals)[-destcluster].clusterareanum
                                      [((aas_portal_t *)aasworld.portals)[-destcluster].frontcluster != areacache->cluster];
            }
            oldtt = ((unsigned short *)(areacache + 1))[destclusterareanum];
            if ( !oldtt || oldtt > newtt )
            {
              ((unsigned short *)(areacache + 1))[destclusterareanum] = newtt;
              upd = &aasworld.areaupdate[srcareanum];
              upd->areanum         = srcareanum;
              upd->tmptraveltime   = newtt;
              upd->areatraveltimes = aasworld.areatraveltimes[srcareanum]
                                       [linknum - aasworld.areasettings[srcareanum].firstreachablearea];
              if ( !upd->inlist )
              {
                upd->next = NULL;
                upd->prev = tail;
                if ( tail )
                  tail->next = upd;
                else
                  head = upd;
                tail = upd;
                upd->inlist = 1;
              }
            }
          }
        }
      }
      link = link->next;
      ++linkidx;
    }
  }
}
#ifndef _WIN32
/* F508 @ 0x0002639c, 100 bytes.  Q3 botlib be_aas_route.c has this verbatim.
 * The disassembly indexes areasettings by 28 (its sizeof) and portals by 20,
 * takes `clusterareanum` at +0x10 when cluster > 0, and otherwise selects
 * clusterareanum[frontcluster != cluster] out of the portal at -cluster.
 *
 * Positioned here, ahead of its only in-TU caller AAS_GetAreaRoutingCache
 * below, rather than with the rest of the .so-only block further down:
 * gladi386.so's AAS_GetAreaRoutingCache has no `call` to this address at
 * all -- the body is folded in -- and gcc -O6 only auto-inlines a same-TU
 * callee whose definition it has already parsed (see BotShutdownDeathmatchAI
 * in be_ai2_dmq2.c for the same mechanism used deliberately in reverse). */
int __cdecl AAS_ClusterAreaNum(int cluster, int areanum)
{
  int side, areacluster;

  areacluster = aasworld.areasettings[areanum].cluster;
  if ( areacluster > 0 )
    return aasworld.areasettings[areanum].clusterareanum;
  side = aasworld.portals[-areacluster].frontcluster != cluster;
  return aasworld.portals[-areacluster].clusterareanum[side];
} //end of the function AAS_ClusterAreaNum
#endif /* !_WIN32 -- gladi386.so-only, see the block further down for why */

//----- (10019A90) --------------------------------------------------------
aas_routingcache_t *__cdecl AAS_GetAreaRoutingCache(int clusternum, int areanum, int travelflags)
{
  /* The per-area chain head is aasworld.clusterareacache[cluster][areaInCluster];
   * prev/next go through typed fields, not the original's +0x20/+0x24 byte
   * accesses.
   *
   * The clusterareanum computation itself is the same two-week source drift
   * documented for AAS_Trace/RotatePoint in be_aas_bspq2.c: the Jul 18 DLL
   * predates the AAS_ClusterAreaNum split above (confirmed byte-identical,
   * 89/89, with the logic written out here), while the Aug 2 .so already
   * calls the factored-out helper (confirmed by the total absence of a
   * `mov`/`test`/`jle` sequence here in gladi386.so -- just a single merge
   * point storing whatever AAS_ClusterAreaNum's two return paths left in
   * eax, which is exactly what an inlined two-return-statement callee
   * collapses to). */
#ifdef _WIN32
  int areacluster; // eax
#endif
  aas_routingcache_t *clustercache, *cache;
  int clusterareanum; // [esp+18h] [ebp+8h]

#ifdef _WIN32
  areacluster = aasworld.areasettings[areanum].cluster;
  clusterareanum = areacluster > 0
      ? aasworld.areasettings[areanum].clusterareanum
      : aasworld.portals[-areacluster].clusterareanum[aasworld.portals[-areacluster].frontcluster != clusternum];
#else
  clusterareanum = AAS_ClusterAreaNum(clusternum, areanum);
#endif
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
//----- (10019C00) --------------------------------------------------------
void __cdecl AAS_UpdatePortalRoutingCache(aas_routingcache_t *portalcache)
{
  /* Walks aasworld.portalupdate through the typed aas_routingupdate_t FIFO
   * rather than the original's 40-byte byte arithmetic. */
  aas_routingupdate_t *cur, *head, *tail, *upd;
  aas_routingcache_t *entry;
  int clusternum, v7, portalnum, v11, v14, clusterareanum, v20;
  unsigned short t, v17, v18;
  aas_cluster_t *clust;
  int i;

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
//----- (10019EB0) --------------------------------------------------------
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
// 10019ED7: conditional instruction was optimized away because esi.4!=0
//----- (10019FA0) --------------------------------------------------------
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
  aas_routingcache_t *v21; // 64-bit fix (was int)
  int v22; // ecx
  int v23; // edx
  __int16 t; // cx
  unsigned __int16 v25; // si
  aas_cluster_t *v26; // [esp+10h] [ebp-8h]
  int v27; // [esp+14h] [ebp-4h]
  aas_routingcache_t *v28; // 64-bit fix (was int)
  unsigned __int16 besttime; // [esp+20h] [ebp+8h]

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
//----- (1001A2E0) --------------------------------------------------------
/* AAS_ReachabilityFromNum — reachability record `num`, returned BY VALUE
 * through MSVC's hidden-retbuf ABI (as AAS_EntityInfo does): the record itself
 * on the valid path, a zeroed local otherwise.  Q3 instead takes an output
 * pointer. */
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
//----- (1001A370) --------------------------------------------------------
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
//----- (1001A410) --------------------------------------------------------
int __cdecl AAS_RandomGoalArea(int areanum, int travelflags, _DWORD *goalareanum, vec3_t goalorigin)
{
  int n; // ebx
  int v8; // eax
  unsigned __int16 t; // travel time, tested as unsigned -> jbe (was &&-folded -> je)
  int i; // [esp+24h] [ebp-64h]
  vec3_t center; // [esp+28h] [ebp-60h] BYREF — area center, passed to AAS_PointAreaNum/AAS_TraceClientBBox
  vec3_t end; // [esp+34h] [ebp-54h] BYREF
  aas_trace_t trace; // [esp+40h] [ebp-48h] (was int v17[9] + char v18[36] hidden return buffer)

  n = (int)(aasworld.numareas * ((float)(rand() & 0x7FFF) * 0.000030518509f));
  i = 0;
  if ( aasworld.numareas <= 0 )
    goto fail;
  while ( 1 )
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
          break;
      }
    }
    ++n;
    if ( ++i >= aasworld.numareas )
      goto fail;
  }
  v8 = AAS_PointAreaNum(trace.endpos);
  *goalareanum = v8;
  VectorCopy(trace.endpos, goalorigin);
  return 1;
fail:
  return 0;
}
//----- (1001A610) --------------------------------------------------------
int AAS_RoutingInfo()
{
  botimport.Print(PRT_MESSAGE, "%d area cache updates\n", numareacacheupdates);
  return botimport.Print(PRT_MESSAGE, "%d portal cache updates\n", numportalcacheupdates);
}

/* ------------------------------------------------------------------------
 * Present in gladi386.so, ABSENT from gladiator.dll.
 *
 * The `.so` is the August 2 1999 build and the DLL this reconstruction is
 * derived from is July 18, so the Linux image carries functions the Windows
 * one does not.  They are gated rather than added unconditionally, because
 * the DLL is the canonical byte-match target and code it does not contain
 * must not appear in it.  Drop the gate for any of these that later turns up
 * in the DLL.
 *
 * AAS_ClusterAreaNum (F508) is ALSO one of these, but lives above, ahead of
 * AAS_GetAreaRoutingCache -- see the comment there for why its position
 * matters and this block cannot just be moved back in one piece.
 * ------------------------------------------------------------------------ */
#ifndef _WIN32

/* F511 @ 0x0002651c, 20 bytes -- a bare tail call to AAS_Time().  Q3 botlib
 * has it as `__inline float AAS_RoutingTime(void) { return AAS_Time(); }`;
 * here it is out of line, which is what an `__inline` compiles to under a
 * compiler that does not honour the hint. */
float __cdecl AAS_RoutingTime(void)
{
  return AAS_Time();
} //end of the function AAS_RoutingTime

/* F525 @ 0x0002727c, 57 bytes.  Appends a routing update to the FIFO that
 * AAS_UpdateAreaRoutingCache / AAS_UpdatePortalRoutingCache drain -- Q3 botlib
 * writes this same sequence INLINE at each of its enqueue sites (the
 * `if (!nextupdate->inlist) { ... }` block), so there is no Q3 name to take
 * and none is invented here: the identifier is the symbol gladi386.so ships.
 *
 * The field offsets settle which struct it is.  It guards on +0x1c and writes
 * +0x20 then +0x24, which is aas_routingupdate_t's inlist / next / prev --
 * NOT aas_routingcache_t, whose +0x1c is travelflags.
 *
 *   arg1 = &updateliststart (set only when the list was empty)
 *   arg2 = &updatelistend
 *   arg3 = the update to append
 */
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

/* F526 @ 0x000272b8, 83 bytes.  Q3 botlib's AAS_GetAreaContentsTravelFlags,
 * minus the DONOTENTER / NOTTEAM / BRIDGE clauses Q3 added later.  The four
 * TFL_ values and the three AREACONTENTS_ bits are read straight out of this
 * function: `test al,1 -> 0x10000`, `test al,4 -> 0x20000`, `test al,2 ->
 * 0x40000`, else `0x8000`. */
int __cdecl AAS_GetAreaContentsTravelFlags(int areanum)
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

/* F524 @ 0x000270e8, 403 bytes.  Sweeps every routing cache older than 15
 * seconds and frees it -- the cluster-area caches first, then the per-area
 * portal caches.  Q3 botlib has no counterpart: it evicts one cache at a time
 * under memory pressure (AAS_FreeOldestCache) rather than sweeping on age, so
 * there is no name to take and none is invented; the identifier is the symbol
 * gladi386.so ships.
 *
 * Read out of the disassembly: cluster stride 12 (aas_cluster_t) with numareas
 * at +0, cache prev/next at +0x20/+0x24, the threshold a QWORD 15.0 at
 * .rodata 0x571e0, and the second loop bounded by numareas (+0x168) because
 * portalcache is indexed by area, not by portal.  It frees through FreeMemory
 * directly rather than through a AAS_FreeRoutingCache wrapper. */
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
#endif /* !_WIN32 -- gladi386.so-only */
