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
 *
 * Every function below carries a two-line address annotation: its extent in the
 * 1999 Windows `gladiator.dll` (PE32) and in the 1999 Linux `gladi386.so`
 * (ELF32, glibc build), each start..end with the end exclusive.  `absent` means
 * the Linux image has no counterpart.  CLAUDE.md, "Function address
 * annotations", records how both ranges are derived.
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
#include "l_log.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_memory.h"
typedef struct logfile_s {
    char  filename[1024];   /* 0x10063A40  (was byte_10063A40)               */
    FILE *fp;               /* 0x10063E40 */
    int   numwrites;        /* 0x10063E44  (was dword_10063E44)              */
} logfile_t;
logfile_t logfile;

// gladiator.dll: 10038BE0..10038CA3
// gladi386.so:   0004A610..0004A712
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

// gladiator.dll: 10038CF0..10038D3C
// gladi386.so:   0004A714..0004A790
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

// gladiator.dll: 10038D60..10038D6F
// gladi386.so:   0004A790..0004A80C
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

// gladiator.dll: 10038D80..10038DB9
// gladi386.so:   0004A80C..0004A85F
/* `if (fp) { body } return fp;`, not an inline early-return, so the body is the
 * warm fall-through and the NULL return the cold forward-`je` target -- same
 * shared-epilogue idiom as the dead Log_WriteTimeStamped below. */
FILE *Log_Write(char *Format, ...)
{
  va_list va; // [esp+8h] [ebp+8h] BYREF

  if ( logfile.fp )
  {
    va_start(va, Format);
    vfprintf(logfile.fp, Format, va);
    fprintf(logfile.fp, "\r\n");
    return (FILE *)fflush(logfile.fp);
  }
  /* No explicit return on the !fp path: the original shares the main epilogue and
   * leaves eax undefined, whereas an explicit `return logfile.fp` forces its own
   * epilogue block. All call sites discard the return value as a bare statement,
   * so the undefined value is never observed. */
}

// gladiator.dll: 10038DD0..10038E8C
// gladi386.so:   0004A860..0004A9C1
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

  /* `if (fp) { body } return fp;`, not an inline early-return, so the body is the
   * warm fall-through and the NULL return the cold forward-`je` target.
   * numwrites++ follows the "\r\n" fprintf, interleaving with the fflush setup
   * as the original does. */
  if ( logfile.fp )
  {
    fprintf(logfile.fp, "%d   %02d:%02d:%02d:%02d   ",
            logfile.numwrites,
            (int)(*(float *)&botstate.bottime * 0.00027777778450399637f),
            (int)(*(float *)&botstate.bottime * 0.01666666753590107f),
            (int)*(float *)&botstate.bottime,
            (int)(100.0f * *(float *)&botstate.bottime) - 100 * (int)*(float *)&botstate.bottime);
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

// gladiator.dll: 10038EC0..10038EC6
// gladi386.so:   0004A9C4..0004A9DF
FILE *Log_FilePointer()
{
  return logfile.fp;
}

// gladiator.dll: 10038EE0..10038EF1
// gladi386.so:   0004A9E0..0004AA08
void Log_Flush()
{
  if ( logfile.fp )
    fflush(logfile.fp);
}
