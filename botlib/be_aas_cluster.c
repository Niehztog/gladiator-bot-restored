/*
 * be_aas_cluster.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10008620..0x100096E0; the boundary evidence -- per-object .text and
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
#include "be_aas_cluster.h"
#include "be_aas_bspq2.h"
#include "be_aas_main.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"

//----- (10008620) --------------------------------------------------------
int AAS_RemoveClusterAreas()
{
  int result; // eax
  int i; // ecx

  result = aasworld.numareas;
  i = 1;
  if ( aasworld.numareas > 1 )
  {
    result = 28;
    do
    {
      ++i;
      *(_DWORD *)((char *)aasworld.areasettings + result + 12) = 0;
      result += 28;
    }
    while ( i < aasworld.numareas );
  }
  return result;
}
//----- (10008660) --------------------------------------------------------
int __cdecl AAS_UpdatePortal(int areanum, int clusternum)
{
  int portalnum; // eax
  aas_portal_t *portal; // edx
  aas_cluster_t *cluster; // ecx

  for ( portalnum = 1; portalnum < aasworld.numportals; ++portalnum )
  {
    if ( aasworld.portals[portalnum].areanum == areanum )
      break;
  }
  if ( portalnum == aasworld.numportals )
  {
    AAS_Error("no portal of area %d", areanum);
    return 1;
  }
  portal = &aasworld.portals[portalnum];
  if ( portal->frontcluster == clusternum )
    return 1;
  if ( portal->backcluster == clusternum )
    return 1;
  if ( !portal->frontcluster )
  {
    portal->frontcluster = clusternum;
  }
  else if ( !portal->backcluster )
  {
    portal->backcluster = clusternum;
  }
  else
  {
    Log_Write("portal using area %d is seperating more than two clusters",
              areanum);
    aasworld.areasettings[areanum].contents &= ~8u;
    return 0;
  }
  if ( aasworld.portalindexsize >= 0x10000 )
  {
    AAS_Error("AAS_MAX_PORTALINDEXSIZE");
    return 1;
  }
  aasworld.areasettings[areanum].cluster = -portalnum;
  cluster = &aasworld.clusters[clusternum];
  aasworld.portalindex[cluster->firstportal + cluster->numreachabilityareas] = portalnum;
  ++aasworld.portalindexsize;
  ++cluster->numreachabilityareas;
  return 1;
}
//----- (100087E0) --------------------------------------------------------
int __cdecl AAS_FloodClusterAreas_r(int areanum, int clusternum)
{
  int i; // ebp
  aas_area_t *area; // esi
  int facenum; // rax
  int v10; // eax
  aas_face_t *face; // ecx
  int v12; // ecx
  int v14; // esi
  int v15; // eax

  if ( areanum <= 0 || areanum >= aasworld.numareas )
  {
    AAS_Error("AAS_FloodClusterAreas_r: areanum out of range");
    return 0;
  }
  if ( aasworld.areasettings[areanum].cluster > 0 )
  {
    if ( aasworld.areasettings[areanum].cluster != clusternum )
    {
      Log_Write("cluster %d touched cluster %d at area %d", clusternum,
                aasworld.areasettings[areanum].cluster, areanum);
      return 0;
    }
  }
  else
  {
    if ( (aasworld.areasettings[areanum].contents & 8) != 0 )
      return AAS_UpdatePortal(areanum, clusternum);
  aasworld.areasettings[areanum].cluster = clusternum;
  aasworld.areasettings[areanum].clusterareanum = (aasworld.clusters[clusternum].numareas)++;
  area = &aasworld.areas[areanum];
  for ( i = 0; i < area->numfaces; i++ )
  {
    facenum = aasworld.faceindex[i + area->firstface];
    facenum = abs(facenum);
    v10 = aasworld.faces[facenum].frontarea;
    face = &aasworld.faces[facenum];
    if ( v10 == areanum )
    {
      v12 = face->backarea;
      if ( v12 && !AAS_FloodClusterAreas_r(v12, clusternum) )
        return 0;
    }
    else if ( v10 && !AAS_FloodClusterAreas_r(v10, clusternum) )
    {
      return 0;
    }
  }
  for ( v14 = 0; v14 < aasworld.areasettings[areanum].numreachableareas; v14++ )
  {
    v15 = aasworld.reachability[v14 + aasworld.areasettings[areanum].firstreachablearea].areanum;
    if ( v15 && !AAS_FloodClusterAreas_r(v15, clusternum) )
      return 0;
  }
  }
  return 1;
}
//----- (100089E0) --------------------------------------------------------
int __cdecl AAS_FloodClusterReachabilities(int clusternum)
{
  int i;
  int j;
  int areanum;

  for ( i = 1; i < aasworld.numareas; ++i )
  {
    if ( aasworld.areasettings[i].cluster )
      continue;
    if ( aasworld.areasettings[i].contents & 8 )
      continue;
    for ( j = 0; j < aasworld.areasettings[i].numreachableareas; ++j )
    {
      areanum = aasworld.reachability[aasworld.areasettings[i].firstreachablearea + j].areanum;
      if ( aasworld.areasettings[areanum].contents & 8 )
        continue;
      if ( aasworld.areasettings[areanum].cluster )
      {
        if ( !AAS_FloodClusterAreas_r(i, clusternum) )
          return 0;
        i = 0;
        break;
      }
    }
  }
  return 1;
}
//----- (10008AC0) --------------------------------------------------------
void __cdecl AAS_NumberClusterPortals(int clusternum)
{
  int i;
  int portalnum;
  aas_cluster_t *cluster;
  aas_portal_t *portal;

  cluster = &aasworld.clusters[clusternum];
  for ( i = 0; i < cluster->numreachabilityareas; i++ )
  {
    portalnum = aasworld.portalindex[cluster->firstportal + i];
    portal = &aasworld.portals[portalnum];
    if ( portal->frontcluster == clusternum )
      portal->clusterareanum[0] = cluster->numareas++;
    else
      portal->clusterareanum[1] = cluster->numareas++;
  }
}
//----- (10008B40) --------------------------------------------------------
int AAS_FindClusters()
{
  int i; // ebx
  aas_cluster_t *cluster; // esi

  AAS_RemoveClusterAreas();
  for ( i = 1; i < aasworld.numareas; i++ )
  {
    if ( aasworld.areasettings[i].cluster == 0 && (aasworld.areasettings[i].contents & 8) == 0 )
    {
      if ( aasworld.numclusters >= 0x10000 )
      {
        AAS_Error("AAS_MAX_CLUSTERS");
        return 0;
      }
      cluster = &aasworld.clusters[aasworld.numclusters];
      cluster->numareas = 0;
      cluster->firstportal = aasworld.portalindexsize;
      cluster->numreachabilityareas = 0;
      if ( !AAS_FloodClusterAreas_r(i, aasworld.numclusters) || !AAS_FloodClusterReachabilities(aasworld.numclusters) )
        return 0;
      AAS_NumberClusterPortals(aasworld.numclusters);
      Log_Write("cluster %d has %d areas", aasworld.numclusters, cluster->numareas);
      aasworld.numclusters++;
    }
  }
  return 1;
}
//----- (10008C80) --------------------------------------------------------
void AAS_CreatePortals()
{
  int i;
  aas_portal_t *portal;

  for ( i = 1; i < aasworld.numareas; i++ )
  {
    if ( (aasworld.areasettings[i].contents & 8) != 0 )
    {
      if ( aasworld.numportals >= 0x10000 )
      {
        AAS_Error("AAS_MAX_PORTALS");
        return;
      }
      portal = &aasworld.portals[aasworld.numportals];
      portal->areanum = i;
      portal->frontcluster = 0;
      portal->backcluster = 0;
      Log_Write("portal %d: area %d", aasworld.numportals, i);
      aasworld.numportals++;
    }
  }
}
//----- (10008D40) --------------------------------------------------------
void __cdecl AAS_ConnectedAreas_r(int *areanums, int numareas, int *connectedareas, int curarea)
{
  int i, j, otherareanum, facenum;
  aas_area_t *area;
  aas_face_t *face;

  connectedareas[curarea] = 1;
  area = &aasworld.areas[areanums[curarea]];
  for ( i = 0; i < area->numfaces; i++ )
  {
    facenum = abs(aasworld.faceindex[area->firstface + i]);
    face = &aasworld.faces[facenum];
    if ( (face->faceflags & 1) != 0 )
      continue;
    if ( face->frontarea != areanums[curarea] )
      otherareanum = face->frontarea;
    else
      otherareanum = face->backarea;
    for ( j = 0; j < numareas; j++ )
    {
      if ( areanums[j] == otherareanum )
        break;
    }
    if ( j == numareas )
      continue;
    if ( connectedareas[j] )
      continue;
    AAS_ConnectedAreas_r(areanums, numareas, connectedareas, j);
  }
}
//----- (10008E20) --------------------------------------------------------
qboolean __cdecl AAS_ConnectedAreas(_DWORD *areanums, int numareas)
{
  int connectedareas[128]; // [esp+8h] [ebp-200h] BYREF
  int i;

  memset(connectedareas, 0, sizeof(connectedareas));
  if ( numareas < 1 )
    return 0;
  if ( numareas == 1 )
    return 1;
  AAS_ConnectedAreas_r(areanums, numareas, connectedareas, 0);
  for ( i = 0; i < numareas; i++ )
  {
    if ( !connectedareas[i] )
      return 0;
  }
  return 1;
}
// 10008E6A: conditional instruction was optimized away because %arg_4.4>=2
//----- (10008EB0) --------------------------------------------------------
int __cdecl AAS_FloodAreas_r(int *areanum, int cluster, int done)
{
  aas_area_t *area;
  aas_face_t *face;
  int presencetype;
  int i, j, nextareanum;

  areanum[cluster++] = done;
  area = &aasworld.areas[done];
  presencetype = aasworld.areasettings[done].presencetype;
  for ( i = 0; i < area->numfaces; i++ )
  {
    face = &aasworld.faces[abs(aasworld.faceindex[area->firstface + i])];
    if ( (face->faceflags & 1) != 0 )
      continue;
    if ( face->frontarea != done )
      nextareanum = face->frontarea;
    else
      nextareanum = face->backarea;
    if ( (~aasworld.areasettings[nextareanum].presencetype & presencetype) == 0
      || (~presencetype & aasworld.areasettings[nextareanum].presencetype) != 0 )
    {
      continue;
    }
    for ( j = 0; j < cluster; j++ )
    {
      if ( nextareanum == areanum[j] )
        break;
    }
    if ( j != cluster )
      continue;
    if ( cluster >= 128 )
    {
      AAS_Error("MAX_PORTALAREAS");
      break;
    }
    cluster = AAS_FloodAreas_r(areanum, cluster, nextareanum);
  }
  return cluster;
}
//----- (10008FF0) --------------------------------------------------------
int __cdecl AAS_CheckAreaForPossiblePortals(int areanum)
{
  int i;
  int j;
  int k;
  int fen;
  int ben;
  int frontedgenum;
  int backedgenum;
  int facenum;
  int areanums[128];
  int numareas;
  int otherareanum;
  int numareafrontfaces[128];
  int numareabackfaces[128];
  int frontfacenums[128];
  int backfacenums[128];
  int numfrontfaces;
  int numbackfaces;
  int frontareanums[128];
  int backareanums[128];
  int numfrontareas;
  int numbackareas;
  int frontplanenum;
  int backplanenum;
  int faceplanenum;
  aas_area_t *area;
  aas_face_t *frontface;
  aas_face_t *backface;
  aas_face_t *face;

  if ( aasworld.areasettings[areanum].contents & 8 )
    return 0;
  if ( !(aasworld.areasettings[areanum].areaflags & 1) )
    return 0;
  memset(numareafrontfaces, 0, sizeof(numareafrontfaces));
  memset(numareabackfaces, 0, sizeof(numareabackfaces));
  numareas = numfrontfaces = numbackfaces = 0;
  numfrontareas = numbackareas = 0;
  frontplanenum = backplanenum = -1;
  numareas = AAS_FloodAreas_r(areanums, 0, areanum);
  for ( i = 0; i < numareas; i++ )
  {
    area = &aasworld.areas[areanums[i]];
    for ( j = 0; j < area->numfaces; j++ )
    {
      facenum = abs(aasworld.faceindex[area->firstface + j]);
      face = &aasworld.faces[facenum];
      if ( face->faceflags & 1 )
        continue;
      for ( k = 0; k < numareas; k++ )
      {
        if ( k == i )
          continue;
        if ( face->frontarea == areanums[k] || face->backarea == areanums[k] )
          break;
      }
      if ( k != numareas )
        continue;
      if ( face->frontarea == areanums[i] )
        otherareanum = face->backarea;
      else
        otherareanum = face->frontarea;
      if ( aasworld.areasettings[otherareanum].contents & 8 )
        return 0;
      faceplanenum = face->planenum & ~1;
      if ( frontplanenum < 0 || faceplanenum == frontplanenum )
      {
        frontplanenum = faceplanenum;
        frontfacenums[numfrontfaces++] = facenum;
        for ( k = 0; k < numfrontareas; k++ )
        {
          if ( frontareanums[k] == otherareanum )
            break;
        }
        if ( k == numfrontareas )
          frontareanums[numfrontareas++] = otherareanum;
        numareafrontfaces[i]++;
      }
      else if ( backplanenum < 0 || faceplanenum == backplanenum )
      {
        backplanenum = faceplanenum;
        backfacenums[numbackfaces++] = facenum;
        for ( k = 0; k < numbackareas; k++ )
        {
          if ( backareanums[k] == otherareanum )
            break;
        }
        if ( k == numbackareas )
          backareanums[numbackareas++] = otherareanum;
        numareabackfaces[i]++;
      }
      else
      {
        return 0;
      }
    }
  }
  for ( i = 0; i < numareas; i++ )
  {
    if ( !numareafrontfaces[i] || !numareabackfaces[i] )
      return 0;
  }
  if ( !AAS_ConnectedAreas(frontareanums, numfrontareas) )
    return 0;
  if ( !AAS_ConnectedAreas(backareanums, numbackareas) )
    return 0;
  for ( i = 0; i < numfrontfaces; i++ )
  {
    frontface = &aasworld.faces[frontfacenums[i]];
    for ( fen = 0; fen < frontface->numedges; fen++ )
    {
      frontedgenum = abs(aasworld.edgeindex[frontface->firstedge + fen]);
      for ( j = 0; j < numbackfaces; j++ )
      {
        backface = &aasworld.faces[backfacenums[j]];
        for ( ben = 0; ben < backface->numedges; ben++ )
        {
          backedgenum = abs(aasworld.edgeindex[backface->firstedge + ben]);
          if ( frontedgenum == backedgenum )
            break;
        }
        if ( ben != backface->numedges )
          break;
      }
      if ( j != numbackfaces )
        break;
    }
    if ( fen != frontface->numedges )
      break;
  }
  if ( i != numfrontfaces )
    return 0;
  for ( i = 0; i < numareas; i++ )
  {
    aasworld.areasettings[areanums[i]].contents |= 8u;
    aasworld.areasettings[areanums[i]].contents |= 0x20u;
    Log_Write("possible portal: %d", areanums[i]);
  }
  return numareas;
}
//----- (10009570) --------------------------------------------------------
int AAS_FindPossiblePortals()
{
  int i; // esi
  int numpossibleportals; // edi

  i = 1;
  for ( numpossibleportals = 0; i < aasworld.numareas; ++i )
    numpossibleportals += AAS_CheckAreaForPossiblePortals(i);
  return botimport.Print(PRT_MESSAGE, "\r%6d possible portals\n", numpossibleportals);
}
//----- (100095C0) --------------------------------------------------------
void AAS_RemoveAllPortals()
{
  int i;

  for ( i = 1; i < aasworld.numareas; i++ )
  {
    aasworld.areasettings[i].contents &= ~8u;
  }
}
//----- (10009610) --------------------------------------------------------
int AAS_TestPortals()
{
  int i;
  aas_portal_t *portal;

  for ( i = 1; i < aasworld.numportals; i++ )
  {
    portal = &((aas_portal_t *)aasworld.portals)[i];
    if ( !portal->frontcluster )
    {
      ((aas_areasettings_t *)aasworld.areasettings)[portal->areanum].contents &= ~8u;
      Log_Write("portal area %d has no front cluster\n", portal->areanum);
      return 0;
    }
    if ( !portal->backcluster )
    {
      ((aas_areasettings_t *)aasworld.areasettings)[portal->areanum].contents &= ~8u;
      Log_Write("portal area %d has no back cluster\n", portal->areanum);
      return 0;
    }
  }
  return 1;
}
//----- (100096E0) --------------------------------------------------------
void AAS_InitClustering()
{
  if ( !aasworld.loaded )
    return;
  if ( aasworld.numclusters >= 1
    && !(int)LibVarGetValue("forceclustering")
    && !(int)LibVarGetValue("forcereachability") )
  {
    return;
  }
  AAS_RemoveAllPortals();
  AAS_RemoveClusterAreas();
  AAS_FindPossiblePortals();
  if ( aasworld.portals )
    FreeMemory(aasworld.portals);
  aasworld.portals = GetClearedMemory(1310720);
  if ( aasworld.portalindex )
    FreeMemory(aasworld.portalindex);
  aasworld.portalindex = (int *)GetClearedMemory(0x40000);
  if ( aasworld.clusters )
    FreeMemory(aasworld.clusters);
  aasworld.clusters = GetClearedMemory(786432);
  while ( 1 )
  {
    aasworld.numportals = 1;
    aasworld.portalindexsize = 0;
    aasworld.numclusters = 1;
    AAS_CreatePortals();
    if ( !AAS_FindClusters() )
      continue;
    if ( AAS_TestPortals() )
      break;
  }
  aasworld.savefile = 1;
  botimport.Print(PRT_MESSAGE, "\r%6d portals created\n", aasworld.numportals);
  botimport.Print(PRT_MESSAGE, "\r%6d clusters created\n", aasworld.numclusters);
}
