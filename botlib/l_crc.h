/* l_crc.h — interface of l_crc.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_L_CRC_H
#define BOTLIB_L_CRC_H

unsigned short __cdecl CRC_Block(const unsigned char *data, int length);
_WORD *__cdecl CRC_Init(_WORD *crcvalue);
void __cdecl CRC_ProcessByte(unsigned short *crcvalue, byte data);
unsigned short __cdecl CRC_Value(unsigned short crcvalue);
void __cdecl sub_100386E0(unsigned __int16 *crc, char *data, int len);

#endif /* BOTLIB_L_CRC_H */
