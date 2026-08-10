/*
 * be_aas_file.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x1000BBA0..0x1000D340; the boundary evidence -- per-object .text and
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
#include "be_aas_file.h"
#include "be_aas_bspq2.h"
#include "be_aas_light.h"
#include "be_aas_main.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_memory.h"
#include "l_utils.h"

//----- (1000BBA0) --------------------------------------------------------
void AAS_SwapAASData()
{
  int v0; // ebp
  int v3; // esi
  int v8; // ebx
  int v10; // edi
  int v11; // ebp
  int v14; // ebx
  int i; // esi
  int j; // esi
  int v21; // edi
  int k; // esi
  int v31; // ebp
  int v36; // ebx
  int v38; // edi
  int v47; // ebp
  int v52; // ebx
  int v56; // edi
  int v61; // edi
  int m; // esi
  int v71; // edi

  v0 = 0;
  if ( aasworld.numbboxes > 0 )
  {
    do
    {
      aasworld.bboxes[v0].presencetype = LittleLong(aasworld.bboxes[v0].presencetype);
      aasworld.bboxes[v0].flags = LittleLong(aasworld.bboxes[v0].flags);
      for ( v3 = 0; v3 < 3; ++v3 )
      {
        aasworld.bboxes[v0].mins[v3] = (float)LittleLong((int)aasworld.bboxes[v0].mins[v3]);
        aasworld.bboxes[v0].maxs[v3] = (float)LittleLong((int)aasworld.bboxes[v0].maxs[v3]);
      }
      ++v0;
    }
    while ( v0 < aasworld.numbboxes );
  }
  v8 = 0;
  if ( aasworld.numvertexes > 0 )
  {
    do
    {
      for ( v10 = 0; v10 < 3; ++v10 )
      {
        aasworld.vertexes[v8][v10] = LittleFloat(aasworld.vertexes[v8][v10]);
      }
      ++v8;
    }
    while ( v8 < aasworld.numvertexes );
  }
  v11 = 0;
  if ( aasworld.numplanes > 0 )
  {
    do
    {
      for ( v14 = 0; v14 < 3; ++v14 )
      {
        aasworld.planes[v11].normal[v14] = LittleFloat(aasworld.planes[v11].normal[v14]);
      }
      aasworld.planes[v11].dist = LittleFloat(aasworld.planes[v11].dist);
      aasworld.planes[v11].type = LittleLong(aasworld.planes[v11].type);
      ++v11;
    }
    while ( v11 < aasworld.numplanes );
  }
  for ( i = 0; i < aasworld.numedges; ++i )
  {
    aasworld.edges[i].v[0] = LittleLong(aasworld.edges[i].v[0]);
    aasworld.edges[i].v[1] = LittleLong(aasworld.edges[i].v[1]);
  }
  for ( j = 0; j < aasworld.edgeindexsize; ++j )
  {
    aasworld.edgeindex[j] = LittleLong(aasworld.edgeindex[j]);
  }
  v21 = 0;
  if ( aasworld.numfaces > 0 )
  {
    do
    {
      aasworld.faces[v21].planenum = LittleLong(aasworld.faces[v21].planenum);
      aasworld.faces[v21].faceflags = LittleLong(aasworld.faces[v21].faceflags);
      aasworld.faces[v21].numedges = LittleLong(aasworld.faces[v21].numedges);
      aasworld.faces[v21].firstedge = LittleLong(aasworld.faces[v21].firstedge);
      aasworld.faces[v21].frontarea = LittleLong(aasworld.faces[v21].frontarea);
      aasworld.faces[v21].backarea = LittleLong(aasworld.faces[v21].backarea);
      ++v21;
    }
    while ( v21 < aasworld.numfaces );
  }
  for ( k = 0; k < aasworld.faceindexsize; ++k )
  {
    aasworld.faceindex[k] = LittleLong(aasworld.faceindex[k]);
  }
  v31 = 0;
  if ( aasworld.numareas > 0 )
  {
    do
    {
      aasworld.areas[v31].areanum = LittleLong(aasworld.areas[v31].areanum);
      aasworld.areas[v31].numfaces = LittleLong(aasworld.areas[v31].numfaces);
      aasworld.areas[v31].firstface = LittleLong(aasworld.areas[v31].firstface);
      for ( v36 = 0; v36 < 3; ++v36 )
      {
        aasworld.areas[v31].mins[v36] = LittleFloat(aasworld.areas[v31].mins[v36]);
        aasworld.areas[v31].maxs[v36] = LittleFloat(aasworld.areas[v31].maxs[v36]);
        aasworld.areas[v31].center[v36] = LittleFloat(aasworld.areas[v31].center[v36]);
      }
      ++v31;
    }
    while ( v31 < aasworld.numareas );
  }
  v38 = 0;
  if ( aasworld.numareasettings > 0 )
  {
    do
    {
      aasworld.areasettings[v38].contents = LittleLong(aasworld.areasettings[v38].contents);
      aasworld.areasettings[v38].areaflags = LittleLong(aasworld.areasettings[v38].areaflags);
      aasworld.areasettings[v38].presencetype = LittleLong(aasworld.areasettings[v38].presencetype);
      aasworld.areasettings[v38].cluster = LittleLong(aasworld.areasettings[v38].cluster);
      aasworld.areasettings[v38].clusterareanum = LittleLong(aasworld.areasettings[v38].clusterareanum);
      aasworld.areasettings[v38].numreachableareas = LittleLong(aasworld.areasettings[v38].numreachableareas);
      aasworld.areasettings[v38].firstreachablearea = LittleLong(aasworld.areasettings[v38].firstreachablearea);
      ++v38;
    }
    while ( v38 < aasworld.numareasettings );
  }
  v47 = 0;
  if ( aasworld.reachabilitysize > 0 )
  {
    do
    {
      aasworld.reachability[v47].areanum = LittleLong(aasworld.reachability[v47].areanum);
      aasworld.reachability[v47].facenum = LittleLong(aasworld.reachability[v47].facenum);
      aasworld.reachability[v47].edgenum = LittleLong(aasworld.reachability[v47].edgenum);
      for ( v52 = 0; v52 < 3; ++v52 )
      {
        aasworld.reachability[v47].start[v52] = LittleFloat(aasworld.reachability[v47].start[v52]);
        aasworld.reachability[v47].end[v52] = LittleFloat(aasworld.reachability[v47].end[v52]);
      }
      aasworld.reachability[v47].traveltype = LittleLong(aasworld.reachability[v47].traveltype);
      aasworld.reachability[v47].traveltime = LittleShort(aasworld.reachability[v47].traveltime);
      ++v47;
    }
    while ( v47 < aasworld.reachabilitysize );
  }
  v56 = 0;
  if ( aasworld.numnodes > 0 )
  {
    do
    {
      aasworld.nodes[v56].planenum = LittleLong(aasworld.nodes[v56].planenum);
      aasworld.nodes[v56].children[0] = LittleLong(aasworld.nodes[v56].children[0]);
      aasworld.nodes[v56].children[1] = LittleLong(aasworld.nodes[v56].children[1]);
      ++v56;
    }
    while ( v56 < aasworld.numnodes );
  }
  v61 = 0;
  if ( aasworld.numportals > 0 )
  {
    do
    {
      aasworld.portals[v61].areanum = LittleLong(aasworld.portals[v61].areanum);
      aasworld.portals[v61].frontcluster = LittleLong(aasworld.portals[v61].frontcluster);
      aasworld.portals[v61].backcluster = LittleLong(aasworld.portals[v61].backcluster);
      aasworld.portals[v61].clusterareanum[0] = LittleLong(aasworld.portals[v61].clusterareanum[0]);
      aasworld.portals[v61].clusterareanum[1] = LittleLong(aasworld.portals[v61].clusterareanum[1]);
      ++v61;
    }
    while ( v61 < aasworld.numportals );
  }
  for ( m = 0; m < aasworld.portalindexsize; ++m )
  {
    aasworld.portalindex[m] = LittleLong(aasworld.portalindex[m]);
  }
  v71 = 0;
  if ( aasworld.numclusters > 0 )
  {
    do
    {
      aasworld.clusters[v71].numareas = LittleLong(aasworld.clusters[v71].numareas);
      aasworld.clusters[v71].numreachabilityareas = LittleLong(aasworld.clusters[v71].numreachabilityareas);
      aasworld.clusters[v71].firstportal = LittleLong(aasworld.clusters[v71].firstportal);
      ++v71;
    }
    while ( v71 < aasworld.numclusters );
  }
}
//----- (1000C490) --------------------------------------------------------
void *AAS_DumpAASData()
{
  void *result; // eax

  aasworld.numvertexes = 0;
  if ( aasworld.vertexes )
    FreeMemory(aasworld.vertexes);
  aasworld.vertexes = 0;
  aasworld.numplanes = 0;
  if ( aasworld.planes )
    FreeMemory(aasworld.planes);
  aasworld.planes = 0;
  aasworld.numedges = 0;
  if ( aasworld.edges )
    FreeMemory(aasworld.edges);
  aasworld.edges = 0;
  aasworld.edgeindexsize = 0;
  if ( aasworld.edgeindex )
    FreeMemory(aasworld.edgeindex);
  aasworld.edgeindex = 0;
  aasworld.numfaces = 0;
  if ( aasworld.faces )
    FreeMemory(aasworld.faces);
  aasworld.faces = 0;
  aasworld.faceindexsize = 0;
  if ( aasworld.faceindex )
    FreeMemory(aasworld.faceindex);
  aasworld.faceindex = 0;
  aasworld.numareas = 0;
  if ( aasworld.areas )
    FreeMemory(aasworld.areas);
  aasworld.areas = 0;
  aasworld.numareasettings = 0;
  if ( aasworld.areasettings )
    FreeMemory(aasworld.areasettings);
  aasworld.areasettings = 0;
  aasworld.reachabilitysize = 0;
  if ( aasworld.reachability )
    FreeMemory(aasworld.reachability);
  aasworld.reachability = 0;
  if ( aasworld.portals )
    FreeMemory(aasworld.portals);
  aasworld.portals = 0;
  aasworld.numportals = 0;
  if ( aasworld.portalindex )
    FreeMemory(aasworld.portalindex);
  result = aasworld.clusters;
  aasworld.portalindex = 0;
  aasworld.portalindexsize = 0;
  if ( aasworld.clusters )
    result = (void *)FreeMemory(aasworld.clusters);
  aasworld.clusters = 0;
  aasworld.numclusters = 0;
  aasworld.loaded = 0;
  aasworld.initialized = 0;
  aasworld.savefile = 0;
  return result;
}
//----- (1000C670) --------------------------------------------------------
void *__cdecl AAS_LoadAASLump(FILE *Stream, int Offset, size_t ElementCount)
{
  void *buf; // edi

  if ( !ElementCount )
    return 0;
  if ( fseek(Stream, Offset, 0) )
  {
    AAS_Error("can't seek to aas lump\n");
    AAS_DumpAASData();
    fclose(Stream);
    return 0;
  }
  buf = GetClearedMemory(ElementCount);
  if ( fread_locked(buf, 1u, ElementCount, Stream) != ElementCount )
  {
    AAS_Error("can't read aas lump\n");
    FreeMemory(buf);
    AAS_DumpAASData();
    fclose(Stream);
    return 0;
  }
  return buf;
}
//----- (1000C730) --------------------------------------------------------
int __cdecl AAS_LoadAASFile(char *FileName, int Offset, int Length)
{
  FILE *v2; // eax
  FILE *fp; // esi
  int v5; // eax
  int v6; // eax
  int v7; // eax
  int v8; // ebp
  size_t v9; // eax
  size_t v10; // ebx
  void *v11; // eax
  int v12; // eax
  int v13; // ebx
  size_t v14; // eax
  size_t v15; // ebp
  void *v16; // ecx
  int v17; // eax
  int v18; // ebx
  size_t v19; // eax
  size_t v20; // ebp
  void *v21; // ecx
  int v22; // eax
  int v23; // ebp
  size_t v24; // eax
  size_t v25; // ebx
  void *v26; // eax
  int v27; // eax
  int v28; // ebp
  size_t v29; // eax
  size_t v30; // ebx
  void *v31; // eax
  int v32; // eax
  int v33; // ebx
  size_t v34; // eax
  size_t v35; // ebp
  void *v36; // ecx
  int v37; // eax
  int v38; // ebp
  size_t v39; // eax
  size_t v40; // ebx
  void *v41; // eax
  int v42; // eax
  int v43; // ebx
  size_t v44; // eax
  size_t v45; // ebp
  void *v46; // ecx
  int v47; // eax
  int v48; // ebp
  size_t v49; // eax
  size_t v50; // ebx
  void *v51; // ecx
  int v52; // eax
  int v53; // ebx
  size_t v54; // eax
  size_t v55; // ebp
  void *v56; // ecx
  int v57; // eax
  int v58; // ebx
  size_t v59; // eax
  size_t v60; // ebp
  void *v61; // ecx
  int v62; // eax
  int v63; // ebx
  size_t v64; // eax
  size_t v65; // ebp
  void *v66; // ecx
  int v67; // eax
  int v68; // ebp
  size_t v69; // eax
  size_t v70; // ebx
  void *v71; // eax
  int v72; // eax
  int v73; // ebx
  size_t v74; // eax
  size_t v75; // edi
  void *v76; // ecx
  /* The lump-0 pointer goes straight to the global aasworld.bboxes. */
  aas_header_t aas_h; /* Gladiator AAS header: ident+version+14 lumps = 0x78 bytes */

  AAS_DumpAASData();
  v2 = fopen(FileName, "rb");
  fp = v2;
  if ( !v2 )
  {
    AAS_Error("can't open %s\n", FileName);
    return BLERR_CANNOTOPENAASFILE;
  }
  if ( fseek(v2, Offset, 0) )
  {
    AAS_Error("can't seek to file %s\n");
    fclose(fp);
    return BLERR_CANNOTSEEKTOAASFILE;
  }
  if ( fread_locked(&aas_h, 0x78u, 1u, fp) != 1 )
  {
    AAS_Error("can't read header of file %s\n", FileName);
    fclose(fp);
    return BLERR_CANNOTREADAASHEADER;
  }
  v5 = aas_h.ident = LittleLong(aas_h.ident);
  if ( v5 != 1396785477 )
  {
    AAS_Error("%s is not an AAS file\n", FileName);
    fclose(fp);
    return BLERR_WRONGAASFILEID;
  }
  v6 = aas_h.version = LittleLong(aas_h.version);
  if ( v6 == 2 )
  {
    botimport.Print(PRT_WARNING, "found an old AAS file, create a new AAS file\n");
  }
  else if ( v6 != 3 )
  {
    AAS_Error("aas file %s is version %i, not %i\n", FileName, v6, 3);
    fclose(fp);
    return BLERR_WRONGAASFILEVERSION;
  }
  v7 = LittleLong(aas_h.lumps[0].fileofs);
  v8 = Offset + v7;
  v9 = (size_t)LittleLong(aas_h.lumps[0].filelen);
  v10 = v9;
  v11 = AAS_LoadAASLump(fp, v8, v9);
  aasworld.bboxes = v11;
  v10 >>= 5;
  aasworld.numbboxes = v10;
  if ( v10 && !v11 )
    return BLERR_CANNOTREADAASLUMP;
  v12 = LittleLong(aas_h.lumps[1].fileofs);
  v13 = Offset + v12;
  v14 = (size_t)LittleLong(aas_h.lumps[1].filelen);
  v15 = v14;
  v16 = AAS_LoadAASLump(fp, v13, v14);
  aasworld.vertexes = v16;
  aasworld.numvertexes = v15 / 0xC;
  if ( v15 / 0xC && !v16 )
    return BLERR_CANNOTREADAASLUMP;
  v17 = LittleLong(aas_h.lumps[2].fileofs);
  v18 = Offset + v17;
  v19 = (size_t)LittleLong(aas_h.lumps[2].filelen);
  v20 = v19;
  v21 = AAS_LoadAASLump(fp, v18, v19);
  aasworld.planes = v21;
  aasworld.numplanes = v20 / 0x14;
  if ( v20 / 0x14 && !v21 )
    return BLERR_CANNOTREADAASLUMP;
  v22 = LittleLong(aas_h.lumps[3].fileofs);
  v23 = Offset + v22;
  v24 = (size_t)LittleLong(aas_h.lumps[3].filelen);
  v25 = v24;
  v26 = AAS_LoadAASLump(fp, v23, v24);
  aasworld.edges = v26;
  v25 >>= 3;
  aasworld.numedges = v25;
  if ( v25 && !v26 )
    return BLERR_CANNOTREADAASLUMP;
  v27 = LittleLong(aas_h.lumps[4].fileofs);
  v28 = Offset + v27;
  v29 = (size_t)LittleLong(aas_h.lumps[4].filelen);
  v30 = v29;
  v31 = AAS_LoadAASLump(fp, v28, v29);
  aasworld.edgeindex = v31;
  v30 >>= 2;
  aasworld.edgeindexsize = v30;
  if ( v30 && !v31 )
    return BLERR_CANNOTREADAASLUMP;
  v32 = LittleLong(aas_h.lumps[5].fileofs);
  v33 = Offset + v32;
  v34 = (size_t)LittleLong(aas_h.lumps[5].filelen);
  v35 = v34;
  v36 = AAS_LoadAASLump(fp, v33, v34);
  aasworld.faces = v36;
  aasworld.numfaces = v35 / 0x18;
  if ( v35 / 0x18 && !v36 )
    return BLERR_CANNOTREADAASLUMP;
  v37 = LittleLong(aas_h.lumps[6].fileofs);
  v38 = Offset + v37;
  v39 = (size_t)LittleLong(aas_h.lumps[6].filelen);
  v40 = v39;
  v41 = AAS_LoadAASLump(fp, v38, v39);
  aasworld.faceindex = v41;
  v40 >>= 2;
  aasworld.faceindexsize = v40;
  if ( v40 && !v41 )
    return BLERR_CANNOTREADAASLUMP;
  v42 = LittleLong(aas_h.lumps[7].fileofs);
  v43 = Offset + v42;
  v44 = (size_t)LittleLong(aas_h.lumps[7].filelen);
  v45 = v44;
  v46 = AAS_LoadAASLump(fp, v43, v44);
  aasworld.areas = v46;
  aasworld.numareas = v45 / 0x30;
  if ( v45 / 0x30 && !v46 )
    return BLERR_CANNOTREADAASLUMP;
  v47 = LittleLong(aas_h.lumps[8].fileofs);
  v48 = Offset + v47;
  v49 = (size_t)LittleLong(aas_h.lumps[8].filelen);
  v50 = v49;
  v51 = AAS_LoadAASLump(fp, v48, v49);
  aasworld.areasettings = v51;
  aasworld.numareasettings = v50 / 0x1C;
  if ( v50 / 0x1C && !v51 )
    return BLERR_CANNOTREADAASLUMP;
  v52 = LittleLong(aas_h.lumps[9].fileofs);
  v53 = Offset + v52;
  v54 = (size_t)LittleLong(aas_h.lumps[9].filelen);
  v55 = v54;
  v56 = AAS_LoadAASLump(fp, v53, v54);
  aasworld.reachability = v56;
  aasworld.reachabilitysize = v55 / 0x2C;
  if ( v55 / 0x2C && !v56 )
    return BLERR_CANNOTREADAASLUMP;
  v57 = LittleLong(aas_h.lumps[10].fileofs);
  v58 = Offset + v57;
  v59 = (size_t)LittleLong(aas_h.lumps[10].filelen);
  v60 = v59;
  v61 = AAS_LoadAASLump(fp, v58, v59);
  aasworld.nodes = v61;
  aasworld.numnodes = v60 / 0xC;
  if ( v60 / 0xC && !v61 )
    return BLERR_CANNOTREADAASLUMP;
  v62 = LittleLong(aas_h.lumps[11].fileofs);
  v63 = Offset + v62;
  v64 = (size_t)LittleLong(aas_h.lumps[11].filelen);
  v65 = v64;
  v66 = AAS_LoadAASLump(fp, v63, v64);
  aasworld.portals = v66;
  aasworld.numportals = v65 / 0x14;
  if ( v65 / 0x14 && !v66 )
    return BLERR_CANNOTREADAASLUMP;
  v67 = LittleLong(aas_h.lumps[12].fileofs);
  v68 = Offset + v67;
  v69 = (size_t)LittleLong(aas_h.lumps[12].filelen);
  v70 = v69;
  v71 = AAS_LoadAASLump(fp, v68, v69);
  aasworld.portalindex = (int *)v71;
  v70 >>= 2;
  aasworld.portalindexsize = v70;
  if ( v70 && !v71 )
    return BLERR_CANNOTREADAASLUMP;
  v72 = LittleLong(aas_h.lumps[13].fileofs);
  v73 = Offset + v72;
  v74 = (size_t)LittleLong(aas_h.lumps[13].filelen);
  v75 = v74;
  v76 = AAS_LoadAASLump(fp, v73, v74);
  aasworld.clusters = v76;
  aasworld.numclusters = v75 / 0xC;
  if ( v75 / 0xC && !v76 )
    return BLERR_CANNOTREADAASLUMP;
  AAS_SwapAASData();
  aasworld.loaded = 1;
  fclose(fp);
  return BLERR_NOERROR;
}
//----- (1000CE40) --------------------------------------------------------
int __cdecl AAS_WriteAASLump(FILE *fp, int *h, int lumpnum, void *data, size_t length)
{
  long v5;
  int *lump; // ebx

  lump = &h[2 * lumpnum + 2];
  v5 = ftell(fp);
  lump[0] = LittleLong((int)v5);
  lump[1] = LittleLong((int)length);
  if ( (int)length <= 0 || fwrite(data, length, 1u, fp) >= 1 )
    return 1;
  botimport.Print(PRT_ERROR, "error writing lump %s\n", (const char *)(intptr_t)lumpnum);
  fclose(fp);
  return 0;
}
//----- (1000CEE0) --------------------------------------------------------
qboolean __cdecl AAS_WriteAASFile(char *filename)
{
  FILE *v3; // eax
  FILE *fp; // esi
  int header[30]; // [esp+Ch] [ebp-78h] BYREF

  botimport.Print(PRT_MESSAGE, "writing %s\n", filename);
  AAS_SwapAASData();
  memset(header, 0, sizeof(header));
  header[0] = LittleLong(0x53414145);
  header[1] = LittleLong(3);
  v3 = fopen(filename, "wb");
  fp = v3;
  if ( !v3 )
  {
    botimport.Print(PRT_ERROR, "error opening %s\n", filename);
    return 0;
  }
  /* fopen mode is "wb" — this is fwrite, not fread (see AAS_WriteAASLump note). */
  if ( fwrite(header, 0x78u, 1u, v3) < 1u )
  {
    fclose(fp);
    return 0;
  }
  if ( !AAS_WriteAASLump(fp, header, 0, aasworld.bboxes, sizeof(aas_bbox_t) * aasworld.numbboxes) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 1, aasworld.vertexes, sizeof(aas_vertex_t) * aasworld.numvertexes) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 2, aasworld.planes, sizeof(aas_plane_t) * aasworld.numplanes) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 3, aasworld.edges, sizeof(aas_edge_t) * aasworld.numedges) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 4, aasworld.edgeindex, 4 * aasworld.edgeindexsize) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 5, aasworld.faces, sizeof(aas_face_t) * aasworld.numfaces) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 6, aasworld.faceindex, 4 * aasworld.faceindexsize) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 7, aasworld.areas, sizeof(aas_area_t) * aasworld.numareas) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 8, aasworld.areasettings, sizeof(aas_areasettings_t) * aasworld.numareasettings) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 9, aasworld.reachability, sizeof(aas_reachability_t) * aasworld.reachabilitysize) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 10, aasworld.nodes, sizeof(aas_node_t) * aasworld.numnodes) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 11, aasworld.portals, sizeof(aas_portal_t) * aasworld.numportals) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 12, aasworld.portalindex, 4 * aasworld.portalindexsize) ) return 0;
  if ( !AAS_WriteAASLump(fp, header, 13, aasworld.clusters, sizeof(aas_cluster_t) * aasworld.numclusters) ) return 0;
  fseek(fp, 0, 0);
  if ( fwrite(header, 0x78u, 1u, fp) < 1u )
  {
    fclose(fp);
    return 0;
  }
  fclose(fp);
  return 1;
}
//----- (1000D340) --------------------------------------------------------
// AAS_InitBSPPointLights — allocate and freelist-initialise the
// bsp_pointlight_t pool used by BotAddPointLight (0x1000D550) and
// AAS_BSPTraceLight (0x1000D5F0).  Pool size = max_aaslights * 52
// bytes; default 128 lights.  After init the entire pool is chained
// via .next/.prev into aasworld.oldestcache (free pool), and the
// pool base is stashed in aasworld._pad_1FC for later FreeMemory.
// DEAD in Gladiator — preserved by /INCREMENTAL: in stock builds the
// engine never sets up max_aaslights and never calls this init, so
// BotAddPointLight's free-list pop in sub_1000D450 always returns
// NULL and the point-light cache stays empty.
/* Not `static`: gladi386.so exports this as one of its 809 .dynsym FUNCs, and
 * a `static` that is never called is not merely inlined by gcc 2.7.2.3 -- it is
 * DROPPED, so the function vanished from the ELF build entirely and audited as
 * a missing 711-byte reconstruction.  MSVC /Od emits an unreferenced static
 * anyway, which is why the DLL has it at 0x1000D340 and the PE audit never
 * noticed. */
void sub_1000D340(void)
{
  /* `i` before `max`: gcc 2.7.2.3 gives each local its stack slot in
   * DECLARATION order (expand_decl numbers the pseudos, reload assigns slots
   * downward), so the order here is readable straight off the frame.  The real
   * image keeps max at [esp+0x1c] and i at [esp+0x20]; declaring max first
   * transposes the two and is the whole of the difference. */
  int i;
  int max;

  max = (int)LibVarValue("max_aaslights", "128");
  if ( max < 0 || max > 0x10000 )
  {
    botimport.Print(PRT_ERROR, "max_aaslights out of range [0, 65536]");
    max = 128;
  }
  aasworld.pointlightheap = (bsp_pointlight_t *)GetMemory(max * sizeof(bsp_pointlight_t));
  aasworld.pointlightheap[0].prev = NULL;
  aasworld.pointlightheap[0].next = &aasworld.pointlightheap[1];
  for ( i = 1; i < max - 1; ++i )
  {
    aasworld.pointlightheap[i].prev = &aasworld.pointlightheap[i - 1];
    aasworld.pointlightheap[i].next = &aasworld.pointlightheap[i + 1];
  }
  aasworld.pointlightheap[max - 1].prev = &aasworld.pointlightheap[max - 2];
  aasworld.pointlightheap[max - 1].next = NULL;
  aasworld.oldestcache = aasworld.pointlightheap;
}
