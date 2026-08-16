/* l_log.h — interface of l_log.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
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
