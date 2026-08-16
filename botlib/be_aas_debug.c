/*
 * be_aas_debug.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10009860..0x1000A810; the boundary evidence -- per-object .text and
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
#include "be_aas_debug.h"
#include "be_aas_bspq2.h"
#include "be_aas_main.h"
#include "be_aas_move.h"
#include "be_interface.h"
#include "l_memory.h"

int numdebuglines;          // 0x10066B14 (was dword_10066B14)
int debuglinevisible[256];  // 0x10066CC0 (was dword_10066CC0)
int debuglines[256];        // 0x100670C0 (was dword_100670C0)

// gladiator.dll: 10009860..10009894
// gladi386.so:   00012828..00012914
void AAS_ClearShownDebugLines(void)
{
  int i; // esi
  int line; // eax

  for ( i = 0; i < 256; ++i )
  {
    line = debuglines[i];
    if ( line )
    {
      botimport.DebugLineShow(line, 0, 0, -1);
      debuglinevisible[i] = 0;
    }
  }
}
// gladiator.dll: 100098B0..10009924
// gladi386.so:   00012914..00012A34
void __cdecl AAS_DebugLine(vec3_t start, vec3_t end, int color)
{
  int line; // esi

  for ( line = 0; line < 256; line++ )
  {
    if ( !debuglines[line] )
    {
      debuglines[line] = botimport.DebugLineCreate();
      debuglinevisible[line] = 0;
      numdebuglines++;
    }
    if ( !debuglinevisible[line] )
    {
      botimport.DebugLineShow(debuglines[line], start, end, color);
      debuglinevisible[line] = 1;
      return;
    }
  }
}
// gladiator.dll: 10009950..100099D2
// gladi386.so:   00012A34..00012BE8
int __cdecl AAS_DrawPermanentCross(vec3_t origin, float size, int color)
{
  int i;
  int debugline;
  int result;
  vec3_t start, end;

  for ( i = 0; i < 3; i++ )
  {
    VectorCopy(origin, start);
    start[i] += size;
    VectorCopy(origin, end);
    end[i] -= size;
    AAS_DebugLine(start, end, color);
    debugline = botimport.DebugLineCreate();
    result = botimport.DebugLineShow(debugline, start, end, color);
  }
  return result;
}
// gladiator.dll: 10009A10..10009C18
// gladi386.so:   00012BE8..00012E1F
/* AAS_DrawPlaneCross — plane-projection debug "X" visualiser, verbatim Q3
 * be_aas_debug.c.  Builds the four corners of a 12x12 square around the hit
 * point in the two transverse axes, projects each onto the plane along axis
 * n0 = type%3, then draws the two diagonals through botimport.DebugLineShow
 * with line-IDs from the shared debuglines pool.
 *
 * DEAD in shipped Gladiator (no .text caller), preserved by /INCREMENTAL.
 * Must keep external linkage: cl /O2 would drop an unreferenced static. */
void AAS_DrawPlaneCross(vec3_t point, vec3_t normal, float dist, int type, int color)
{
  int    n0, n1, n2, j, line, lines[2];
  vec3_t start1, end1, start2, end2;

  //make a cross in the hit plane at the hit point
  VectorCopy(point, start1);
  VectorCopy(point, end1);
  VectorCopy(point, start2);
  VectorCopy(point, end2);

  n0 = type % 3;
  n1 = (type + 1) % 3;
  n2 = (type + 2) % 3;
  start1[n1] -= 6;
  start1[n2] -= 6;
  end1[n1] += 6;
  end1[n2] += 6;
  start2[n1] += 6;
  start2[n2] -= 6;
  end2[n1] -= 6;
  end2[n2] += 6;

  start1[n0] = (dist - (start1[n1] * normal[n1] + start1[n2] * normal[n2])) / normal[n0];
  end1[n0]   = (dist - (end1[n1] * normal[n1] + end1[n2] * normal[n2])) / normal[n0];
  start2[n0] = (dist - (start2[n1] * normal[n1] + start2[n2] * normal[n2])) / normal[n0];
  end2[n0]   = (dist - (end2[n1] * normal[n1] + end2[n2] * normal[n2])) / normal[n0];

  for (j = 0, line = 0; j < 2 && line < 256; line++)
  {
    if (!debuglines[line])
    {
      debuglines[line] = botimport.DebugLineCreate();
      lines[j++] = debuglines[line];
      debuglinevisible[line] = 1;
      numdebuglines++;
    }
    else if (!debuglinevisible[line])
    {
      lines[j++] = debuglines[line];
      debuglinevisible[line] = 1;
    }
  }
  botimport.DebugLineShow(lines[0], start1, end1, color);
  botimport.DebugLineShow(lines[1], start2, end2, color);
}
// gladiator.dll: 10009CB0..10009E59
// gladi386.so:   00012E20..0001300F
/* AAS_ShowBoundingBox — Q3's AAS_ShowBoundingBox (be_aas_debug.c): draw the
 * AABB at `origin` with mins/maxs offsets as a wireframe cube in colour
 * 0xF2F2F0F0.  NOTE the param order is the reverse of Q3's
 * (origin, mins, maxs) — the .text takes the 2nd param as the TOP corners and
 * the 3rd as the bottom — and is kept as-is to stay byte-faithful.
 *
 *   bottom_corners[4], z = origin.z + mins.z:  SW, SE, NE, NW
 *   top_corners[4],    z = origin.z + maxs.z:  memcpy of bottom, then a
 *     4x stride-12 fst loop patches each copy's third float.
 *
 * Per iteration i in [1..4] it draws bot[i-1]->bot[i&3], top[i-1]->top[i&3]
 * and bot[i-1]->top[i-1], covering all 12 edges — but only THREE line IDs are
 * held across them, so which 3 edges are actually on screen cycles.  Almost
 * certainly a development-time "flicker the bbox" visualisation.
 *
 * The line-ID allocation up front inlines a private copy of AAS_DebugLine's
 * slot scan over debuglines[0..255].
 *
 * DEAD in Gladiator — no live caller.  Preserved by /INCREMENTAL. */
void __cdecl AAS_ShowBoundingBox(vec3_t origin, vec3_t mins, vec3_t maxs)
{
  vec3_t bboxcorners[8];
  int lines[3];
  int i, j, line;

  VectorAdd(origin, maxs, bboxcorners[0]);
  bboxcorners[1][0] = origin[0] + mins[0];
  bboxcorners[1][1] = origin[1] + maxs[1];
  bboxcorners[1][2] = origin[2] + maxs[2];
  bboxcorners[2][0] = origin[0] + mins[0];
  bboxcorners[2][1] = origin[1] + mins[1];
  bboxcorners[2][2] = origin[2] + maxs[2];
  bboxcorners[3][0] = origin[0] + maxs[0];
  bboxcorners[3][1] = origin[1] + mins[1];
  bboxcorners[3][2] = origin[2] + maxs[2];
  memcpy(bboxcorners[4], bboxcorners[0], sizeof(vec3_t) * 4);
  for ( i = 0; i < 4; i++ )
    bboxcorners[4 + i][2] = origin[2] + mins[2];
  for ( i = 0; i < 4; i++ )
  {
    for ( j = 0, line = 0; j < 3 && line < 256; line++ )
    {
      if ( !debuglines[line] )
      {
        debuglines[line] = botimport.DebugLineCreate();
        lines[j++] = debuglines[line];
        debuglinevisible[line] = 1;
        numdebuglines++;
      }
      else if ( !debuglinevisible[line] )
      {
        lines[j++] = debuglines[line];
        debuglinevisible[line] = 1;
      }
    }
    botimport.DebugLineShow(lines[0], bboxcorners[i], bboxcorners[(i + 1) & 3], 0xF2F2F0F0);
    botimport.DebugLineShow(lines[1], bboxcorners[4 + i], bboxcorners[4 + ((i + 1) & 3)], 0xF2F2F0F0);
    botimport.DebugLineShow(lines[2], bboxcorners[i], bboxcorners[4 + i], 0xF2F2F0F0);
  }
}
// gladiator.dll: 10009ED0..1000A032
// gladi386.so:   00013010..0001335B
/* AAS_ShowFace — draw every edge of one face as a line, cycling a 4-colour
 * debug palette (0xDCDDDEDF -> 0xF2F2F0F0 -> 0xD0D1D2D3 -> 0xF3F3F1F1),
 * then a 20-unit normal arrow from the face's first vertex in 0xF2F2F0F0.
 * Sibling of AAS_ShowArea@1000A0A0, but takes a face index directly.
 *
 * The original's colour cycle is a sub/neg/sbb/and/add bit-twiddle in the
 * fall-through branch; the algebra reduces exactly to the 4-entry table and
 * modulo step used here.
 *
 * DEAD in shipped Gladiator — no .text caller. */
void __cdecl AAS_ShowFace(int facenum)
{
  int i, color, edgenum;
  aas_edge_t *edge;
  aas_face_t *face;
  aas_plane_t *plane;
  vec3_t start, end;

  color = LINECOLOR_YELLOW;
  if ( facenum >= aasworld.numfaces )
    botimport.Print(PRT_ERROR, "facenum %d out of range\n", facenum);
  face = &aasworld.faces[facenum];
  for ( i = 0; i < face->numedges; i++ )
  {
    edgenum = abs(aasworld.edgeindex[face->firstedge + i]);
    if ( edgenum >= aasworld.numedges )
      botimport.Print(PRT_ERROR, "edgenum %d out of range\n", edgenum);
    edge = &aasworld.edges[edgenum];
    if ( color == LINECOLOR_RED )
      color = LINECOLOR_GREEN;
    else if ( color == LINECOLOR_GREEN )
      color = LINECOLOR_BLUE;
    else if ( color == LINECOLOR_BLUE )
      color = LINECOLOR_YELLOW;
    else
      color = LINECOLOR_RED;
    AAS_DebugLine(aasworld.vertexes[edge->v[0]], aasworld.vertexes[edge->v[1]], color);
  }
  plane = &aasworld.planes[face->planenum];
  edgenum = abs(aasworld.edgeindex[face->firstedge]);
  edge = &aasworld.edges[edgenum];
  VectorCopy(aasworld.vertexes[edge->v[0]], start);
  VectorMA(start, 20.0f, plane->normal, end);
  AAS_DebugLine(start, end, LINECOLOR_RED);
}
// gladiator.dll: 1000A0A0..1000A2DA
// gladi386.so:   0001335C..00013683
void __cdecl AAS_ShowArea(int areanum, int groundfacesonly)
{
  int areaedges[256];
  int numareaedges, i, j, n, color, line;
  int facenum, edgenum;
  aas_area_t *area;
  aas_face_t *face;
  aas_edge_t *edge;
  vec3_t v9; /* write-only in original disasm — no later read; left in place */

  color = 0;
  v9[0] = 0.0f;
  v9[1] = 0.0f;
  v9[2] = 1.0f;
  numareaedges = 0;
  if ( areanum < 0 || areanum >= aasworld.numareas )
  {
    botimport.Print(PRT_ERROR, "area %d out of range [0, %d]\n", areanum, aasworld.numareas);
    return;
  }
  area = &aasworld.areas[areanum];
  for ( i = 0; i < area->numfaces; i++ )
  {
    facenum = abs(aasworld.faceindex[area->firstface + i]);
    if ( facenum >= aasworld.numfaces )
      botimport.Print(PRT_ERROR, "facenum %d out of range\n", facenum);
    face = &aasworld.faces[facenum];
    if ( groundfacesonly )
    {
      if ( (face->faceflags & 6) == 0 )
        continue;
    }
    for ( j = 0; j < face->numedges; j++ )
    {
      edgenum = abs(aasworld.edgeindex[face->firstedge + j]);
      if ( edgenum >= aasworld.numedges )
        botimport.Print(PRT_ERROR, "edgenum %d out of range\n", edgenum);
      for ( n = 0; n < numareaedges; n++ )
      {
        if ( areaedges[n] == edgenum )
          break;
      }
      if ( n == numareaedges && numareaedges < 256 )
        areaedges[numareaedges++] = edgenum;
    }
  }
  for ( n = 0; n < numareaedges; n++ )
  {
    for ( line = 0; line < 256; line++ )
    {
      if ( !debuglines[line] )
      {
        debuglines[line] = botimport.DebugLineCreate();
        debuglinevisible[line] = 0;
        numdebuglines++;
      }
      if ( !debuglinevisible[line] )
        break;
    }
    if ( line >= 256 )
      return;
    edge = &aasworld.edges[areaedges[n]];
    if ( color == LINECOLOR_RED )
      color = LINECOLOR_BLUE;
    else if ( color == LINECOLOR_BLUE )
      color = LINECOLOR_GREEN;
    else if ( color == LINECOLOR_GREEN )
      color = LINECOLOR_YELLOW;
    else
      color = LINECOLOR_RED;
    botimport.DebugLineShow(
      debuglines[line],
      aasworld.vertexes[edge->v[0]],
      aasworld.vertexes[edge->v[1]],
      color);
    debuglinevisible[line] = 1;
  }
}
// 1000A228: conditional instruction was optimized away because esi.4<100
// gladiator.dll: 1000A370..1000A3DA
// gladi386.so:   00013684..0001380D
/* AAS_DrawCross (was sub_1000A370) — a 3D cross at `origin`: three line
 * segments, one per axis, each spanning [origin-size, origin+size]. */
void __cdecl AAS_DrawCross(vec3_t origin, float size, int color)
{
  int i;
  vec3_t start, end;

  for ( i = 0; i < 3; i++ )
  {
    VectorCopy(origin, start);
    start[i] += size;
    VectorCopy(origin, end);
    end[i] -= size;
    AAS_DebugLine(start, end, color);
  }
}
// gladiator.dll: 1000A400..1000A401
// gladi386.so:   00013810..00013811
/* AAS_PrintTravelType — pretty-prints a TRAVEL_* type, but compiled out to a
 * bare `ret` in this build (presumably a debug #ifdef that was off).  The
 * empty body must stay: AAS_ShowReachableAreas still calls it through the
 * thunk at 0x10001F7D. */
void __cdecl AAS_PrintTravelType(int traveltype)
{
  (void)traveltype;
}
// gladiator.dll: 1000A420..1000A572
// gladi386.so:   00013814..00013AF4
/* AAS_DrawArrow (was sub_1000A420) — an arrow from `start` to `end`: a shaft
 * in `linecolor` plus two arrowhead strokes in `arrowcolor`.  The head offset
 * is normalize(end-start) crossed with the up vector, falling back to (1,0,0)
 * when |dot| > 0.99. */
void __cdecl AAS_DrawArrow(vec3_t start, vec3_t end, int linecolor, int arrowcolor)
{
  double dot; // st7
  /* vec3_t locals, not separate scalars: CrossProduct / VectorNormalize /
   * VectorMA need three contiguous floats. */
  vec3_t dir;  // [esp+8h] [ebp-3Ch] BYREF
  vec3_t cross; // [esp+2Ch] [ebp-18h] BYREF
  vec3_t p1; // [esp+20h] [ebp-24h] BYREF
  vec3_t p2; // [esp+38h] [ebp-Ch] BYREF
  vec3_t up; // [esp+14h] [ebp-30h] BYREF

  up[0] = 0.0;
  up[1] = 0.0;
  up[2] = 1.0;
  VectorSubtract(end, start, dir);
  VectorNormalize(dir);
  dot = dir[0] * up[0] + dir[1] * up[1] + dir[2] * up[2];
  if ( dot > 0.99 || dot < -0.99 )
  {
    cross[0] = 1.0f;   /* 1065353216 = 0x3F800000 = 1.0f as bit-pattern;
                       cross is float[3] so use the float literal */
    cross[1] = 0;
    cross[2] = 0;
  }
  else
  {
    CrossProduct(dir, up, cross);
  }
  VectorMA((float *)end, -6.0, dir, (float *)p1);
  VectorCopy(p1, p2);
  VectorMA((float *)p1, 6.0, (float *)cross, (float *)p1);
  VectorMA((float *)p2, -6.0, (float *)cross, (float *)p2);
  AAS_DebugLine(start, end, linecolor);
  AAS_DebugLine(p1, end, arrowcolor);
  AAS_DebugLine(p2, end, arrowcolor);
}
// gladiator.dll: 1000A5E0..1000A791
// gladi386.so:   00013AF4..00013E73
/*
 * AAS_ShowReachability (was sub_1000A5E0) — debug visualisation of one
 * aas_reachability_t, matching Q3's AAS_ShowReachability: an ShowArea +
 * DrawArrow header, then a switch on traveltype running
 * HorizontalVelocityForJump + ClientMovement prediction per branch, with a
 * TRAVEL_JUMP-only DrawCross from JumpReachRunStart.
 *
 * Appears dead to static analysis: its only xref is the thunk at 0x1000160e,
 * called only from AAS_ShowReachableAreas (0x1000A810), whose own thunk has no
 * callers either — the whole debug family is reached through a string-keyed
 * console dispatcher the decompilers do not resolve.
 */
void __cdecl AAS_ShowReachability(aas_reachability_t *reach)
{
  int traveltype; // eax
  float speed; // [esp+Ch] [ebp-7Ch] BYREF
  float zvel; // [esp+10h] [ebp-78h]
  vec3_t dir; // [esp+14h] [ebp-74h] BYREF (12 B / vec3)
  vec3_t cmdmove; // [esp+20h] [ebp-68h] BYREF
  vec3_t v12; // [esp+2Ch] [ebp-5Ch] BYREF

  AAS_ShowArea(reach->areanum, 1);
  AAS_DrawArrow(reach->start, reach->end, -202116623, -589439265);
  traveltype = reach->traveltype;
  if ( traveltype == 5 || traveltype == 7 ) /* TRAVEL_JUMP || TRAVEL_WALKOFFLEDGE */
  {
    AAS_HorizontalVelocityForJump(libvar_sv_jumpvel->value, reach->start, reach->end, &speed);
    dir[0] = reach->end[0] - reach->start[0];
    dir[1] = reach->end[1] - reach->start[1];
    dir[2] = 0.0f;
    VectorNormalize(dir);
    VectorScale(dir, speed, (float *)cmdmove);
    cmdmove[2] = libvar_sv_jumpvel->value;
    AAS_ClientMovementPrediction(-1, reach->start, 2, 1, vec3_origin, cmdmove, 3, 30, 0.1, 61, 1);
    if ( reach->traveltype == 5 ) /* TRAVEL_JUMP only */
    {
      AAS_JumpReachRunStart((intptr_t)reach, (intptr_t)dir);
      AAS_DrawCross(dir, 4.0, -202116623); /* LINECOLOR_BLUE = -202116623 (0xF3F3F3F1) */
    }
  }
  else if ( traveltype == 12 ) /* TRAVEL_ROCKETJUMP */
  {
    zvel = AAS_RocketJumpZVelocity(reach->start); /* AAS_RocketJumpZVelocity(reach->start) → Z-velocity */
    AAS_HorizontalVelocityForJump(zvel, reach->start, reach->end, &speed);
    dir[0] = reach->end[0] - reach->start[0];
    dir[1] = reach->end[1] - reach->start[1];
    dir[2] = 0.0f;
    VectorNormalize(dir);
    VectorScale(dir, speed, (float *)cmdmove);
    v12[0] = 0;
    v12[1] = 0;
    v12[2] = zvel;
    AAS_ClientMovementPrediction(-1, reach->start, 2, 1, v12, cmdmove, 3, 30, 0.1, 61, 1);
  }
}
// gladiator.dll: 1000A810..1000A8D4
// gladi386.so:   00013E74..00013F57
/*
 * AAS_ShowReachableAreas — walks one area's reachability list, one entry per
 * call and throttled to 1.5 s per advance, printing each travel type and
 * handing the reach to AAS_ShowReachability.  Matches Q3's version in
 * be_aas_debug.c: same control flow, same statics, same throttle.
 *
 * Statics in the original DLL: reach @0x10062908 (44 B), lasttime @0x10062934,
 * lastareanum @0x10062938, index @0x1006293c.
 *
 * Reached only via the thunk at 0x10001F87 — see AAS_ShowReachability.
 */
static int   showreach_reach[11];       /* aas_reachability_t (44 B) */
static int   showreach_lastareanum;
static int   showreach_index;
static float showreach_lasttime;

void __cdecl AAS_ShowReachableAreas(int areanum)
{
  int numreach;
  int firstreach;
  aas_areasettings_t *as;

  if ( areanum != showreach_lastareanum )
  {
    showreach_index = 0;
    showreach_lastareanum = areanum;
  }
  as = &aasworld.areasettings[areanum];
  numreach = as->numreachableareas;
  if ( !numreach )
    return;
  if ( showreach_index >= numreach )
    showreach_index = 0;
  if ( AAS_Time() - showreach_lasttime > 1.5 )
  {
    firstreach = as->firstreachablearea;
    memcpy(showreach_reach,
            &aasworld.reachability[firstreach + showreach_index],
            44);
    showreach_index++;
    showreach_lasttime = AAS_Time();
    AAS_PrintTravelType(showreach_reach[9]);                /* reach.traveltype at +36 */
    botimport.Print(PRT_MESSAGE, "\n");
  }
  AAS_ShowReachability(showreach_reach);
}
