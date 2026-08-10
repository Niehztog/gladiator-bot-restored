/*
 * l_crc.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x100385B0..0x1003874F (five functions) and it owns the `crctable`
 * lookup table; both facts are recovered in .claude/memory/tu_partition.md
 * (the Linux .so's .dynsym keeps `crctable` under its real name, and this
 * TU's F-number run there holds exactly five functions).
 *
 * The include block below is botlib.c's, verbatim, so every macro and typedef
 * this file compiles against is the environment these functions had before the
 * split.
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
#include "l_crc.h"
#include "l_memory.h"
__int16 crctable[308] =
{
  0,
  4129,
  8258,
  12387,
  16516,
  20645,
  24774,
  28903,
  -32504,
  -28375,
  -24246,
  -20117,
  -15988,
  -11859,
  -7730,
  -3601,
  4657,
  528,
  12915,
  8786,
  21173,
  17044,
  29431,
  25302,
  -27847,
  -31976,
  -19589,
  -23718,
  -11331,
  -15460,
  -3073,
  -7202,
  9314,
  13379,
  1056,
  5121,
  25830,
  29895,
  17572,
  21637,
  -23190,
  -19125,
  -31448,
  -27383,
  -6674,
  -2609,
  -14932,
  -10867,
  13907,
  9842,
  5649,
  1584,
  30423,
  26358,
  22165,
  18100,
  -18597,
  -22662,
  -26855,
  -30920,
  -2081,
  -6146,
  -10339,
  -14404,
  18628,
  22757,
  26758,
  30887,
  2112,
  6241,
  10242,
  14371,
  -13876,
  -9747,
  -5746,
  -1617,
  -30392,
  -26263,
  -22262,
  -18133,
  23285,
  19156,
  31415,
  27286,
  6769,
  2640,
  14899,
  10770,
  -9219,
  -13348,
  -1089,
  -5218,
  -25735,
  -29864,
  -17605,
  -21734,
  27814,
  31879,
  19684,
  23749,
  11298,
  15363,
  3168,
  7233,
  -4690,
  -625,
  -12820,
  -8755,
  -21206,
  -17141,
  -29336,
  -25271,
  32407,
  28342,
  24277,
  20212,
  15891,
  11826,
  7761,
  3696,
  -97,
  -4162,
  -8227,
  -12292,
  -16613,
  -20678,
  -24743,
  -28808,
  -28280,
  -32343,
  -20022,
  -24085,
  -12020,
  -16083,
  -3762,
  -7825,
  4224,
  161,
  12482,
  8419,
  20484,
  16421,
  28742,
  24679,
  -31815,
  -27752,
  -23557,
  -19494,
  -15555,
  -11492,
  -7297,
  -3234,
  689,
  4752,
  8947,
  13010,
  16949,
  21012,
  25207,
  29270,
  -18966,
  -23093,
  -27224,
  -31351,
  -2706,
  -6833,
  -10964,
  -15091,
  13538,
  9411,
  5280,
  1153,
  29798,
  25671,
  21540,
  17413,
  -22565,
  -18438,
  -30823,
  -26696,
  -6305,
  -2178,
  -14563,
  -10436,
  9939,
  14066,
  1681,
  5808,
  26199,
  30326,
  17941,
  22068,
  -9908,
  -13971,
  -1778,
  -5841,
  -26168,
  -30231,
  -18038,
  -22101,
  22596,
  18533,
  30726,
  26663,
  6336,
  2273,
  14466,
  10403,
  -13443,
  -9380,
  -5313,
  -1250,
  -29703,
  -25640,
  -21573,
  -17510,
  19061,
  23124,
  27191,
  31254,
  2801,
  6864,
  10931,
  14994,
  -722,
  -4849,
  -8852,
  -12979,
  -16982,
  -21109,
  -25112,
  -29239,
  31782,
  27655,
  23652,
  19525,
  15522,
  11395,
  7392,
  3265,
  -4321,
  -194,
  -12451,
  -8324,
  -20581,
  -16454,
  -28711,
  -24584,
  28183,
  32310,
  20053,
  24180,
  11923,
  16050,
  3793,
  7920,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
}; // weak

//----- (100385B0) --------------------------------------------------------
_WORD *__cdecl CRC_Init(_WORD *crcvalue)
{
  _WORD *result; // eax

  result = crcvalue;
  *crcvalue = -1;
  return result;
}

//----- (100385D0) --------------------------------------------------------
// Standard CCITT CRC-16 single-byte update
// against the lookup table at crctable.  Mirrors Q3 CRC_ProcessByte.
// Dead in Gladiator (CRC_Block at 10038640 inlines the same step), but
// preserved by the linker.
void __cdecl CRC_ProcessByte(unsigned short *crcvalue, byte data)
{
	 *crcvalue = (*crcvalue << 8) ^ crctable[(*crcvalue >> 8) ^ data];
}

//----- (10038620) --------------------------------------------------------
__int16 __cdecl CRC_Value(__int16 crcvalue)
{
  return crcvalue;
}

//----- (10038640) --------------------------------------------------------
__int16 __cdecl CRC_Block(const unsigned char *data, int length)
{
  unsigned __int16 crcvalue;
  int i, ind;

  CRC_Init(&crcvalue);
  for ( i = 0; i < length; i++ )
  {
    ind = (crcvalue >> 8) ^ data[i];
    if ( ind < 0 || ind > 256 )
      ind = 0;
    crcvalue = (crcvalue << 8) ^ crctable[ind];
  }
  return CRC_Value(crcvalue);
}

//----- (100386E0) --------------------------------------------------------
// Restored dead stub:
// CRC-16 multi-byte update over `data[0..len-1]`, applying the same per-byte
// transform as CRC_ProcessByte in a tight loop and writing the result back
// through `crc`.  Mirrors Q3 CRC_ProcessByteString — dead in Gladiator (the
// equivalent loop is inlined in CRC_Block at 10038640).  Note the disasm
// scans the data buffer using `[eax+ebp*1]` with eax=loop-counter and
// ebp=base — the classic index/base swap, equivalent to
// data[i] for i in [0..len).
void __cdecl sub_100386E0(unsigned __int16 *crc, char *data, int len)
{
  int i;

  for ( i = 0; i < len; i++ )
  {
    *crc = (*crc << 8) ^ crctable[(*crc >> 8) ^ data[i]];
  }
}
