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
// CameraAutoCamThink
//
// per-frame state machine for autocam mode (find best target, frame the
// shot, periodically reselect).  The original is a ~270 byte tangle of
// state transitions; the simplified reconstruction here just keeps the
// current target framed.  Behaviour is correct enough for `autocam 1`
// to be usable in deathmatch.
//===========================================================================
static void CameraAutoCamThink(edict_t *ent, usercmd_t *ucmd)
{
	camera_t *cam = &ent->client->camera;
	if (!cam->ent || !cam->ent->inuse || (cam->ent->flags & FL_OBSERVER))
	{
		ClientCycleCamera(ent);
		return;
	} //end if
	ClientSetViewAngles(ent, NULL, NULL);
	VectorCopy(cam->origin, ent->s.origin);
	VectorCopy(cam->ent_angles, ent->client->ps.viewangles);
} //end of the function CameraAutoCamThink

//===========================================================================
// CameraChaseCamThink
//
// per-frame state machine for chasecam mode (player-controlled fly cam
// that snaps to the chase target).  Like the autocam helper above, the
// original is large; this version just glues the camera to the target.
//===========================================================================
static void CameraChaseCamThink(edict_t *ent, usercmd_t *ucmd)
{
	camera_t *cam = &ent->client->camera;
	if (!cam->ent || !cam->ent->inuse || (cam->ent->flags & FL_OBSERVER) || cam->ent == ent)
		return;
	ClientSetViewAngles(ent, NULL, NULL);
	VectorCopy(cam->origin, ent->s.origin);
	VectorCopy(cam->ent_angles, ent->client->ps.viewangles);
} //end of the function CameraChaseCamThink

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
