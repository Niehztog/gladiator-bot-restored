/*
 * be_aas_entity.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x1000A920..0x1000BB30; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * Its own interface is in the matching .h; botlib_local.h, which that header
 * pulls in, carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs, the side-band scheme and the externs for
 * botlib.c's remaining globals).
 *
 * Every function below carries a two-line address annotation: its extent in the
 * 1999 Windows `gladiator.dll` (PE32) and in the 1999 Linux `gladi386.so`
 * (ELF32, glibc build), each start..end with the end exclusive.  `absent` means
 * the Linux image has no counterpart.  CLAUDE.md, "Function address
 * annotations", records how both ranges are derived.
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
#include "be_aas_entity.h"
#include "be_aas_bspq2.h"
#include "be_aas_main.h"
#include "be_aas_reach.h"
#include "be_aas_sample.h"
#include "be_interface.h"
#include "l_memory.h"
#include "l_utils.h"

// gladiator.dll: 1000A920..1000AB48
// gladi386.so:   00013F58..0001419B
int __cdecl AAS_UpdateEntity(int entnum, bot_updateentity_t *state)
{
  aas_entityinfo_t *ent; // esi (entities[entnum].i)
  int relink;            // [esp+1Ch] [ebp+4h]  (was v13)
  vec3_t absmins;      // [esp+Ch]  [ebp-Ch] BYREF (was v12)
  vec3_t absmaxs;      // [esp+0h]  [ebp-18h] BYREF (was v11)

  if ( !aasworld.loaded )
  {
    botimport.Print(PRT_MESSAGE, "AAS_UpdateEntity: not loaded\n");
    return BLERR_NOAASFILE;
  }
  ent = &aasworld.entities[entnum].i;
  ent->update_time = AAS_Time() - ent->ltime;
  ent->ltime = AAS_Time();
  /* remember the previous origin before it is overwritten below */
  VectorCopy(ent->origin, ent->lastvisorigin);
  VectorCopy(state->old_origin, ent->old_origin);
  ent->solid = state->solid;
  ent->modelindex = state->modelindex;
  ent->modelindex2 = state->modelindex2;
  ent->modelindex3 = state->modelindex3;
  ent->modelindex4 = state->modelindex4;
  ent->frame = state->frame;
  ent->effects = state->effects;
  ent->renderfx = state->renderfx;
  ent->number = entnum;
  ent->valid = 1;
  relink = 0;
  if ( ent->solid == 3 )                 /* SOLID_BSP */
  {
    if ( !VectorCompare(state->angles, ent->angles) )
    {
      relink = 1;
      VectorCopy(state->angles, ent->angles);
    }
    AAS_BSPModelMinsMaxsOrigin(ent->modelindex - 1, ent->angles, ent->mins, ent->maxs, NULL);
  }
  else if ( ent->solid == 2             /* SOLID_BBOX */
            && (!VectorCompare(state->mins, ent->mins) || !VectorCompare(state->maxs, ent->maxs)) )
  {
    relink = 1;
    VectorCopy(state->mins, ent->mins);
    VectorCopy(state->maxs, ent->maxs);
  }
  if ( !VectorCompare(state->origin, ent->origin) )
  {
    VectorCopy(state->origin, ent->origin);
    relink = 1;
  }
  if ( relink && entnum > 0 )
  {
    VectorAdd(ent->mins, ent->origin, absmins);
    VectorAdd(ent->maxs, ent->origin, absmaxs);
#if BOTLIB_NEED_SIDEBAND
    AAS_UnlinkFromAreas(AAS_EntAreaLink(entnum));
    AAS_EntAreaLink(entnum) = AAS_LinkEntityClientBBox(absmins, absmaxs, entnum, 2);
    AAS_UnlinkFromBSPLeaves(AAS_EntBspLink(entnum));
    AAS_EntBspLink(entnum) = AAS_BSPLinkEntity(absmins, absmaxs, entnum, 0);
#else
    /* Reach the link heads through the already-cached `ent` pointer so MSVC
     * reuses one register for both, rather than recomputing the index. */
    AAS_UnlinkFromAreas(((aas_entity_t *)ent)->areas);
    ((aas_entity_t *)ent)->areas = AAS_LinkEntityClientBBox(absmins, absmaxs, entnum, 2);
    AAS_UnlinkFromBSPLeaves(((aas_entity_t *)ent)->leaves);
    ((aas_entity_t *)ent)->leaves = AAS_BSPLinkEntity(absmins, absmaxs, entnum, 0);
#endif
  }
  return 0;
}
// gladiator.dll: 1000ABE0..1000AC71
// gladi386.so:   0001419C..00014262
/* AAS_EntityInfo — the AAS info snapshot for an engine entity, returned BY
 * VALUE through MSVC's hidden-retbuf ABI: aasworld.entities[entnum].i on the
 * valid path, a zeroed local on the error paths.  Q3 instead takes an output
 * pointer, so `entnum` is the only explicit parameter here. */
aas_entityinfo_t __cdecl AAS_EntityInfo(int entnum)
{
  aas_entityinfo_t entinfo; // [esp+8h] [ebp-7Ch] BYREF

  if ( !aasworld.initialized )
  {
    botimport.Print(PRT_FATAL, "AAS_EntityInfo: aasworld not initialized\n");
    memset(&entinfo, 0, sizeof(entinfo));
    return entinfo;
  }
  if ( entnum < 0 || entnum >= aasworld.numentities )
  {
    botimport.Print(PRT_FATAL, "AAS_EntityInfo: entnum %d out of range\n", entnum);
    memset(&entinfo, 0, sizeof(entinfo));
    return entinfo;
  }
  return aasworld.entities[entnum].i;
}
// gladiator.dll: 1000ACB0..1000AD1F
// gladi386.so:   00014264..000142F4
// Bounds-checked copy of entities[entnum].origin
// (struct offsets +0x10..+0x18) into the caller-provided vec3 out.  On OOR
// prints via bi_Print(PRT_FATAL, "AAS_EntityOrigin: entnum %d out of range\n", ...)
// and zeroes the destination vec3.
void __cdecl AAS_EntityOrigin(int entnum, vec3_t origin)
{
  if ( entnum < 0 || entnum >= aasworld.numentities )
  {
    botimport.Print(PRT_FATAL, "AAS_EntityOrigin: entnum %d out of range\n", entnum);
    origin[2] = 0.0f;
    origin[1] = 0.0f;
    origin[0] = 0.0f;
    return;
  }
  VectorCopy(aasworld.entities[entnum].i.origin, origin);
}
// gladiator.dll: 1000AD40..1000AD76
// gladi386.so:   000142F4..0001434A
int __cdecl AAS_EntityModelindex(int entnum)
{
  if ( entnum < 0 || entnum >= aasworld.numentities )
  {
    botimport.Print(PRT_FATAL, "AAS_EntityModelindex: entnum %d out of range\n", entnum);
    return 0;
  }
  return aasworld.entities[entnum].i.modelindex;
}
// gladiator.dll: 1000AD90..1000ADCF
// gladi386.so:   0001434C..000143AE
int __cdecl AAS_EntityRenderFX(int entnum)
{
  if ( !aasworld.initialized )
    return 0;
  if ( entnum < 0 || entnum >= aasworld.numentities )
  {
    botimport.Print(PRT_FATAL, "AAS_EntityRenderFX: entnum %d out of range\n", entnum);
    return 0;
  }
  return aasworld.entities[entnum].i.renderfx;
}
// gladiator.dll: 1000ADE0..1000AE20
// gladi386.so:   000143B0..00014410
int __cdecl AAS_EntityModelNum(int entnum)
{
  if ( !aasworld.initialized )
    return 0;
  if ( entnum < 0 || entnum >= aasworld.numentities )
  {
    botimport.Print(PRT_FATAL, "AAS_EntityModelNum: entnum %d out of range\n", entnum);
    return 0;
  }
  return aasworld.entities[entnum].i.modelindex - 1;
}
// gladiator.dll: 1000AE30..1000AE7B
// gladi386.so:   00014410..00014495
int __cdecl AAS_OriginOfMoverWithModelNum(int modelnum, vec3_t origin)
{
  int i;
  aas_entityinfo_t *ent;

  for ( i = 0; i < aasworld.numentities; i++ )
  {
    ent = &aasworld.entities[i].i;
    if ( ent->modelindex - 1 == modelnum )
    {
      VectorCopy(ent->origin, origin);
      return 1;
    }
  }
  return 0;
}
// gladiator.dll: 1000AEA0..1000AF06
// gladi386.so:   00014498..0001451E
// Returns the bbox of entities[entnum] via two vec3 out-params (mins from
// struct offsets +0x40..+0x48, maxs from +0x4C..+0x54).  Guards on
// aasworld.initialized then bounds-checks entnum; on OOR prints via
// bi_Print(PRT_FATAL, "AAS_EntitySize: entnum %d out of range\n", entnum) and
// returns *without* writing to mins/maxs (matches the original — the
// disasm has no clear path on the OOR exit).
void __cdecl AAS_EntitySize(int entnum, vec3_t mins, vec3_t maxs)
{
  aas_entityinfo_t *ent;

  if ( !aasworld.initialized )
    return;
  if ( entnum < 0 || entnum >= aasworld.numentities )
  {
    botimport.Print(PRT_FATAL, "AAS_EntitySize: entnum %d out of range\n", entnum);
    return;
  }
  ent = &aasworld.entities[entnum].i;
  VectorCopy(ent->mins, mins);
  VectorCopy(ent->maxs, maxs);
}
// gladiator.dll: 1000AF30..1000AFAF
// gladi386.so:   00014520..000145B3
int __cdecl AAS_EntityBSPData(int entnum, bsp_entdata_t *entdata)
{
  aas_entityinfo_t *ent;
  int result;

  ent = &aasworld.entities[entnum].i;
  VectorCopy(ent->origin, entdata->origin);
  VectorCopy(ent->angles, entdata->angles);
  VectorAdd(ent->origin, ent->mins, entdata->absmins);
  VectorAdd(ent->origin, ent->maxs, entdata->absmaxs);
  entdata->solid      = ent->solid;
  result = ent->modelindex - 1;
  entdata->modelnum   = result;
  return result;
}
// gladiator.dll: 1000AFD0..1000B053
// gladi386.so:   000145B4..00014649
int __cdecl AAS_DropToFloor(vec3_t origin, vec3_t mins, vec3_t maxs)
{
  vec3_t end;
  bsp_trace_t trace;

  VectorCopy(origin, end);
  end[2] -= 100.0f;
  trace = AAS_Trace(origin, mins, maxs, end, 0, 3);
  if ( trace.startsolid )
    return 0;
  VectorCopy(trace.endpos, origin);
  return 1;
}
// gladiator.dll: 1000B090..1000B0C9
// gladi386.so:   0001464C..000146A3
void AAS_ResetEntityLinks()
{
  int i;

  /* The macros clear the inline +124/+128 link heads on 32-bit, the side-band
   * arrays on 64-bit. */
  for ( i = 0; i < aasworld.numentities; i++ )
  {
    AAS_EntAreaLink(i) = NULL;
    AAS_EntBspLink(i) = NULL;
  }
  /* No return value: Q3's AAS_ResetEntityLinks is void. */
}
// gladiator.dll: 1000B0E0..1000B119
// gladi386.so:   000146A4..000146F3
void __cdecl AAS_InvalidateEntities()
{
  int i;
  for ( i = 0; i < aasworld.numentities; i++ )
  {
    aasworld.entities[i].i.valid = 0;
    aasworld.entities[i].i.number = i;
  }
}
// gladiator.dll: 1000B130..1000B18B
// gladi386.so:   000146F4..0001475C
int __cdecl AAS_BestReachableLinkArea(aas_link_t *areas)
{
  aas_link_t *link;

  for ( link = areas; link; link = link->next_area )
  {
    if ( AAS_AreaGrounded(link->areanum) || AAS_AreaSwim(link->areanum) )
      return link->areanum;
  }
  for ( link = areas; link; link = link->next_area )
  {
    if ( link->areanum )
      return link->areanum;
  }
  return 0;
}
// gladiator.dll: 1000B300..1000B585
// gladi386.so:   000148EC..00014CBC
int __cdecl AAS_BestReachableArea(int *origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin)
{
  int result; // eax
  int areanum; // ebp
  int k; // edi
  int l; // esi
  float v12; // st7
  float *v13; // ecx
  aas_link_t *areas; // esi - holds aas_link_t* from AAS_AASLinkEntity; was int, truncated on aarch64 → AAS_BestReachableLinkArea+0x3c SIGSEGV walking corrupted list
  /* Must be one vec3_t, not three floats: the original passes its address to
   * AAS_PointAreaNum / AAS_TraceClientBBox as a vec3 pointer.  Split into
   * separate locals, y/z read as garbage and every level item ends up with
   * areanum 0, so BotChooseLTGItem can never pick one. */
  int j; // [esp+1Ch] [ebp-80h]
  int i; // [esp+20h] [ebp-7Ch]
  vec3_t absmins; // [esp+3Ch] [ebp-60h] BYREF
  vec3_t end; // [esp+30h] [ebp-6Ch] BYREF
  vec3_t start; // [esp+10h] [ebp-8Ch] BYREF (was v18+v19+v20)
  vec3_t absmaxs; // [esp+48h] [ebp-54h] BYREF
  aas_trace_t trace; // [esp+54h] [ebp-48h] (was int v29[9] + char v30[36] hidden return buffer)

  if ( !aasworld.loaded )
  {
    botimport.Print(PRT_ERROR, "AAS_BestReachableArea: aas not loaded\n");
    return 0;
  }
  {
    VectorCopy(((float *)origin), start);
    areanum = AAS_PointAreaNum(start);
    for ( i = 0; i < 5 && !areanum; ++i )
    {
      for ( j = 0; j < 5 && !areanum; ++j )
      {
        for ( k = -1; k <= 1 && !areanum; ++k )
        {
          for ( l = -1; l <= 1 && !areanum; ++l )
          {
            VectorCopy(((float *)origin), start);
            start[0] = (float)j * 4.0f * (float)k + start[0];
            start[1] = (float)j * 4.0f * (float)l + start[1];
            start[2] = (float)i * 4.0f + start[2];
            areanum = AAS_PointAreaNum(start);
          }
        }
      }
    }
    if ( areanum )
    {
      v12 = start[2];
      *(float *)end = start[0];
      end[1] = start[1];
      start[2] = start[2] + 0.25;
      end[2] = v12 - 50.0f;
      trace = AAS_TraceClientBBox(start, (float *)end, 4, -1);
      if ( !trace.startsolid )
      {
        result = AAS_PointAreaNum(trace.endpos);
        v13 = goalorigin;
        VectorCopy(trace.endpos, goalorigin);
        if ( result )
          return result;
      }
      else
      {
        VectorCopy(start, goalorigin);
        return areanum;
      }
    }
    else
    {
      v13 = goalorigin;
    }
    *(int *)&v13[0] = *origin;
    *(int *)&v13[1] = origin[1];
    *(int *)&v13[2] = origin[2];
    absmins[0] = *(float *)origin + *mins;
    absmins[1] = mins[1] + *((float *)origin + 1);
    absmins[2] = mins[2] + *((float *)origin + 2);
    absmaxs[0] = *(float *)origin + *maxs;
    absmaxs[1] = maxs[1] + *((float *)origin + 1);
    absmaxs[2] = maxs[2] + *((float *)origin + 2);
    areas = AAS_AASLinkEntity(absmins, absmaxs, -1);
    result = AAS_BestReachableLinkArea(areas);
    AAS_UnlinkFromAreas(areas);
    return result;
  }
}
// gladiator.dll: 1000B640..1000B703
// gladi386.so:   00014CBC..00014E69
int InFieldOfVision(float *viewangles, float fov, float *angles)
{
  int v5; // edi
  float delta;
  float v8;
  float v9;

  for ( v5 = 0; v5 < 2; v5++ )
  {
    v8 = anglemod(viewangles[v5]);
    v9 = anglemod(angles[v5]);
    angles[v5] = v9;
    delta = v9 - v8;
    if ( v9 > (float)v8 )
    {
      if ( delta > 180.0 )
        delta = delta - 360.0;
    }
    else
    {
      if ( delta < -180.0 )
        delta = delta + 360.0;
    }
    if ( delta > 0.0f )
    {
      if ( delta > fov * 0.5 )
        return 0;
    }
    else
    {
      if ( delta < -fov * 0.5 )
        return 0;
    }
  }
  return 1;
}
// gladiator.dll: 1000B750..1000B9E2
// gladi386.so:   00014E6C..000152B1
int __cdecl BotEntityVisible(int viewer, float *eye, float *viewangles, float fov, int a5)
{
  int contents_mask;             // contentmask
  int eyecontents;     // PointContents(eye)
  int fromcontents;    // PointContents(viewer)
  int i;             // iteration counter
  int passent;             // passent
  int hitent;             // hitent
  aas_entityinfo_t *ent; // entity info (was float *v24 byte-arith)
  bsp_trace_t trace;       // [ebp-FCh] BYREF
  vec3_t dir;          // [ebp-114h] BYREF — was v31[3]
  vec3_t entangles;    // [ebp-108h] BYREF — was v32[3]
  vec3_t start;        // [ebp-120h] BYREF — was v28+v29+v30 split locals
  vec3_t end;          // [ebp-12Ch] BYREF — was v25+v26+v27 split locals
  vec3_t middle;       // [ebp-148h] BYREF — was v18+v19+v20 split locals

  /* Q3's be_aas_entity.c order throughout: VectorAdd(mins, maxs, middle) —
   * mins first in all three components, IDA had the first two the other way
   * round — and VectorAdd(origin, middle, middle) below, origin first.  Both
   * are real gcc levers (the leading operand is the one that gets the `fld`).
   * `a5` is also used directly rather than through IDA's `v5 = a5` alias,
   * which cost a store/reload pair through a frame slot. */
  ent = &aasworld.entities[a5].i;
  middle[0] = ent->mins[0] + ent->maxs[0];
  middle[1] = ent->mins[1] + ent->maxs[1];
  middle[2] = ent->mins[2] + ent->maxs[2];
  VectorScale((float *)middle, 0.5, (float *)middle);
  middle[0] = ent->origin[0] + middle[0];
  middle[1] = ent->origin[1] + middle[1];
  middle[2] = ent->origin[2] + middle[2];
  VectorSubtract(middle, ((float *)eye), dir);
  vectoangles(dir, (float *)entangles);
  if ( !InFieldOfVision(viewangles, fov, entangles) )
    return 0;
  for ( i = 0; i < 3; i++ )
  {
    if ( AAS_inPVS(eye, middle) )
    {
    /* default: trace from viewer (a2) to entity middle */
    contents_mask = 0x2030003;        /* CONTENTS_SOLID | CONTENTS_PLAYERCLIP (Q2 trace mask) */
    passent = viewer;
    hitent = a5;
    VectorCopy(((float *)eye), start);
    VectorCopy(middle, end);
    /* Both PointContents() calls are required: without them eyecontents and
     * fromcontents stay uninitialised, the trace direction never swaps when one
     * endpoint is underwater, and visibility across water surfaces fails. */
    eyecontents = sub_10003080((float *)middle);
    if ( (eyecontents & 0x38) != 0 )
      contents_mask = 0x203003B;      /* | CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER */
    fromcontents = sub_10003080(eye);
    if ( (fromcontents & 0x38) != 0 )
    {
      if ( (contents_mask & 0x38) == 0 )
      {
        passent = a5;
        hitent = viewer;
        /* swap: trace from entity middle to viewer instead */
        VectorCopy(middle, start);
        VectorCopy(((float *)eye), end);
      }
      contents_mask ^= 0x38u;
    }
    trace = AAS_Trace((float*)(start), (float*)(uintptr_t)(0), (float*)(uintptr_t)(0), (float*)(end), passent, contents_mask);
    /* if trace hit a translucent water/slime surface, retrace through it */
    if ( (LOBYTE(trace.contents) & 0x38) != 0 && (LOBYTE(trace.surface.flags) & 0x30) != 0 )
      trace = AAS_Trace(trace.endpos, (float*)(uintptr_t)(0), (float*)(uintptr_t)(0), (float*)(end), passent, contents_mask & 0xFFFFFFC7);
    if ( trace.fraction >= 1.0f || LODWORD(trace.ent) == hitent )
      return 1;
    /* try alternate z-positions: foot, then head */
    if ( !i )
      middle[2] = middle[2] + ent->mins[2];
    else if ( i == 1 )
      middle[2] = ent->maxs[2] - ent->mins[2] + middle[2];
    }
  }
  return 0;
}
// gladiator.dll: 1000B1F0..1000B2BE
// gladi386.so:   000147D8..000148EB
// FindNearestEntity-by-classnum: scan aasworld.entities[0..numentities)
// for the entry whose 4-byte field at +0x5C equals target and whose
// origin is within 40 units of the caller's reference point in both
// the x and y axes (integer-truncated abs(dx) < 40 && abs(dy) < 40).
// Of all candidates passing the gate, return the index whose 3D
// straight-line distance to ref is smallest (initial best = 99999.0,
// .rdata literal 0x47C34F80).  Result index lives in eax; best
// distance is dropped on return.  When numentities <= 0 the function
// short-circuits to eax = 0 without touching arg1.
// Stride 132 (=sizeof(aas_entity_t)); field +0x10 = origin xyz, +0x5C
// = classnum/spawnflags-style key (matches the disasm).  Distance via
// VectorLength thunk @0x10001D75 → VectorLength@10043500.  DEAD in
// Gladiator — /INCREMENTAL.
int __cdecl sub_1000B1F0(float *ref, int target)
{
  int i;
  int best_index;
  float best_dist;
  aas_entityinfo_t *ent;
  vec3_t delta;
  float d;

  best_index = 0;
  best_dist  = 99999.0f;
  /* No explicit `if (numentities <= 0) return` guard: the loop's own entry test
   * covers it and the single tail returns 0, sharing one epilogue between the
   * empty and normal exits as the original does. */
  for ( i = 0; i < aasworld.numentities; i++ )
  {
    ent = &aasworld.entities[i].i;
    if ( ent->modelindex != target )
      continue;
    VectorSubtract(ent->origin, ref, delta);
    if ( abs((int)delta[0]) >= 40 )
      continue;
    if ( abs((int)delta[1]) >= 40 )
      continue;
    d = VectorLength(delta);
    if ( d < best_dist )
    {
      best_dist  = (float)d;
      best_index = i;
    }
  }
  return best_index;
}
// gladiator.dll: 1000B1B0..1000B1D2
// gladi386.so:   0001475C..000147D8
/* Thin wrapper handing aasworld.entities[entnum].areas (the entity's
 * aas_link_t chain head at +0x7c) to AAS_BestReachableLinkArea.
 * DEAD in Gladiator — preserved by /INCREMENTAL. */
int __cdecl AAS_BestReachableEntityArea(int entnum)
{
#if BOTLIB_NEED_SIDEBAND
  aas_link_t *links = AAS_EntAreaLink(entnum);
  return AAS_BestReachableLinkArea(links);
#else
  aas_entity_t *ent;

  ent = &aasworld.entities[entnum];
  return AAS_BestReachableLinkArea(ent->areas);
#endif
}
// gladiator.dll: 1000BAA0..1000BB0E
// gladi386.so:   000152B4..00015340
int __cdecl sub_1000BAA0(int a1, float *a2, float *a3, float a4, int a5, int *a6)
{
  int v6; // esi
  int v7; // edi

  v7 = 0;
  v6 = 1;
  while ( v6 <= aasworld.aas_maxclients )
  {
    aas_entity_t *ent = &aasworld.entities[v6];
    if ( ent->i.valid )
    {
      if ( BotEntityVisible(a1, a2, a3, a4, v6) )
      {
        a6[v7] = v6;
        ++v7;
        if ( v7 >= a5 )
          return v7;
      }
    }
    ++v6;
  }
  return v7;
}
// gladiator.dll: 1000BB30..1000BB74
// gladi386.so:   00015340..000153B2
int __cdecl AAS_NextBSPEntity(int ent)
{
  int v1; // eax

  v1 = ent;
  if ( !aasworld.loaded )
    return 0;
  if ( ent < 0 )
    v1 = -1;
  while ( ++v1 < aasworld.numentities )
  {
    if ( aasworld.entities[v1].i.valid )
      return v1;
  }
  return 0;
}
