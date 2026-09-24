/*
 * l_struct.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x100404B0..0x1004123F.
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
#include "l_struct.h"
#include "be_ai_weight.h"
#include "l_memory.h"
#include "l_precomp.h"
#include "l_script.h"

// gladiator.dll: 100404B0..1004051A
// gladi386.so:   00052E9C..00052EE2
/* Q3's loop over the TYPED descriptor array, verbatim.  Each compiler
 * strength-reduces it its own way: cl.exe walks a pointer but keeps `i` alive
 * to rebuild `&defs[i]` as defs + 28*i on the hit path (the DLL's
 * `lea eax,[ebp*8]; sub eax,ebp`), gcc drops `i` and returns the pointer.
 * IDA's char** walk plus a counter matched the DLL only; `defs[i]` with
 * `i += 7` on the char** view, the .so only. */
fielddef_t *__cdecl FindField(fielddef_t *defs, const char *name)
{
  int i;

  for ( i = 0; defs[i].name; i++ )
  {
    if ( !strcmp(defs[i].name, name) ) return &defs[i];
  }
  return NULL;
}

// gladiator.dll: 10040540..100408AD
// gladi386.so:   00052EE4..000532DD
/* Q3's text over Q3's typed descriptor -- `fd->type` read at every test (the
 * .so reloads it each time; IDA had cached one `v8`), `fd->floatmin` /
 * `fd->floatmax` as real float members (cl.exe compares them with a
 * memory-operand fcom; the old cast accessors gave fld/fcompp), Q3's
 * Maximum/Minimum -- but WITHOUT Q3's `intmin = 0, intmax = 0` initialisers,
 * which the DLL does not have (they cost it a 148-line register cascade). */
int __cdecl ReadNumber(source_t *source, fielddef_t *fd, float *p)
{
  token_t token;
  int negative = 0;
  int intval, intmin, intmax;
  double floatval;

  if ( !PC_ExpectAnyToken(source, token.string) ) return 0;
  /* check for minus sign */
  if ( token.type == 5 )
  {
    if ( fd->type & 0x400 )
    {
      SourceError(source, "expected unsigned value, found %s", token.string);
      return 0;
    }
    /* if not a minus sign */
    if ( strcmp(token.string, "-") )
    {
      SourceError(source, "unexpected punctuation %s", token.string);
      return 0;
    }
    negative = 1;
    /* read the number */
    if ( !PC_ExpectAnyToken(source, token.string) ) return 0;
  }
  /* check if it is a number */
  if ( token.type != 3 )
  {
    SourceError(source, "expected number, found %s", token.string);
    return 0;
  }
  /* check for a float value */
  if ( token.subtype & 0x800 )
  {
    if ( (fd->type & 0xFF) != 3 )
    {
      SourceError(source, "unexpected float");
      return 0;
    }
    floatval = token.floatvalue;
    if ( negative ) floatval = -floatval;
    if ( fd->type & 0x200 )
    {
      if ( floatval < fd->floatmin || floatval > fd->floatmax )
      {
        SourceError(source, "float out of range [%f, %f]", fd->floatmin, fd->floatmax);
        return 0;
      }
    }
    *(float *) p = (float) floatval;
    return 1;
  }
  intval = token.intvalue;
  if ( negative ) intval = -intval;
  /* check bounds */
  if ( (fd->type & 0xFF) == 1 )
  {
    if ( fd->type & 0x400 ) {intmin = 0; intmax = 255;}
    else {intmin = -128; intmax = 127;}
  }
  if ( (fd->type & 0xFF) == 2 )
  {
    if ( fd->type & 0x400 ) {intmin = 0; intmax = 65535;}
    else {intmin = -32768; intmax = 32767;}
  }
  if ( (fd->type & 0xFF) == 1 || (fd->type & 0xFF) == 2 )
  {
    if ( fd->type & 0x200 )
    {
      intmin = Maximum(intmin, fd->floatmin);
      intmax = Minimum(intmax, fd->floatmax);
    }
    if ( intval < intmin || intval > intmax )
    {
      SourceError(source, "value %d out of range [%d, %d]", intval, intmin, intmax);
      return 0;
    }
  }
  else if ( (fd->type & 0xFF) == 3 )
  {
    if ( fd->type & 0x200 )
    {
      if ( intval < fd->floatmin || intval > fd->floatmax )
      {
        SourceError(source, "value %d out of range [%f, %f]", intval, fd->floatmin, fd->floatmax);
        return 0;
      }
    }
  }
  /* store the value */
  if ( (fd->type & 0xFF) == 1 )
  {
    if ( fd->type & 0x400 ) *(unsigned char *) p = (unsigned char) intval;
    else *(char *) p = (char) intval;
  }
  else if ( (fd->type & 0xFF) == 2 )
  {
    if ( fd->type & 0x400 ) *(unsigned int *) p = (unsigned int) intval;
    else *(int *) p = (int) intval;
  }
  else if ( (fd->type & 0xFF) == 3 )
  {
    *(float *) p = (float) intval;
  }
  return 1;
}

// gladiator.dll: 10040990..10040A14
// gladi386.so:   000532E0..0005336C
int __cdecl ReadChar(source_t *source, fielddef_t *fd, float *p)
{
  token_t token;

  if ( !PC_ExpectAnyToken(source, token.string) ) return 0;
  /* take literals into account */
  if ( token.type == 2 )
  {
    StripSingleQuotes(token.string);
    *(_BYTE *)p = token.string[0];
  }
  else
  {
    PC_UnreadLastToken(source);
    if ( !ReadNumber(source, fd, p) )
      return 0;
  }
  return 1;
}

// gladiator.dll: 10040A50..10040AAA
// gladi386.so:   0005336C..000533D8
int __cdecl ReadString(source_t *source, char **fd, char *p)
{
  char Source[sizeof(token_t)] __attribute__((aligned(8))); // [esp+0h] [ebp-430h] BYREF

  if ( !PC_ExpectTokenType(source, 1, 0, Source) )
    return 0;
  StripDoubleQuotes(Source);
  strncpy(p, Source, 0x50u);
  p[79] = 0;
  return 1;
}

// gladiator.dll: 10040AD0..10040D81
// gladi386.so:   000533D8..00053764
int __cdecl ReadStructure(source_t *source, structdef_t *def, char *structure)
{
  token_t token;
  fielddef_t *fd; // ebp
  char *p; // edi
  int num; // [esp+10h] [ebp-434h]

  if ( !PC_ExpectTokenString(source, "{") )
    return 0;
  while ( 1 )
  {
    if ( !PC_ExpectAnyToken(source, token.string) )
      return 0;
    if ( !strcmp(token.string, "}") )
      break;
    fd = FindField((fielddef_t *)def->fields, token.string);
    if ( !fd )
    {
      SourceError(source, "unknown structure field %s", token.string);
      return 0;
    }
    if ( (fd->type & 0x100) != 0 )
    {
      num = (int)fd->maxarray;
      if ( !PC_ExpectTokenString(source, "{") )
        return 0;
    }
    else
    {
      num = 1;
    }
    p = structure + fd->offset;
    while ( num-- > 0 )
    {
      if ( (fd->type & 0x100) != 0 )
      {
        if ( PC_CheckTokenString(source, "}") )
          break;
      }
      switch ( fd->type & 0xFF )
      {
        case 1:
          if ( !ReadChar(source, fd, (float *)p) )
            return 0;
          p += sizeof(char);
          break;
        case 2:
          if ( !ReadNumber(source, fd, (float *)p) )
            return 0;
          p += sizeof(int);
          break;
        case 3:
          if ( !ReadNumber(source, fd, (float *)p) )
            return 0;
          p += sizeof(float);
          break;
        case 4:
          if ( !ReadString(source, (char **)fd, p) )
            return 0;
          p += 80;
          break;
        case 6:
          if ( !fd->substruct )
          {
            SourceError(source, "BUG: no sub structure defined");
            return 0;
          }
          ReadStructure(source, fd->substruct, p);
          p += fd->substruct->size;
          break;
      }
      if ( (fd->type & 0x100) != 0 )
      {
        if ( !PC_ExpectAnyToken(source, token.string) )
          return 0;
        if ( !strcmp(token.string, "}") )
          break;
        if ( strcmp(token.string, ",") )
        {
          SourceError(source, "expected a comma, found %s", token.string);
          return 0;
        }
      }
    }
  }
  return 1;
}

// gladiator.dll: 10040E30..10040E67
// gladi386.so:   00053764..000537AC
int __cdecl WriteIndent(FILE *fp, int indent)
{
  while ( indent-- > 0 )
  {
    if ( fprintf(fp, "\t") < 0 )
      return 0;
  }
  return 1;
}

// gladiator.dll: 10040E80..10040EFE
// gladi386.so:   000537AC..00053860
int __cdecl WriteFloat(FILE *fp, float value)
{
  char buf[128];
  int l;

  sprintf(buf, "%f", value);
  l = strlen(buf);
  while ( l-- > 1 )
  {
    if ( buf[l] != '0' && buf[l] != '.' ) break;
    if ( buf[l] == '.' )
    {
      buf[l] = 0;
      break;
    }
    buf[l] = 0;
  }
  if ( fprintf(fp, "%s", buf) < 0 )
    return 0;
  return 1;
}

// gladiator.dll: 10040F20..1004114B
// gladi386.so:   00053860..00053C11
int __cdecl WriteStructWithIndent(FILE *fp, structdef_t *def, int structure, int indent)
{
  int i; // ebp (strength-reduced to a byte offset)
  int num; // reuses the dead Stream param slot [esp+14h]
  char *p; // esi
  fielddef_t *fd; // ebx + ebp

  /* Thunk 0x10001ac3 -> WriteIndent, not fputc. */
  if ( !WriteIndent(fp, indent) )
    return 0;
  if ( fprintf(fp, "{\r\n") < 0 )
    return 0;
  ++indent;
  for ( i = 0; ((fielddef_t *)def->fields)[i].name; i++ )
  {
    fd = &((fielddef_t *)def->fields)[i];
    if ( !WriteIndent(fp, indent) )
      return 0;
    if ( fprintf(fp, "%s\t", fd->name) < 0 )
      return 0;
    p = (char *)(structure + fd->offset);
    if ( (fd->type & 0x100) != 0 )
    {
      num = fd->maxarray;
      if ( fprintf(fp, "{") < 0 )
        return 0;
    }
    else
    {
      num = 1;
    }
    while ( num-- > 0 )
    {
      switch ( fd->type & 0xFF )
      {
        case 1:
          if ( fprintf(fp, "%d", *(char *)p) < 0 )
            return 0;
          p += 1;
          break;
        case 2:
          if ( fprintf(fp, "%d", *(int *)p) < 0 )
            return 0;
          p += 4;
          break;
        case 3:
          if ( !WriteFloat(fp, *(float *)p) )
            return 0;
          p += 4;
          break;
        case 4:
          if ( fprintf(fp, "\"%s\"", p) < 0 )
            return 0;
          p += 80;
          break;
        case 6:
          /* Nested struct: recurse. */
          if ( !WriteStructWithIndent(fp, fd->substruct, structure, indent) )
            return 0;
          p += fd->substruct->size;
          break;
      }
      if ( (fd->type & 0x100) != 0 )
      {
        if ( num > 0 )
        {
          if ( fprintf(fp, ",") < 0 )
            return 0;
        }
        else if ( fprintf(fp, "}") < 0 )
        {
          return 0;
        }
      }
    }
    if ( fprintf(fp, "\r\n") < 0 )
      return 0;
  }
  /* A plain `return 0` guard, not `result = WriteIndent(...); … return result;`: the
   * original's tail exit has no `xor eax,eax` because the fall-through already proves
   * eax == 0 from WriteIndent's return.  A trailing `return result;` makes cl.exe
   * canonicalise every `return 0` into one block at the textual end, un-pinning the
   * shared exit the original keeps inline at the first guard. */
  --indent;
  if ( !WriteIndent(fp, indent) )
    return 0;
  if ( fprintf(fp, "}\r\n") < 0 )
    return 0;
  return 1;
}

// gladiator.dll: 10041210..1004122A
// gladi386.so:   00053C14..00053C3C
int __cdecl WriteStructure(FILE *fp, int def, int structure)
{
  /* Thin entry point: WriteStructWithIndent with indent = 0. */
  return WriteStructWithIndent(fp, (structdef_t *)def, structure, 0);
}
