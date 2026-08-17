/*
 * be_aas_optimize.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10010860..0x10010E90.
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
#include "be_aas_optimize.h"
#include "be_aas_bspq2.h"
#include "be_aas_main.h"
#include "be_interface.h"
#include "l_memory.h"

// gladiator.dll: 10010860..10010866
// gladi386.so:   0001D314..0001D31A
int AAS_KeepEdge(aas_edge_t *edge)
{
  (void)edge;
  return 1;
}

// gladiator.dll: 10010880..1001098D
// gladi386.so:   0001D31C..0001D4A4
int __cdecl AAS_OptimizeEdge(optimized_t *optimized, int edgenum)
{
  int i, optedgenum;
  aas_edge_t *edge;
  aas_edge_t *optedge;

  edge = &aasworld.edges[abs(edgenum)];
  if ( !AAS_KeepEdge(edge) )
    return 0;

  optedgenum = optimized->edgeremap[abs(edgenum)];
  if ( optedgenum )
  {
    if ( edgenum > 0 )
      return optedgenum;
    else
      return -optedgenum;
  }

  optedge = &((aas_edge_t *)optimized->edges)[optimized->numedges];

  for ( i = 0; i < 2; i++ )
  {
    if ( optimized->vertexremap[edge->v[i]] )
    {
      optedge->v[i] = optimized->vertexremap[edge->v[i]];
    }
    else
    {
      VectorCopy(aasworld.vertexes[edge->v[i]],
                 ((vec3_t *)optimized->vertexes)[optimized->numvertexes]);
      optedge->v[i] = optimized->numvertexes;
      optimized->vertexremap[edge->v[i]] = optimized->numvertexes;
      optimized->numvertexes++;
    }
  }
  optimized->edgeremap[abs(edgenum)] = optimized->numedges;
  optedgenum = optimized->numedges;
  optimized->numedges++;
  if ( edgenum > 0 )
    return optedgenum;
  else
    return -optedgenum;
}

// gladiator.dll: 100109E0..100109EE
// gladi386.so:   0001D4A4..0001D4B7
int __cdecl AAS_KeepFace(aas_face_t *face)
{
  if ( !(face->faceflags & 2) )
    return 0;
  else
    return 1;
}

// gladiator.dll: 10010A00..10010AF4
// gladi386.so:   0001D4B8..0001D611
int __cdecl AAS_OptimizeFace(optimized_t *optimized, int facenum)
{
  int i;
  int edgenum;
  int optedgenum;
  int optfacenum;
  aas_face_t *face;
  aas_face_t *optface;

  face = &aasworld.faces[abs(facenum)];
  if ( !AAS_KeepFace(face) )
    return 0;
  optfacenum = optimized->faceremap[abs(facenum)];
  if ( optfacenum )
  {
    if ( facenum > 0 )
      return optfacenum;
    else
      return -optfacenum;
  }
  optface = &((aas_face_t *)optimized->faces)[optimized->numfaces];
  memcpy(optface, face, sizeof(aas_face_t));
  optface->numedges = 0;
  optface->firstedge = optimized->edgeindexsize;
  i = 0;
  for ( ; i < face->numedges; ++i )
  {
    edgenum = aasworld.edgeindex[face->firstedge + i];
    optedgenum = AAS_OptimizeEdge(optimized, edgenum);
    if ( optedgenum )
    {
      optimized->edgeindex[optface->firstedge + optface->numedges] = optedgenum;
      optface->numedges++;
      ++optimized->edgeindexsize;
    }
  }
  optimized->faceremap[abs(facenum)] = optimized->numfaces;
  optfacenum = optimized->numfaces;
  optimized->numfaces++;
  if ( facenum > 0 )
    return optfacenum;
  else
    return -optfacenum;
}

// gladiator.dll: 10010B40..10010BD2
// gladi386.so:   0001D614..0001D6D0
void __cdecl AAS_OptimizeArea(optimized_t *optimized, int areanum)
{
  aas_area_t *area; // edx
  aas_area_t *optarea; // ebx
  int i; // esi
  int optfacenum; // eax

  area = &aasworld.areas[areanum];
  optarea = &((aas_area_t *)optimized->areas)[areanum];
  memcpy(optarea, area, 0x30u);
  optarea->numfaces = 0;
  optarea->firstface = optimized->faceindexsize;
  for ( i = 0; i < area->numfaces; ++i )
  {
    optfacenum = AAS_OptimizeFace(optimized, aasworld.faceindex[area->firstface + i]);
    if ( optfacenum )
    {
      optimized->faceindex[optarea->firstface + optarea->numfaces] = optfacenum;
      ++optarea->numfaces;
      ++optimized->faceindexsize;
    }
  }
}

// gladiator.dll: 10010C10..10010D01
// gladi386.so:   0001D6D0..0001D813
int __cdecl AAS_OptimizeAlloc(optimized_t *optimized)
{
  void *result; // eax

  optimized->vertexes = GetClearedMemory(sizeof(aas_vertex_t) * aasworld.numvertexes);
  optimized->numvertexes = 0;
  optimized->edges = GetClearedMemory(sizeof(aas_edge_t) * aasworld.numedges);
  optimized->numedges = 1;
  optimized->edgeindex = (int *)GetClearedMemory(4 * aasworld.edgeindexsize);
  optimized->edgeindexsize = 0;
  optimized->faces = GetClearedMemory(sizeof(aas_face_t) * aasworld.numfaces);
  optimized->numfaces = 1;
  optimized->faceindex = (int *)GetClearedMemory(4 * aasworld.faceindexsize);
  optimized->faceindexsize = 0;
  optimized->areas = GetClearedMemory(sizeof(aas_area_t) * aasworld.numareas);
  optimized->numareas = aasworld.numareas;
  optimized->vertexremap = (int *)GetClearedMemory(4 * aasworld.numvertexes);
  optimized->edgeremap = (int *)GetClearedMemory(4 * aasworld.numedges);
  result = GetClearedMemory(4 * aasworld.numfaces);
  optimized->faceremap = (int *)result;
  return (intptr_t)result;
}

// gladiator.dll: 10010D50..10010E48
// gladi386.so:   0001D814..0001D990
int __cdecl AAS_OptimizeStore(optimized_t *optimized)
{
  if ( aasworld.vertexes )
    FreeMemory(aasworld.vertexes);
  aasworld.vertexes = optimized->vertexes;
  aasworld.numvertexes = optimized->numvertexes;
  if ( aasworld.edges )
    FreeMemory(aasworld.edges);
  aasworld.edges = optimized->edges;
  aasworld.numedges = optimized->numedges;
  if ( aasworld.edgeindex )
    FreeMemory(aasworld.edgeindex);
  aasworld.edgeindex = optimized->edgeindex;
  aasworld.edgeindexsize = optimized->edgeindexsize;
  if ( aasworld.faces )
    FreeMemory(aasworld.faces);
  aasworld.faces = optimized->faces;
  aasworld.numfaces = optimized->numfaces;
  if ( aasworld.faceindex )
    FreeMemory(aasworld.faceindex);
  aasworld.faceindex = optimized->faceindex;
  aasworld.faceindexsize = optimized->faceindexsize;
  if ( aasworld.areas )
    FreeMemory(aasworld.areas);
  aasworld.areas = optimized->areas;
  aasworld.numareas = optimized->numareas;
  FreeMemory(optimized->vertexremap);
  FreeMemory(optimized->edgeremap);
  return FreeMemory(optimized->faceremap);
}

// gladiator.dll: 10010E90..10010F2B
// gladi386.so:   0001D990..0001DAFA
void AAS_Optimize()
{
  int i; // esi
  optimized_t optimized; // [esp+4h] [ebp-3Ch] BYREF (was int[15])

  AAS_OptimizeAlloc(&optimized);
  for ( i = 1; i < aasworld.numareas; ++i )
    AAS_OptimizeArea(&optimized, i);
  for ( i = 0; i < aasworld.reachabilitysize; ++i )
  {
    if ( aasworld.reachability[i].traveltype != 11 )
      aasworld.reachability[i].facenum = optimized.faceremap[aasworld.reachability[i].facenum];
  }
  AAS_OptimizeStore(&optimized);
  botimport.Print(PRT_MESSAGE, "AAS data optimized.\n");
}
