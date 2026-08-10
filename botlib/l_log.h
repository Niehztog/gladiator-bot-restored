/*
 * l_log.h — interface of l_log.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_L_LOG_H
#define BOTLIB_L_LOG_H

#include "botlib_local.h"

int __cdecl Log_Close(void);
FILE *Log_FilePointer();
void Log_Flush();
void Log_Open(char *FileName);  
FILE *Log_Shutdown();
FILE *Log_Write(char *Format, ...);
FILE *__cdecl Log_WriteTimeStamped(const char *Format, ...);

#endif /* BOTLIB_L_LOG_H */
