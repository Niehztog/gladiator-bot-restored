/*
 * be_aas_move.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x1000EEB0..0x10010780; the boundary evidence -- per-object .text and
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
/* Tests whether a point 2 units below `origin` is in liquid
 * (LAVA|SLIME|WATER = 0x38).  BotMoveInDirection uses it to pick a swim/jump
 * movement style.  The bi_PointContents call is required — without it the
 * result is uninitialised. */
BOOL __cdecl AAS_Swimming(vec3_t origin)
{
  /* int[3], not float[3]: X/Y are raw 32-bit copies of origin's float bit
   * patterns.  As float[3] the compiler injects an int->float conversion on
   * those two stores, garbling the test point so bi_PointContents never sees
   * water and swimming bots get stuck. */
  int testorg[3]; // [esp+0h] [ebp-Ch] BYREF

  testorg[0] = *(int *)&origin[0];
  testorg[1] = *(int *)&origin[1];
  testorg[2] = *(int *)&origin[2];
  *(float *)&testorg[2] -= 2.0f;
  if ( sub_10003080((float *)testorg) & 0x38 )
    return 1;
  return 0;
}
// gladiator.dll: 1000F010..1000F0EA
// gladi386.so:   0001B854..0001B938
/*
 * AAS_JumpReachRunStart (was sub_1000F010) — the run-up start position for a
 * TRAVEL_JUMP reachability: predict movement back from reach->start along the
 * horizontal (start - end) direction at speed 400 and write the predicted
 * endpos into `runstart`, falling back to `start` if the prediction hits
 * liquid or a ground-damage drop (mask 0x38 here; Q3 uses 0x3C).
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
  start_pos[0] = reach->start[0];
  start_pos[1] = reach->start[1];
  start_pos[2] = reach->start[2];
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
// Probes the engine's PointContents() at six positions around a 3D
// origin, looking for the high-bit-29 content flag (0x20000000 —
// Gladiator's "do-not-enter / bot-area-block" overlay).  The probes
// trace a 16x16 box at the eye-height offset (+48 on Z) plus the
// floor point itself:
//   (x,   y,   z+48)
//   (x+8, y+8, z+48)
//   (x-8, y+8, z+48)
//   (x-8, y-8, z+48)
//   (x+8, y-8, z+48)
//   (x,   y,   z)
// The first five take the early-exit "return 1" path on a hit; the
// floor probe returns the bit as 0/1 directly via shr 29 / and 1.
// Constants from .rdata 0x100580dc/e0/e4 = 16.0f / 8.0f / 48.0f.
// DEAD in Gladiator — /INCREMENTAL.
int __cdecl sub_1000F130(vec3_t origin)
{
  vec3_t p;

  p[0] = origin[0];
  p[1] = origin[1];
  p[2] = origin[2];
  p[2] += 48.0f;
  if ( sub_10003080(p) & 0x20000000 ) return 1;
  p[0] += 8.0f;
  p[1] += 8.0f;
  if ( sub_10003080(p) & 0x20000000 ) return 1;
  p[0] += -16.0f;
  if ( sub_10003080(p) & 0x20000000 ) return 1;
  p[1] += -16.0f;
  if ( sub_10003080(p) & 0x20000000 ) return 1;
  p[0] += 16.0f;
  if ( sub_10003080(p) & 0x20000000 ) return 1;
  p[0] -= 8.0f;
  p[1] += 8.0f;
  p[2] -= 48.0f;
  if ( sub_10003080(p) & 0x20000000 ) return 1;
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
double __cdecl AAS_WeaponJumpZVelocity(vec3_t origin, float radiusdamage)
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
/* AAS_RocketJumpZVelocity (was sub_1000F750) — Z-velocity from self-rocketing
 * at `origin`; a one-line wrapper over AAS_WeaponJumpZVelocity with the
 * launcher's 120-unit radius damage.  `double`, not `float`: AAS_BFGJumpZVelocity
 * immediately below is the same one-line wrapper and byte-matches with `double`,
 * where the `float` narrowing here made gcc round-trip the callee's ST(0) return
 * through memory (`fstp DWORD; fld DWORD`) and grow a 4-byte frame for it. */
double __cdecl AAS_RocketJumpZVelocity(vec3_t origin)
{
  return AAS_WeaponJumpZVelocity(origin, 120.0);
}
// gladiator.dll: 1000F780..1000F793
// gladi386.so:   0001BF64..0001BF85
double __cdecl AAS_BFGJumpZVelocity(vec3_t origin)
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
 * AAS_ClientMovementPrediction — predict client movement up to `maxframes`
 * ahead.  This is the OLDER form; it diverges from Q3 by design — do NOT
 * "upgrade" it to the Q3 algorithm:
 *   - physics come from the libvar_sv_* handles, not an aassettings struct;
 *   - acceleration is the inline per-axis velchange/clamp loop below, which Q3
 *     later replaced with AAS_Accelerate + wishdir/wishspeed;
 *   - maxwalk/crouch/swim velocities are pre-scaled by frametime here;
 *   - the only stop-events are SE_HITGROUND(1) / SE_LEAVEGROUND(2) /
 *     SE_ENTER{LAVA,SLIME,WATER} / SE_HITGROUNDDAMAGE(0x20) / SE_GAP(0x40) —
 *     no SE_ENTERAREA, jumppad, teleporter or SE_HITBOUNDINGBOX, hence no
 *     mins/maxs/stopareanum params and no AAS_TraceAreas scan.
 * The result is built in the move_buf[] scratch and copied to `move` at the
 * tail.  Names and types follow Q3's for readability only.
 */
aas_clientmove_t __cdecl AAS_ClientMovementPrediction(
        int entnum,           // a2
        float *origin,        // a3: vec3_t start origin
        int presencetype,     // a4
        int onground,         // a5
        float *velocity,      // a6: vec3_t initial velocity
        float *cmdmove,       // a7: vec3_t client command movement
        int cmdframes,        // a8: number of frames cmdmove is valid for
        int maxframes,        // a9: maximum frames to predict
        float frametime,      // a10
        int stopevent,        // a11: SE_* stop-event mask
        int visualize)        // a12: draw AAS debug lines
{
  // Declaration order follows the author's own Q3 be_aas_move.c: floats, then
  // the int cluster (n, i, j, pc, ...), then vectors, then planes/traces.
  float phys_friction;
  float phys_stopspeed;
  float phys_gravity;
  float phys_waterfriction;
  float phys_watergravity;
  float phys_maxwalkvelocity;
  float phys_maxcrouchvelocity;
  float phys_maxswimvelocity;
  float phys_maxacceleration;
  float phys_maxstep;
  float phys_maxsteepness;
  float phys_jumpvel;
  float friction;
  float gravity; // float, NOT long double (Q3 be_aas_move.c declares it float)
  float delta; // NOT long double: keeps the fall-damage chain's fmul/fcomp on DWORD operands
  float damage; // st7
  float maxvel;
  float velchange; // float, NOT long double: long double compares suppress the
                   // memory-operand fcom forms the original uses
  float newvel;    // ditto; ref round-trips it through the temp region (1000fb56/fb5c)
  float backoff_left; // [esp+0h] [ebp-1F8h]
  float backoff_frame; // [esp+0h] [ebp-1F8h]
  int n;
  int i;           // per-axis accel loop index (Q3 be_aas_move.c:620 fossil)
  int j;           // trace-loop safety counter (Q3 be_aas_move.c:648/876). Plain int;
                   // in ref it loses the ebx contest to presencetype and spills into
                   // the compiler temp region ([esp+0x2c], shared with the QWORD
                   // gravity CSE at 1000f9e5 -- proof this was never a declared float)
  int pc; // eax
  int ax; // eax
  int crouch; // edi
  int event; // ecx
  int jump_frame;
  int landed; // ecx
  BOOL swimming; // esi
  int v57;               // swimming flag during the frame loop; the same slot carries
                         // point-contents (pc) on the landing/liquid path — ONE variable,
                         // matching the original's merged slot.
                         // The slot has exactly these two non-overlapping roles --
                         // the apparent extra touches nearby are push-depth-shifted reads of
                         // start[2]/&start/&left_test_vel, not this slot. Do not re-chase.
  char gap_pc; // al
  vec3_t org;            // BYREF
  vec3_t end;            // BYREF
  float feet[3]; // BYREF
  vec3_t start;          // BYREF
  float stepend[3]; // BYREF
  vec3_t lastorg; // BYREF (Q3 be_aas_move.c's lastorg)
  vec3_t frame_test_vel; // BYREF
  vec3_t old_frame_test_vel; // Q3 be_aas_move.c:525 vec3_t old_frame_test_vel) --
                             // ref allocates the full 3-float slot but only [2] is ever stored
                             // (1000fe8d) or read (1000fee4/fefc/ff1f); [0]/[1] are dead padding,
                             // so only its z component is ever read.
  vec3_t left_test_vel;  // BYREF
  vec3_t up = {0, 0, 1};
  aas_plane_t *plane; // ebp
  aas_plane_t *plane2; // eax
  float v32;
  float v33;
  /* aas_clientmove_t scratch, copied to `move` at the tail.  Stays a raw int[20]
   * — do NOT retype to Q3's struct, whose layout differs (no `endarea`, and an
   * int `endcontents` where this has a float).  dword index:
   *   [0..2]=endpos [3..5]=velocity [6..14]=trace [15]=presencetype
   *   [16]=stopevent [17]=float@0x44 (4.0f bits, or (float)pc on the liquid
   *   path) [18]=time (n*frametime) [19]=frames (n). */
  int move_buf[20]; // BYREF
  aas_trace_t trace; // (plus its hidden return buffer)
  aas_trace_t steptrace; // (plus its hidden return buffer)

  phys_friction = libvar_sv_friction->value;
  phys_stopspeed = libvar_sv_stopspeed->value;
  phys_gravity = libvar_sv_gravity->value;
  phys_waterfriction = libvar_sv_waterfriction->value;
  phys_watergravity = libvar_sv_watergravity->value;
  phys_maxwalkvelocity = frametime * libvar_sv_maxwalkvelocity->value;
  phys_maxcrouchvelocity = frametime * libvar_sv_maxcrouchvelocity->value;
  phys_maxswimvelocity = frametime * libvar_sv_maxswimvelocity->value;
  phys_maxacceleration = frametime * libvar_sv_maxaccelerate->value;
  phys_maxstep = libvar_sv_step->value;
  phys_maxsteepness = libvar_sv_maxsteepness->value;
  phys_jumpvel = frametime * libvar_sv_jumpvel->value;
  memset(move_buf, 0, sizeof(move_buf));
  memset(&trace, 0, sizeof(trace));
  VectorCopy(origin, org);
  org[2] += 0.25;
  VectorScale(velocity, frametime, frame_test_vel);
  jump_frame = -1;
  for ( n = 0; n < maxframes; n++ )
  {
    swimming = AAS_Swimming(org);
    v57 = swimming;
    gravity = swimming ? phys_watergravity : phys_gravity;
    frame_test_vel[2] = frame_test_vel[2] - gravity * frametime * 0.1;
    if ( onground || swimming )
    {
      friction = swimming ? phys_friction : phys_waterfriction;
      VectorScale(frame_test_vel, 10.0f, frame_test_vel);
      AAS_ApplyFriction(frame_test_vel, friction, phys_stopspeed, frametime);
      VectorScale(frame_test_vel, 0.1f, frame_test_vel);
    }
    crouch = 0;
    if ( n < cmdframes )
    {
      maxvel = phys_maxwalkvelocity;
      ax = 0;
      if ( onground )
      {
        if ( cmdmove[2] < -300.0f )
        {
          maxvel = phys_maxcrouchvelocity;
          crouch = 1;
        }
        if ( !swimming && cmdmove[2] > 1.0f )
        {
          jump_frame = n;
          frame_test_vel[2] = (float)((float)phys_jumpvel - gravity * (float)frametime * 0.1 + 5.0f);
        }
        ax = 2;
      }
      if ( swimming )
      {
        maxvel = phys_maxswimvelocity;
        ax = 3;
      }
      if ( swimming || ax > 0 )
      {
        for ( i = 0; i < ax; i++ )
        {
          velchange = frametime * cmdmove[i] - frame_test_vel[i];
          if ( velchange > phys_maxacceleration )
            velchange = phys_maxacceleration;
          else if ( velchange < -phys_maxacceleration )
            velchange = -phys_maxacceleration;
          frame_test_vel[i] = newvel = velchange + frame_test_vel[i];
          if ( newvel > maxvel )
            frame_test_vel[i] = maxvel;
          else if ( newvel < -maxvel )
            frame_test_vel[i] = -maxvel;
        }
      }
    }
    if ( crouch )
    {
      presencetype = 4;
      goto LABEL_12;
    }
    if ( presencetype == 4 && (AAS_PointContents((float *)org) & 2) != 0 )
      presencetype = 2;
LABEL_12:
    VectorCopy(org, lastorg);
    VectorCopy(frame_test_vel, left_test_vel);
    j = 0;
    do
    {
      VectorAdd(left_test_vel, org, end);
      trace = AAS_TraceClientBBox(org, end, presencetype, entnum);
      if ( visualize )
      {
        if ( trace.startsolid )
          botimport.Print(PRT_MESSAGE, "PredictMovement: start solid\n");
        AAS_DebugLine(org, trace.endpos, -218959632);
      }
      VectorCopy(trace.endpos, org);
      if ( trace.fraction >= 1.0 )
        goto LABEL_66;
      plane = (aas_plane_t *)AAS_PlaneFromNum(trace.planenum);
      if ( plane->normal[2] == 0.0f && (jump_frame < 0 || n - jump_frame > 2) )
      {
        VectorMA(org, -0.25f, plane->normal, start);
        VectorCopy(start, stepend);
        start[2] = start[2] + phys_maxstep;
        steptrace = AAS_TraceClientBBox(start, stepend, presencetype, entnum);
        if ( !steptrace.startsolid )
        {
          plane2 = (aas_plane_t *)AAS_PlaneFromNum(steptrace.planenum);
          if ( DotProduct(plane2->normal, up) > (float)phys_maxsteepness )
          {
            VectorSubtract(end, steptrace.endpos, left_test_vel);
            left_test_vel[2] = 0.0f;
            frame_test_vel[2] = 0.0f;
            if ( visualize )
            {
              if ( steptrace.endpos[2] - org[2] > 0.125 )
              {
                VectorCopy(org, start);
                start[2] = steptrace.endpos[2];
                AAS_DebugLine(org, start, -202116623);
              }
            }
            org[2] = steptrace.endpos[2];
            goto LABEL_66;
          }
        }
      }
      v32 = (float)left_test_vel[1] * (float)plane->normal[1];
      v33 = (float)left_test_vel[2] * (float)plane->normal[2];
      backoff_left = (float)(-(v32 + v33 + (float)left_test_vel[0] * (float)plane->normal[0]));
      VectorMA(left_test_vel, backoff_left, plane->normal, left_test_vel);
      v32 = (float)frame_test_vel[1] * (float)plane->normal[1];
      v33 = (float)frame_test_vel[2] * (float)plane->normal[2];
      old_frame_test_vel[2] = frame_test_vel[2];
      backoff_frame = (float)(-(v32 + v33 + (float)frame_test_vel[0] * (float)plane->normal[0]));
      VectorMA(frame_test_vel, backoff_frame, plane->normal, frame_test_vel);
      if ( DotProduct(plane->normal, up) > (float)phys_maxsteepness )
      {
        landed = 1;
        onground = 1;
      }
      else
      {
        landed = onground;
      }
      if ( (stopevent & 0x20) == 0 )    // !SE_HITGROUNDDAMAGE
        goto LABEL_66;
      // Q3 be_aas_move.c's landing-damage delta:
      if ( old_frame_test_vel[2] < 0.0f && (float)frame_test_vel[2] > (float)old_frame_test_vel[2] && !landed )
        delta = (float)old_frame_test_vel[2];                              // still falling
      else if ( landed )
        delta = (float)frame_test_vel[2] - (float)old_frame_test_vel[2];   // landed this frame
      else
        goto LABEL_66;                                        // neither -> no fall damage
      if ( delta != 0.0f )
      {
        delta = delta * 10.0f;
        damage = delta * delta * 0.0001;
        if ( !v57 && damage > 30.0f )
        {
          move_buf[0] = *(int *)&org[0];
          move_buf[1] = *(int *)&org[1];
          move_buf[2] = *(int *)&org[2];
          move_buf[3] = *(int *)&frame_test_vel[0];
          *(float *)&move_buf[4] = frame_test_vel[1];
          move_buf[5] = *(int *)&frame_test_vel[2];
          memcpy(&move_buf[6], &trace, sizeof(aas_trace_t));
          move_buf[16] = 32;
          goto LABEL_86;
        }
      }
LABEL_66:
      if ( ++j > 20 )
        return *(aas_clientmove_t *)move_buf;
    }
    while ( trace.fraction < 1.0 );
    // Q3 be_aas_move.c:880 — probe the feet only when descending; the onground
    // check below then runs unconditionally.
    if ( frame_test_vel[2] <= 0.0f )
    {
      VectorCopy(org, feet);
      feet[2] = feet[2] - 22.0f;
      pc = sub_10003080((float *)feet);   
      event = 0;
      v57 = pc;   // slot reused: the 'swimming' (v57) slot now carries point-contents pc
      // assemble SE_ENTER* from the Q2 contents bits at the feet (cf. Q3 be_aas_move.c:888):
      //   CONTENTS_LAVA(8) -> SE_ENTERLAVA(16); CONTENTS_SLIME(0x10) -> SE_ENTERSLIME(8).
      //   NB CONTENTS_WATER(0x20) also maps to bit 8 here, not SE_ENTERWATER(4) —
      //   faithful to the original DLL; Q3 later split water out to SE_ENTERWATER.
      if ( (pc & 8) != 0 )
        event = 16;
      if ( (pc & 0x10) != 0 )
        event |= 8u;
      if ( (pc & 0x20) != 0 )
        event |= 8u;
      if ( (event & stopevent) != 0 )
      {
        move_buf[0] = *(int *)&org[0];
        move_buf[1] = *(int *)&org[1];
        *(float *)&move_buf[17] = (float)v57;
        move_buf[2] = *(int *)&org[2];
        move_buf[16] = stopevent & event;
        move_buf[3] = *(int *)&frame_test_vel[0];
        *(float *)&move_buf[4] = frame_test_vel[1];
        move_buf[5] = *(int *)&frame_test_vel[2];
        move_buf[15] = presencetype;
        *(float *)&move_buf[18] = (float)n * frametime;
        move_buf[19] = n;
        goto LABEL_88;
      }
    }
    onground = AAS_OnGround(org, presencetype, entnum);
    if ( onground )
    {
      if ( (stopevent & 1) != 0 )
      {
        move_buf[0] = *(int *)&org[0];
        move_buf[1] = *(int *)&org[1];
        move_buf[2] = *(int *)&org[2];
        move_buf[3] = *(int *)&frame_test_vel[0];
        *(float *)&move_buf[4] = frame_test_vel[1];
        move_buf[5] = *(int *)&frame_test_vel[2];
        memcpy(&move_buf[6], &trace, sizeof(aas_trace_t));
        move_buf[16] = 1;
        goto LABEL_86;
      }
    }
    else if ( (stopevent & 2) != 0 )
    {
      move_buf[2] = *(int *)&org[2];
      move_buf[0] = *(int *)&org[0];
      move_buf[1] = *(int *)&org[1];
      move_buf[5] = *(int *)&frame_test_vel[2];
      move_buf[3] = *(int *)&frame_test_vel[0];
      *(float *)&move_buf[4] = frame_test_vel[1];
      memcpy(&move_buf[6], &trace, sizeof(aas_trace_t));
      move_buf[16] = 2;
      move_buf[15] = presencetype;
      move_buf[17] = 1082130432;
      move_buf[19] = n;
      *(float *)&move_buf[18] = (float)n * frametime;
      goto LABEL_88;
    }
    else
    {
      aas_trace_t gaptrace;

      if ( (stopevent & 0x40) == 0 )
        goto LABEL_84;
      /* Q3 writes this as `VectorCopy(org, start); VectorCopy(start, end);
       * end[2] -= 48 + phys_maxbarrier;`, and the dead `end[2] = org[2];` below is
       * that second copy's third component.  The interleave here is faithful, not
       * a decompiler scramble — both grouped forms were tried and regress.  Keep
       * the element order. */
      start[0] = org[0];
      end[0] = org[0];
      start[1] = org[1];
      start[2] = org[2];
      end[1] = org[1];
      end[2] = org[2];
      end[2] = org[2] - (libvar_sv_maxbarrier->value + 48.0f);
      gaptrace = AAS_TraceClientBBox(start, end, 4, -1);
      if ( gaptrace.startsolid )
        goto LABEL_84;
      if ( org[2] - libvar_sv_step->value - 1.0f <= gaptrace.endpos[2] )
        goto LABEL_84;
      gap_pc = sub_10003080((float *)end);   /* barrier-water check */
      if ( (gap_pc & 0x20) != 0 )
        goto LABEL_84;
      move_buf[1] = *(int *)&lastorg[1];
      move_buf[0] = *(int *)&lastorg[0];
      move_buf[2] = *(int *)&lastorg[2];
      *(float *)&move_buf[4] = frame_test_vel[1];
      move_buf[3] = *(int *)&frame_test_vel[0];
      move_buf[5] = *(int *)&frame_test_vel[2];
      memcpy(&move_buf[6], &trace, sizeof(aas_trace_t));
      move_buf[16] = 64;
      move_buf[15] = presencetype;
      move_buf[17] = 1082130432;
      move_buf[19] = n;
      *(float *)&move_buf[18] = (float)n * frametime;
      goto LABEL_88;
    }
LABEL_84:
    ;
  }
  move_buf[0] = *(int *)&org[0];
  move_buf[1] = *(int *)&org[1];
  move_buf[2] = *(int *)&org[2];
  move_buf[3] = *(int *)&frame_test_vel[0];
  *(float *)&move_buf[4] = frame_test_vel[1];
  move_buf[5] = *(int *)&frame_test_vel[2];
  move_buf[16] = 0;
LABEL_86:
  move_buf[15] = presencetype;
  move_buf[17] = 1082130432;
  move_buf[19] = n;
  *(float *)&move_buf[18] = (float)n * frametime;
LABEL_88:
  return *(aas_clientmove_t *)move_buf;
}
// gladiator.dll: 10010690..1001074D
// gladi386.so:   0001D124..0001D222
/* AAS_TestMovementPrediction — movement-prediction debug helper: flatten the
 * direction's Z unless swimming, normalize it, scale to 400 u/s, force a
 * +Z=224 jump impulse, clear the debug lines, then run one
 * AAS_ClientMovementPrediction of 13 frames at 0.1 s (presence=NORMAL,
 * onground, stopevent, visualize) and print "leave ground" if the result
 * flags set bit 0x02.
 *
 * DEAD — nothing references it; almost certainly a leftover development
 * test harness. */
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
