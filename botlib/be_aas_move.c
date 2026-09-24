/*
 * be_aas_move.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x1000EEB0..0x10010780.
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
#include "be_aas_move.h"
#include "be_aas_bspq2.h"
#include "be_aas_debug.h"
#include "be_aas_main.h"
#include "be_aas_sample.h"
#include "be_interface.h"
#include "l_memory.h"

// gladiator.dll: 1000EEB0..1000EF7B
// gladi386.so:   0001B6DC..0001B7F5
BOOL __cdecl AAS_OnGround(vec3_t origin, int presencetype, int passent)
{
  aas_trace_t trace;
  vec3_t end, up = {0, 0, 1};
  aas_plane_t *plane;

  VectorCopy(origin, end);
  end[2] -= 4.0f;

  trace = AAS_TraceClientBBox(origin, end, presencetype, passent);

  if ( trace.startsolid ) return 0;
  if ( trace.fraction >= 1.0 ) return 0;
  if ( origin[2] - trace.endpos[2] > 2.0f ) return 0;
  plane = (aas_plane_t *)AAS_PlaneFromNum(trace.planenum);
  if ( DotProduct(plane->normal, up) < libvar_sv_maxsteepness->value ) return 0;
  return 1;
}

// gladiator.dll: 1000EFC0..1000EFF7
// gladi386.so:   0001B7F8..0001B852
/* Tests whether a point 2 units below `origin` is in liquid (LAVA|SLIME|WATER =
 * 0x38).  BotMoveInDirection uses it to pick a swim/jump movement style.  The
 * bi_PointContents call is required — without it the result is uninitialised. */
BOOL __cdecl AAS_Swimming(vec3_t origin)
{
  /* A real vec3_t written with VectorCopy, then bumped down by 2 — NOT an int[3] with
   * `*(int *)&origin[i]` bit copies.  Both originals agree and each optimises the pair
   * its own way: MSVC6 dead-store-eliminates the [2] copy and folds origin[2] straight
   * into the `fld`, gcc 2.7 keeps the copy and re-reads the slot.  The int-cast form
   * reproduces neither. */
  vec3_t testorg; // [esp+0h] [ebp-Ch] BYREF

  VectorCopy(origin, testorg);
  testorg[2] -= 2.0f;
  if ( AAS_PointContents(testorg) & 0x38 )
    return 1;
  return 0;
}

// gladiator.dll: 1000F010..1000F0EA
// gladi386.so:   0001B854..0001B938
/*
 * The run-up start position for a TRAVEL_JUMP reachability: predict movement back
 * from reach->start along the horizontal (start - end) direction at speed 400 and
 * write the predicted endpos into `runstart`, falling back to `start` if the
 * prediction hits liquid or a ground-damage drop (mask 0x38 here; Q3 uses 0x3C).
 */
void __cdecl AAS_JumpReachRunStart(aas_reachability_t* reach, intptr_t runstart)
{
  float *runstart_vec; // edi
  char stopevent; // cl
  /* One vec3_t: the original passes its address to AAS_ClientMovementPrediction,
   * and three separate float locals are not guaranteed to be adjacent. */
  vec3_t hordir; // [esp+14h] [ebp-68h] BYREF
  vec3_t start_pos; // [esp+8h] [ebp-74h] BYREF (was v11/v12/v13)
  vec3_t cmdmove; // [esp+20h] [ebp-5Ch] BYREF
  aas_clientmove_t move; // [esp+2Ch] [ebp-50h] BYREF (coalesced with the by-value return temp)

  hordir[0] = reach->start[0] - reach->end[0];
  hordir[1] = reach->start[1] - reach->end[1];
  hordir[2] = 0;
  VectorNormalize(hordir);
  VectorCopy(reach->start, start_pos);
  start_pos[2] += 1.0f;
  runstart_vec = (float *)runstart;
  VectorScale((float *)hordir, 400.0f, (float *)cmdmove);
  move = AAS_ClientMovementPrediction(-1, start_pos, 2, 1, vec3_origin, cmdmove, 1, 2, 0.1f, 124, 0);
  VectorCopy(move.endpos, runstart_vec);
  stopevent = move.stopevent;
  if ( (stopevent & 0x38) != 0 )
  {
    VectorCopy(start_pos, runstart_vec);
  }
}

// gladiator.dll: 1000F130..1000F269
// gladi386.so:   0001B938..0001BA6B
// Probe the engine's PointContents() at six positions around a 3D origin, looking
// for content flag 0x20000000 (Gladiator's "do-not-enter / bot-area-block" overlay).
// The probes trace a 16x16 box at the eye-height offset (+48 on Z) plus the floor
// point itself:
//   (x,   y,   z+48)
//   (x+8, y+8, z+48)
//   (x-8, y+8, z+48)
//   (x-8, y-8, z+48)
//   (x+8, y-8, z+48)
//   (x,   y,   z)
// The first five take the early-exit "return 1" path on a hit; the floor probe
// returns the bit as 0/1 directly via shr 29 / and 1.
// DEAD in Gladiator — /INCREMENTAL.
int __cdecl sub_1000F130(vec3_t origin)
{
  vec3_t p;

  VectorCopy(origin, p);
  p[2] += 48.0f;
  if ( AAS_PointContents(p) & 0x20000000 ) return 1;
  p[0] += 8.0f;
  p[1] += 8.0f;
  if ( AAS_PointContents(p) & 0x20000000 ) return 1;
  p[0] += -16.0f;
  if ( AAS_PointContents(p) & 0x20000000 ) return 1;
  p[1] += -16.0f;
  if ( AAS_PointContents(p) & 0x20000000 ) return 1;
  p[0] += 16.0f;
  if ( AAS_PointContents(p) & 0x20000000 ) return 1;
  p[0] -= 8.0f;
  p[1] += 8.0f;
  p[2] -= 48.0f;
  if ( AAS_PointContents(p) & 0x20000000 ) return 1;
  return 0;
}

// gladiator.dll: 1000F2C0..1000F453
// gladi386.so:   0001BA6C..0001BC41
int __cdecl AAS_AgainstLadder(vec3_t origin)
{
  int areanum; // eax
  int i; // ebx
  int facenum; // esi
  int side;
  vec3_t org;
  aas_plane_t *plane;
  aas_face_t *face; // eax
  aas_area_t *area; // ebp

  VectorCopy(origin, org);
  areanum = AAS_PointAreaNum(org);
  if ( !areanum )
  {
    org[0] += 1;
    areanum = AAS_PointAreaNum(org);
    if ( !areanum )
    {
      org[1] += 1;
      areanum = AAS_PointAreaNum(org);
      if ( !areanum )
      {
        org[0] -= 2;
        areanum = AAS_PointAreaNum(org);
        if ( !areanum )
        {
          org[1] -= 2;
          areanum = AAS_PointAreaNum(org);
        }
      }
    }
  }
  if ( !areanum ) return 0;
  if ( !(aasworld.areasettings[areanum].areaflags & 2)) return 0;
  if ( !(aasworld.areasettings[areanum].presencetype & 2)) return 0;

  area = &aasworld.areas[areanum];
  for (i = 0; i < area->numfaces; i++)
  {
    facenum = aasworld.faceindex[area->firstface + i];
    side = facenum < 0;
    face = &aasworld.faces[abs(facenum)];
    if (!(face->faceflags & 2)) continue;
    plane = &aasworld.planes[face->planenum ^ side];
    if (abs(DotProduct(plane->normal, origin) - plane->dist) < 3)
    {
      if (AAS_PointInsideFace(abs(facenum), origin, 0.1f)) return 1;
    }
  }
  return 0;
}

// gladiator.dll: 1000F4D0..1000F6C8
// gladi386.so:   0001BC44..0001BF3E
float __cdecl AAS_WeaponJumpZVelocity(vec3_t origin, float radiusdamage)
{
  vec3_t kvel, v, start, end, forward, right, viewangles, dir;
  float mass, knockback, points;
  vec3_t rocketoffset = {8, 8, -8};
  vec3_t botmins = {-16, -16, -24};
  vec3_t botmaxs = {16, 16, 32};
  bsp_trace_t bsptrace;

  viewangles[PITCH] = 90.0f;
  viewangles[YAW] = 0.0f;
  viewangles[ROLL] = 0.0f;

  VectorCopy(origin, start);
  start[2] += 8.0f;
  AngleVectors(viewangles, forward, right, NULL);
  start[0] += forward[0] * rocketoffset[0] + right[0] * rocketoffset[1];
  start[1] += forward[1] * rocketoffset[0] + right[1] * rocketoffset[1];
  start[2] += forward[2] * rocketoffset[0] + right[2] * rocketoffset[1] + rocketoffset[2];

  VectorMA(start, 500.0, forward, end);
  bsptrace = AAS_Trace(start, NULL, NULL, end, 1, 3);

  VectorAdd(botmins, botmaxs, v);
  VectorMA(origin, 0.5, v, v);
  VectorSubtract(bsptrace.endpos, v, v);

  points = radiusdamage - 0.5 * VectorLength(v);
  if ( points < 0.0f )
    points = 0.0f;
  points *= 0.5;

  mass = 200.0f;
  knockback = points;
  VectorSubtract(origin, bsptrace.endpos, dir);
  VectorNormalize(dir);
  VectorScale(dir, 1600.0 * (float)knockback / mass, kvel);
  return kvel[2] + libvar_sv_jumpvel->value;
}

// gladiator.dll: 1000F750..1000F763
// gladi386.so:   0001BF40..0001BF61
/* Z-velocity from self-rocketing at `origin`; a one-line wrapper over
 * AAS_WeaponJumpZVelocity with the launcher's 120-unit radius damage.  All three are
 * Q3's `float`.  A lone `float` wrapper over a `double` callee made gcc round-trip
 * ST(0) through memory, which is why they were once `double`; but `double` returns
 * make every caller's `float zvel = ...` a float_truncate with its own stack temp,
 * which in AAS_Reachability_WeaponJump and AAS_ShowReachability sat ahead of an
 * address-taken local and shifted it 4 bytes in the .so's frame. */
float __cdecl AAS_RocketJumpZVelocity(vec3_t origin)
{
  return AAS_WeaponJumpZVelocity(origin, 120.0);
}

// gladiator.dll: 1000F780..1000F793
// gladi386.so:   0001BF64..0001BF85
float __cdecl AAS_BFGJumpZVelocity(vec3_t origin)
{
  return AAS_WeaponJumpZVelocity(origin, 120.0);
}

// gladiator.dll: 1000F7B0..1000F81A
// gladi386.so:   0001BF88..0001C00C
void __cdecl AAS_ApplyFriction(vec3_t vel, float friction, float stopspeed, float frametime)
{
  float speed; // st5
  float control; // st6
  float newspeed; // st6

  speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
  if ( speed != 0.0f )
  {
    if ( speed < stopspeed )
      control = stopspeed;
    else
      control = speed;
    newspeed = speed - frametime * control * friction;
    if ( newspeed < 0.0f )
      newspeed = 0.0f;
    newspeed /= speed;
    vel[0] *= newspeed;
    vel[1] *= newspeed;
  }
}

// gladiator.dll: 1000F840..100103AA
// gladi386.so:   0001C00C..0001D122
/*
 * Predict client movement up to `maxframes` ahead.  This is the OLDER form of Q3's
 * function; do NOT "upgrade" it to Q3's algorithm:
 *   - physics come from the libvar_sv_* handles, not an aassettings struct;
 *   - acceleration is the per-axis velchange/clamp loop Q3 later replaced with
 *     AAS_Accelerate (Q3 keeps a later version of it commented out), clamping the new
 *     velocity alone where Q3's comment also tests the old one;
 *   - maxwalk/crouch/swim velocities and the acceleration are pre-scaled by frametime;
 *   - the only stop-events are SE_HITGROUND(1) / SE_LEAVEGROUND(2) /
 *     SE_ENTER{LAVA,SLIME}(0x10/8) / SE_HITGROUNDDAMAGE(0x20) / SE_GAP(0x40), there is
 *     no endarea, and the velocity is returned unscaled.
 * Q3's text and declarations otherwise, with the result built in a local `move` returned
 * by value; every exit's shared tail is gcc's cross-jump, not a goto.
 */
aas_clientmove_t __cdecl AAS_ClientMovementPrediction(int entnum, vec3_t origin,
        int presencetype, int onground, vec3_t velocity, vec3_t cmdmove,
        int cmdframes, int maxframes, float frametime, int stopevent, int visualize)
{
  float phys_friction, phys_stopspeed, phys_gravity, phys_waterfriction;
  float phys_watergravity;
  float phys_maxacceleration;
  float phys_maxwalkvelocity, phys_maxcrouchvelocity, phys_maxswimvelocity;
  float phys_maxstep, phys_maxsteepness, phys_jumpvel, friction;
  float gravity, delta, maxvel;
  float velchange, newvel;
  int n, i, j, pc, step, swimming, ax, crouch, event, jump_frame;
  vec3_t org, end, feet, start, stepend, lastorg;
  vec3_t frame_test_vel, old_frame_test_vel, left_test_vel;
  vec3_t up = {0, 0, 1};
  aas_plane_t *plane, *plane2;
  aas_trace_t trace, steptrace;
  aas_clientmove_t move;

  phys_friction = libvar_sv_friction->value;
  phys_stopspeed = libvar_sv_stopspeed->value;
  phys_gravity = libvar_sv_gravity->value;
  phys_waterfriction = libvar_sv_waterfriction->value;
  phys_watergravity = libvar_sv_watergravity->value;
  phys_maxwalkvelocity = libvar_sv_maxwalkvelocity->value * frametime;
  phys_maxcrouchvelocity = libvar_sv_maxcrouchvelocity->value * frametime;
  phys_maxswimvelocity = libvar_sv_maxswimvelocity->value * frametime;
  phys_maxacceleration = libvar_sv_maxaccelerate->value * frametime;
  phys_maxstep = libvar_sv_step->value;
  phys_maxsteepness = libvar_sv_maxsteepness->value;
  phys_jumpvel = libvar_sv_jumpvel->value * frametime;
  //
  memset(&move, 0, sizeof(aas_clientmove_t));
  memset(&trace, 0, sizeof(aas_trace_t));
  //start at the current origin
  VectorCopy(origin, org);
  org[2] += 0.25;
  //velocity to test for the first frame
  VectorScale(velocity, frametime, frame_test_vel);
  //
  jump_frame = -1;
  //predict a maximum of 'maxframes' ahead
  for (n = 0; n < maxframes; n++)
  {
    swimming = AAS_Swimming(org);
    //get gravity depending on swimming or not
    gravity = swimming ? phys_watergravity : phys_gravity;
    //apply gravity at the START of the frame
    frame_test_vel[2] = frame_test_vel[2] - (gravity * 0.1 * frametime);
    //if on the ground or swimming
    if (onground || swimming)
    {
      friction = swimming ? phys_friction : phys_waterfriction;
      //apply friction
      VectorScale(frame_test_vel, 10, frame_test_vel);
      AAS_ApplyFriction(frame_test_vel, friction, phys_stopspeed, frametime);
      VectorScale(frame_test_vel, 0.1, frame_test_vel);
    } //end if
    crouch = 0;
    //apply command movement
    if (n < cmdframes)
    {
      ax = 0;
      maxvel = phys_maxwalkvelocity;
      if (onground)
      {
        if (cmdmove[2] < -300)
        {
          crouch = 1;
          maxvel = phys_maxcrouchvelocity;
        } //end if
        //if not swimming and upmove is positive then jump
        if (!swimming && cmdmove[2] > 1)
        {
          //jump velocity minus the gravity for one frame + 5 for safety
          frame_test_vel[2] = phys_jumpvel - (gravity * 0.1 * frametime) + 5;
          jump_frame = n;
        } //end if
        ax = 2;
      } //end if
      if (swimming)
      {
        maxvel = phys_maxswimvelocity;
        ax = 3;
      } //end if
      for (i = 0; i < ax; i++)
      {
        velchange = (cmdmove[i] * frametime) - frame_test_vel[i];
        if (velchange > phys_maxacceleration) velchange = phys_maxacceleration;
        else if (velchange < -phys_maxacceleration) velchange = -phys_maxacceleration;
        newvel = frame_test_vel[i] + velchange;
        frame_test_vel[i] = newvel;
        /* The clamp tests the stored velocity: cl.exe then reloads newvel from its
         * home after the store, as the DLL does (testing `newvel` keeps it on the
         * x87 stack).  Q3's later version also tests the old velocity. */
        if (frame_test_vel[i] > maxvel) frame_test_vel[i] = maxvel;
        else if (frame_test_vel[i] < -maxvel) frame_test_vel[i] = -maxvel;
      } //end for
    } //end if
    if (crouch)
    {
      presencetype = 4;
    } //end if
    else if (presencetype == 4)
    {
      if (AAS_PointPresenceType(org) & 2)
      {
        presencetype = 2;
      } //end if
    } //end else
    //save the current origin
    VectorCopy(org, lastorg);
    //move linear during one frame
    VectorCopy(frame_test_vel, left_test_vel);
    j = 0;
    do
    {
      VectorAdd(org, left_test_vel, end);
      //trace a bounding box
      trace = AAS_TraceClientBBox(org, end, presencetype, entnum);
      //
      if (visualize)
      {
        if (trace.startsolid) botimport.Print(PRT_MESSAGE, "PredictMovement: start solid\n");
        AAS_DebugLine(org, trace.endpos, -218959632);
      } //end if
      //move the entity to the trace end point
      VectorCopy(trace.endpos, org);
      //if there was a collision
      if (trace.fraction < 1.0)
      {
        //get the plane the bounding box collided with
        plane = (aas_plane_t *)AAS_PlaneFromNum(trace.planenum);
        //assume there's no step
        step = 0;
        //if it is a vertical plane and the bot didn't jump recently
        if (plane->normal[2] == 0 && (jump_frame < 0 || n - jump_frame > 2))
        {
          //check for a step
          VectorMA(org, -0.25, plane->normal, start);
          VectorCopy(start, stepend);
          start[2] += phys_maxstep;
          steptrace = AAS_TraceClientBBox(start, stepend, presencetype, entnum);
          //
          if (!steptrace.startsolid)
          {
            plane2 = (aas_plane_t *)AAS_PlaneFromNum(steptrace.planenum);
            if (DotProduct(plane2->normal, up) > phys_maxsteepness)
            {
              VectorSubtract(end, steptrace.endpos, left_test_vel);
              left_test_vel[2] = 0;
              frame_test_vel[2] = 0;
              if (visualize)
              {
                if (steptrace.endpos[2] - org[2] > 0.125)
                {
                  VectorCopy(org, start);
                  start[2] = steptrace.endpos[2];
                  AAS_DebugLine(org, start, -202116623);
                } //end if
              } //end if
              org[2] = steptrace.endpos[2];
              step = 1;
            } //end if
          } //end if
        } //end if
        //
        if (!step)
        {
          //velocity left to test for this frame is the projection
          //of the current test velocity into the hit plane
          VectorMA(left_test_vel, -DotProduct(left_test_vel, plane->normal),
                    plane->normal, left_test_vel);
          //store the old velocity for landing check
          VectorCopy(frame_test_vel, old_frame_test_vel);
          //test velocity for the next frame is the projection
          //of the velocity of the current frame into the hit plane
          VectorMA(frame_test_vel, -DotProduct(frame_test_vel, plane->normal),
                    plane->normal, frame_test_vel);
          //check for a landing on an almost horizontal floor
          if (DotProduct(plane->normal, up) > phys_maxsteepness)
          {
            onground = 1;
          } //end if
          if (stopevent & 0x20)
          {
            delta = 0;
            if (old_frame_test_vel[2] < 0 &&
                frame_test_vel[2] > old_frame_test_vel[2] &&
                !onground)
            {
              delta = old_frame_test_vel[2];
            } //end if
            else if (onground)
            {
              delta = frame_test_vel[2] - old_frame_test_vel[2];
            } //end else
            if (delta)
            {
              delta = delta * 10;
              delta = delta * delta * 0.0001;
              if (swimming) delta = 0;
              if (delta > 30)
              {
                VectorCopy(org, move.endpos);
                VectorCopy(frame_test_vel, move.velocity);
                move.trace = trace;
                move.stopevent = 0x20;
                move.presencetype = presencetype;
                move.endcontents = 4;
                move.time = n * frametime;
                move.frames = n;
                return move;
              } //end if
            } //end if
          } //end if
        } //end if
      } //end if
      //extra check to prevent endless loop
      if (++j > 20) return move;
    //while there is a plane hit
    } while(trace.fraction < 1.0);
    //if going down
    if (frame_test_vel[2] <= 0)
    {
      //check for a liquid at the feet of the bot
      VectorCopy(org, feet);
      feet[2] -= 22;
      pc = AAS_PointContents(feet);
      //get event from pc
      event = 0;
      if (pc & 8) event |= 0x10;
      if (pc & 0x10) event |= 8;
      if (pc & 0x20) event |= 8;
      //if in lava or slime
      if (event & stopevent)
      {
        VectorCopy(org, move.endpos);
        VectorCopy(frame_test_vel, move.velocity);
        move.stopevent = event & stopevent;
        move.presencetype = presencetype;
        move.endcontents = pc;
        move.time = n * frametime;
        move.frames = n;
        return move;
      } //end if
    } //end if
    //
    onground = AAS_OnGround(org, presencetype, entnum);
    //if onground and on the ground for at least one whole frame
    if (onground)
    {
      if (stopevent & 1)
      {
        VectorCopy(org, move.endpos);
        VectorCopy(frame_test_vel, move.velocity);
        move.trace = trace;
        move.stopevent = 1;
        move.presencetype = presencetype;
        move.endcontents = 4;
        move.time = n * frametime;
        move.frames = n;
        return move;
      } //end if
    } //end if
    else if (stopevent & 2)
    {
      VectorCopy(org, move.endpos);
      VectorCopy(frame_test_vel, move.velocity);
      move.trace = trace;
      move.stopevent = 2;
      move.presencetype = presencetype;
      move.endcontents = 4;
      move.time = n * frametime;
      move.frames = n;
      return move;
    } //end else if
    else if (stopevent & 0x40)
    {
      aas_trace_t gaptrace;

      VectorCopy(org, start);
      VectorCopy(start, end);
      end[2] -= 48 + libvar_sv_maxbarrier->value;
      gaptrace = AAS_TraceClientBBox(start, end, 4, -1);
      //if solid is found the bot cannot walk any further and will not fall into a gap
      if (!gaptrace.startsolid)
      {
        //if it is a gap (lower than one step height)
        if (gaptrace.endpos[2] < org[2] - libvar_sv_step->value - 1)
        {
          if (!(AAS_PointContents(end) & 0x20))
          {
            VectorCopy(lastorg, move.endpos);
            VectorCopy(frame_test_vel, move.velocity);
            move.trace = trace;
            move.stopevent = 0x40;
            move.presencetype = presencetype;
            move.endcontents = 4;
            move.time = n * frametime;
            move.frames = n;
            return move;
          } //end if
        } //end if
      } //end if
    } //end else if
  } //end for
  //
  VectorCopy(org, move.endpos);
  VectorCopy(frame_test_vel, move.velocity);
  move.stopevent = 0;
  move.presencetype = presencetype;
  move.endcontents = 4;
  move.time = n * frametime;
  move.frames = n;
  //
  return move;
}

// gladiator.dll: 10010690..1001074D
// gladi386.so:   0001D124..0001D222
/* Movement-prediction debug helper: flatten the direction's Z unless swimming,
 * normalize it, scale to 400 u/s, force a +Z=224 jump impulse, clear the debug lines,
 * then run one AAS_ClientMovementPrediction of 13 frames at 0.1 s (presence=NORMAL,
 * onground, stopevent, visualize) and print "leave ground" if the result flags set
 * bit 0x02.
 *
 * DEAD — almost certainly a leftover development test harness. */
void AAS_TestMovementPrediction(int entnum, vec3_t origin, vec3_t dir)
{
  vec3_t  velocity;          /* zero vector */
  vec3_t  cmd_move;          /* desired-input vec: scaled forward + Z jump */
  aas_clientmove_t result;   /* filled by the by-value prediction return */

  VectorClear(velocity);

  if (!AAS_Swimming(origin))
    dir[2] = 0.0f;

  VectorNormalize(dir);
  VectorScale(dir, 400.0f, cmd_move);
  cmd_move[2] = 224.0f;

  AAS_ClearShownDebugLines();
  result = AAS_ClientMovementPrediction(entnum, origin, 2, 1,
                               velocity, cmd_move, 13, 13,
                               0.1f, 1, 1);

  if (result.stopevent & 0x02)
    botimport.Print(PRT_MESSAGE, "leave ground\n");
}

// gladiator.dll: 10010780..1001082B
// gladi386.so:   0001D224..0001D313
int __cdecl AAS_HorizontalVelocityForJump(float zvel, vec3_t start, vec3_t end, float *velocity)
{
  float phys_gravity, phys_maxvelocity;
  float maxjump, height2fall, t, top;
  vec3_t dir;

  phys_gravity = libvar_sv_gravity->value;
  phys_maxvelocity = libvar_sv_maxvelocity->value;

  maxjump = 0.5 * phys_gravity * (zvel / phys_gravity) * (zvel / phys_gravity);
  top = start[2] + maxjump;
  height2fall = top - end[2];
  if ( height2fall < 0 )
  {
    *velocity = phys_maxvelocity;
    return 0;
  }
  t = sqrt(height2fall / (0.5 * phys_gravity));
  VectorSubtract(end, start, dir);
  *velocity = sqrt(dir[0]*dir[0] + dir[1]*dir[1]) / (t + zvel / phys_gravity);
  if ( *velocity > phys_maxvelocity )
  {
    *velocity = phys_maxvelocity;
    return 0;
  }
  return 1;
}
