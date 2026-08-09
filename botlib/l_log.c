/*
 * l_log.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10038BE0..0x10038F0F (seven functions) and it owns the `logfile` state
 * block; both facts are recovered in .claude/memory/tu_partition.md (the Linux
 * .so's .dynsym keeps `logfile` under its real name and sizes it 1032 B, and
 * this TU's F-number run there holds exactly seven functions).
 *
 * The include block below is botlib.c's, verbatim, so every macro and typedef
 * this file compiles against is the environment these functions had before the
 * split.
 */

#include "botlib_local.h"
typedef struct logfile_s {
    char  filename[1024];   /* 0x10063A40  (was byte_10063A40)               */
    FILE *fp;               /* 0x10063E40 */
    int   numwrites;        /* 0x10063E44  (was dword_10063E44)              */
} logfile_t;
logfile_t logfile;

//----- (10038BE0) --------------------------------------------------------
/* One parameter, the filename — the libvar value the function fcomps at entry
 * is read internally, not passed in. */
void Log_Open(char *filename)
{
  if ( LibVarValue("log", (char *)"0") != 0.0f )      /* "log" libvar enabled (default "0") */
  {
    if ( !filename || !strlen(filename) )
    {
      botimport.Print(PRT_MESSAGE, "openlog <filename>\n");
    }
    else if ( logfile.fp )
    {
      botimport.Print(PRT_ERROR, "log file %s is already opened\n", logfile.filename);
    }
    else
    {
      logfile.fp = fopen(filename, "wb");          /* Mode = "wb" */
      if ( !logfile.fp )
      {
        botimport.Print(PRT_ERROR, "can't open the log file %s\n", filename);
      }
      else
      {
        strncpy(logfile.filename, filename, 0x400u);
        botimport.Print(PRT_MESSAGE, "Opened log %s\n", logfile.filename);
      }
    }
  }
}

//----- (10038CF0) --------------------------------------------------------
/* Close logfile.fp, clear it on success and print the close message (or an
 * error if fclose fails).  Live: Log_Shutdown@0x10038D60 is the guard wrapper
 * that tail-jumps here. */
int __cdecl Log_Close(void)
{
  int result; // eax

  result = (int)logfile.fp;
  if ( logfile.fp )
  {
    if ( fclose(logfile.fp) )
    {
      return botimport.Print(PRT_ERROR, "can't close log file %s\n", logfile.filename);
    }
    else
    {
      logfile.fp = 0;
      result = botimport.Print(PRT_MESSAGE, "Closed log %s\n", logfile.filename);
    }
  }
  return result;
}

//----- (10038D60) --------------------------------------------------------
/* `if (logfile.fp) Log_Close();` — the guarded wrapper, tail-jumping to
 * Log_Close, not a duplicate of it. */
FILE *Log_Shutdown()
{
  FILE *result; // eax

  result = logfile.fp;
  if ( logfile.fp )
    result = (FILE *)Log_Close();
  return result;
}

//----- (10038D80) --------------------------------------------------------
FILE *Log_Write(char *Format, ...)
{
  FILE *result; // eax
  va_list va; // [esp+8h] [ebp+8h] BYREF

  va_start(va, Format);
  result = logfile.fp;
  if ( logfile.fp )
  {
    vfprintf(logfile.fp, Format, va);
    fprintf(logfile.fp, "\r\n");
    return (FILE *)fflush(logfile.fp);
  }
  return result;
}

//----- (10038DD0) --------------------------------------------------------
/* An older, more elaborate Log_Write that prefixes each line with a counter
 * and an uptime timestamp, preserved alongside the live minimal
 * Log_Write@10038D80.
 *
 * Faithful original bug: the 4th %02d field is fed the TOTAL uptime seconds
 * rather than seconds-within-minute, so timestamps past 60 s read oddly.
 *
 * DEAD in Gladiator.
 */
FILE *__cdecl Log_WriteTimeStamped(const char *Format, ...)
{
  va_list va;
  int sec_total;
  int hund;
  int min;
  int hour;

  /* `if (fp) { body } return fp;`, not an inline early-return, so the body is the
   * warm fall-through and the NULL return the cold forward-`je` target.
   * numwrites++ follows the "\r\n" fprintf, interleaving with the fflush setup
   * as the original does. */
  if ( logfile.fp )
  {
    sec_total = (int)*(float *)&botstate.bottime;
    hund      = -100 * sec_total - (int)(*(float *)&botstate.bottime * -100.0f);
    min       = (int)(*(float *)&botstate.bottime * 0.01666666753590107f);
    hour      = (int)(*(float *)&botstate.bottime * 0.00027777778450399637f);
    fprintf(logfile.fp, "%d   %02d:%02d:%02d:%02d   ",
            logfile.numwrites, hour, min, sec_total, hund);
    va_start(va, Format);
    vfprintf(logfile.fp, Format, va);
    va_end(va);
    fprintf(logfile.fp, "\r\n");
    logfile.numwrites++;
    return (FILE *)fflush(logfile.fp);
  }
  /* No explicit return on the !fp path: the original shares the main epilogue and
   * leaves eax undefined, whereas an explicit `return logfile.fp` forces its own
   * epilogue block.  The function is dead, so the undefined value is never
   * observed. */
}

//----- (10038EC0) --------------------------------------------------------
FILE *Log_FilePointer()
{
  return logfile.fp;
}

//----- (10038EE0) --------------------------------------------------------
void Log_Flush()
{
  if ( logfile.fp )
    fflush(logfile.fp);
}
