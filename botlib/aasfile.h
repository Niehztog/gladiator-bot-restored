/*
 * aasfile.h — the on-disk AAS file format: the presence types, the lump directory
 * and the file header.  Q3 botlib ships an aasfile.h holding exactly this, which is
 * why the name is his and not ours.
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
