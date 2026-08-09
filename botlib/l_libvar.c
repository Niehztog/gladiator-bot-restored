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

#include "botlib_local.h"
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
