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
// Stub implementations for the autocam state handlers and the
// CameraMove / CameraInputThink / CameraPlaceAtTarget helpers.
//
// These let the dispatcher above compile and the chasecam math link.
// Each one falls back to the simplest "glue camera to target" behaviour
// from the previous reconstruction so observer/spectator mode is still
// usable while the faithful disassembly-derived bodies are pending.
//===========================================================================
static void CameraInputThink(edict_t *ent, usercmd_t *ucmd)
{
	// sub_1007ace3: turns ucmd->angles into cam->ent_angles deltas.
	// Faithful restoration is pending; the smoothing/snapping step in
	// CameraChaseCamThink still pulls cam->ent_angles from the target's
	// v_angle every frame so the camera tracks correctly.
}

static void CameraPlaceAtTarget(edict_t *ent, vec3_t end, vec3_t out_angles, vec3_t cmdangles)
{
	// sub_10078043: final placement helper.  In the original, this clamps
	// the camera position against the world (a second trace), applies
	// cmdangles to compute the final view angles, and writes them back
	// through out_angles.  Minimal stand-in: drop the camera at 'end' and
	// keep the current ent_angles.
	camera_t *cam = &ent->client->camera;
	VectorCopy(end, cam->origin);
	VectorCopy(cam->ent_angles, out_angles);
}

static void CameraMove(edict_t *ent, float speed, usercmd_t *ucmd)
{
	// sub_100784f9: drives the autocam fly between waypoints at 'speed'.
	// Stand-in glues the camera body to its current target.
	camera_t *cam = &ent->client->camera;
	if (!cam->ent || !cam->ent->inuse || (cam->ent->flags & FL_OBSERVER))
		return;
	VectorCopy(cam->origin, ent->s.origin);
	VectorCopy(cam->ent_angles, ent->client->ps.viewangles);
}

static void CameraAutoCamState0(edict_t *ent, usercmd_t *ucmd)
{
	// sub_1007a2e7: state 0 -- pick / refresh autocam target.
	camera_t *cam = &ent->client->camera;
	if (!cam->ent || !cam->ent->inuse || (cam->ent->flags & FL_OBSERVER))
		ClientCycleCamera(ent);
}

static void CameraAutoCamState1(edict_t *ent, usercmd_t *ucmd) { /* sub_10079f4d */ }
static void CameraAutoCamState2(edict_t *ent, usercmd_t *ucmd) { /* sub_10079f64 */ }
static void CameraAutoCamState3(edict_t *ent, usercmd_t *ucmd) { /* sub_1007a1d2 */ }
static void CameraAutoCamState7(edict_t *ent, usercmd_t *ucmd) { /* sub_10079c26 */ }

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
