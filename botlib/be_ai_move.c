/*
 * be_ai_move.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10030A50..0x10034B90; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * botlib_local.h carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs and every prototype), so this file compiles in
 * exactly the environment these functions had before the split.
 */

#include "botlib_local.h"

int dword_1006295C = 0; // weak
libvar_t *libvar_laserhook; /* libvar handle */

//----- (10030A50) --------------------------------------------------------
double __cdecl AngleDiff(float ang1, float ang2)
{
  double result; // st7

  result = ang1 - ang2;
  if ( ang1 > ang2 )
  {
    if ( result > 180.0 )
      return result - 360.0;
  }
  else if ( result < -180.0 )
  {
    return result + 360.0;
  }
  return result;
}
//----- (10030AA0) --------------------------------------------------------
int __cdecl BotReachabilityArea(int *origin, int client)
{
  float v4; // st7
  int v5; // eax
  int v6; // esi
  int dx; // ebp
  int v8; // edi
  int v9; // esi
  int v12; // [esp+10h] [ebp-A4h]
  int dy; // [esp+14h] [ebp-A0h]
  int dz; // [esp+18h] [ebp-9Ch]
  /* start/end must be real vec3_t locals: both are passed by address to
   * AAS_PointAreaNum / AAS_TraceClientBBox / AAS_TraceAreas, and split into
   * separate slots BotReachabilityArea always returns 0 and the bot goes
   * inert. */
  vec3_t start; // [esp+1Ch] [ebp-98h] BYREF
  int v18; // [esp+28h] [ebp-8Ch]
  int v19; // [esp+2Ch] [ebp-88h]
  vec3_t end; // [esp+30h] [ebp-84h] BYREF
  float fdz; // [esp+3Ch] [ebp-78h]
  float fdy; // [esp+40h] [ebp-74h]
  aas_trace_t trace; // [esp+44h] [ebp-70h] (was int v25[9] + char v27[36] hidden return buffer)
  int v26[10]; // [esp+68h] [ebp-4Ch] BYREF

  for ( v19 = 0; v19 < 2; v19++ )
  {
    VectorCopy(((float *)origin), start);
    if ( v19 > 0 )
    {
      v4 = ((float *)origin)[2];
      end[0] = ((float *)origin)[0];
      end[1] = ((float *)origin)[1];
      end[2] = v4 - 800.0f;
      trace = AAS_TraceClientBBox((float *)origin, end, 4, -1);
      if ( !trace.startsolid )
      {
        VectorCopy(trace.endpos, start);
      }
    }
    v12 = 0;
    v5 = AAS_PointAreaNum(start);
    v6 = v5;
    if ( v5 )
    {
      v12 = v5;
      if ( AAS_AreaReachability(v5) )
        return v6;
    }
    for ( dz = 5; dz >= -5; dz -= 5 )
    {
      dy = 5;
      fdz = (float)dz;
      while ( 2 )
      {
        dx = 5;
        v18 = 5;
        fdy = (float)dy;
        do
        {
          end[0] = fdz + start[0];
          end[1] = fdy + start[1];
          end[2] = (float)v18 + start[2];
          /* thunk 0x10001C8F -> AAS_TraceAreas */
          v8 = AAS_TraceAreas(start, end, (int *)v26, 10);
          if ( v8 > 0 )
          {
            if ( !v12 )
              v12 = v26[0];
            for ( v9 = 0; v9 < v8; ++v9 )
            {
              if ( AAS_AreaReachability(v26[v9]) )
                return v26[v9];
            }
          }
          dx -= 5;
          v18 = dx;
        }
        while ( dx >= -5 );
        dy -= 5;
        if ( dy >= -5 )
          continue;
        break;
      }
    }
    if ( !client )
      break;
  }
  return v12;
}
//----- (10030D00) --------------------------------------------------------
BOOL __cdecl BotOnMover(float *origin, int entnum, aas_reachability_t* reach)
{
  /* origin is a vec3 pointer and reach a reach_t pointer; as ints they truncate
   * the callers' pointers (BotTravel_Elevator passes an intptr_t reach). */
  int v3; // ecx
  int i; // edi
  /* org/end/boxmins/boxmaxs/maxs/mins/modelorigin are all plain vec3_t — the
   * float typing is what lets cl.exe strength-reduce the compare loop and emit
   * the per-store float immediates below. */
  vec3_t org; // [esp+10h] [ebp-B4h] BYREF
  vec3_t angles; // [esp+1Ch] [ebp-A8h] BYREF
  vec3_t boxmins; // [esp+28h] [ebp-9Ch] BYREF
  vec3_t boxmaxs; // [esp+34h] [ebp-90h] BYREF
  vec3_t end; // [esp+40h] [ebp-84h] BYREF
  vec3_t maxs; // [esp+4Ch] [ebp-78h] BYREF
  vec3_t modelorigin; // [esp+58h] [ebp-6Ch] BYREF
  vec3_t mins; // [esp+64h] [ebp-60h] BYREF
  int trace[21]; // [esp+70h] [ebp-54h] BYREF

  v3 = reach->traveltype;
  /* Float lvalues, so each of these nine stores emits its own immediate — as int
   * bit patterns cl.exe would CSE the constant into a register.  The angles
   * zeroing must come FIRST, and all of these must be assignments rather than
   * declaration initialisers. */
  angles[0] = 0.0f;
  angles[1] = 0.0f;
  angles[2] = 0.0f;
  boxmins[0] = -16.0f;
  boxmins[1] = -16.0f;
  boxmins[2] = -8.0f;
  boxmaxs[0] = 16.0f;
  boxmaxs[1] = 16.0f;
  boxmaxs[2] = 8.0f;
  if ( v3 == 11 )
  {
    AAS_BSPModelMinsMaxsOrigin(reach->facenum, angles, mins, maxs, modelorigin);
    /* Plain vec3_t arrays indexed by the loop counter: cl.exe /O2 then does the
     * strength reduction itself, picking `origin` as the induction pointer and
     * expressing maxs/mins/modelorigin as base differences, exactly as the
     * original does.  Byte arithmetic blocks that, and writing the differences
     * out as named locals is NOT equivalent — let the compiler derive them. */
    for ( i = 0; i < 2; i++ )
    {
      if ( maxs[i] + modelorigin[i] + 16.0f < origin[i] )
        return 0;
      if ( origin[i] < mins[i] + modelorigin[i] - 16.0f )
        return 0;
    }
    /* Two VectorCopys from ONE source with the [2] copies forwarded into the
     * adjustments, so the surviving integer copies fill the fld->fadd gap.  The
     * STATEMENT ORDER is load-bearing: copy-adjust/copy-adjust (as Q3 writes it)
     * puts the fadd after the int loads and sinks both fstps into the argument
     * pushes; copy/copy/adjust/adjust does not. */
    VectorCopy(origin, org);
    org[2] += 24.0f;
    VectorCopy(origin, end);
    end[2] -= 48.0f;
    *(bsp_trace_t *)trace = AAS_Trace(org, boxmins, boxmaxs, end, entnum, 33619971);
    return !trace[1] && !trace[0] && trace[20] && AAS_EntityModelNum(trace[20]) == reach->facenum;
  }
  return 0;
}
//----- (10030F10) --------------------------------------------------------
BOOL __cdecl MoverDown(aas_reachability_t* reach)
{
  vec3_t angles; // [esp+4h] [ebp-30h] BYREF
  vec3_t origin; // [esp+10h] [ebp-24h] BYREF
  vec3_t maxs; // [esp+1Ch] [ebp-18h] BYREF
  vec3_t mins; // [esp+28h] [ebp-Ch] BYREF

  angles[0] = 0;
  angles[1] = 0;
  angles[2] = 0;
  if ( reach->traveltype != 11 )
    return 0;
  AAS_BSPModelMinsMaxsOrigin(reach->facenum, angles, (float *)mins, (float *)maxs, (float *)origin);
  if ( !AAS_OriginOfMoverWithModelNum(reach->facenum, origin) )
  {
    botimport.Print(PRT_MESSAGE, "no entity with model %d\n", reach->facenum);
    return 0;
  }
  return origin[2] + maxs[2] < reach->start[2];
}
//----- (10030FE0) --------------------------------------------------------
BOOL __cdecl BotValidTravel(float *a1, int a2, aas_reachability_t *a3, int a4)
{
  return (~a4 & AAS_TravelFlagForType(a3->traveltype)) == 0;
}
//----- (10031010) --------------------------------------------------------
void __cdecl BotAddToAvoidReach(intptr_t ms_, int number, float avoidtime)
{
  int i;
  bot_movestate_t *ms = (bot_movestate_t *)ms_;

  for ( i = 0; i < 1; i++ )
  {
    if ( ms->avoidreach[i] == number )
    {
      if ( ms->avoidreachtimes[i] > AAS_Time() )
        ++ms->avoidreachtries[i];
      else
        ms->avoidreachtries[i] = 1;
      ms->avoidreachtimes[i] = AAS_Time() + avoidtime;
      return;
    }
  }
  for ( i = 0; i < 1; i++ )
  {
    if ( ms->avoidreachtimes[i] < AAS_Time() )
    {
      ms->avoidreach[i] = number;
      ms->avoidreachtimes[i] = AAS_Time() + avoidtime;
      ms->avoidreachtries[i] = 1;
      return;
    }
  }
}
//----- (100310E0) --------------------------------------------------------
int __cdecl BotGetReachabilityToGoal(float *origin, int areanum, int entnum, int lastgoalareanum, int lastareanum, int *avoidreach, float *avoidreachtimes, int *avoidreachtries, bot_goal_t *goal, int travelflags)
{
  int reachnum; // ebp
  int i; // esi
  int t; // eax
  int besttime; // [esp+8h] [ebp-60h]
  int bestreachnum; // [esp+Ch] [ebp-5Ch]
  aas_reachability_t reach; // [esp+10h] [ebp-58h] BYREF

  besttime = 0;
  bestreachnum = 0;
  reachnum = AAS_NextAreaReachability(areanum, 0);
  if ( reachnum )
  {
  do
  {
    for ( i = 0; i < 1; i++ )
    {
      if ( avoidreach[i] == reachnum && avoidreachtimes[i] >= AAS_Time() )
        break;
    }
    if ( i == 1 || avoidreachtries[i] <= 4 )
    {
      reach = AAS_ReachabilityFromNum(reachnum);
      if ( entnum != goal->areanum || reach.areanum != lastgoalareanum )
      {
        if ( BotValidTravel(origin, lastareanum, &reach, travelflags) )
        {
          t = (unsigned __int16)AAS_AreaTravelTimeToGoalArea(reach.areanum, goal->areanum, travelflags);
          if ( t )
          {
            t += (unsigned __int16)reach.traveltime;
            if ( !besttime || t < besttime )
            {
              besttime = t;
              bestreachnum = reachnum;
            }
          }
        }
      }
    }
    reachnum = AAS_NextAreaReachability(areanum, reachnum);
  }
  while ( reachnum );
  }
  return bestreachnum;
}
//----- (10031270) --------------------------------------------------------
int __cdecl BotMovementViewTarget(bot_movestate_t *ms, bot_goal_t *goal, int travelflags, float *target)
{
  int reachnum; // eax
  aas_reachability_t reach; // [esp+10h] [ebp-58h] BYREF

  if ( !ms->lastreachnum || !goal )
    return 0;
  reach = AAS_ReachabilityFromNum(ms->lastreachnum);
  reachnum = BotGetReachabilityToGoal(
         reach.end,
         reach.areanum,
         ms->lastgoalareanum,
         ms->lastareanum,
         ms->entitynum,
         ms->avoidreach,
         ms->avoidreachtimes,
         ms->avoidreachtries,
         goal,
         travelflags);
  if ( !reachnum )
    return 0;
  reach = AAS_ReachabilityFromNum(reachnum);
  target[0] = reach.end[0];
  target[2] = reach.end[2] - 15.0f;
  *(int *)&target[1] = *(int *)&reach.end[1];
  return 1;
}
//----- (10031380) --------------------------------------------------------
void __cdecl MoverBottomCenter(aas_reachability_t *reach, vec3_t bottomcenter)
{
  vec3_t angles; // [esp+4h] [ebp-3Ch] BYREF
  vec3_t mins; // [esp+10h] [ebp-30h] BYREF
  vec3_t maxs; // [esp+1Ch] [ebp-24h] BYREF
  vec3_t mids; // [esp+28h] [ebp-18h] BYREF
  vec3_t origin; // [esp+34h] [ebp-Ch] BYREF

  angles[0] = 0;
  angles[1] = 0;
  angles[2] = 0;
  if ( reach->traveltype == 11 )
  {
    AAS_BSPModelMinsMaxsOrigin(reach->facenum, angles, mins, maxs, (float *)origin);
    *(float *)mids = mins[0] + maxs[0];
    mids[1] = mins[1] + maxs[1];
    mids[2] = mins[2] + maxs[2];
    VectorMA((float *)origin, 0.5, (float *)mids, bottomcenter);
    bottomcenter[2] = reach->start[2];
  }
}
//----- (10031450) --------------------------------------------------------
float __cdecl BotGapDistance(bot_movestate_t *ms, float *dir)
{
  float startz; // [esp+10h] [ebp-64h]
  /* end/start are real vec3_t and ALL three components of each are set before
   * every AAS_TraceClientBBox / PointContents call.  Miss the end[1]/end[2]
   * stores and the trace returns a near-random dist, which BotTravel_Walk turns
   * into `speed = 2 * dist` — the bot then crawls and hitches. */
  vec3_t end; // [esp+14h] [ebp-60h] BYREF — trace end vector
  vec3_t start; // [esp+20h] [ebp-54h] BYREF — trace start vector (was start/v14/v15)
  aas_trace_t trace; // [esp+2Ch] [ebp-48h] (was int v16[9] + char v17[36] hidden return buffer)
  float dist; // [esp+78h] [ebp+4h]

  VectorCopy(ms->origin, start);
  VectorCopy(ms->origin, end);
  end[2] -= 60.0f;
  trace = AAS_TraceClientBBox(start, end, 4, -1);
  startz = trace.endpos[2] + 1.0f;
  for ( dist = 8.0f; dist <= 100.0f; dist += 8.0f )
  {
    VectorMA(ms->origin, dist, dir, start);
    start[2] = startz + 24.0f;
    VectorCopy(start, end);
    end[2] -= 48.0f + libvar_sv_maxbarrier->value;
    trace = AAS_TraceClientBBox(start, end, 4, -1);
    if ( !trace.startsolid )
    {
      //if it is a gap
      if ( trace.endpos[2] < startz - libvar_sv_step->value - 8.0f )
      {
        VectorCopy(trace.endpos, end);
        end[2] -= 20.0f;
        /* barrier-jump under-water check */
        if ( (sub_10003080((float *)end) & 0x20) != 0 )
          break;
        return dist;
      }
      startz = trace.endpos[2];
    }
  }
  return 0.0f;
}
//----- (10031650) --------------------------------------------------------
int __cdecl BotCheckBarrierJump(bot_movestate_t *ms, float *dir, float speed)
{
  int result; // eax
  float v11; // [esp+0h] [ebp-84h]
  /* end/start are real vec3_t with all three components set before every
   * AAS_TraceClientBBox / VectorMA call. */
  vec3_t end; // [esp+18h] [ebp-6Ch] BYREF
  vec3_t start; // [esp+24h] [ebp-60h] BYREF
  vec3_t hordir; // [esp+30h] [ebp-54h] BYREF
  aas_trace_t trace; // [esp+3Ch] [ebp-48h] (was int v19[9] + char v20[36] hidden return buffer)

  VectorCopy(ms->origin, end);
  end[2] = end[2] + libvar_sv_maxbarrier->value;
  trace = AAS_TraceClientBBox(ms->origin, end, 2, ms->entitynum);
  if ( trace.startsolid )
    return 0;
  if ( trace.endpos[2] - ms->origin[2] < libvar_sv_step->value )
    return 0;
  hordir[0] = dir[0];
  hordir[1] = dir[1];
  hordir[2] = 0;
  VectorNormalize(hordir);
  v11 = speed * ms->thinktime * 0.5;
  VectorMA(ms->origin, v11, hordir, end);
  VectorCopy(trace.endpos, start);
  end[2] = trace.endpos[2];
  trace = AAS_TraceClientBBox(start, end, 2, ms->entitynum);
  if ( trace.startsolid )
    return 0;
  VectorCopy(trace.endpos, start);
  end[0] = trace.endpos[0];
  end[1] = trace.endpos[1];
  end[2] = ms->origin[2];
  trace = AAS_TraceClientBBox(start, end, 2, ms->entitynum);
  if ( trace.startsolid )
    return 0;
  if ( trace.fraction >= 1.0 )
    return 0;
  if ( trace.endpos[2] - ms->origin[2] < libvar_sv_step->value )
    return 0;
  EA_Jump(ms->client);
  EA_Move(ms->client, hordir, speed);
  result = 1;
  ms->moveflags |= 1u;
  return result;
}
//----- (100318D0) --------------------------------------------------------
/* `type` is declared but unused — the original BotMoveInDirection pushes the
 * same 4 args (ms, dir, speed, type) to BOTH the swim and walk branches and
 * shares the arg-push, so this __cdecl callee must accept 4 args even though
 * the swim path ignores `type`. */
int __cdecl BotSwimInDirection(bot_movestate_t *ms, float *dir, float speed, int type)
{
  int v3; // edx
  int v4; // eax
  /* int[3]: the stores are raw 32-bit copies of dir's float bit patterns.  As
   * float[3] they become int->float conversions and the swim direction is
   * destroyed (0.5f becomes ~1.06e9). */
  int normdir[3]; // [esp+0h] [ebp-Ch] BYREF

  v3 = *(int *)&dir[1];
  v4 = *(int *)&dir[2];
  normdir[0] = *(int *)&dir[0];
  normdir[1] = v3;
  normdir[2] = v4;
  VectorNormalize((float *)normdir);
  EA_Move(ms->client, (float *)normdir, speed);
  return 1;
}
//----- (10031940) --------------------------------------------------------
int __cdecl BotWalkInDirection(bot_movestate_t *ms, float *dir, float speed, int type)
{
  int v5; // eax
  int presencetype; // edi
  float v10; // st7
  int maxframes; // ebx
  float v13; // st7
  /* Real vec3_t (see the BotTravel_Walk note); named hordir, as in Q3, to free
   * `dir` for the input direction. */
  vec3_t hordir; // [esp+8h] [ebp-68h] BYREF (was v14 + 8 unnamed bytes)
  vec3_t cmdmove; // [esp+14h] [ebp-5Ch] BYREF
  aas_clientmove_t move; // [esp+20h] [ebp-50h] BYREF (coalesced with the by-value return temp)
  int v19; // [esp+74h] [ebp+4h]
  float v20; // [esp+78h] [ebp+8h]

  v5 = ms->moveflags;
  if ( (v5 & 2) != 0 )
  {
    if ( !BotCheckBarrierJump(ms, dir, speed) )
    {
      if ( (type & 2) == 0 || (presencetype = 4, (type & 4) != 0) )
        presencetype = 2;
      hordir[0] = dir[0];
      hordir[1] = dir[1];
      hordir[2] = 0.0f;
      VectorNormalize(hordir);
      if ( (type & 4) == 0 )
      {
        if ( BotGapDistance(ms, hordir) > 0.0f )
        {
          type |= 4;
        }
      }
      VectorScale(hordir, speed, (float *)cmdmove);
      v19 = type & 4;
      if ( (type & 4) != 0 )
      {
        v10 = 3.0f;
        cmdmove[2] = libvar_sv_jumpvel->value;
        v20 = ms->thinktime;
      }
      else
      {
        v10 = 2.0f;
        v20 = ms->thinktime;
      }
      maxframes = (__int64)(v10 / v20);
      move = AAS_ClientMovementPrediction(
                        ms->entitynum,
                        ms->origin,
                        presencetype,
                        1,
                        ms->velocity,
                        cmdmove,
                        maxframes,
                        maxframes,
                        v20,
                        61,
                        0);
      if ( move.frames >= maxframes )
        return 0;
      if ( (move.stopevent & 0x38) != 0 )
        return 0;
      v13 = move.endpos[0] - ms->origin[0];
      hordir[0] = v13;
      hordir[2] = 0.0f;
      hordir[1] = move.endpos[1] - ms->origin[1];
      if ( VectorLength(hordir) < speed * ms->thinktime * 0.5 )
        return 0;
      if ( v19 )
        EA_Jump(ms->client);
      if ( (type & 2) != 0 )
        EA_Crouch(ms->client);
      EA_Move(ms->client, hordir, speed);
    }
  }
  else if ( (v5 & 1) != 0 && ms->velocity[2] < 50.0f )
  {
    EA_Move(ms->client, dir, speed);
  }
  return 1;
}
//----- (10031BE0) --------------------------------------------------------
/* BotMoveInDirection (was sub_10031BE0) — the public movement dispatcher:
 * route (ms, dir, speed, type) to BotSwimInDirection when the bot's origin is
 * in liquid, else to BotWalkInDirection.  The original pushes the four args
 * once and `je`s to pick the target, so BotSwimInDirection is a 4-arg callee
 * too even though it ignores `type`. */
int __cdecl BotMoveInDirection(bot_movestate_t *movestate, float *dir, float speed, int type)
{
  if ( AAS_Swimming(movestate->origin) )
    return BotSwimInDirection(movestate, dir, speed, type);
  else
    return BotWalkInDirection(movestate, dir, speed, type);
}
//----- (10031C30) --------------------------------------------------------
// 2D line-line intersection (infinite lines through {p1,p2} and {p3,p4}).
// Returns 0 if the lines are parallel (det == 0); otherwise writes the
// truncated intersection-point coords (via _ftol, i.e. C casts to int)
// to out[0]/out[1] and returns 1.  Derived directly from the FPU
// sequence at 10031C30 — det = (p2.x-p1.x)*(p4.y-p3.y) -
// (p2.y-p1.y)*(p4.x-p3.x), the two side-crosses c1 / c2 are formed
// against p1 and p3 respectively, and the result is the rational
// formula (c2*d1 - c1*d2)/det per coordinate.  DEAD in Gladiator —
// /INCREMENTAL.
int __cdecl Intersection(float *p1, float *p2, float *p3, float *p4, float *out)
{
  float d1x = p2[0] - p1[0];
  float d1y = p2[1] - p1[1];
  float d2x = p4[0] - p3[0];
  float d2y = p4[1] - p3[1];
  float det = d2x * d1y - d2y * d1x;
  float c1;
  float c2;

  if ( det != 0.0f )
  {
    c1 = d1x * p1[1] - d1y * p1[0];
    c2 = d2x * p3[1] - d2y * p3[0];
    out[0] = (int)((c2 * d1x - c1 * d2x) / det);
    out[1] = (int)((c2 * d1y - c1 * d2y) / det);
    return 1;
  }
  return 0;
}
//----- (10031D10) --------------------------------------------------------
int __cdecl BotCheckBlocked(bot_movestate_t *ms, float *dir, bot_moveresult_t *moveresult)
{
  int result; // eax
  int v4; // ecx
  /* mins/maxs/end must be real vec3_t: otherwise
   * AAS_PresenceTypeBoundingBox writes mins[2]/maxs[2] into stray slots and
   * AAS_Trace reads garbage past the bounding box, blocking every movement
   * with phantom collisions. */
  vec3_t maxs; // [esp+8h] [ebp-78h] BYREF (was _DWORD v5[2] + float v6)
  vec3_t mins; // [esp+14h] [ebp-6Ch] BYREF (was _DWORD v7[2] + float v8)
  vec3_t end;  // [esp+20h] [ebp-60h] BYREF (was float v9[3])
  int trace[21]; // [esp+2Ch] [ebp-54h] BYREF

  AAS_PresenceTypeBoundingBox(ms->presencetype, mins, maxs);
  if ( fabs(dir[2]) < 0.7f )
  {
    mins[2] = mins[2] + libvar_sv_step->value;
    maxs[2] = maxs[2] - 10.0f;
  }
  VectorMA(ms->origin, 3.0f, dir, end);
  *(bsp_trace_t *)trace = AAS_Trace(ms->origin, mins, maxs, end, ms->entitynum, 33619971);
  result = trace[1];
  if ( !trace[1] )
  {
    v4 = trace[20];
    if ( trace[20] )
    {
      result = (int)(intptr_t)moveresult;
      moveresult->blocked = 1;
      moveresult->blockentity = v4;
    }
  }
  return result;
}
//----- (10031E20) --------------------------------------------------------
bot_moveresult_t *__cdecl BotClearMoveResult(bot_moveresult_t *moveresult)
{
  bot_moveresult_t *result; // eax

  result = moveresult;
  moveresult->failure = 0;
  moveresult->type = 0;
  moveresult->blocked = 0;
  moveresult->blockentity = 0;
  moveresult->traveltype = 0;
  moveresult->flags = 0;
  return result;
}
//----- (10031E50) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_Walk(bot_movestate_t *ms, aas_reachability_t *reach)
{
  float v4; // st7
  float v5; // st7
  /* Real vec3_t: split into separate locals, VectorNormalize / EA_Move read
   * garbage as dir[2] and the bot sends near-vertical movement, which the game
   * rejects as upmove. */
  vec3_t dir; // [esp+8h] [ebp-3Ch] BYREF
  bot_moveresult_t moveresult; // [esp+14h] [ebp-30h] BYREF
  float dist; // [esp+50h] [ebp+Ch]
  float speed; // [esp+50h] [ebp+Ch]

  BotClearMoveResult(&moveresult);
  dir[2] = 0.0f;
  dir[0] = reach->start[0] - ms->origin[0];
  dir[1] = reach->start[1] - ms->origin[1];
  dist = VectorNormalize(dir);
  BotCheckBlocked(ms, dir, &moveresult);
  v4 = dist;
  if ( dist < 10.0f )
  {
    v5 = reach->end[0] - ms->origin[0];
    dir[2] = 0.0f;
    dir[0] = v5;
    dir[1] = reach->end[1] - ms->origin[1];
    v4 = VectorNormalize(dir);
    dist = v4;
  }
  if ( (AAS_AreaPresenceType(reach->areanum) & 2) == 0 )
  {
    if ( dist < 20.0f )
      EA_Crouch(ms->client);
  }
  /* BotGapDistance's FPU return is what the 0.0 compare and the doubling below
   * consume — not the dir length computed above.  Confusing the two makes speed
   * 2 * distance-to-goal instead of 2 * jump-distance, so the bot crawls toward
   * every goal and stalls as the distance approaches 0. */
  v4 = BotGapDistance(ms, dir);
  if ( v4 > 0.0f )
    speed = 300.0f - (300.0f - (v4 + v4));
  else
    speed = 400.0f;
  EA_Move(ms->client, dir, speed);
  VectorCopy(dir, moveresult.movedir);
  return moveresult;
}
//----- (10031FE0) --------------------------------------------------------
// 2D-direction "finish travel walk" helper.  Q3's sibling is
// BotFinishTravel_Walk: walk straight to reach->end with speed capped
// at 100 units before the preserved `400 - (400 - 3*dist)` form.
// DEAD in Gladiator — only kept because /INCREMENTAL left it in the
// original DLL; Q3 cognate in be_ai_move.c.
bot_moveresult_t __cdecl BotFinishTravel_Walk(bot_movestate_t *ms, aas_reachability_t *reach)
{
  bot_moveresult_t moveresult;
  vec3_t hordir;
  float dist;
  float speed;

  BotClearMoveResult(&moveresult);
  hordir[0] = reach->end[0] - ms->origin[0];
  hordir[1] = reach->end[1] - ms->origin[1];
  hordir[2] = 0.0f;
  dist = VectorNormalize(hordir);
  if ( dist > 100.0f )
    dist = 100.0f;
  /* Faithful original oddity: algebraically this is just 3*dist, but the
   * original emits the nested subtraction as two consecutive fsubr, so keep the
   * literal form. */
  speed = 400.0f - (400.0f - 3.0f * dist);
  EA_Move(ms->client, hordir, speed);
  VectorCopy(hordir, moveresult.movedir);
  return moveresult;
}
//----- (100320C0) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_Crouch(bot_movestate_t *ms, aas_reachability_t *reach)
{
  /* Real vec3_t — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+8h] [ebp-3Ch] BYREF
  bot_moveresult_t moveresult; // [esp+14h] [ebp-30h] BYREF

  BotClearMoveResult(&moveresult);
  dir[2] = 0.0f;
  dir[0] = reach->end[0] - ms->origin[0];
  dir[1] = reach->end[1] - ms->origin[1];
  VectorNormalize(dir);
  BotCheckBlocked(ms, dir, &moveresult);
  EA_Crouch(ms->client);
  EA_Move(ms->client, dir, 400.0);
  VectorCopy(dir, moveresult.movedir);
  return moveresult;
}
//----- (10032190) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_BarrierJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
  float speed; // [esp+0h] [ebp-48h]
  /* Real vec3_t — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+Ch] [ebp-3Ch] BYREF
  bot_moveresult_t moveresult; // [esp+18h] [ebp-30h] BYREF
  float dist; // [esp+54h] [ebp+Ch]

  BotClearMoveResult(&moveresult);
  dir[2] = 0.0f;
  dir[0] = reach->start[0] - ms->origin[0];
  dir[1] = reach->start[1] - ms->origin[1];
  dist = VectorNormalize(dir);
  BotCheckBlocked(ms, dir, &moveresult);
  if ( dist < 7.0f )
  {
    EA_Jump(ms->client);
  }
  else
  {
    if ( dist > 60.0f )
      dist = 60.0f;
    speed = 360.0f - (360.0f - dist * 6.0f);
    EA_Move(ms->client, dir, speed);
  }
  VectorCopy(dir, moveresult.movedir);
  return moveresult;
}
//----- (100322C0) --------------------------------------------------------
bot_moveresult_t __cdecl BotFinishTravel_BarrierJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
  float v5; // [esp+0h] [ebp-48h]
  /* Real vec3_t — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+Ch] [ebp-3Ch] BYREF
  bot_moveresult_t moveresult; // [esp+18h] [ebp-30h] BYREF
  float dist; // [esp+50h] [ebp+8h]

  BotClearMoveResult(&moveresult);
  if ( ms->velocity[2] < 250.0f )
  {
    dir[2] = 0.0f;
    dir[0] = reach->end[0] - ms->origin[0];
    dir[1] = reach->end[1] - ms->origin[1];
    dist = VectorNormalize(dir);
    BotCheckBlocked(ms, dir, &moveresult);
    if ( dist > 60.0f )
      dist = 60.0f;
    v5 = 400.0f - (400.0f - dist * 6.0f);
    EA_Move(ms->client, dir, v5);
    VectorCopy(dir, moveresult.movedir);
  }
  return moveresult;
}
//----- (100323E0) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_Swim(bot_movestate_t *ms, aas_reachability_t *reach)
{
  /* Real vec3_t — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+8h] [ebp-3Ch] BYREF
  bot_moveresult_t moveresult; // [esp+14h] [ebp-30h] BYREF

  BotClearMoveResult(&moveresult);
  VectorSubtract(reach->start, ms->origin, dir);
  VectorNormalize(dir);
  BotCheckBlocked(ms, dir, &moveresult);
  EA_Move(ms->client, dir, 400.0);
  VectorCopy(dir, moveresult.movedir);
  vectoangles(dir, moveresult.ideal_viewangles);
  moveresult.flags |= 2;
  return moveresult;
}
//----- (100324C0) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_WaterJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
  int v4; // eax
  /* Real vec3_t — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+8h] [ebp-48h] BYREF
  vec3_t hordir; // [esp+14h] [ebp-3Ch] BYREF
  bot_moveresult_t moveresult; // [esp+20h] [ebp-30h] BYREF
  float dist; // [esp+5Ch] [ebp+Ch]

  BotClearMoveResult(&moveresult);
  VectorSubtract(reach->end, ms->origin, dir);
  VectorCopy(dir, hordir);
  hordir[2] = 0.0;
  v4 = rand();
  dir[2] = (2 * ((float)(v4 & 0x7FFF) * 0.000030518509f - 0.5)) * 40.0
         + dir[2]
         + 15.0;
  VectorNormalize(dir);
  dist = VectorNormalize(hordir);
  EA_MoveForward(ms->client);
  if ( dist < 40.0f )
    EA_MoveUp(ms->client);
  vectoangles(dir, moveresult.ideal_viewangles);
  moveresult.flags |= 1;
  VectorCopy(dir, moveresult.movedir);
  return moveresult;
}
//----- (10032620) --------------------------------------------------------
bot_moveresult_t __cdecl BotFinishTravel_WaterJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
  int v6; // eax
  int v7; // eax
  int v8; // eax
  /* Real vec3_t — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+8h] [ebp-48h] BYREF
  vec3_t pnt; // [esp+14h] [ebp-3Ch] BYREF
  bot_moveresult_t moveresult; // [esp+20h] [ebp-30h] BYREF

  BotClearMoveResult(&moveresult);
  if ( (ms->moveflags & 0x10) == 0 )
  {
    VectorCopy(ms->origin, pnt);
    pnt[2] -= 32.0f;
    if ( (AAS_PointContents(pnt) & 0x38) != 0 )   /* under-foot liquid check */
    {
      VectorSubtract(reach->end, ms->origin, dir);
      v6 = rand();
      dir[0] = (2 * ((float)(v6 & 0x7FFF) * 0.000030518509f - 0.5))
             * 10.0
             + dir[0];
      v7 = rand();
      dir[1] = (2 * ((float)(v7 & 0x7FFF) * 0.000030518509f - 0.5))
             * 10.0
             + dir[1];
      v8 = rand();
      dir[2] = (2 * ((float)(v8 & 0x7FFF) * 0.000030518509f - 0.5))
             * 10.0
             + dir[2]
             + 70.0;
      VectorNormalize(dir);
      EA_Move(ms->client, dir, 400.0f);
      vectoangles(dir, moveresult.ideal_viewangles);
      moveresult.flags |= 1;
      VectorCopy(dir, moveresult.movedir);
    }
  }
  return moveresult;
}
//----- (100327F0) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_WalkOffLedge(bot_movestate_t *ms, aas_reachability_t *reach)
{
  float *v4; // ebx
  float dist; // st7
  float v6; // st7
  float v7; // st7
  float speed; // [esp+10h] [ebp-4Ch] BYREF
  /* Real vec3_t locals — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+14h] [ebp-48h] BYREF (was v10/v11/v12)
  vec3_t pos; // [esp+20h] [ebp-3Ch] BYREF (was v13/v14/<hole>)
  bot_moveresult_t moveresult; // [esp+2Ch] [ebp-30h] BYREF
  float v17; // [esp+68h] [ebp+Ch]

  BotClearMoveResult(&moveresult);
  v4 = reach->end;
  VectorSubtract(reach->end, ms->origin, pos);
  BotCheckBlocked(ms, pos, &moveresult);
  dir[0] = pos[0];
  dir[1] = pos[1];
  dir[2] = 0.0f;
  dist = VectorNormalize(dir);
  v17 = dist;
  if ( dist < 60.0f )
  {
    v6 = *v4 - ms->origin[0];
    dir[2] = 0.0f;
    dir[0] = v6;
    dir[1] = reach->end[1] - ms->origin[1];
    VectorNormalize(dir);
    v7 = *v4 - reach->start[0];
    pos[0] = v7;
    pos[1] = reach->end[1] - reach->start[1];
    pos[2] = 0.0f;
    if ( (float)VectorLength(pos) < 15.0f )
    {
      if ( v17 > 0.0f && v17 < 150.0f )
        speed = 380.0f - (300.0f - (v17 + v17));
      else
        speed = 400.0f;
    }
    else
    {
      if ( !AAS_HorizontalVelocityForJump(0.0, reach->start, v4, &speed) )
        speed = 200.0f;
    }
  }
  else
  {
    speed = 400.0f;
  }
  BotCheckBlocked(ms, dir, &moveresult);
  EA_Move(ms->client, dir, speed);
  VectorCopy(dir, moveresult.movedir);
  return moveresult;
}
//----- (10032A00) --------------------------------------------------------
bot_moveresult_t __cdecl BotFinishTravel_WalkOffLedge(bot_movestate_t *ms, aas_reachability_t *reach)
{
  /* Real vec3_t locals — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+8h] [ebp-48h] BYREF (was v4/v5/v6)
  vec3_t pos; // [esp+14h] [ebp-3Ch] BYREF (was v7/v8/<hole>)
  bot_moveresult_t moveresult; // [esp+20h] [ebp-30h] BYREF

  BotClearMoveResult(&moveresult);
  VectorSubtract(reach->end, ms->origin, pos);
  BotCheckBlocked(ms, pos, &moveresult);
  dir[0] = pos[0];
  dir[1] = pos[1];
  dir[2] = 0.0f;
  VectorNormalize(dir);
  EA_Move(ms->client, dir, 400.0);
  VectorCopy(dir, moveresult.movedir);
  return moveresult;
}
//----- (10032AE0) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_Jump(bot_movestate_t *ms, aas_reachability_t *reach)
{
  float speed; // [esp+0h] [ebp-90h]
  float dist1; // [esp+10h] [ebp-80h]  reused: loop counter dist1, then botd length (was v13)
  float dist2; // [esp+14h] [ebp-7Ch]  reused: dist1+10 temp, then predd length dist2 (was v15)
  /* Real vec3_t locals — see the BotTravel_Walk note. */
  vec3_t hordir;   // [esp+18h] [ebp-78h] BYREF (was v16/v17/v18)
  vec3_t runstart; // [esp+24h] [ebp-6Ch] BYREF (was v19/v20/<hole>)
  vec3_t dir2; // [esp+30h] [ebp-60h] BYREF (was v21/v22/v23)
  vec3_t dir1;  // [esp+3Ch] [ebp-54h] BYREF (was v24/v25/v26)
  vec3_t start; // [esp+48h] [ebp-48h] BYREF
  /* Real vec3_t, as in AAS_NearbySolidOrGap: VectorMA writes three floats here
   * and the z-bump below has to land in the same slot. */
  vec3_t end; // [esp+54h] [ebp-3Ch] BYREF
  bot_moveresult_t moveresult; // [esp+60h] [ebp-30h] BYREF

  BotClearMoveResult(&moveresult);
  AAS_JumpReachRunStart(reach, (intptr_t)runstart);
  hordir[0] = runstart[0] - reach->start[0];
  hordir[1] = runstart[1] - reach->start[1];
  hordir[2] = 0.0f;
  VectorNormalize(hordir);
  VectorCopy(reach->start, start);
  start[2] += 1.0f;
  VectorMA(reach->start, 80.0f, hordir, runstart);
  for ( dist1 = 0.0f; dist1 < 80.0f; dist1 = dist1 + 10.0f )
  {
    dist2 = dist1 + 10.0f;
    VectorMA(start, dist2, hordir, end);
    end[2] += 1.0f;
    if ( AAS_PointAreaNum(end) != ms->reachareanum )
      break;
  }
  if ( dist1 < 80.0f )
    VectorMA(reach->start, dist1, hordir, runstart);
  VectorSubtract(ms->origin, reach->start, dir1);
  dir1[2] = 0.0f;
  dist1 = VectorNormalize(dir1);
  VectorSubtract(ms->origin, runstart, dir2);
  dir2[2] = 0.0f;
  dist2 = VectorNormalize(dir2);
  if ( DotProduct(dir1, dir2) < -0.8 || dist2 < 5.0f )
  {
    hordir[0] = reach->end[0] - ms->origin[0];
    hordir[1] = reach->end[1] - ms->origin[1];
    hordir[2] = 0.0f;
    VectorNormalize(hordir);
    /* Keep `dist1 < 24` first, so EA_Jump is the warm fall-through: the
     * equivalent `if (dist1 >= 24) {...} else Jump;` inverts the branch. */
    if ( dist1 < 24.0f )
    {
      EA_Jump(ms->client);
    }
    else if ( dist1 < 32.0f )
    {
      EA_DelayedJump(ms->client);
    }
    EA_Move(ms->client, hordir, 600.0f);
    ms->jumpreach = ms->lastreachnum;
  }
  else
  {
    hordir[0] = runstart[0] - ms->origin[0];
    hordir[1] = runstart[1] - ms->origin[1];
    hordir[2] = 0.0f;
    VectorNormalize(hordir);
    if ( dist2 > 80.0f )
      dist2 = 80.0f;
    speed = 400.0f - (400.0f - dist2 * 5.0f);
    EA_Move(ms->client, hordir, speed);
  }
  VectorCopy(hordir, moveresult.movedir);
  return moveresult;
}
//----- (10032E80) --------------------------------------------------------
bot_moveresult_t __cdecl BotFinishTravel_Jump(bot_movestate_t *ms, aas_reachability_t *reach)
{
  /* Real vec3_t locals — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+8h] [ebp-48h] BYREF (was v7/v8/v9)
  vec3_t reach_dir; // [esp+14h] [ebp-3Ch] BYREF (was v10/v11/v12)
  bot_moveresult_t moveresult; // [esp+20h] [ebp-30h] BYREF
  float dist; // [esp+58h] [ebp+8h]

  BotClearMoveResult(&moveresult);
  if ( ms->jumpreach )
  {
    dir[2] = 0.0f;
    dir[0] = reach->end[0] - ms->origin[0];
    dir[1] = reach->end[1] - ms->origin[1];
    dist = VectorNormalize(dir);
    reach_dir[2] = 0.0f;
    reach_dir[0] = reach->end[0] - reach->start[0];
    reach_dir[1] = reach->end[1] - reach->start[1];
    VectorNormalize(reach_dir);
    if ( ((reach_dir[2] * dir[2]) + reach_dir[1] * dir[1]) + reach_dir[0] * dir[0] >= -0.5 || dist >= 24.0f )
    {
      EA_Move(ms->client, dir, 800.0);
      VectorCopy(dir, moveresult.movedir);
    }
  }
  return moveresult;
}
//----- (10032FC0) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_Ladder(bot_movestate_t *ms, aas_reachability_t *reach)
{
  /* Real vec3_t — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+8h] [ebp-54h] BYREF (was v5/v6/v7)
  vec3_t viewdir; // [esp+14h] [ebp-48h] BYREF
  vec3_t origin; // [esp+20h] [ebp-3Ch] BYREF
  bot_moveresult_t moveresult; // [esp+2Ch] [ebp-30h] BYREF

  origin[0] = 0.0f;
  origin[1] = 0.0f;
  origin[2] = 0.0f;
  BotClearMoveResult(&moveresult);
  VectorSubtract(reach->end, ms->origin, dir);
  VectorNormalize(dir);
  viewdir[0] = dir[0];
  viewdir[1] = dir[1];
  viewdir[2] = dir[2] * 3.0f;
  vectoangles(viewdir, moveresult.ideal_viewangles);
  EA_Move(ms->client, origin, 0.0);
  EA_MoveForward(ms->client);
  moveresult.flags |= 1;
  VectorCopy(dir, moveresult.movedir);
  return moveresult;
}
//----- (100330E0) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_Teleport(bot_movestate_t *ms, aas_reachability_t *reach)
{
  int v4; // ecx
  /* Real vec3_t — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+8h] [ebp-3Ch] BYREF
  bot_moveresult_t moveresult; // [esp+14h] [ebp-30h] BYREF
  float dist; // [esp+4Ch] [ebp+8h]

  BotClearMoveResult(&moveresult);
  v4 = ms->moveflags;
  if ( (v4 & 0x20) == 0 )
  {
    VectorSubtract(reach->start, ms->origin, dir);
    if ( (v4 & 4) == 0 )
      dir[2] = 0.0f;
    dist = VectorNormalize(dir);
    BotCheckBlocked(ms, dir, &moveresult);
    if ( dist < 30.0f )
      EA_Move(ms->client, dir, 200.0);
    else
      EA_Move(ms->client, dir, 400.0);
    if ( (ms->moveflags & 4) != 0 )
      moveresult.flags |= 2;
    VectorCopy(dir, moveresult.movedir);
  }
  return moveresult;
}
//----- (10033210) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_Elevator(bot_movestate_t *ms, aas_reachability_t *reach)
{
  float v4; // st7
  float v5; // st7
  float v6; // st7
  char v7; // al
  char v9; // al
  char v11; // al
  float dist2; // st7
  char v14; // al
  float v17; // [esp+0h] [ebp-7Ch]
  float v18; // [esp+0h] [ebp-7Ch]
  /* Real vec3_t locals — see the BotTravel_Walk note. */
  vec3_t final; // [esp+10h] [ebp-6Ch] BYREF (was v19/v20/v21)
  vec3_t dir;   // [esp+1Ch] [ebp-60h] BYREF (was v22/v23/v24)
  vec3_t reachdir; // [esp+28h] [ebp-54h] BYREF (was v25/v26/v27; renamed from 'reach' to free the param name)
  vec3_t telegoaldir; // [esp+34h] [ebp-48h] BYREF (was v28/v29/v30)
  vec3_t telegoal; // [esp+40h] [ebp-3Ch] BYREF (was v31/v32/v33)
  bot_moveresult_t moveresult; // [esp+4Ch] [ebp-30h] BYREF
  int dist1; // [esp+84h] [ebp+8h]
  float v36; // [esp+88h] [ebp+Ch]
  float dist; // [esp+88h] [ebp+Ch]

  BotClearMoveResult(&moveresult);
  if ( BotOnMover(ms->origin, ms->entitynum, reach) )
  {
    if ( (float)abs((__int64)(ms->origin[2] - reach->end[2])) < libvar_sv_maxbarrier->value )
    {
      v4 = reach->end[0] - ms->origin[0];
      dir[2] = 0.0f;
      dir[0] = v4;
      dir[1] = reach->end[1] - ms->origin[1];
      VectorNormalize(dir);
      if ( !BotCheckBarrierJump(ms, dir, 100.0f) )
        EA_Move(ms->client, dir, 400.0f);
      VectorCopy(dir, moveresult.movedir);
    }
    else
    {
      MoverBottomCenter(reach, telegoal);
      v5 = telegoal[0] - ms->origin[0];
      dir[2] = 0.0f;
      dir[0] = v5;
      dir[1] = telegoal[1] - ms->origin[1];
      v6 = VectorNormalize(dir);
      if ( v6 > 5.0f )
      {
        if ( v6 > 100.0f )
          v6 = 100.0f;
        v17 = 400.0f - (400.0f - v6 * 4.0f);
        EA_Move(ms->client, dir, v17);
        VectorCopy(dir, moveresult.movedir);
      }
    }
  }
  else
  {
    v7 = ms->moveflags;
    VectorSubtract(reach->start, ms->origin, reachdir);
    if ( (v7 & 4) == 0 )
      reachdir[2] = 0.0f;
    *(float *)&dist1 = VectorNormalize(reachdir);
    if ( !MoverDown(reach) )
    {
      dist = *(float *)&dist1;
      VectorCopy(reachdir, final);
      BotCheckBlocked(ms, final, &moveresult);
      if ( dist > 60.0f )
        dist = 60.0f;
      v36 = 360.0f - (360.0f - dist * 6.0f);
      if ( (ms->moveflags & 4) == 0 && !BotCheckBarrierJump(ms, final, 50.0f) && v36 > 5.0f )
        EA_Move(ms->client, final, v36);
      VectorCopy(final, moveresult.movedir);
      v9 = ms->moveflags;
      if ( (v9 & 4) != 0 )
      {
        moveresult.flags |= 2;
      }
      moveresult.type = 1;
      moveresult.flags |= 4u;
      return moveresult;
    }
    MoverBottomCenter(reach, telegoal);
    v11 = ms->moveflags;
    VectorSubtract(telegoal, ms->origin, telegoaldir);
    if ( (v11 & 4) == 0 )
      telegoaldir[2] = 0.0f;
    dist2 = VectorNormalize(telegoaldir);
    if ( *(float *)&dist1 < 20.0f
      || dist2 < *(float *)&dist1
      || telegoaldir[2] * reachdir[2] + telegoaldir[1] * reachdir[1] + telegoaldir[0] * reachdir[0] < 0.0f )
    {
      dist = dist2;
      VectorCopy(telegoaldir, final);
    }
    else
    {
      dist = *(float *)&dist1;
      VectorCopy(reachdir, final);
    }
    BotCheckBlocked(ms, final, &moveresult);
    if ( dist > 60.0f )
      dist = 60.0f;
    if ( (ms->moveflags & 4) == 0 && !BotCheckBarrierJump(ms, final, 50.0f) )
    {
      v18 = 400.0f - (400.0f - dist * 6.0f);
      EA_Move(ms->client, final, v18);
    }
    moveresult.movedir[0] = final[0];
    v14 = ms->moveflags;
    moveresult.movedir[1] = final[1];
    moveresult.movedir[2] = final[2];
    if ( (v14 & 4) != 0 )
    {
      moveresult.flags |= 2;
    }
  }
  return moveresult;
}
//----- (10033790) --------------------------------------------------------
bot_moveresult_t __cdecl BotFinishTravel_Elevator(bot_movestate_t *ms, aas_reachability_t *reach)
{
  /* Real vec3_t locals — see the BotTravel_Walk note. */
  vec3_t reachdir;    // [esp+14h] [ebp-48h] BYREF (was v6[2]/v7)
  vec3_t telegoaldir; // [esp+8h] [ebp-54h] BYREF (was v4[2]/v5)
  vec3_t telegoal;    // [esp+20h] [ebp-3Ch] BYREF (was v8[3])
  bot_moveresult_t moveresult;      // [esp+2Ch] [ebp-30h] BYREF

  BotClearMoveResult(&moveresult);
  MoverBottomCenter(reach, telegoal);
  VectorSubtract(telegoal, ms->origin, telegoaldir);
  VectorSubtract(reach->end, ms->origin, reachdir);
  if ( fabs(telegoaldir[2]) < fabs(reachdir[2]) )
  {
    VectorNormalize(telegoaldir);
    EA_Move(ms->client, telegoaldir, 300.0);
  }
  else
  {
    VectorNormalize(reachdir);
    EA_Move(ms->client, reachdir, 300.0);
  }
  return moveresult;
}
//----- (100338A0) --------------------------------------------------------
int __cdecl GrappleState(bot_movestate_t *ms, aas_reachability_t *reach)
{
  libvar_t *v2; // eax
  int i; // ebx
  vec3_t v5; // [esp+0h] [ebp-104h] BYREF
  float entinfo[31]; // [esp+Ch] [ebp-F8h] BYREF
  (void)ms; /* movestate handle is unused by the entity scan */

  v2 = libvar_laserhook;
  if ( !libvar_laserhook )
  {
    v2 = LibVar("laserhook", (char *)"0");
    libvar_laserhook = v2;
  }
  if ( v2->value == 0.0f && !dword_1006295C )
    dword_1006295C = IndexFromModel("models/weapons/grapple/hook/tris.md2");
  for ( i = AAS_NextBSPEntity(0); i; i = AAS_NextBSPEntity(i) )
  {
    if ( (libvar_laserhook->value != 0.0f || AAS_EntityModelindex(i) != dword_1006295C)
      && (libvar_laserhook->value == 0.0f || (AAS_EntityRenderFX(i) & 0x80u) == 0) )
    {
      continue;
    }
    *(aas_entityinfo_t *)entinfo = AAS_EntityInfo(i);
    if ( !VectorCompare(&entinfo[4], &entinfo[13]) )
      return 1;
    v5[0] = entinfo[4] - reach->end[0];
    v5[1] = entinfo[5] - reach->end[1];
    v5[2] = entinfo[6] - reach->end[2];
    if ( (float)VectorLength(v5) < 32.0f )
      return 2;
  }
  return 0;
}
//----- (10033A70) --------------------------------------------------------
void __cdecl BotResetGrapple(bot_movestate_t *ms)
{
  int v2[11]; // [esp+Ch] [ebp-2Ch] BYREF — reach buffer

  *(aas_reachability_t *)v2 = AAS_ReachabilityFromNum(ms->lastreachnum);
  /* `& 0x40` reads the integer moveflags field directly; through a float lens it
   * would be a float->byte conversion that truncates the bit to 0. */
  if ( v2[9] != 14 && ((ms->moveflags & 0x40) != 0 || ms->grapplevisible_time != 0.0f) )
  {
    EA_Command(ms->client, "hookoff", (char *)0);
    ms->moveflags &= 0xFFFFFFBFu;
    ms->grapplevisible_time = 0.0f;
  }
}
//----- (10033B00) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_Grapple(bot_movestate_t *ms, aas_reachability_t *reach)
{
  int v3; // eax
  int v4; // eax
  int state; // ebx
  double v10; // st7
  char v11; // cl
  long double v13; // st7
  int areanum; // eax
  double v17; // [esp+Ch] [ebp-54h]
  /* Real vec3_t — see the BotTravel_Walk note.  org[0] lives only on the x87
   * stack. */
  vec3_t org; // [esp+14h] [ebp-4Ch] BYREF (was v18/v19)
  vec3_t dir; // [esp+18h] [ebp-48h] BYREF (was v20/v21/v22)
  vec3_t viewdir; // [esp+24h] [ebp-3Ch] BYREF
  bot_moveresult_t moveresult; // [esp+30h] [ebp-30h] BYREF
  float dist; // [esp+6Ch] [ebp+Ch]
  float v26; // [esp+6Ch] [ebp+Ch]
  float speed; // [esp+6Ch] [ebp+Ch]

  BotClearMoveResult(&moveresult);
  v3 = ms->moveflags;
  if ( (v3 & 0x80u) != 0 )
  {
    EA_Command(ms->client, "hookoff", (char *)0);
    v4 = ms->moveflags;
    v4 &= ~0x40u;
    ms->moveflags = v4;
    { return moveresult; }
  }
  if ( (v3 & 0x40) != 0 )
  {
    state = GrappleState(ms, reach);
    VectorSubtract(reach->end, ms->origin, dir);
    dir[2] = 0.0f;
    dist = VectorLength(dir);
    if ( state )
    {
      if ( dist < 48.0f )
      {
        if ( ms->lastgrappledist - dist < 1.0f )
        {
          EA_Command(ms->client, "hookoff", (char *)0);
          ms->reachability_time = 0;
          ms->moveflags = ms->moveflags & 0xFFFFFFBF | 0x80;
        }
        ms->lastgrappledist = dist;
        { return moveresult; }
      }
      if ( state != 2 || ms->lastgrappledist - 2.0f >= dist )
      {
        ms->grapplevisible_time = AAS_Time();
        ms->lastgrappledist = dist;
        { return moveresult; }
      }
    }
    /* POSITIVE guard, as Q3 writes it (`if (grapplevisible_time < AAS_Time() -
     * 0.4) { hookoff; …; return; }`), leaving the shared
     * `lastgrappledist = dist` as the fall-through. */
    v17 = ms->grapplevisible_time;
    if ( AAS_Time() - 0.4 > v17 )
    {
      EA_Command(ms->client, "hookoff", (char *)0);
      ms->moveflags = ms->moveflags & 0xFFFFFFBF | 0x80;
      ms->reachability_time = 0;
      { return moveresult; }
    }
    ms->lastgrappledist = dist;
    { return moveresult; }
  }
  v10 = AAS_Time();
  v11 = ms->moveflags;
  ms->grapplevisible_time = v10;
  VectorSubtract(reach->start, ms->origin, dir);
  if ( (v11 & 4) == 0 )
    dir[2] = 0.0f;
  VectorAdd(ms->viewoffset, ms->origin, org);
  VectorSubtract(reach->end, org, viewdir);
  v26 = VectorNormalize(dir);
  vectoangles(viewdir, moveresult.ideal_viewangles);
  moveresult.flags |= 1;
  /* The fabs applies to AngleDiff's FPU return, not to the length computed
   * above: the gate is "yaw/pitch aligned to within 2 degrees".  Bound to the
   * length instead, a far hookable surface keeps the bot in the walk-toward
   * branch forever and the hookon command never fires. */
  if ( v26 >= 5.0f
    || (v13 = fabs(AngleDiff(moveresult.ideal_viewangles[0], ms->viewangles[0])), v13 >= 2.0)
    || (v13 = fabs(AngleDiff(moveresult.ideal_viewangles[1], ms->viewangles[1])), v13 >= 2.0) )
  {
    /* Q3's polarity: the `dist < 70` ARITHMETIC arm is the warm fall-through, not
     * the `speed = 400` constant store. */
    if ( v26 < 70.0f )
      speed = 300.0f - (300.0f - v26 * 4.0f);
    else
      speed = 400.0f;
    BotCheckBlocked(ms, dir, &moveresult);
    EA_Move(ms->client, dir, speed);
    VectorCopy(dir, moveresult.movedir);
  }
  else
  {
    EA_Command(ms->client, "hookon", (char *)0);
    /* int bit-pattern store: the original writes raw float bits (~956415.0f) into
     * lastgrappledist, so go through the int lens rather than converting. */
    *(int *)&ms->lastgrappledist = 1232348144;
    ms->moveflags |= 0x40;
  }
  areanum = AAS_PointAreaNum(ms->origin);
  /* Inline, as Q3 writes it — not a backward goto into the earlier hookoff tail,
   * which would give that tail a second, far predecessor. */
  if ( areanum && areanum != ms->reachareanum )
    ms->reachability_time = 0;
  return moveresult;
}
//----- (10033EC0) --------------------------------------------------------
bot_moveresult_t __cdecl BotTravel_RocketJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
  float dist; // st7
  float v4; // st7
  int v6; // [esp-14h] [ebp-5Ch]
  int v7; // [esp-Ch] [ebp-54h]
  float speed; // [esp+0h] [ebp-48h]
  /* Real vec3_t — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+Ch] [ebp-3Ch] BYREF (was v9/v10/v11)
  bot_moveresult_t moveresult; // [esp+18h] [ebp-30h] BYREF

  BotClearMoveResult(&moveresult);
  dir[2] = 0.0f;
  dir[0] = reach->start[0] - ms->origin[0];
  dir[1] = reach->start[1] - ms->origin[1];
  dist = VectorNormalize(dir);
  if ( dist < 5.0f )
  {
    v4 = reach->end[0] - ms->origin[0];
    dir[2] = 0.0f;
    dir[0] = v4;
    dir[1] = reach->end[1] - ms->origin[1];
    VectorNormalize(dir);
    EA_Jump(ms->client);
    EA_Attack(ms->client);
    EA_Move(ms->client, dir, 800.0f);
    ms->jumpreach = ms->lastreachnum;
  }
  else
  {
    if ( dist > 80.0f )
      dist = 80.0f;
    speed = 400.0f - (400.0f - dist * 5.0f);
    EA_Move(ms->client, dir, speed);
  }
  vectoangles(dir, ms->viewangles);
  v7 = ms->client;
  /* int bit-pattern store: the original sets pitch to 90.0f via raw bits. */
  *(int *)&ms->viewangles[0] = 1119092736;
  EA_View(v7, ms->viewangles);
  v6 = ms->client;
  moveresult.flags |= 8u;
  EA_UseItem(v6, "Rocket Launcher");
  VectorCopy(dir, moveresult.movedir);
  return moveresult;
}
//----- (10034070) --------------------------------------------------------
/* `bot_moveresult_t r; BotClearMoveResult(&r); *out = r;` — returning `out`,
 * which the original keeps in eax across the copy.  DEAD in Gladiator. */
void *__cdecl sub_10034070(void *out)
{
  bot_moveresult_t moveresult;
  BotClearMoveResult(&moveresult);
  return memcpy(out, &moveresult, 48);
}
//----- (100340B0) --------------------------------------------------------
bot_moveresult_t __cdecl BotFinishTravel_WeaponJump(bot_movestate_t *ms, aas_reachability_t *reach)
{
  /* Real vec3_t — see the BotTravel_Walk note. */
  vec3_t hordir; // [esp+8h] [ebp-3Ch] BYREF (was v4/v5/v6)
  bot_moveresult_t moveresult; // [esp+14h] [ebp-30h] BYREF

  BotClearMoveResult(&moveresult);
  if ( ms->jumpreach )
  {
    hordir[2] = 0.0f;
    hordir[0] = reach->end[0] - ms->origin[0];
    hordir[1] = reach->end[1] - ms->origin[1];
    VectorNormalize(hordir);
    EA_Move(ms->client, hordir, 800.0);
    VectorCopy(hordir, moveresult.movedir);
  }
  return moveresult;
}
//----- (10034170) --------------------------------------------------------
int __cdecl BotReachabilityTime(aas_reachability_t* reach)
{
  switch ( reach->traveltype )
  {
    case 2:           // TRAVEL_WALK
      return 5;
    case 3:           // TRAVEL_CROUCH
      return 5;
    case 4:           // TRAVEL_BARRIERJUMP
      return 5;
    case 5:           // TRAVEL_JUMP
      return 5;
    case 6:           // TRAVEL_LADDER
      return 6;
    case 7:           // TRAVEL_WALKOFFLEDGE
      return 5;
    case 8:           // TRAVEL_SWIM
      return 5;
    case 9:           // TRAVEL_WATERJUMP
      return 5;
    case 10:          // TRAVEL_TELEPORT
      return 5;
    case 11:          // TRAVEL_ELEVATOR
      return 10;
    case 12:          // TRAVEL_ROCKETJUMP
      return 6;
    case 14:          // TRAVEL_GRAPPLEHOOK (silent)
      return 8;
    default:          // incl. 13 TRAVEL_BFGJUMP and out-of-range
      botimport.Print(PRT_ERROR, "travel type %d not implemented yet\n", reach->traveltype);
      return 8;
  }
}
//----- (10034210) --------------------------------------------------------
bot_moveresult_t __cdecl BotMoveInGoalArea(bot_movestate_t *ms, bot_goal_t *goal)
{
  float dist; // st7
  float speed; // st7
  /* Real vec3_t — see the BotTravel_Walk note. */
  vec3_t dir; // [esp+8h] [ebp-3Ch] BYREF (was v13/v14/v15)
  bot_moveresult_t moveresult; // [esp+14h] [ebp-30h] BYREF
  float v17; // [esp+50h] [ebp+Ch]

  BotClearMoveResult(&moveresult);
  dir[0] = goal->origin[0] - ms->origin[0];
  dir[1] = goal->origin[1] - ms->origin[1];
  if ( (ms->moveflags & 4) != 0 )
  {
    dir[2] = goal->origin[2] - ms->origin[2];
    moveresult.traveltype = 8;
  }
  else
  {
    dir[2] = 0.0f;
    moveresult.traveltype = 2;
  }
  dist = VectorNormalize(dir);
  if ( dist > 100.0f )
    dist = 100.0f;
  speed = 400.0f - (400.0f - dist * 4.0f);
  v17 = speed;
  if ( speed < 10.0f )
    v17 = 0.0f;
  BotCheckBlocked(ms, dir, &moveresult);
  EA_Move(ms->client, dir, v17);
  VectorCopy(dir, moveresult.movedir);
  if ( (ms->moveflags & 4) != 0 )
  {
    vectoangles(dir, moveresult.ideal_viewangles);
    moveresult.flags |= 2;
  }
  ms->lastreachnum = 0;
  ms->lastareanum = 0;
  ms->lastgoalareanum = goal->areanum;
  VectorCopy(ms->origin, ms->lastorigin);
  return moveresult;
}
//----- (100343A0) --------------------------------------------------------
/* BotMoveToGoal — build a bot_moveresult_t into the output buffer from the
 * movement state, goal and AAS travel flags, returning that buffer.  Walks the
 * AAS reachability chain and dispatches to the right BotTravel_* builder for
 * each of the 14 travel types (Walk / Swim / Jump / Ladder / Grapple /
 * Elevator / RocketJump / WaterJump / …).
 */
bot_moveresult_t *__cdecl BotMoveToGoal(bot_moveresult_t *a1, bot_movestate_t *movestate, bot_goal_t *goal, int travelflags)
{
  int v8; // eax
  bot_moveresult_t *result; // eax
  int reachnum; // ebp
  int v12; // eax
  int v14; // ecx
  int v15; // eax
  int v17; // ecx
  int v18; // edx
  float v19; // [esp+10h] [ebp-2A0h]
  aas_reachability_t reach; // [esp+14h] [ebp-29Ch] BYREF
  bot_moveresult_t moveresult; // [esp+40h] [ebp-270h] BYREF (was v21 — result accumulator)
  /* lastreach never escapes, so MSVC6 coalesces the hidden struct-return temp of
   * its by-value AAS_ReachabilityFromNum() assignment with the slot itself, and
   * reuses the same hole for the sequential BotMoveInGoalArea / BotTravel_*
   * return temps.  Those per-arm temps are compiler-managed — the original
   * source declares none of them. */
  aas_reachability_t lastreach; // [esp+70h] [ebp-240h] BYREF

  BotClearMoveResult(&moveresult);
  BotResetGrapple(movestate);
  if ( !goal )
  {
    moveresult.failure = 1;
    result = a1;
    *a1 = moveresult;
    return result;
  }
  {
    movestate->moveflags &= 0xFFFFFFF3;
    if ( AAS_OnGround(movestate->origin, movestate->presencetype, movestate->entitynum) )
      movestate->moveflags |= 2;
    if ( AAS_Swimming(movestate->origin) )
      movestate->moveflags |= 4;
    if ( AAS_AgainstLadder((int *)movestate->origin) )
      movestate->moveflags |= 8;
    if ( (movestate->moveflags & 0xE) != 0 )
    {
      lastreach = AAS_ReachabilityFromNum(movestate->lastreachnum);
      v8 = BotReachabilityArea((int *)movestate, lastreach.traveltype != 11);
      movestate->areanum = v8;
      if ( v8 == goal->areanum )
      {
        *a1 = BotMoveInGoalArea(movestate, goal);
        return a1;
      }
      reachnum = movestate->lastreachnum;
      if ( reachnum )
      {
        reach = AAS_ReachabilityFromNum(movestate->lastreachnum);
        if ( (travelflags & AAS_TravelFlagForType(reach.traveltype)) != 0 )
        {
          if ( reach.traveltype == 14 )
          {
            if ( AAS_Time() <= movestate->reachability_time && (movestate->moveflags & 0x80) == 0 )
              goto LABEL_27;
          }
          else if ( reach.traveltype == 11 )
          {
            if ( movestate->areanum != reach.areanum && AAS_Time() <= movestate->reachability_time )
              goto LABEL_27;
          }
          else if ( movestate->lastgoalareanum == goal->areanum
                 && AAS_Time() <= movestate->reachability_time
                 && movestate->lastareanum == movestate->areanum )
          {
            goto LABEL_27;
          }
        }
      }
      AAS_AreaReachability(movestate->areanum);
      v12 = BotGetReachabilityToGoal(
              movestate->origin,
              movestate->areanum,
              movestate->lastgoalareanum,
              movestate->lastareanum,
              movestate->entitynum,
              movestate->avoidreach,
              movestate->avoidreachtimes,
              movestate->avoidreachtries,
              goal,
              travelflags);
      reachnum = v12;
      movestate->reachareanum = movestate->areanum;
      movestate->jumpreach = 0;
      movestate->moveflags &= 0xFFFFFF7F;
      if ( v12 )
      {
        reach = AAS_ReachabilityFromNum(v12);
        v19 = (float)BotReachabilityTime(&reach);
        movestate->reachability_time = AAS_Time() + v19;
        BotAddToAvoidReach((intptr_t)movestate, reachnum, 6.0);
      }
LABEL_27:
      v14 = movestate->areanum;
      movestate->lastreachnum = reachnum;
      v15 = goal->areanum;
      movestate->lastareanum = v14;
      movestate->lastgoalareanum = v15;
      if ( reachnum )
      {
        reach = AAS_ReachabilityFromNum(reachnum);
        moveresult.traveltype = reach.traveltype;
        switch ( reach.traveltype )
        {
          case 2:
            moveresult = BotTravel_Walk(movestate, &reach); break;
          case 3:
            moveresult = BotTravel_Crouch(movestate, &reach); break;
          case 4:
            moveresult = BotTravel_BarrierJump(movestate, &reach); break;
          case 6:
            moveresult = BotTravel_Ladder(movestate, &reach); break;
          case 7:
            moveresult = BotTravel_WalkOffLedge(movestate, &reach); break;
          case 5:
            moveresult = BotTravel_Jump(movestate, &reach); break;
          case 8:
            moveresult = BotTravel_Swim(movestate, &reach); break;
          case 9:
            moveresult = BotTravel_WaterJump(movestate, &reach); break;
          case 0xA:
            moveresult = BotTravel_Teleport(movestate, &reach); break;
          case 0xB:
            moveresult = BotTravel_Elevator(movestate, &reach); break;
          case 0xE:
            moveresult = BotTravel_Grapple(movestate, &reach); break;
          case 0xC:
            moveresult = BotTravel_RocketJump(movestate, &reach); break;
          default:
            botimport.Print(PRT_FATAL, "travel type %d not implemented yet\n", reach.traveltype);
            break;
        }
      }
      else
      {
        moveresult.failure = 1;
      }
    }
    else
    {
      if ( movestate->lastreachnum )
      {
        reach = AAS_ReachabilityFromNum(movestate->lastreachnum);
        moveresult.traveltype = reach.traveltype;
        switch ( reach.traveltype )
        {
          case 2:
            moveresult = BotTravel_Walk(movestate, &reach); break;
          case 3:
          case 0xA:
            break;
          case 4:
            moveresult = BotFinishTravel_BarrierJump(movestate, &reach); break;
          case 6:
            moveresult = BotTravel_Ladder(movestate, &reach); break;
          case 7:
            moveresult = BotFinishTravel_WalkOffLedge(movestate, &reach); break;
          case 5:
            moveresult = BotFinishTravel_Jump(movestate, &reach); break;
          case 8:
            moveresult = BotTravel_Swim(movestate, &reach); break;
          case 9:
            moveresult = BotFinishTravel_WaterJump(movestate, &reach); break;
          case 0xB:
            moveresult = BotFinishTravel_Elevator(movestate, &reach); break;
          case 0xE:
            moveresult = BotTravel_Grapple(movestate, &reach); break;
          case 0xC:
            moveresult = BotFinishTravel_WeaponJump(movestate, &reach); break;
          default:
            botimport.Print(PRT_FATAL, "(last) travel type %d not implemented yet\n", reach.traveltype);
            break;
        }
      }
    }
  }
  if ( moveresult.blocked )
    movestate->reachability_time = movestate->reachability_time - movestate->thinktime * 10.0f;
  v17 = *(int *)&movestate->origin[1];
  v18 = *(int *)&movestate->origin[2];
  movestate->lastorigin[0] = movestate->origin[0];
  *(int *)&movestate->lastorigin[1] = v17;
  *(int *)&movestate->lastorigin[2] = v18;
  result = a1;
  *a1 = moveresult;
  return result;
}
//----- (10034AF0) --------------------------------------------------------
_DWORD *__cdecl BotResetAvoidReach(_DWORD *movestate)
{
  _DWORD *result; // eax

  memset(&movestate[29], 0, sizeof(int));
  memset(&movestate[30], 0, sizeof(int));
  result = &movestate[31];
  memset(&movestate[31], 0, sizeof(int));
  return result;
}
//----- (10034B20) --------------------------------------------------------
void __cdecl BotResetLastAvoidReach(intptr_t movestate)
{
  bot_movestate_t *ms = (bot_movestate_t *)movestate;
  int i, latest;
  float latesttime;

  latesttime = 0;
  for ( i = 0; i < 1; i++ )
  {
    if ( ms->avoidreachtimes[i] > latesttime )
    {
      latesttime = ms->avoidreachtimes[i];
      latest = i;
    }
  }
  if ( latesttime != 0 )
  {
    *(int *)&ms->avoidreachtimes[latest] = 0;
    /* FAITHFUL ORIGINAL BUG — do NOT "fix": the guard reads avoidreachtries[1],
     * ONE PAST the 1-element array, then decrements avoidreachtries[latest].
     * Q3's cognate guards on avoidreachtries[latest].  Indexed through a decayed
     * pointer so the out-of-bounds subscript is explicit. */
    if ( ((int *)ms->avoidreachtries)[1] > 0 )
      --ms->avoidreachtries[latest];
  }
}
//----- (10034B90) --------------------------------------------------------
int __cdecl BotResetMoveState(void *movestate)
{
  int result; // eax

  result = 0;
  memset(movestate, 0, 0x80u);
  return result;
}
