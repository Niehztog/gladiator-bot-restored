/* be_aas_debug.h — interface of be_aas_debug.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_BE_AAS_DEBUG_H
#define BOTLIB_BE_AAS_DEBUG_H

/* Declarations for what this TU defines — last, so the types above are in scope. */
/* be_aas_debug.c debug-line state (MAX_DEBUGLINES=256): debuglines[] holds the
 * DebugLineCreate handles passed to DebugLineShow, debuglinevisible[] the 0/1
 * shown flag. */
extern int numdebuglines;
extern int debuglinevisible[256];
extern int debuglines[256];

void AAS_ClearShownDebugLines(void);
void __cdecl AAS_DebugLine(vec3_t start, vec3_t end, int color);
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
