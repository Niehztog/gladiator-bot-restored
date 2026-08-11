/*
 * l_crc.h — interface of l_crc.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_L_CRC_H
#define BOTLIB_L_CRC_H

unsigned short __cdecl CRC_Block(const unsigned char *data, int length);
_WORD *__cdecl CRC_Init(_WORD *crcvalue);
void __cdecl CRC_ProcessByte(unsigned short *crcvalue, byte data);
unsigned short __cdecl CRC_Value(unsigned short crcvalue);
void __cdecl sub_100386E0(unsigned __int16 *crc, char *data, int len);

#endif /* BOTLIB_L_CRC_H */
