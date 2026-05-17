/*
 * botlib_structdefs.c — Structure descriptor tables recovered from gladiator.dll .data section
 *
 * sub_10040AD0() takes a pointer to a structdef: { int size; char **fields; }.
 * sub_100404B0() scans the field table looking for a field name.
 * Each field table entry has 7 (char *)-wide slots:
 *   [0] field name string pointer
 *   [1] byte offset of field in struct (integer stored as char *)
 *   [2] type flags (integer stored as char *; low byte = type, high byte = array flag)
 *   [3] array element count (integer stored as char *)
 *   [4] 0
 *   [5] default value as float-bits (integer stored as char *)
 *   [6] 0
 * Terminated by a 7-slot entry whose first slot is NULL.
 *
 * All four descriptors recovered by disassembling gladiator.dll at their
 * .data addresses (0x1005C138, 0x1005D890, 0x1005DFD8, 0x1005DFE0).
 */

#include <stdint.h>

/* Cast integer constant to char * for mixed pointer/int field table slots */
#define P(x) ((char *)(uintptr_t)(x))

/* One field table entry (7 slots): name, offset, flags, arrcount, 0, default, 0 */
#define FE(name, off, flags, arr, def) \
    (name), P(off), P(flags), P(arr), P(0), P(def), P(0)

/* Null terminator entry */
#define FE_END \
    P(0),  P(0),   P(0),   P(0),  P(0),  P(0),  P(0)

/* =========================================================================
 * unk_1005C138 — sound info struct descriptor (struct size = 176 = 0xB0)
 * Field table at original binary address 0x1005C070.
 * Fields: name(str,0x00), volume(f,0x50,def=80), duration(f,0x54,def=10),
 *         type(i,0x58), recognition(f,0x5C,def=1), string(str,0x60)
 * ========================================================================= */
static char *snd_fields[] = {
    FE("name",        0x00, 0x004, 0, 0x00000000),
    FE("volume",      0x50, 0x203, 0, 0x42A00000),  /* 80.0f */
    FE("duration",    0x54, 0x203, 0, 0x41200000),  /* 10.0f */
    FE("type",        0x58, 0x002, 0, 0x00000000),
    FE("recognition", 0x5C, 0x003, 0, 0x3F800000),  /* 1.0f */
    FE("string",      0x60, 0x004, 0, 0x00000000),
    FE_END
};
int unk_1005C138[] = { 176, (int)(uintptr_t)snd_fields };

/* =========================================================================
 * unk_1005D890 — item/entity struct descriptor (struct size = 284 = 0x11C)
 * Field table at original binary address 0x1005D7B0.
 * Fields: name(str,0x00), model(str,0xA0), type(i,0xF4), index(i,0xF8),
 *         respawntime(f,0xFC), mins(f[3],0x100), maxs(f[3],0x10C)
 * ========================================================================= */
static char *item_fields[] = {
    FE("name",        0x000, 0x004, 0, 0x00000000),
    FE("model",       0x0A0, 0x004, 0, 0x00000000),
    FE("type",        0x0F4, 0x002, 0, 0x00000000),
    FE("index",       0x0F8, 0x002, 0, 0x00000000),
    FE("respawntime", 0x0FC, 0x003, 0, 0x00000000),
    FE("mins",        0x100, 0x103, 3, 0x00000000),  /* vec3, flags 0x103 */
    FE("maxs",        0x10C, 0x103, 3, 0x00000000),  /* vec3, flags 0x103 */
    FE_END
};
int unk_1005D890[] = { 284, (int)(uintptr_t)item_fields };

/* =========================================================================
 * unk_1005DFD8 — weapon config struct descriptor (struct size = 344 = 0x158)
 * Field table at original binary address 0x1005DBC8.
 * ========================================================================= */
static char *weap_fields[] = {
    FE("name",            0x004, 0x004, 0, 0x00000000),
    FE("level",           0x0A4, 0x002, 0, 0x00000000),
    FE("model",           0x054, 0x004, 0, 0x00000000),
    FE("weaponindex",     0x0A8, 0x002, 0, 0x00000000),
    FE("flags",           0x0AC, 0x002, 0, 0x00000000),
    FE("projectile",      0x0B0, 0x004, 0, 0x00000000),
    FE("numprojectiles",  0x100, 0x002, 0, 0x00000000),
    FE("hspread",         0x104, 0x003, 0, 0x00000000),
    FE("vspread",         0x108, 0x003, 0, 0x00000000),
    FE("speed",           0x10C, 0x003, 0, 0x00000000),
    FE("acceleration",    0x110, 0x003, 0, 0x00000000),
    FE("recoil",          0x114, 0x103, 3, 0x00000000),  /* vec3 */
    FE("offset",          0x120, 0x103, 3, 0x00000000),  /* vec3 */
    FE("angleoffset",     0x12C, 0x103, 3, 0x00000000),  /* vec3 */
    FE("extrazvelocity",  0x138, 0x003, 0, 0x00000000),
    FE("ammoamount",      0x13C, 0x002, 0, 0x00000000),
    FE("ammoindex",       0x140, 0x002, 0, 0x00000000),
    FE("activate",        0x144, 0x003, 0, 0x00000000),
    FE("reload",          0x148, 0x003, 0, 0x00000000),
    FE("spinup",          0x14C, 0x003, 0, 0x00000000),
    FE("spindown",        0x150, 0x003, 0, 0x00000000),
    FE_END
};
int unk_1005DFD8[] = { 344, (int)(uintptr_t)weap_fields };

/* =========================================================================
 * unk_1005DFE0 — projectile config struct descriptor (struct size = 208 = 0xD0)
 * Field table at original binary address 0x1005DE30.
 * ========================================================================= */
static char *proj_fields[] = {
    FE("name",        0x000, 0x004, 0, 0x00000000),
    FE("model",       0x054, 0x004, 0, 0x00000000),
    FE("flags",       0x0A0, 0x002, 0, 0x00000000),
    FE("gravity",     0x0A4, 0x003, 0, 0x00000000),
    FE("damage",      0x0A8, 0x002, 0, 0x00000000),
    FE("radius",      0x0AC, 0x003, 0, 0x00000000),
    FE("visdamage",   0x0B0, 0x002, 0, 0x00000000),
    FE("damagetype",  0x0B4, 0x002, 0, 0x00000000),
    FE("healthinc",   0x0B8, 0x002, 0, 0x00000000),
    FE("push",        0x0BC, 0x003, 0, 0x00000000),
    FE("detonation",  0x0C0, 0x003, 0, 0x00000000),
    FE("bounce",      0x0C4, 0x003, 0, 0x00000000),
    FE("bouncefric",  0x0C8, 0x003, 0, 0x00000000),
    FE("bouncestop",  0x0CC, 0x003, 0, 0x00000000),
    FE_END
};
int unk_1005DFE0[] = { 208, (int)(uintptr_t)proj_fields };

/* =========================================================================
 * Vec3 constants from .rdata section
 * Used by VectorCompare() — must be 12 bytes (3 floats), not a single int.
 * ========================================================================= */

/* unk_1005C56C: vec3 {0.0, -1.0, 0.0}  (0xBF800000 = -1.0f) */
float unk_1005C56C[3] = { 0.0f, -1.0f,  0.0f };

/* unk_1005C584: vec3 {0.0, -2.0, 0.0}  (0xC0000000 = -2.0f) */
float unk_1005C584[3] = { 0.0f, -2.0f,  0.0f };

/* velocity: vec3 {0.0, 0.0, 0.0} — zero vector passed to AAS_ClientMovementHV */
float velocity[3] = { 0.0f,  0.0f,  0.0f };

/* =========================================================================
 * unk_1005E678 — CRC16 weapon ID table (91 entries + NULL terminator)
 *
 * sub_100377E0() iterates: v2 = &unk_1005E678; do { if (a2 == *v2) break;
 *   v2 += 4; } while (v2 < &unk_1005E958);
 * Each entry is 8 bytes: { uint16 crc16, uint16 pad, uint32 weapon_type }.
 * unk_1005E958 must immediately follow this array (end-of-table sentinel address).
 * GCC initialises globals in declaration order within a TU, so adjacency holds.
 * ========================================================================= */
int unk_1005E678[] = {
    0x0000A991, 0x00000001,
    0x0000A757, 0x00000001,
    0x00007267, 0x00000001,
    0x00007A0D, 0x00000001,
    0x0000937C, 0x00000001,
    0x0000CF9B, 0x00000001,
    0x0000C661, 0x00000001,
    0x0000AAA3, 0x00000001,
    0x00009795, 0x00000001,
    0x00009C59, 0x00000001,
    0x00002528, 0x00000001,
    0x000055B2, 0x00000001,
    0x0000879D, 0x00000001,
    0x0000AE75, 0x00000001,
    0x0000E512, 0x00000001,
    0x0000218B, 0x00000001,
    0x00008E97, 0x00000001,
    0x00007437, 0x00000001,
    0x00000AE2, 0x00000001,
    0x000007C1, 0x00000001,
    0x00005CAD, 0x00000001,
    0x000074D6, 0x00000001,
    0x0000694A, 0x00000001,
    0x00000E67, 0x00000001,
    0x0000F2C4, 0x00000001,
    0x0000EB92, 0x00000001,
    0x00006322, 0x00000001,
    0x0000B8A5, 0x00000001,
    0x0000E1CC, 0x00000001,
    0x00004E75, 0x00000001,
    0x00002BB0, 0x00000001,
    0x0000C54F, 0x00000001,
    0x0000CDD2, 0x00000001,
    0x0000DD83, 0x00000001,
    0x00000CA7, 0x00000001,
    0x0000107E, 0x00000001,
    0x00002874, 0x00000001,
    0x0000CE27, 0x00000001,
    0x0000DADC, 0x00000001,
    0x000097A9, 0x00000001,
    0x0000A84B, 0x00000001,
    0x000036FC, 0x00000001,
    0x000090DA, 0x00000001,
    0x00005214, 0x00000001,
    0x0000D714, 0x00000001,
    0x00009384, 0x00000001,
    0x00006490, 0x00000001,
    0x00001617, 0x00000001,
    0x00007113, 0x00000001,
    0x0000CEFE, 0x00000001,
    0x000060E6, 0x00000001,
    0x00001F50, 0x00000001,
    0x0000C7F8, 0x00000001,
    0x0000568B, 0x00000001,
    0x00007CF6, 0x00000001,
    0x00000A17, 0x00000001,
    0x00005491, 0x00000001,
    0x00002920, 0x00000001,
    0x0000C438, 0x00000001,
    0x0000B379, 0x00000001,
    0x00003418, 0x00000001,
    0x0000AC0B, 0x00000006,
    0x000035FB, 0x00000006,
    0x00005FC8, 0x00000006,
    0x0000A486, 0x00000006,
    0x00009AAF, 0x00000006,
    0x000020C2, 0x00000006,
    0x0000FB60, 0x00000006,
    0x00004FDE, 0x00000006,
    0x0000F0AB, 0x00000004,
    0x0000A9D4, 0x00000004,
    0x0000DF88, 0x00000004,
    0x0000E5CC, 0x00000004,
    0x00000ED6, 0x00000004,
    0x00008BE0, 0x00000006,
    0x0000A236, 0x00000006,
    0x00000BCB, 0x00000006,
    0x0000CC7C, 0x00000006,
    0x00003E22, 0x00000006,
    0x00000E04, 0x00000006,
    0x00004578, 0x00000006,
    0x0000343F, 0x00000006,
    0x0000FE11, 0x00000040,
    0x00008C2E, 0x00000040,
    0x0000C665, 0x00000040,
    0x00008AC0, 0x00000003,
    0x0000B1B7, 0x00000006,
    0x00006A8E, 0x00000008,
    0x00008DF3, 0x00000008,
    0x0000BC7D, 0x00000008,
    0x0000E488, 0x00000020,
    0x00000000, 0x00000000,   /* NULL terminator (entry 91) */
};
/* End-of-table sentinel: address used by sub_100377E0 as loop bound.
   Must immediately follow unk_1005E678 in memory. */
int unk_1005E958 = 0;

/* =========================================================================
 * unk_10060418 — 72-byte data blob; &[3] = "You are not allowed to..." message
 * Copied with qmemcpy(v10, &unk_10060418, 0x48); then bi_Print(5, &v10[3]).
 * ========================================================================= */
char unk_10060418[72] = {
    0x00, 0x00, 0x00, 0x59, 0x6F, 0x75, 0x20, 0x61, 0x72, 0x65, 0x20, 0x6E, 0x6F, 0x74, 0x20, 0x61,
    0x6C, 0x6C, 0x6F, 0x77, 0x65, 0x64, 0x20, 0x74, 0x6F, 0x0A, 0x6D, 0x6F, 0x64, 0x69, 0x66, 0x79,
    0x20, 0x74, 0x68, 0x65, 0x20, 0x62, 0x6F, 0x74, 0x20, 0x63, 0x68, 0x61, 0x72, 0x61, 0x63, 0x74,
    0x65, 0x72, 0x73, 0x20, 0x69, 0x6E, 0x0A, 0x69, 0x6E, 0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x76,
    0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x2E, 0x00,
};
