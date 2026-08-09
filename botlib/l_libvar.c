/*
 * l_libvar.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10038750..0x10038BDF (thirteen functions) and it owns the `libvarlist`
 * head pointer; both facts are recovered in .claude/memory/tu_partition.md
 * (the Linux .so's .dynsym keeps `libvarlist` under its real name, and this
 * TU's F-number run there holds exactly thirteen functions).
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

void *__cdecl GetMemory(int size);
void *__cdecl GetClearedMemory(unsigned int size);
int __cdecl FreeMemory(void *ptr);  /* dummy `int` return; see definition */
int __cdecl MemoryByteSize(void *ptr);
/* The shared empty-string sentinel at 0x1006294C; still defined in botlib.c
 * (its owning TU has not been split out yet). */
extern char byte_1006294C;

libvar_t *libvarlist; /* head of singly-linked libvar list (was dword_10063F20) */

//----- (10038750) --------------------------------------------------------
float __cdecl LibVarStringValue(char *string)
{
  int dotfound;
  float value;

  dotfound = 0;
  value = 0.0f;
  while ( *string )
  {
    if ( *string < '0' || *string > '9' )
    {
      if ( dotfound || *string != '.' )
        return 0.0f;
      dotfound = 10;
      ++string;
    }
    if ( dotfound )
    {
      value = value + (float)(*string - '0') / (float)dotfound;
      dotfound *= 10;
    }
    else
    {
      value = value * 10.0 + (float)(*string - '0');
    }
    ++string;
  }
  return value;
}

//----- (10038810) --------------------------------------------------------
libvar_t *__cdecl LibVarAlloc(const char *var_name)
{
  libvar_t *v;

  v = (libvar_t *)GetMemory(strlen(var_name) + sizeof(libvar_t) + 1);
  memset(v, 0, sizeof(libvar_t));
  v->name = (char *)v + sizeof(libvar_t);
  strcpy(v->name, var_name);
  v->next = libvarlist;
  libvarlist = v;
  return v;
}

//----- (100388A0) --------------------------------------------------------
void __cdecl LibVarDeAlloc(libvar_t *v)
{
  if ( v->string )
    FreeMemory(v->string);
  FreeMemory(v);
}

//----- (100388D0) --------------------------------------------------------
// Drains the libvarlist by repeatedly popping
// the head, advancing libvarlist to head->next (struct offset +0x14),
// and calling LibVarDeAlloc on the popped node.  Final
// libvarlist = NULL is redundant after the loop but is present in the
// disasm (a literal "mov DWORD PTR ds:0x10063F20, 0").  This is the
// shutdown path that Q3 botlib exposes as LibVarDeAllocAll.
void __cdecl LibVarDeAllocAll(void)
{
  libvar_t *v;

  while ( (v = libvarlist) != NULL )
  {
    libvarlist = v->next;
    LibVarDeAlloc(v);
  }
  libvarlist = NULL;
}

//----- (10038910) --------------------------------------------------------
libvar_t *__cdecl LibVarGet(const char *var_name)
{
  libvar_t *v;

  for ( v = libvarlist; v; v = v->next )
  {
    if ( !_strcmpi(v->name, var_name) )
      return v;
  }
  return NULL;
}

//----- (10038960) --------------------------------------------------------
char *__cdecl LibVarGetString(const char *var_name)
{
  libvar_t *v = LibVarGet(var_name);
  if ( v )
    return v->string;
  return &byte_1006294C;
}

//----- (10038990) --------------------------------------------------------
float __cdecl LibVarGetValue(const char *var_name)
{
  libvar_t *v = LibVarGet(var_name);
  if ( v )
    return v->value;
  return 0.0f;
}

//----- (100389C0) --------------------------------------------------------
libvar_t *__cdecl LibVar(char *var_name, char *value)
{
  libvar_t *v = LibVarGet(var_name);
  if ( !v )
  {
    v = LibVarAlloc(var_name);
    v->string = (char *)GetMemory(strlen(value) + 1);
    strcpy(v->string, value);
    v->value = LibVarStringValue(v->string);
    v->modified = 1;
  }
  return v;
}

//----- (10038A60) --------------------------------------------------------
char *__cdecl LibVarString(char *var_name, char *value)
{
  /* Returns libvar->string.  Thunk 0x100013BB forwards all file-path lookups
   * here, not to LibVar. */
  return LibVar(var_name, value)->string;
}

//----- (10038A90) --------------------------------------------------------
float __cdecl LibVarValue(char *var_name, char *value)
{
  /* Registers the libvar if missing, then returns its numeric value. */
  return LibVar(var_name, value)->value;
}

//----- (10038AC0) --------------------------------------------------------
void __cdecl LibVarSet(char *var_name, char *value)
{
  libvar_t *v = LibVarGet(var_name);
  if ( v )
    FreeMemory(v->string);
  else
    v = LibVarAlloc(var_name);
  v->string = (char *)GetMemory(strlen(value) + 1);
  strcpy(v->string, value);
  v->value = LibVarStringValue(v->string);
  v->modified = 1;
}

//----- (10038B80) --------------------------------------------------------
/* Q3's LibVarGetModified — the `modified` flag for the named libvar, or 0 if
 * there is no such libvar.  DEAD in Gladiator. */
int __cdecl LibVarChanged(const char *var_name)
{
  libvar_t *v;

  v = LibVarGet(var_name);
  if ( v )
    return v->modified;
  return 0;
}

//----- (10038BB0) --------------------------------------------------------
/* Q3's LibVarSetNotModified — clear the `modified` flag on the named libvar,
 * a no-op if it does not exist.  DEAD in Gladiator. */
void __cdecl LibVarSetNotModified(const char *var_name)
{
  libvar_t *v;

  v = LibVarGet(var_name);
  if ( v )
    v->modified = 0;
}
