/*
 * l_crc.h — interface of l_crc.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_L_CRC_H
#define BOTLIB_L_CRC_H

#include "botlib_local.h"

__int16 __cdecl CRC_Block(const unsigned char *data, int length);
_WORD *__cdecl CRC_Init(_WORD *crcvalue);
void __cdecl CRC_ProcessByte(unsigned short *crcvalue, byte data);
__int16 __cdecl CRC_Value(__int16 crcvalue);
void __cdecl sub_100386E0(unsigned __int16 *crc, char *data, int len);

#endif /* BOTLIB_L_CRC_H */
