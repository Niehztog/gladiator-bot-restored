/*
 * aasfile.h — the on-disk AAS file format: the presence and travel types, the face,
 * area-contents and area flags, the lump directory and the file header.  Q3 botlib
 * ships an aasfile.h holding exactly this, which is why the name is his and not ours.
 */
#ifndef BOTLIB_AASFILE_H
#define BOTLIB_AASFILE_H

/* Q3's presence types, verbatim from its aasfile.h.  They are file-format values:
 * the .aas bbox lump stores 2 with the standing box and 4 with the crouch box and
 * its crouch flag.  Q3's game-side copy in ai_main.h is headed "copied from the aas
 * file header", which is why the one definition lives here. */
//presence types
#define PRESENCE_NONE				1
#define PRESENCE_NORMAL				2
#define PRESENCE_CROUCH				4

/* Q3's travel types, verbatim, up to TRAVEL_GRAPPLEHOOK.  The values are the
 * original's: BotMoveToGoal dispatches 2 to BotTravel_Walk through 14 to
 * BotTravel_Grapple in Q3's own case order, the reachability generators create
 * them (8 in AAS_Reachability_Swim, 10 for teleporters, 13 or 12 for weapon
 * jumps), and AAS_InitTravelFlagFromType maps 1..14 to one flag bit each.
 * MAX_TRAVELTYPES is AAS_TravelFlagForType's bound and that table's size.  Q3's
 * 15..19 (double jump to func_bob) are left out: Gladiator's flag bits go straight
 * from the grapple's 0x4000 to TFL_AIR at 0x8000, leaving no room for theirs. */
//travel types
#define MAX_TRAVELTYPES				32
#define TRAVEL_INVALID				1		//temporary not possible
#define TRAVEL_WALK					2		//walking
#define TRAVEL_CROUCH				3		//crouching
#define TRAVEL_BARRIERJUMP			4		//jumping onto a barrier
#define TRAVEL_JUMP					5		//jumping
#define TRAVEL_LADDER				6		//climbing a ladder
#define TRAVEL_WALKOFFLEDGE			7		//walking of a ledge
#define TRAVEL_SWIM					8		//swimming
#define TRAVEL_WATERJUMP			9		//jump out of the water
#define TRAVEL_TELEPORT				10		//teleportation
#define TRAVEL_ELEVATOR				11		//travel by elevator
#define TRAVEL_ROCKETJUMP			12		//rocket jumping required for travel
#define TRAVEL_BFGJUMP				13		//bfg jumping required for travel
#define TRAVEL_GRAPPLEHOOK			14		//grappling hook required for travel

/* Q3's face flags, area contents and area flags, verbatim, with the entries the 1999
 * files and code use.  The committed Gladiator .aas files (version 3, from the v0.96
 * BSPC) carry face flags 1, 2, 4, 16 and 32, area contents 1, 2, 4 and 8, and area
 * flags 1, 2 and 4.  The botlib sets AREACONTENTS_ROUTEPORTAL beside the cluster
 * portal itself, as Q3 does.  Lava is 2 and slime 4, as in Q3: in every map whose
 * areas carry either bit, an area centre in liquid lies in CONTENTS_LAVA for bit 2
 * and in CONTENTS_SLIME for bit 4.  Q3's gap and bridge faces, its teleporter,
 * jump-pad, do-not-enter, mover and team contents, and its disabled and bridge
 * areas, do not occur. */
//face flags
#define FACE_SOLID					1		//just solid at the other side
#define FACE_LADDER					2		//ladder
#define FACE_GROUND					4		//standing on ground when in this face
#define FACE_LIQUID					16		//face seperating two areas with liquid
#define FACE_LIQUIDSURFACE			32		//face seperating liquid and air

//area contents
#define AREACONTENTS_WATER				1
#define AREACONTENTS_LAVA				2
#define AREACONTENTS_SLIME				4
#define AREACONTENTS_CLUSTERPORTAL		8
#define AREACONTENTS_ROUTEPORTAL		32

//area flags
#define AREA_GROUNDED				1		//bot can stand on the ground
#define AREA_LADDER					2		//area contains one or more ladder faces
#define AREA_LIQUID					4		//area contains a liquid

/* AAS file header (AAS_LoadAASFile).  Q3 adds a bspchecksum field (124 B);
 * Gladiator omits it. */
typedef struct { int fileofs; int filelen; } aas_lump_t;
#define AAS_LUMPS_Q2 14
typedef struct {
    int         ident;              /* "EAAS" = 0x53414145     */
    int         version;            /* 2 = old, 3 = current    */
    aas_lump_t  lumps[AAS_LUMPS_Q2];/* 14 lumps × 8 = 112 B   */
} aas_header_t;                     /* sizeof = 120 = 0x78     */

/* bsp_surface_t, bot_settings_t, bot_clientsettings_t — defined in game/botlib.h. */

#endif /* BOTLIB_AASFILE_H */
