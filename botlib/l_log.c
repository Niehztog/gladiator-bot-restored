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

#include <math.h>
#include <stdarg.h>
#include <stddef.h>    /* offsetof, size_t */
#include <stdint.h>    /* intptr_t */
#include <stdio.h>     /* file I/O, sprintf, sscanf */
#include <string.h>    /* string and memory ops */
#include <stdlib.h>    /* malloc, atoi, rand, abs */
#include <ctype.h>     /* toupper */
#include <errno.h>     /* errno */
#include <time.h>      /* time(), ctime() */
#include <unistd.h>    /* access(), chdir(), getcwd() */

/* q_shared.h first, so its qboolean/vec3_t/cplane_t/… are the canonical types.
 * It sets Q_SHARED_H, which makes botlib.h skip its own Q2-type stubs. */
#include "../game/q_shared.h"
/* q_shared.h's VectorNegate is a 2-arg macro, but botlib has a 1-arg in-place
 * VectorNegate(v) function at 0x10043540.  Drop the macro so it stays callable. */
#undef VectorNegate
#include "../game/botlib.h"  /* bot_export_t, bot_import_t + prerequisite Q2 types */
#include "gladiator.dll.h"
#include "ea_state.h"    /* ea_state_t: 36-byte per-client EA struct (reconstructed) */
#include "aas_world.h"   /* aas_t, aas_area_t etc. (reconstructed from aasworld_* globals) */

#include "bot_state.h"   /* bot_state_t, BOT_* offset constants (reconstructed) */
#include "chat_state.h"  /* bot_match_t, bot_matchpiece_t etc. */
#include "libvar.h"      /* libvar_t: 24-byte botlib cvar (reconstructed) */
#include "botlib_state.h"   /* botimport / botstate / bot_exports + libvar aliases */
#include "botlib_structs.h" /* config, parser, weight, goal and item structs */
#include "struct_sizes_asserts.h" /* compile-time struct-layout guard */
#include "q2files.h"

/* signed high-word/dword accessors */
#define SHIDWORD(x)  (*((int *)&(x) + 1))

/* Stubs for the statically-linked MSVC CRT helpers the bot code still calls. */
static size_t fread_locked(void *buf, size_t sz, size_t n, FILE *f) { return fread(buf, sz, n, f); }
#ifdef _WIN32
/* Windows-only winbspc/zip subsystem (sub_1000E140/sub_1000E430, sub_10041FF0);
 * the Linux botlib has neither a process-spawn nor a chdir-for-bspc path. */
static int    remove_file(const char *path) { return remove(path); }
static int    getcwd_locked(char *buf, int size) { return getcwd(buf, size) ? 0 : -1; }
static intptr_t SpawnProcess(int mode, char *file, char *args, char *cmd, char *addargs) { (void)mode; (void)file; (void)args; (void)cmd; (void)addargs; return -1; }
/* _access is provided by MinGW's <io.h>; _chdir by <direct.h> — use POSIX wrappers */
static int    _chdir(const char *path) { return chdir(path); }
#endif /* _WIN32 */

#ifndef _WIN32
/* POSIX equivalents for the Windows CRT functions used below. */
#include <strings.h>    /* strcasecmp */
#define _strcmpi   strcasecmp
#define _access    access
typedef int __time32_t;   /* Windows 32-bit time type; int is 32-bit on all targets */
#endif

float     __cdecl LibVarValue(char *var_name, char *value);    /* register, return value */

/* l_log.c logfile state @ VA 0x10063A40: filename[1024]@0x10063A40,
 * fp@0x10063E40 and numwrites@0x10063E44 are contiguous in .data — one
 * logfile_t, as in Q3 l_log.c. */
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
