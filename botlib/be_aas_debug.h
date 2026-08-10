/*
 * be_aas_debug.h — interface of be_aas_debug.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_BE_AAS_DEBUG_H
#define BOTLIB_BE_AAS_DEBUG_H

#include "botlib_local.h"

int AAS_ClearShownDebugLines();
int __cdecl AAS_DebugLine(vec3_t start, vec3_t end, int color);
void __cdecl AAS_DrawArrow(vec3_t start, vec3_t end, int linecolor, int arrowcolor);
void __cdecl AAS_DrawCross(vec3_t origin, float size, int color);
int __cdecl AAS_DrawPermanentCross(vec3_t origin, float size, int color);
void AAS_DrawPlaneCross(vec3_t point, vec3_t normal, float dist, int type, int color);
void __cdecl AAS_PrintTravelType(int traveltype);
void __cdecl AAS_ShowArea(int areanum, int groundfacesonly);
void __cdecl AAS_ShowBoundingBox(vec3_t origin, vec3_t mins, vec3_t maxs);
void __cdecl AAS_ShowFace(int facenum);
void __cdecl AAS_ShowReachability(aas_reachability_t *reach);
void __cdecl AAS_ShowReachableAreas(int areanum);

#endif /* BOTLIB_BE_AAS_DEBUG_H */
