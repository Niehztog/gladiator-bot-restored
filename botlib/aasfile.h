/*
 * aasfile.h — the on-disk AAS file format: the lump directory and the file
 * header.  Q3 botlib ships an aasfile.h holding exactly this, which is why the
 * name is his and not ours.
 */
#ifndef BOTLIB_AASFILE_H
#define BOTLIB_AASFILE_H

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
