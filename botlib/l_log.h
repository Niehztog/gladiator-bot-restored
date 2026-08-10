/*
 * l_log.h — interface of l_log.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_L_LOG_H
#define BOTLIB_L_LOG_H

int __cdecl Log_Close(void);
FILE *Log_FilePointer();
void Log_Flush();
void Log_Open(char *FileName);  
FILE *Log_Shutdown();
FILE *Log_Write(char *Format, ...);
FILE *__cdecl Log_WriteTimeStamped(const char *Format, ...);

#endif /* BOTLIB_L_LOG_H */
