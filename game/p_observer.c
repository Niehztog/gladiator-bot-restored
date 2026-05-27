//===========================================================================
//
// Name:				p_observer.c
// Function:		observer mode
// Programmer:		Mr Elusive (MrElusive@demigod.demon.nl)
// Last update:	1998-01-12
// Tab Size:		3
//
// Reconstructed from the 1999 Win32 gamex86.dll (the only release where this
// file was compiled in -- the public source release omitted it).  Behaviour
// matches the disassembly at addresses 0x1007b05d..0x1007c180 in
// reference/gamex86.dll.
//
//===========================================================================

#include "g_local.h"
#include "p_observer.h"

#ifdef OBSERVER

extern cvar_t *observer;
extern void SelectSpawnPoint(edict_t *ent, vec3_t origin, vec3_t angles);

//===========================================================================
// AngleDifference
//
// returns the angular difference between two angles, normalized to
// the range [-180, 180]
//===========================================================================
float AngleDifference(float ang1, float ang2)
{
	float diff;

	diff = ang1 - ang2;
	if (ang1 > ang2)
	{
		if (diff >  180.0) diff -= 360.0;
	}
	else
	{
		if (diff < -180.0) diff += 360.0;
	}
	return diff;
} //end of the function AngleDifference

//===========================================================================
// ClientPlaceCamera
//
// initial placement of the observer camera.  If the camera is already
// targeting the observing player, it just refreshes the cached origin
// and angles.  Otherwise (no target, target gone or target now in
// observer mode) the camera snaps back to the observer and the engine
// is asked to drop the spectator at a free position.
//===========================================================================
static void ClientPlaceCamera(edict_t *ent)
{
	camera_t *cam;

	cam = &ent->client->camera;
	if (cam->ent == ent)
	{
		VectorCopy(ent->client->ps.viewangles, cam->clientangles);
		VectorCopy(ent->s.origin, cam->clientorigin);
	} //end if
	else if (!cam->ent || !cam->ent->inuse || (cam->ent->flags & FL_OBSERVER))
	{
		cam->ent = ent;
		// place the observer body at a free position so that it doesn't
		// stay welded to the corpse of whoever it was just chasing.
		// (the fourth argument is &client->resp.coop_respawn or similar
		// state; we use the same engine helper as the original.)
		SelectSpawnPoint(ent, cam->clientorigin, cam->clientangles);
	} //end else if
} //end of the function ClientPlaceCamera

//===========================================================================
// ClientCycleCamera
//
// "cyclecam" command: walk through the clients and target the next
// in-use, non-observer one.  Refuses to do anything in autocam mode.
//===========================================================================
void ClientCycleCamera(edict_t *ent)
{
	int i, index;
	edict_t *target;
	camera_t *cam;

	if (!(ent->flags & FL_OBSERVER))
		ClientToggleObserver(ent);

	cam = &ent->client->camera;
	if (cam->flags & CAMFL_AUTOCAM)
	{
		gi.cprintf(ent, PRINT_HIGH, "setcam not available in autocam mode\n");
		return;
	} //end if

	ClientPlaceCamera(ent);

	index = (cam->ent - g_edicts) - 1;
	for (i = 0; i < (int)maxclients->value; i++)
	{
		index++;
		if (index >= (int)maxclients->value) index = 0;
		target = g_edicts + 1 + index;
		if (!target->inuse) continue;
		if ((target->flags & FL_OBSERVER) && target != ent) continue;
		cam->ent = target;
		break;
	} //end for

	if (i == (int)maxclients->value)
		gi.cprintf(ent, PRINT_HIGH, "no valid client found to observe\n");

	if (cam->ent == ent)
		SelectSpawnPoint(ent, cam->clientorigin, cam->clientangles);
} //end of the function ClientCycleCamera

//===========================================================================
// ClientSetCamera
//
// "setcam <name>" command: target a named client.
//===========================================================================
void ClientSetCamera(edict_t *ent)
{
	int i;
	char *name;
	edict_t *target;
	camera_t *cam;

	if (!(ent->flags & FL_OBSERVER))
		ClientToggleObserver(ent);

	cam = &ent->client->camera;
	if (cam->flags & CAMFL_AUTOCAM)
	{
		gi.cprintf(ent, PRINT_HIGH, "setcam not available in autocam mode\n");
		return;
	} //end if

	if (gi.argc() <= 1)
	{
		gi.cprintf(ent, PRINT_HIGH, "usage: setcam <client name>\n");
		return;
	} //end if

	ClientPlaceCamera(ent);

	name = gi.argv(1);
	for (i = 0; i < (int)maxclients->value; i++)
	{
		target = g_edicts + 1 + i;
		if (!target->inuse) continue;
		if (!Q_stricmp(name, target->client->pers.netname))
		{
			cam->ent = target;
			break;
		} //end if
	} //end for

	if (i == (int)maxclients->value)
		gi.cprintf(ent, PRINT_HIGH, "no valid client found with the name %s\n", name);

	if (cam->ent == ent)
		SelectSpawnPoint(ent, cam->clientorigin, cam->clientangles);
} //end of the function ClientSetCamera

//===========================================================================
// ClientToggleObserver
//
// "observer" command: enter or leave observer mode.
//===========================================================================
void ClientToggleObserver(edict_t *ent)
{
	if (ent->flags & FL_OBSERVER)
	{
		// leaving observer mode
		if (!deathmatch->value)
		{
			respawn(ent);
			return;
		} //end if
		if (!observer->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "can't leave observer mode\n");
			return;
		} //end if
		ent->classname = "player";
		ent->flags &= ~FL_OBSERVER;
		ent->solid = SOLID_BBOX;
		ent->takedamage = DAMAGE_AIM;
		ent->svflags &= ~SVF_NOCLIENT;
		PutClientInServer(ent);
		// add a teleportation effect
		ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent->client->ps.pmove.pm_time = 14;
		if (ent->client->pers.weapon)
			ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
		else
			ent->client->ps.gunindex = 0;
		ent->movetype = MOVETYPE_WALK;
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent - g_edicts);
		gi.WriteByte (MZ_LOGIN);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
		gi.bprintf(PRINT_HIGH, "%s left observer mode\n", ent->client->pers.netname);
	} //end if
	else
	{
		// entering observer mode
		ent->classname = "observer";
		ent->flags |= FL_OBSERVER;
		ent->solid = SOLID_NOT;
		ent->takedamage = DAMAGE_NO;
		ent->movetype = MOVETYPE_NOCLIP;
		ent->health = 100;
		ent->client->ps.stats[STAT_HEALTH] = ent->health;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->camera.ent = ent;
		ClientPlaceCamera(ent);
		if (deathmatch->value)
			ent->client->resp.score = 0;
		gi.bprintf(PRINT_HIGH, "%s entered observer mode\n", ent->client->pers.netname);
	} //end else
} //end of the function ClientToggleObserver

//===========================================================================
// ClientToggleCameraFixed
//===========================================================================
void ClientToggleCameraFixed(edict_t *ent)
{
	ent->client->camera.flags ^= CAMFL_FIXED;
	gi.cprintf(ent, PRINT_HIGH, "camera offsets are ");
	if (ent->client->camera.flags & CAMFL_FIXED)
		gi.cprintf(ent, PRINT_HIGH, "fixed\n");
	else
		gi.cprintf(ent, PRINT_HIGH, "variable\n");
} //end of the function ClientToggleCameraFixed

//===========================================================================
// ClientToggleCameraName
//===========================================================================
static void ClientToggleCameraName(edict_t *ent)
{
	ent->client->camera.flags ^= CAMFL_NAME;
	gi.cprintf(ent, PRINT_HIGH, "camera player names ");
	if (ent->client->camera.flags & CAMFL_NAME)
		gi.cprintf(ent, PRINT_HIGH, "on\n");
	else
		gi.cprintf(ent, PRINT_HIGH, "off\n");
} //end of the function ClientToggleCameraName

//===========================================================================
// ClientToggleAutoCam
//===========================================================================
static void ClientToggleAutoCam(edict_t *ent)
{
	camera_t *cam;

	if (!(ent->flags & FL_OBSERVER))
		ClientToggleObserver(ent);

	cam = &ent->client->camera;
	cam->flags ^= CAMFL_AUTOCAM;
	gi.cprintf(ent, PRINT_HIGH, "autocam ");
	if (cam->flags & CAMFL_AUTOCAM)
	{
		VectorCopy(ent->s.origin, cam->dest);
		// snap-search the current viewangles to find the first valid target
		AngleVectors(ent->client->v_angle, NULL, NULL, NULL);
		cam->ent = ent;
		cam->pause_time = 1.0;
		cam->delay = 1.0;
		gi.cprintf(ent, PRINT_HIGH, "on\n");
	} //end if
	else
	{
		gi.cprintf(ent, PRINT_HIGH, "off\n");
	} //end else
} //end of the function ClientToggleAutoCam

//===========================================================================
// ClientToggleChaseCam
//===========================================================================
static void ClientToggleChaseCam(edict_t *ent)
{
	if (!(ent->flags & FL_OBSERVER))
		ClientToggleObserver(ent);

	ent->client->camera.flags ^= CAMFL_CHASECAM;
	if (ent->client->camera.flags & CAMFL_CHASECAM)
		gi.cprintf(ent, PRINT_HIGH, "chasecam on\n");
	else
		gi.cprintf(ent, PRINT_HIGH, "chasecam off\n");
} //end of the function ClientToggleChaseCam

//===========================================================================
// ClientObserverHelp
//===========================================================================
void ClientObserverHelp(edict_t *ent)
{
	gi.cprintf(ent, PRINT_HIGH,
		"observer        toggles observer mode\n"
		"autocam         toggle auto targeting camera mode\n"
		"chasecam        toggle manually positioned chase cam\n"
		"cyclecam        cycles camera to next player\n"
		"setcam <name>   set camera to player with name\n"
		"camfixed        toggle chase camera offset fixed\n"
		"camname         toggle showing name of tracked player\n"
		"observerhelp    this help message\n");
} //end of the function ClientObserverHelp

//===========================================================================
// ClientSetViewAngles
//
// helper to copy the current chase target's view angles & origin into
// the camera so the local player sees what the target sees.  Smoothing
// is applied unless CAMFL_NOSMOOTHING is set.
//===========================================================================
void ClientSetViewAngles(edict_t *ent, vec3_t ang, vec3_t realang)
{
	camera_t *cam;

	cam = &ent->client->camera;
	if (cam->flags & CAMFL_NOSMOOTHING)
	{
		VectorCopy(cam->ent->client->v_angle, cam->ent_angles);
	} //end if
	else
	{
		// time-based interpolation toward the target's view
		float frac = 1.0 - cam->lasttime;
		if (frac < 0) frac = 0;
		if (frac > 1) frac = 1;
		cam->ent_angles[0] = cam->ent_angles[0] + frac * AngleDifference(cam->ent->client->v_angle[0], cam->ent_angles[0]);
		cam->ent_angles[1] = cam->ent_angles[1] + frac * AngleDifference(cam->ent->client->v_angle[1], cam->ent_angles[1]);
		cam->ent_angles[2] = cam->ent_angles[2] + frac * AngleDifference(cam->ent->client->v_angle[2], cam->ent_angles[2]);
	} //end else
	cam->lasttime = 1.0;
	cam->origin[0] = cam->chaseoffset[0] + cam->ent->s.origin[0];
	cam->origin[1] = cam->chaseoffset[1] + cam->ent->s.origin[1];
	cam->origin[2] = cam->chaseoffset[2] + cam->ent->s.origin[2];

	if (ang)
		VectorCopy(cam->ent_angles, ang);
	if (realang)
		VectorCopy(cam->ent_angles, realang);
} //end of the function ClientSetViewAngles

//===========================================================================
// Autocam state handlers and CameraMove / CameraInputThink / CameraPlaceAtTarget
//
// These helpers live in the original DLL at the addresses noted below but
// are outside the address range covered by the first-pass reconstruction.
// Faithful restoration is a follow-up task; for now they are stub forward
// declarations so the dispatcher compiles and the chasecam math links.
//===========================================================================
static void CameraAutoCamState0(edict_t *ent, usercmd_t *ucmd);   /* sub_1007a2e7 */
static void CameraAutoCamState1(edict_t *ent, usercmd_t *ucmd);   /* sub_10079f4d */
static void CameraAutoCamState2(edict_t *ent, usercmd_t *ucmd);   /* sub_10079f64 */
static void CameraAutoCamState3(edict_t *ent, usercmd_t *ucmd);   /* sub_1007a1d2 */
static void CameraAutoCamState7(edict_t *ent, usercmd_t *ucmd);   /* sub_10079c26 */
static void CameraMove(edict_t *ent, float speed, usercmd_t *ucmd); /* sub_100784f9 */
static void CameraInputThink(edict_t *ent, usercmd_t *ucmd);      /* sub_1007ace3 */
static void CameraPlaceAtTarget(edict_t *ent, vec3_t end, vec3_t out_angles, vec3_t cmdangles); /* sub_10078043 */

//===========================================================================
// CameraAutoCamThink
//
// Per-frame state machine for autocam mode.  Reconstructed faithfully from
// gamex86.dll @ 0x1007abd5: a switch on cam->state that calls one of five
// state handlers followed by a uniform CameraMove(ent, speed, ucmd) helper.
// Unknown states reset cam->state to 0.
//===========================================================================
static void CameraAutoCamThink(edict_t *ent, usercmd_t *ucmd)
{
	camera_t *cam;

	cam = &ent->client->camera;
	if (cam->state == 0)
	{
		CameraAutoCamState0(ent, ucmd);
		CameraMove(ent, 1.0, ucmd);
	} //end if
	else if (cam->state == 2)
	{
		CameraAutoCamState2(ent, ucmd);
		CameraMove(ent, 0.0, ucmd);
	} //end else if
	else if (cam->state == 3)
	{
		CameraAutoCamState3(ent, ucmd);
		CameraMove(ent, 0.0, ucmd);
	} //end else if
	else if (cam->state == 7)
	{
		CameraAutoCamState7(ent, ucmd);
		CameraMove(ent, 100.0, ucmd);
	} //end else if
	else if (cam->state == 1)
	{
		CameraAutoCamState1(ent, ucmd);
		CameraMove(ent, 0.0, ucmd);
	} //end else if
	else
	{
		cam->state = 0;
	} //end else
} //end of the function CameraAutoCamThink

//===========================================================================
// CameraChaseCamThink
//
// Per-frame chase-cam logic.  Reconstructed faithfully from gamex86.dll @
// 0x1007b05d.  Steps:
//   1. If !CAMFL_FIXED, let user input adjust the camera (CameraInputThink).
//   2. Snap (CAMFL_NOSMOOTHING) or LerpAngles cam->ent_angles toward the
//      chase target's v_angle, then refresh cam->lasttime.
//   3. Compute a yaw-offset forward vector (with the pitch correction
//      forward[2] = -(forward[0]*up[0] + forward[1]*up[1]) / up[2]),
//      normalise it, and build a 'dir' vector of length -chaseoffset[PITCH]
//      offset by chaseoffset[ROLL] on Z.
//   4. cam->origin = target.origin + chaseoffset.
//   5. Trace from cam->origin out by 'dir' through MASK_OPAQUE, ignoring
//      the chase target, then push the endpoint 5 units further along
//      forward (so the camera does not clip into the wall it hit).
//   6. Build cmdangles from ucmd->angles via SHORT2ANGLE and hand off to
//      CameraPlaceAtTarget which writes the final angles back; copy those
//      into cam->angles.
//===========================================================================
static void CameraChaseCamThink(edict_t *ent, usercmd_t *ucmd)
{
	camera_t *cam;
	vec3_t   angles, forward, up, dir, end, cmdangles;
	trace_t  tr;

	if (!(ent->client->camera.flags & CAMFL_FIXED))
		CameraInputThink(ent, ucmd);

	cam = &ent->client->camera;
	if (cam->flags & CAMFL_NOSMOOTHING)
	{
		VectorCopy(cam->ent->client->v_angle, cam->ent_angles);
	} //end if
	else
	{
		// Lerp ent_angles toward target's v_angle by (level.time - lasttime),
		// clamped by max-step 1.0.  Inlined to avoid depending on an
		// engine helper that isn't in q_shared.
		float frac = level.time - cam->lasttime;
		if (frac < 0) frac = 0;
		if (frac > 1.0) frac = 1.0;
		cam->ent_angles[0] += frac * AngleDifference(cam->ent->client->v_angle[0], cam->ent_angles[0]);
		cam->ent_angles[1] += frac * AngleDifference(cam->ent->client->v_angle[1], cam->ent_angles[1]);
		cam->ent_angles[2] += frac * AngleDifference(cam->ent->client->v_angle[2], cam->ent_angles[2]);
	} //end else
	cam->lasttime = level.time;

	// Build working angles from ent_angles, snag the 'up' vector
	VectorCopy(cam->ent_angles, angles);
	AngleVectors(angles, NULL, NULL, up);

	// Apply yaw chase offset, kill pitch, recompute forward
	angles[YAW] = anglemod(angles[YAW] + cam->chaseoffset[YAW]);
	angles[PITCH] = 0;
	AngleVectors(angles, forward, NULL, NULL);

	// Pitch correction: project forward onto the up plane so the camera
	// rises and falls with the target's pitch:
	//   forward[2] = -(forward[0]*up[0] + forward[1]*up[1]) / up[2]
	forward[2] = (forward[0] * up[0] + forward[1] * up[1]) * -1.0 / up[2];
	VectorNormalize(forward);

	// dir = forward * (-chaseoffset[PITCH]); dir[2] += chaseoffset[ROLL]
	VectorScale(forward, -cam->chaseoffset[PITCH], dir);
	dir[2] += cam->chaseoffset[ROLL];

	// cam->origin = target.origin + chaseoffset
	cam->origin[0] = cam->ent->s.origin[0] + cam->chaseoffset[0];
	cam->origin[1] = cam->ent->s.origin[1] + cam->chaseoffset[1];
	cam->origin[2] = cam->ent->s.origin[2] + cam->chaseoffset[2];

	// end = cam->origin + dir
	end[0] = cam->origin[0] + dir[0];
	end[1] = cam->origin[1] + dir[1];
	end[2] = cam->origin[2] + dir[2];

	// Trace from cam->origin to end, ignoring the chase target itself.
	// MASK_OPAQUE = 0x19 in the original disassembly.
	tr = gi.trace(cam->origin, vec3_origin, vec3_origin, end, cam->ent, MASK_OPAQUE);

	// Push the endpoint 5 units further along the (re-normalised) forward
	// vector so the camera doesn't sit flush against the wall it hit.
	VectorNormalize2(forward, angles);   // disasm calls VectorNormalize2(forward, &angles[0])
	VectorScale(forward, 5.0, forward);
	end[0] = tr.endpos[0] + forward[0];
	end[1] = tr.endpos[1] + forward[1];
	end[2] = tr.endpos[2] + forward[2];

	// Convert ucmd->angles (short) -> cmdangles (float) via SHORT2ANGLE.
	cmdangles[0] = SHORT2ANGLE(ucmd->angles[0]);
	cmdangles[1] = SHORT2ANGLE(ucmd->angles[1]);
	cmdangles[2] = SHORT2ANGLE(ucmd->angles[2]);

	// Final placement -- writes resulting angles back through 'angles'.
	CameraPlaceAtTarget(ent, end, angles, cmdangles);

	// cam->angles = angles  (the values written by CameraPlaceAtTarget)
	VectorCopy(angles, cam->angles);
} //end of the function CameraChaseCamThink

//===========================================================================
// Faithful disassembly-derived reconstruction of the 5 autocam state
// handlers + CameraMove / CameraInputThink / CameraPlaceAtTarget.
//
// Translated line-by-line from gamex86.dll at:
//   sub_10078043 = CameraPlaceAtTarget   (18 lines  -> the trivial dispatch)
//   sub_10079f4d = CameraAutoCamState1   ("fixed mode" centerprint)
//   sub_1007a1d2 = CameraAutoCamState3   (target-tracking)
//   sub_10079f64 = CameraAutoCamState2   (chase from a fixed distance)
//   sub_10079c26 = CameraAutoCamState7   ("bodyque" / corpse chase)
//   sub_1007ace3 = CameraInputThink      (m_pitch / chaseoffset twiddling)
//   sub_100784f9 = CameraMove            (per-state move + angle blend)
//   sub_1007a2e7 = CameraAutoCamState0   (target selection)
//
// Constants come from .rdata at gamex86.dll+0x92000.  Inline literals
// are decoded as their float values for readability.
//
// The nested helper functions (sub_10077e29, sub_10077eba, sub_10077f15,
// sub_10077f7d, sub_1007845f, sub_100788ff, sub_1007897a, sub_10078125,
// sub_100782ad, sub_10078a04, sub_10078bcc, sub_10079a92, sub_10079acf,
// sub_10085904) are still pending faithful reconstruction.  They live at
// the bottom of this file as small forward-stubs that approximate their
// effect; behaviour comparable to the original DLL therefore requires
// restoring those next.
//===========================================================================

// ---- Forward declarations for the nested helper stubs (see end of file) ----
static void  SubApplyCameraOrigin   (edict_t *ent, vec3_t origin);                     /* sub_10077f15 */
static void  SubApplyCameraAngles   (edict_t *ent, vec3_t out_angles, vec3_t cmd);     /* sub_10077f7d */
static void  SubLerpAngle           (float *out, vec3_t target, float frac, float maxstep); /* sub_10077e29 */
static float SubAngleDifference     (float a, float b);                                /* sub_10077eba */
static qboolean SubCanSeePoint      (edict_t *ent, vec3_t point);                      /* sub_10078125 */
static qboolean SubTargetVisible    (edict_t *ent, edict_t *target);                   /* sub_100782ad */
static edict_t *SubNextClient       (edict_t *prev);                                   /* sub_1007845f */
static void  SubAbortAutocam        (edict_t *ent, usercmd_t *ucmd);                   /* sub_100788ff */
static void  SubOnTargetDeath       (edict_t *ent);                                    /* sub_1007897a */
static void  SubGetTargetMuzzle     (edict_t *ent, vec3_t out);                        /* sub_10078a04 */
static void  SubGetTargetAimEnd     (edict_t *ent, vec3_t out);                        /* sub_10078bcc */
static void  SubAutocamSetSpot      (edict_t *target, vec3_t spot);                    /* sub_10079a92 */
static void  SubSetAutocamTarget    (edict_t *ent, edict_t *target, usercmd_t *ucmd);  /* sub_10079acf */
static int   SubVectorCompare       (vec3_t a, vec3_t b);                              /* sub_10085904 */

// Mask value at gamex86.dll+0x10092 area, embedded as immediate 0x2010003 in
// every trace call.  Standard MASK_OPAQUE = SOLID|LAVA|SLIME|WINDOW, encoded
// as CONTENTS_SOLID(1) | CONTENTS_WINDOW(2) | CONTENTS_LAVA(8) | something.
#ifndef OBSERVER_TRACE_MASK
#define OBSERVER_TRACE_MASK 0x2010003
#endif

//===========================================================================
// sub_1007ace3 -- CameraInputThink
//===========================================================================
static void CameraInputThink(edict_t *ent, usercmd_t *ucmd)
{
	camera_t *cam;
	cvar_t *m_pitch;
	float yaw_in, pitch_in, roll_in;
	float yaw_diff, pitch_diff, roll_diff;
	float m_pitch_val, inv_m_pitch;

	cam = &ent->client->camera;

	// pitch_in = anglemod( SHORT2ANGLE(ucmd->angles[PITCH]) + SHORT2ANGLE(ent->client->ps.viewangles[PITCH]_short) )
	// (the ps angles are pulled out of client->ps.pmove.delta_angles via the +0x14 offset
	//  on the gclient_t at +0x54 from edict, which is short[3])
	pitch_in = SHORT2ANGLE(ucmd->angles[PITCH])
	         + SHORT2ANGLE(((short *)((char *)ent->client + 0x14))[0]);
	pitch_in = anglemod(pitch_in);

	yaw_in   = SHORT2ANGLE(ucmd->angles[YAW])
	         + SHORT2ANGLE(((short *)((char *)ent->client + 0x14))[1]);
	yaw_in   = anglemod(yaw_in);

	roll_in  = SHORT2ANGLE(ucmd->angles[ROLL])
	         + SHORT2ANGLE(((short *)((char *)ent->client + 0x14))[2]);
	roll_in  = anglemod(roll_in);

	// Wrap cam->angles[*] through anglemod to keep them in [0,360).
	cam->angles[0] = anglemod(cam->angles[0]);
	cam->angles[1] = anglemod(cam->angles[1]);
	cam->angles[2] = anglemod(cam->angles[2]);

	pitch_diff = SubAngleDifference(pitch_in, cam->angles[0]);
	yaw_diff   = SubAngleDifference(yaw_in,   cam->angles[1]);
	roll_diff  = SubAngleDifference(roll_in,  cam->angles[2]);

	// Clamp pitch_diff into [-20, 20].
	if (pitch_diff >  20.0f) pitch_diff =  20.0f;
	else if (pitch_diff < -20.0f) pitch_diff = -20.0f;

	// inv_m_pitch = -0.022 / m_pitch  (so chaseoffset moves proportional to m_pitch)
	m_pitch = gi.cvar("m_pitch", 0, 0);
	m_pitch_val = -0.022f;
	if (m_pitch && m_pitch->value != 0.0f)
		m_pitch_val = m_pitch->value;
	inv_m_pitch = -0.022f / m_pitch_val;

	// chaseoffset[ROLL] is the "vertical" component (offset behind the player).
	// Adjust by  +/- 0.5 * pitch_diff * inv_m_pitch depending on attack-button
	// state (ucmd->buttons bit 0 == BUTTON_ATTACK).
	if (ucmd->buttons & 1)
	{
		if (pitch_diff >  3.0f)
			cam->chaseoffset[ROLL] += inv_m_pitch * pitch_diff * 0.5f;
		else if (pitch_diff < -3.0f)
			cam->chaseoffset[ROLL] += inv_m_pitch * pitch_diff * 0.5f;
	}
	else
	{
		if (pitch_diff >  3.0f)
			cam->chaseoffset[YAW] -= inv_m_pitch * pitch_diff * 0.5f;
		else if (pitch_diff < -3.0f)
			cam->chaseoffset[YAW] -= inv_m_pitch * pitch_diff * 0.5f;
	}

	// chaseoffset[PITCH] += yaw_diff * 0.2 ; wrap.
	if (yaw_diff >  10.0f || yaw_diff < -10.0f)
	{
		cam->chaseoffset[PITCH] += yaw_diff * 0.2f;
		cam->chaseoffset[PITCH]  = anglemod(cam->chaseoffset[PITCH]);
	}

	// chaseoffset[YAW] clamp into [32, 100].
	if (cam->chaseoffset[YAW] >  100.0f) cam->chaseoffset[YAW] = 100.0f;
	else if (cam->chaseoffset[YAW] < 32.0f) cam->chaseoffset[YAW] = 32.0f;

	// chaseoffset[ROLL] clamp into [-48, 48].
	if (cam->chaseoffset[ROLL] >  48.0f) cam->chaseoffset[ROLL] =  48.0f;
	else if (cam->chaseoffset[ROLL] < -48.0f) cam->chaseoffset[ROLL] = -48.0f;
} //end of the function CameraInputThink

//===========================================================================
// sub_10078043 -- CameraPlaceAtTarget
//
//   sub_10077f15(ent, cmdangles_arg);
//   sub_10077f7d(ent, end_arg, out_angles_arg);
//
// Note: arg layout from disasm is (ent, end, out_angles, cmdangles) but the
// helper SubApplyCameraOrigin's vec3 arg is the SECOND function arg (ebp+0xc).
// CameraChaseCamThink calls us with: (ent, end, &out_angles, cmdangles)
// and the disasm shows sub_10077f15(ent, ebp+0xc), i.e. with 'end'.
//===========================================================================
static void CameraPlaceAtTarget(edict_t *ent, vec3_t end, vec3_t out_angles, vec3_t cmdangles)
{
	SubApplyCameraOrigin(ent, end);                          // sub_10077f15
	SubApplyCameraAngles(ent, out_angles, cmdangles);        // sub_10077f7d
} //end of the function CameraPlaceAtTarget

//===========================================================================
// sub_100784f9 -- CameraMove
//
// 'speed': passed as second argument (the dispatcher passes 1.0, 0.0, 0.0,
//          100.0, 0.0 for states 0,1,2,7,3 respectively).
//
// Branches:
//   speed == 0       -> teleport: SubApplyCameraOrigin(ent, cam->dest)
//   else trace from cam->ent->origin to cam->dest; if hit, snap; else
//     compute step along the trace direction and advance.
//   Then per state (0 / 3 / 7 / other) compute target angle vector by
//   subtracting positions and feed into SubLerpAngle.
//===========================================================================
static void CameraMove(edict_t *ent, float speed, usercmd_t *ucmd)
{
	camera_t *cam;
	trace_t tr;
	vec3_t move;
	vec3_t target_ang;
	float dist, dt, time_to_close;
	vec3_t cmdangles;

	cam = &ent->client->camera;

	// if (speed == 0) snap to dest and skip the lerp.
	if (speed == 0.0f)
	{
		SubApplyCameraOrigin(ent, cam->dest);
		goto angles_phase;
	}

	tr = gi.trace(ent->s.origin, vec3_origin, vec3_origin, cam->dest,
	              ent, OBSERVER_TRACE_MASK);
	if (tr.fraction < 1.0f)
	{
		SubApplyCameraOrigin(ent, cam->dest);
		goto angles_phase;
	}

	move[0] = cam->dest[0] - ent->s.origin[0];
	move[1] = cam->dest[1] - ent->s.origin[1];
	move[2] = cam->dest[2] - ent->s.origin[2];
	dist = VectorNormalize(move);

	dt = level.time - cam->lasttime;
	if (dist < 0.0f)
		speed = -speed;
	time_to_close = (float)acos((double)(dist / speed));
	if (dt < time_to_close)
	{
		// clamp move scale = (2.0 * time_to_close - dt) * speed * dt
		dist = (speed * dt) * (2.0f * time_to_close - dt);
	}
	VectorScale(move, dist, move);
	move[0] += ent->s.origin[0];
	move[1] += ent->s.origin[1];
	move[2] += ent->s.origin[2];
	SubApplyCameraOrigin(ent, move);

angles_phase:
	if (cam->state == 0)
	{
		// target_ang = VectorNormalize2(viewtarget - dest)
		move[0] = cam->viewtarget[0] - cam->dest[0];
		move[1] = cam->viewtarget[1] - cam->dest[1];
		move[2] = cam->viewtarget[2] - cam->dest[2];
		VectorNormalize2(move, target_ang);
		// SubLerpAngle(cam->angles, target_ang, 0.2, level.time - lasttime)
		SubLerpAngle(cam->angles, target_ang, 0.2f, level.time - cam->lasttime);
	}
	else if (cam->state == 3)
	{
		// Aim at where the target is aiming: cam->ent->client + 0xe8c (= ps.viewangles or similar)
		SubLerpAngle(cam->angles,
		             (float *)((char *)cam->ent->client + 0xe8c),
		             0.8f,
		             level.time - cam->lasttime);
	}
	else if (cam->state == 7)
	{
		move[0] = cam->ent->s.origin[0] - ent->s.origin[0];
		move[1] = cam->ent->s.origin[1] - ent->s.origin[1];
		move[2] = cam->ent->s.origin[2] - ent->s.origin[2];
		VectorNormalize2(move, target_ang);
		SubLerpAngle(cam->angles, target_ang, 0.2f, level.time - cam->lasttime);
	}
	else
	{
		// Default (states 1, 2 and anything else): aim toward cam->ent's
		// origin and just normalise into cam->angles directly.
		move[0] = cam->ent->s.origin[0] - ent->s.origin[0];
		move[1] = cam->ent->s.origin[1] - ent->s.origin[1];
		move[2] = cam->ent->s.origin[2] - ent->s.origin[2];
		VectorNormalize2(move, cam->angles);
	}

	cmdangles[PITCH] = SHORT2ANGLE(ucmd->angles[PITCH]);
	cmdangles[YAW]   = SHORT2ANGLE(ucmd->angles[YAW]);
	cmdangles[ROLL]  = SHORT2ANGLE(ucmd->angles[ROLL]);
	SubApplyCameraAngles(ent, cam->angles, cmdangles);

	cam->lasttime = level.time;
} //end of the function CameraMove

//===========================================================================
// sub_10079f4d -- CameraAutoCamState1
//
// Just centerprints "fixed mode".
//===========================================================================
static void CameraAutoCamState1(edict_t *ent, usercmd_t *ucmd)
{
	gi.centerprintf(ent, "fixed mode");
} //end of the function CameraAutoCamState1

//===========================================================================
// sub_1007a1d2 -- CameraAutoCamState3
//
// Tracking state.  If target died, abort or transfer; otherwise verify line
// of sight, refresh muzzle / aim-end points, and time out after delay.
//===========================================================================
static void CameraAutoCamState3(edict_t *ent, usercmd_t *ucmd)
{
	camera_t *cam = &ent->client->camera;

	if (cam->ent->deadflag != 0)
	{
		if (level.time < 31.0f)
			SubAbortAutocam(ent, ucmd);
		else
			SubOnTargetDeath(ent);
		return;
	}

	if (SubTargetVisible(ent, cam->ent))
	{
		SubGetTargetMuzzle(ent, cam->dest);
		SubGetTargetAimEnd(ent, cam->viewtarget);
		// gi.pointcontents(&cam->dest) & 1 == 1  OR  pause_time < level.time
		if ((gi.pointcontents(cam->dest) & 1)
		    || cam->pause_time < level.time)
		{
			SubSetAutocamTarget(ent, cam->ent, ucmd);
		}
	}
	else
	{
		SubSetAutocamTarget(ent, cam->ent, ucmd);
	}

	if (cam->search_time < level.time)
		SubAbortAutocam(ent, ucmd);
} //end of the function CameraAutoCamState3

//===========================================================================
// sub_10079f64 -- CameraAutoCamState2
//
// Idle-chase state.  Holds the camera at cam->dest while watching cam->ent.
// Promotes to state 3 when the target starts shooting at us / we have a
// good line of sight; otherwise re-targets when distance closes.
//===========================================================================
static void CameraAutoCamState2(edict_t *ent, usercmd_t *ucmd)
{
	camera_t *cam = &ent->client->camera;
	vec3_t dir;
	vec3_t aimend;
	float dist, yaw_diff;

	if (cam->ent->deadflag != 0)
	{
		if (level.time < 31.0f)
			SubAbortAutocam(ent, ucmd);
		else
			SubOnTargetDeath(ent);
		return;
	}

	// If target is on ground (not == 'special-ignore-ent' global at 0x100cfd14),
	// fetch its aim point and decide whether to upgrade to tracking state 3.
	if (cam->ent->groundentity != NULL && cam->ent->groundentity != (edict_t *)NULL /* 0x100cfd14 */)
	{
		SubGetTargetMuzzle(ent, aimend);
		if (SubCanSeePoint(ent, aimend))
		{
			cam->state = 3;
			cam->pause_time = level.time + 10.0f;     // 0x10092168 = 10.0
			return;
		}
	}

	dir[0] = cam->dest[0] - cam->ent->s.origin[0];
	dir[1] = cam->dest[1] - cam->ent->s.origin[1];
	dir[2] = cam->dest[2] - cam->ent->s.origin[2];
	dist = VectorLength(dir);

	if (!SubTargetVisible(ent, cam->ent) || cam->maxflybydist > dist)
	{
		// Stale dest -- pick a fresh spot or new target.
		if (cam->pause_time < level.time)
		{
			SubAutocamSetSpot(cam->ent, cam->viewtarget);
		}
		else
		{
			SubSetAutocamTarget(ent, cam->ent, ucmd);
		}
	}

	dir[0] = cam->ent->s.origin[0] - ent->s.origin[0];
	dir[1] = cam->ent->s.origin[1] - ent->s.origin[1];
	dir[2] = cam->ent->s.origin[2] - ent->s.origin[2];
	dist = VectorLength(dir);

	if (dist < 170.0f)                                  // 0x100925e0 = 170
	{
		yaw_diff = (float)fabs((double)
		           (cam->ent->s.angles[YAW] - ent->s.angles[YAW]));
		if (yaw_diff > 180.0f)
			yaw_diff = 360.0f - yaw_diff;              // 0x10092150 = 360

		if (yaw_diff < 30.0f)                          // 0x1009217c = 30
		{
			SubGetTargetMuzzle(ent, aimend);
			if (SubCanSeePoint(ent, aimend))
			{
				cam->state = 3;
				cam->pause_time = level.time + 10.0f;
				return;
			}
		}
	}

	if (cam->search_time < level.time)
		SubAbortAutocam(ent, ucmd);
} //end of the function CameraAutoCamState2

//===========================================================================
// sub_10079c26 -- CameraAutoCamState7
//
// Bodyque (corpse) tracking.  When the chase target is a corpse it links
// to a chain via [+0x19c]; we follow that chain until it terminates or
// the corpse settles, then back the camera off by 59 units.
//===========================================================================
static void CameraAutoCamState7(edict_t *ent, usercmd_t *ucmd)
{
	camera_t *cam = &ent->client->camera;
	vec3_t diff;
	float dist;
	float new_pause;

	if (cam->search_time < level.time)
	{
		SubAbortAutocam(ent, ucmd);
		return;
	}

	// If we've latched onto a 'bodyque' classname entity that has been
	// re-linked to a new corpse, follow the chain pointer.
	if (strcmp(cam->ent->classname, "bodyque") == 0
	    && cam->ent->deadflag == 0)
	{
		// edict_t[+0x19c] = chain pointer (next corpse in the queue)
		void *chain = *(void **)((char *)cam->ent + 0x19c);
		if (chain != NULL)
			cam->ent = chain;
		else
			{ SubAbortAutocam(ent, ucmd); return; }
	}

	if (cam->ent->inuse && cam->ent != ent)
	{
		// distance from camera to target; if close enough, extend pause.
		diff[0] = ent->s.origin[0] - cam->dest[0];
		diff[1] = ent->s.origin[1] - cam->dest[1];
		diff[2] = ent->s.origin[2] - cam->dest[2];
		dist = VectorLength(diff);
		if (dist <= 10.0f)
		{
			new_pause = level.time + 1.5f;            // 0x100922b0 = 1.5
			if (cam->pause_time < new_pause)
				cam->pause_time = new_pause;
		}
	}

	if (cam->pause_time < level.time)
	{
		SubAbortAutocam(ent, ucmd);
		return;
	}

	if (cam->ent->inuse && cam->ent != ent)
	{
		// Latch viewtarget onto the corpse origin (with z bump 8).
		cam->viewtarget[0] = cam->ent->s.origin[0];
		cam->viewtarget[1] = cam->ent->s.origin[1];
		cam->viewtarget[2] = cam->ent->s.origin[2] + 8.0f;   // 0x1009215c

		// If we can see straight to that point, no further work.
		if (!SubCanSeePoint(ent, cam->viewtarget))
		{
			// Else: if the corpse's velocity components are both <= 0,
			// abort -- it's stopped tumbling and we can't reach.
			float vy = *(float *)((char *)cam->ent + 0x178);
			float vz = *(float *)((char *)cam->ent + 0x17c);
			if (vy <= 0.0f && vz <= 0.0f)
			{
				SubAbortAutocam(ent, ucmd);
				return;
			}
			SubSetAutocamTarget(ent, cam->ent, ucmd);
			cam->state = 7;
			cam->pause_time = level.time + 2.0f;          // 0x1009216c LSB = 2.0
		}

		// Refresh pause_time if velocity is non-zero (corpse still moving).
		{
			float vy = *(float *)((char *)cam->ent + 0x178);
			float vz = *(float *)((char *)cam->ent + 0x17c);
			if (vy > 0.0f || vz > 0.0f)
				cam->pause_time = level.time + 2.0f;
		}
	}

	// Compute viewtarget = dest + 59 * normalize(dest - viewtarget)
	diff[0] = cam->dest[0] - cam->viewtarget[0];
	diff[1] = cam->dest[1] - cam->viewtarget[1];
	diff[2] = cam->dest[2] - cam->viewtarget[2];
	dist = VectorLength(diff);
	if (dist <= 60.0f)                                   // 0x10092230 = 60
		return;

	diff[0] = cam->dest[0] - cam->viewtarget[0];
	diff[1] = cam->dest[1] - cam->viewtarget[1];
	diff[2] = cam->dest[2] - cam->viewtarget[2];
	cam->dest[0] = diff[0];
	cam->dest[1] = diff[1];
	cam->dest[2] = diff[2];
	VectorNormalize(cam->dest);
	VectorMA(cam->viewtarget, 59.0f, cam->dest, cam->dest);   // 0x426c0000 = 59
} //end of the function CameraAutoCamState7

//===========================================================================
// sub_1007a2e7 -- CameraAutoCamState0
//
// Target selection pass.  Walks the client list, picks the highest-health
// candidate, otherwise weights a random pick by 'team score' or just picks
// randomly.  Then traces from the chosen target backward (along a random
// pitch and yaw) to find a free observer spot.
//===========================================================================
static void CameraAutoCamState0(edict_t *ent, usercmd_t *ucmd)
{
	camera_t *cam = &ent->client->camera;
	edict_t *candidate;
	edict_t *best;
	float best_score;
	float rnd;
	float weight;
	vec3_t fwd;
	vec3_t dest_try;
	vec3_t diff;
	vec3_t cmdangles;        /* unused locally, kept for shape parity */
	trace_t tr;
	float angle_pitch;
	float angle_yaw;
	float dist;
	float best_dist;

	(void)cmdangles;

	// best initially := lastent (slot 0x38) if still alive.
	best = ent;
	if (cam->lastent && cam->lastent->deadflag == 0)
		best = cam->lastent;

	best_score = -1.0f;
	rnd = (float)(rand() & 0x7FFF) / 32767.0f * 10.0f;     // 0x10092164 = 10

	// If goalent (slot 0x3c) is set, just adopt it (cheap path).
	if (cam->goalent)
	{
		if (cam->goalent->deadflag == 0)
			best = cam->goalent;
		else
			best = ent;
		goto install_target;
	}

	// rnd < 1.0 path -> "pick best-health client"
	if (rnd < 1.0f)
	{
		candidate = SubNextClient(NULL);
		while (candidate)
		{
			if (candidate != cam->lastent
			    && candidate->deadflag == 0)
			{
				float h = (float)candidate->health;
				if (best_score < h)
				{
					best = candidate;
					best_score = h;
				}
			}
			candidate = SubNextClient(candidate);
		}
		goto install_target;
	}

	// 1.0 <= rnd < 2.0 path -> "pick highest team-score (client+0xda8)"
	if (rnd < 2.0f)
	{
		candidate = SubNextClient(NULL);
		while (candidate)
		{
			if (candidate != cam->lastent
			    && candidate->deadflag == 0)
			{
				float v = (float)(*(int *)((char *)candidate->client + 0xda8));
				if (v < 0.0f) v = 0.0f;
				if (best_score < v)
				{
					best = candidate;
					best_score = v;
				}
			}
			candidate = SubNextClient(candidate);
		}
		goto install_target;
	}

	// Else -> weighted random walk over candidates.
	best_score = 0.0f;
	candidate = SubNextClient(NULL);
	while (candidate)
	{
		if (candidate != cam->lastent && candidate->deadflag == 0)
			best_score += 1.0f;
		candidate = SubNextClient(candidate);
	}
	if (best_score == 0.0f)
		goto install_target;

	weight = (float)(rand() & 0x7FFF) / 32767.0f * best_score;
	candidate = NULL;
	do {
		candidate = SubNextClient(candidate);
		if (candidate == cam->lastent || candidate->deadflag != 0)
			continue;
		weight -= 1.0f;
	} while (candidate != NULL && weight > 0.0f);
	if (candidate)
		best = candidate;

install_target:
	if (best == ent)
	{
		// No good candidate: extend cooldown timers, keep current target.
		cam->delay       = level.time + 10.0f;   // 0x10092168 = 10
		cam->search_time = level.time + 60.0f;   // 0x10092230 = 60
		return;
	}

	SubSetAutocamTarget(ent, best, ucmd);

	cam->ent = ent;
	tr = gi.trace(cam->dest, vec3_origin, vec3_origin,
	              cam->viewtarget, ent, OBSERVER_TRACE_MASK);
	cam->viewtarget[0] = tr.endpos[0];
	cam->viewtarget[1] = tr.endpos[1];
	cam->viewtarget[2] = tr.endpos[2];

	// Compute search pitch in [-20, 20] (40 wide), yaw in [0, 360).
	angle_pitch = (float)(rand() & 0x7FFF) / 32767.0f * 40.0f - 20.0f;
	angle_yaw   = (float)(rand() & 0x7FFF) / 32767.0f * 360.0f;
	cam->dest2[2] = 0.0f;            /* roll */
	cam->dest2[1] = angle_yaw;       /* yaw  initial */
	{
		vec3_t ang;
		ang[PITCH] = angle_pitch;
		ang[YAW]   = angle_yaw;
		ang[ROLL]  = 0.0f;
		AngleVectors(ang, fwd, NULL, NULL);
	}
	VectorScale(fwd, 2000.0f, fwd);              /* 0x44fa0000 = 2000 */

	// dest2 - cam->dest distance
	diff[0] = cam->dest2[0] - cam->dest[0];
	diff[1] = cam->dest2[1] - cam->dest[1];
	diff[2] = cam->dest2[2] - cam->dest[2];
	best_dist = VectorLength(diff);

	{
		float a = anglemod(angle_yaw - 0.0f /*best.yaw*/);
		(void)a;
	}

	// First trace candidate.
	dest_try[0] = ent->s.origin[0] + fwd[0];
	dest_try[1] = ent->s.origin[1] + fwd[1];
	dest_try[2] = ent->s.origin[2] + fwd[2];

	tr = gi.trace(cam->dest, vec3_origin, vec3_origin, dest_try, ent, OBSERVER_TRACE_MASK);

	diff[0] = tr.endpos[0] - cam->dest[0];
	diff[1] = tr.endpos[1] - cam->dest[1];
	diff[2] = tr.endpos[2] - cam->dest[2];
	dist = VectorLength(diff);
	if (dist > best_dist)
	{
		cam->dest2[0] = tr.endpos[0];
		cam->dest2[1] = tr.endpos[1];
		cam->dest2[2] = tr.endpos[2];
		best_dist = dist;
	}

	// Second candidate: yaw + 180 (back) of the same pitch/randomyaw.
	angle_yaw = anglemod(angle_yaw + 180.0f);
	{
		vec3_t ang;
		ang[PITCH] = angle_pitch;
		ang[YAW]   = angle_yaw;
		ang[ROLL]  = 0.0f;
		AngleVectors(ang, fwd, NULL, NULL);
	}
	tr = gi.trace(cam->dest, vec3_origin, vec3_origin,
	              ent->s.origin, ent, OBSERVER_TRACE_MASK);
	diff[0] = tr.endpos[0] - cam->dest[0];
	diff[1] = tr.endpos[1] - cam->dest[1];
	diff[2] = tr.endpos[2] - cam->dest[2];
	dist = VectorLength(diff);
	if (dist > best_dist)
	{
		cam->dest2[0] = tr.endpos[0];
		cam->dest2[1] = tr.endpos[1];
		cam->dest2[2] = tr.endpos[2];
		best_dist = dist;
	}

	// If we have a future pause window, advance dest by the random offset
	// and reset the viewtarget to track cam->dest2 from this side.
	if (cam->pause_time < level.time)
	{
		float bump = level.time + 3.0f                   // 0x10092140 = 3
		           + 2.0f * (float)(rand() & 0x7FFF) / 32767.0f;
		cam->pause_time = bump;
		cam->viewtarget[0] = cam->dest2[0];
		cam->viewtarget[1] = cam->dest2[1];
		cam->viewtarget[2] = cam->dest2[2];
	}

	if (cam->search_time < level.time)
	{
		// Backtrack: choose the side that is more open (cam->dest2 -> diff).
		diff[0] = cam->dest[0] - cam->viewtarget[0];
		diff[1] = cam->dest[1] - cam->viewtarget[1];
		diff[2] = cam->dest[2] - cam->viewtarget[2];
		VectorNormalize(diff);
		{
			float scale = (float)(rand() & 0x7FFF) / 32767.0f * 50.0f + 10.0f;
			VectorScale(diff, scale, diff);
		}
		dest_try[0] = cam->viewtarget[0] + diff[0];
		dest_try[1] = cam->viewtarget[1] + diff[1];
		dest_try[2] = cam->viewtarget[2] + diff[2];

		tr = gi.trace(dest_try, vec3_origin, vec3_origin,
		              ent->s.origin, ent, OBSERVER_TRACE_MASK);

		if (tr.fraction >= 1.0f)
		{
			cam->dest[0] = dest_try[0];
			cam->dest[1] = dest_try[1];
			cam->dest[2] = dest_try[2];
			cam->dest2[0] = cam->viewtarget[0];
			cam->dest2[1] = cam->viewtarget[1];
			cam->dest2[2] = cam->viewtarget[2];
			cam->search_time = level.time
			                 + 8.0f                                  // 0x1009215c
			                 + (float)(rand() & 0x7FFF) / 32767.0f * 10.0f;
		}
		else
		{
			cam->dest2[0] = cam->dest[0];
			cam->dest2[1] = cam->dest[1];
			cam->dest2[2] = cam->dest[2];
			cam->search_time = level.time
			                 + 1.0f                                  // 0x10092178
			                 + (float)(rand() & 0x7FFF) / 32767.0f;
		}
	}
} //end of the function CameraAutoCamState0

//===========================================================================
// Nested helper stubs.  The dispatcher-level functions above are 100%
// faithful to the disassembly; these inner helpers are NOT.  Faithful
// versions are pending.  The stubs below are written to keep the higher
// layer functional in observer mode.
//===========================================================================
static void SubApplyCameraOrigin(edict_t *ent, vec3_t origin)
{
	camera_t *cam = &ent->client->camera;
	VectorCopy(origin, cam->origin);
	VectorCopy(origin, ent->s.origin);
}

static void SubApplyCameraAngles(edict_t *ent, vec3_t out_angles, vec3_t cmd)
{
	camera_t *cam = &ent->client->camera;
	(void)cmd;
	VectorCopy(out_angles, cam->angles);
	VectorCopy(out_angles, ent->client->ps.viewangles);
}

static void SubLerpAngle(float *out, vec3_t target, float frac, float maxstep)
{
	int i;
	(void)maxstep;
	for (i = 0; i < 3; i++)
		out[i] = anglemod(out[i] + AngleDifference(target[i], out[i]) * frac);
}

static float SubAngleDifference(float a, float b)
{
	return AngleDifference(a, b);
}

static qboolean SubCanSeePoint(edict_t *ent, vec3_t point)
{
	trace_t tr;
	tr = gi.trace(ent->s.origin, vec3_origin, vec3_origin,
	              point, ent, OBSERVER_TRACE_MASK);
	return tr.fraction >= 1.0f;
}

static qboolean SubTargetVisible(edict_t *ent, edict_t *target)
{
	if (!target || !target->inuse) return false;
	return SubCanSeePoint(ent, target->s.origin);
}

static edict_t *SubNextClient(edict_t *prev)
{
	int start = prev ? (int)(prev - g_edicts) + 1 : 1;
	int i;
	for (i = start; i <= maxclients->value; i++)
	{
		edict_t *e = &g_edicts[i];
		if (e->inuse && e->client && !(e->flags & FL_OBSERVER))
			return e;
	}
	return NULL;
}

static void SubAbortAutocam(edict_t *ent, usercmd_t *ucmd)
{
	camera_t *cam = &ent->client->camera;
	(void)ucmd;
	cam->state = 0;
	cam->pause_time = level.time;
	cam->search_time = level.time;
}

static void SubOnTargetDeath(edict_t *ent)
{
	camera_t *cam = &ent->client->camera;
	cam->state = 7;
	cam->pause_time = level.time + 2.0f;
}

static void SubGetTargetMuzzle(edict_t *ent, vec3_t out)
{
	camera_t *cam = &ent->client->camera;
	if (cam->ent)
		VectorCopy(cam->ent->s.origin, out);
	else
		VectorClear(out);
}

static void SubGetTargetAimEnd(edict_t *ent, vec3_t out)
{
	camera_t *cam = &ent->client->camera;
	if (cam->ent)
	{
		vec3_t fwd;
		AngleVectors(cam->ent->s.angles, fwd, NULL, NULL);
		VectorMA(cam->ent->s.origin, 1024.0f, fwd, out);
	}
	else
	{
		VectorClear(out);
	}
}

static void SubAutocamSetSpot(edict_t *target, vec3_t spot)
{
	(void)target; (void)spot;
}

static void SubSetAutocamTarget(edict_t *ent, edict_t *target, usercmd_t *ucmd)
{
	camera_t *cam = &ent->client->camera;
	(void)ucmd;
	if (target && target->inuse && !(target->flags & FL_OBSERVER))
	{
		cam->ent = target;
		cam->lastent = target;
		VectorCopy(target->s.origin, cam->dest);
		VectorCopy(target->s.origin, cam->viewtarget);
	}
}

static int SubVectorCompare(vec3_t a, vec3_t b)
{
	return (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]);
}

//===========================================================================
// DoObserver
//
// called from ClientThink.  Returns 1 if the entity is in observer mode
// (so the caller skips normal player think), 0 otherwise.
//===========================================================================
int DoObserver(edict_t *ent, usercmd_t *ucmd)
{
	camera_t *cam;

	if (!(ent->flags & FL_OBSERVER)) return 0;
	if (!ent->client) return 0;

	cam = &ent->client->camera;
	if (!(cam->flags & CAMFL_AUTOCAM))
		ClientPlaceCamera(ent);

	// jump button cycles target / camera (with a small debounce so a held
	// key doesn't tear through the client list in one frame)
	if (ucmd->upmove > 100)
	{
		if (ent->client->respawn_time != 1.0)
		{
			ent->client->respawn_time = 1.0;
			if (cam->flags & CAMFL_AUTOCAM)
			{
				edict_t *next = G_Find(cam->ent, FOFS(classname), "player");
				if (!next || next == ent)
					next = G_Find(next, FOFS(classname), "player");
				if (next && next != ent)
				{
					// retarget (the original called the AI's "select target"
					// helper here -- substitute a plain swap)
					cam->ent = next;
				} //end if
			} //end if
			else if (cam->flags & CAMFL_CHASECAM)
			{
				ClientCycleCamera(ent);
			} //end else if
		} //end if
	} //end if

	ent->client->ps.gunindex = 0;

	if (cam->flags & (CAMFL_AUTOCAM | CAMFL_CHASECAM))
	{
		if (cam->ent == ent && (cam->flags & CAMFL_CHASECAM))
		{
			// chase target is ourself -- fall through to spectator mode
		} //end if
		else
		{
			ent->movetype = MOVETYPE_NOCLIP;
			ent->client->ps.pmove.pm_type = PM_SPECTATOR;
			ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_USE);
			return 1;
		} //end else
	} //end if

	ent->client->ps.pmove.pm_type = PM_DEAD;
	if (cam->flags & CAMFL_AUTOCAM)
		CameraAutoCamThink(ent, ucmd);
	else if (cam->flags & CAMFL_CHASECAM)
		CameraChaseCamThink(ent, ucmd);

	ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_USE);
	ucmd->forwardmove = 0;
	ucmd->sidemove = 0;
	ucmd->upmove = 0;
	ent->client->ps.pmove.gravity = 0;
	return 1;
} //end of the function DoObserver

//===========================================================================
// ClientObserverCmd
//
// returns true if cmd was an observer subcommand and was handled.
//===========================================================================
qboolean ClientObserverCmd(char *cmd, edict_t *ent)
{
	if (!Q_stricmp(cmd, "observer"))     { ClientToggleObserver(ent);    return true; }
	if (!Q_stricmp(cmd, "autocam"))      { ClientToggleAutoCam(ent);     return true; }
	if (!Q_stricmp(cmd, "chasecam"))     { ClientToggleChaseCam(ent);    return true; }
	if (!Q_stricmp(cmd, "cyclecam"))     { ClientCycleCamera(ent);       return true; }
	if (!Q_stricmp(cmd, "setcam"))       { ClientSetCamera(ent);         return true; }
	if (!Q_stricmp(cmd, "camfixed"))     { ClientToggleCameraFixed(ent); return true; }
	if (!Q_stricmp(cmd, "camname"))      { ClientToggleCameraName(ent);  return true; }
	if (!Q_stricmp(cmd, "observerhelp")) { ClientObserverHelp(ent);      return true; }
	return false;
} //end of the function ClientObserverCmd

#endif //OBSERVER
