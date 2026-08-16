/*
 * l_libvar.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10038750..0x10038BDF.
 */

#include "botlib_port.h"
#include "l_libvar.h"
#undef VectorNegate
#include "be_ea.h"
#include "q2files.h"
#include "aasfile.h"
#include "be_aas_def.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "be_ai_def.h"
#include "be_interface.h"
#include "struct_sizes_asserts.h"
#include "l_libvar.h"
#include "l_memory.h"
#include "l_utils.h"
libvar_t *libvarlist; /* head of singly-linked libvar list (was dword_10063F20) */

// gladiator.dll: 10038750..100387D2
// gladi386.so:   00049FE8..0004A07E
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

// gladiator.dll: 10038810..10038872
// gladi386.so:   0004A080..0004A0E0
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

// gladiator.dll: 100388A0..100388C0
// gladi386.so:   0004A0E0..0004A10E
void __cdecl LibVarDeAlloc(libvar_t *v)
{
  if ( v->string )
    FreeMemory(v->string);
  FreeMemory(v);
}

// gladiator.dll: 100388D0..100388FF
// gladi386.so:   0004A110..0004A161
// Drain the libvarlist by repeatedly popping the head, advancing libvarlist to
// head->next (struct offset +0x14), and calling LibVarDeAlloc on the popped node.  The
// final libvarlist = NULL is redundant after the loop but is present in the disasm.
// Q3 botlib exposes this as LibVarDeAllocAll.
void __cdecl LibVarDeAllocAll(void)
{
  libvar_t *v;

  v = libvarlist;
  if ( v )
  {
    do
    {
      libvarlist = libvarlist->next;
      LibVarDeAlloc(v);
    }
    while ( (v = libvarlist) != NULL );
  }
  libvarlist = NULL;
}

// gladiator.dll: 10038910..10038941
// gladi386.so:   0004A164..0004A1A9
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

// gladiator.dll: 10038960..1003897B
// gladi386.so:   0004A1AC..0004A1FF
char *__cdecl LibVarGetString(const char *var_name)
{
  libvar_t *v = LibVarGet(var_name);
  if ( v )
    return v->string;
  return "";
}

// gladiator.dll: 10038990..100389AC
// gladi386.so:   0004A200..0004A24F
float __cdecl LibVarGetValue(const char *var_name)
{
  libvar_t *v = LibVarGet(var_name);
  if ( v )
    return v->value;
  return 0.0f;
}

// gladiator.dll: 100389C0..10038A31
// gladi386.so:   0004A250..0004A3CA
libvar_t *__cdecl LibVar(char *var_name, char *value)
{
  libvar_t *v = LibVarGet(var_name);
  if ( v )
    return v;
  v = LibVarAlloc(var_name);
  v->string = (char *)GetMemory(strlen(value) + 1);
  strcpy(v->string, value);
  v->value = LibVarStringValue(v->string);
  v->modified = 1;
  return v;
}

// gladiator.dll: 10038A60..10038A76
// gladi386.so:   0004A3CC..0004A3F0
char *__cdecl LibVarString(char *var_name, char *value)
{
  /* Returns libvar->string.  Thunk 0x100013BB forwards all file-path lookups
   * here, not to LibVar. */
  return LibVar(var_name, value)->string;
}

// gladiator.dll: 10038A90..10038AA6
// gladi386.so:   0004A3F0..0004A414
float __cdecl LibVarValue(char *var_name, char *value)
{
  /* Registers the libvar if missing, then returns its numeric value. */
  return LibVar(var_name, value)->value;
}

// gladiator.dll: 10038AC0..10038B42
// gladi386.so:   0004A414..0004A574
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

// gladiator.dll: 10038B80..10038B98
// gladi386.so:   0004A574..0004A5C3
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

// gladiator.dll: 10038BB0..10038BC9
// gladi386.so:   0004A5C4..0004A610
/* Q3's LibVarSetNotModified — clear the `modified` flag on the named libvar,
 * a no-op if it does not exist.  DEAD in Gladiator. */
void __cdecl LibVarSetNotModified(const char *var_name)
{
  libvar_t *v;

  v = LibVarGet(var_name);
  if ( v )
    v->modified = 0;
}
