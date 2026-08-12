/*
 * l_struct.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x100404B0..0x1004123F (nine functions) and it owns no globals -- see
 * .claude/memory/tu_partition.md, where the Linux .so's F-number run for this
 * TU likewise holds exactly nine functions and references no named global.
 *
 * The include block below is botlib.c's, verbatim, so every macro and typedef
 * this file compiles against is the environment these functions had before the
 * split.
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
#include "l_struct.h"
#include "be_ai_weight.h"
#include "l_memory.h"
#include "l_precomp.h"
#include "l_script.h"

//----- (100404B0) --------------------------------------------------------
const char **__cdecl FindField(const char **defs, const char *name)
{
  const char **v2;
  int i;

  i = 0;
  for ( v2 = defs; *v2; v2 += 7 )
  {
    if ( !strcmp(*v2, name) )
      return &defs[7 * i];
    ++i;
  }
  return 0;
}

/* Field-table slot helpers.  Field tables are char *[7] entries addressed by
 * slot index rather than byte offset, so they work on both word widths:
 *   [0] name  [1] offset  [2] type|flags  [3] arr
 *   [4] minrange (float bits)  [5] maxrange (float bits)
 *   [6] substruct (structdef_t *) */
static inline int fielddef_flags(char **f) { return (int)(intptr_t)f[2]; }
static inline float fielddef_float(char **f, int slot) {
    return *(float *)&f[slot];   /* direct low-32 read (LE-safe); matches ref's fld [ebp+off] */
}

/* fielddef_t — Q3's l_struct.h field descriptor, overlaid on the char *[7]
 * entries.  Members are pointer-sized so the struct lines up with the slots on
 * either word width. */
typedef struct fielddef_s {
    const char *name;            /* slot 0 */
    intptr_t    offset;          /* slot 1 */
    intptr_t    type;            /* slot 2 — low byte FT_*, 0x100 = FT_ARRAY */
    intptr_t    maxarray;        /* slot 3 */
    intptr_t    floatmin;        /* slot 4 — float bits */
    intptr_t    floatmax;        /* slot 5 — float bits */
    structdef_t *substruct;      /* slot 6 */
} fielddef_t;

//----- (10040540) --------------------------------------------------------
/* Test `(v8 & 0xFF)` directly at each site rather than caching it in a `type`
 * local, and keep the intmin/intmax clamp as Q3-style ternaries: the recomputed
 * mask is what blocks jump-threading and reproduces the original's repeated
 * `and edi,0xff` dispatch.
 *
 * The residual is four float comparisons that compile to fld/fcompp here where
 * the original has a single memory-operand fcom.  Caching the
 * fielddef_float(fd,4)/(fd,5) results into named locals to match was tried and
 * regresses badly — do not re-attempt without a new idea. */
int __cdecl ReadNumber(source_t *source, char **fd, float *p)
{
  int negative; // esi
  int v5; // eax
  double floatval; // st7
  int intval; // ebx
  int v8; // ecx
  int intmin; // esi
  int intmax; // rax
  float v18; // [esp+28h] [ebp-438h]
  token_t token;

  negative = 0;
  if ( !PC_ExpectAnyToken(source, token.string) )
    return 0;
  if ( token.type == 5 )
  {
    if ( (fielddef_flags(fd) & 0x400) != 0 )
    {
      SourceError(source, "expected unsigned value, found %s", token.string);
      return 0;
    }
    if ( strcmp(token.string, "-") )
    {
      SourceError(source, "unexpected punctuation %s", token.string);
      return 0;
    }
    negative = 1;
    if ( !PC_ExpectAnyToken(source, token.string) )
      return 0;
  }
  if ( token.type != 3 )
  {
    SourceError(source, "expected number, found %s", token.string);
    return 0;
  }
  if ( (token.subtype & 0x800) != 0 )
  {
    v5 = fielddef_flags(fd);
    if ( (_BYTE)v5 != 3 )
    {
      SourceError(source, "unexpected float");
      return 0;
    }
    floatval = token.floatvalue;
    if ( negative )
      floatval = -token.floatvalue;
    if ( (v5 & 0x200) != 0 && ((v18 = fielddef_float(fd, 4), floatval < v18) || floatval > fielddef_float(fd, 5)) )
    {
      SourceError(source, "float out of range [%f, %f]", v18, fielddef_float(fd, 5));
      return 0;
    }
    *p = floatval;
    return 1;
  }
  intval = token.intvalue;
  if ( negative )
    intval = -token.intvalue;
  v8 = fielddef_flags(fd);
  if ( (v8 & 0xFF) == 1 )
  {
    if ( (v8 & 0x400) != 0 )
    {
      intmin = 0;
      intmax = 255;
    }
    else
    {
      intmin = -128;
      intmax = 127;
    }
  }
  if ( (v8 & 0xFF) == 2 )
  {
    if ( (v8 & 0x400) != 0 )
    {
      intmin = 0;
      intmax = 0xFFFF;
    }
    else
    {
      intmin = -32768;
      intmax = 0x7FFF;
    }
  }
  if ( (v8 & 0xFF) == 1 || (v8 & 0xFF) == 2 )
  {
    if ( (v8 & 0x200) != 0 )
    {
      intmin = intmin > *(float *)&fd[4] ? intmin : *(float *)&fd[4];
      intmax = intmax < *(float *)&fd[5] ? intmax : *(float *)&fd[5];
    }
    if ( intval < intmin || intval > intmax )
    {
      SourceError(source, "value %d out of range [%d, %d]", intval, intmin, intmax);
      return 0;
    }
  }
  else if ( (v8 & 0xFF) == 3 )
  {
    if ( (v8 & 0x200) != 0 )
    {
      if ( (float)intval < *(float *)&fd[4] || (float)intval > *(float *)&fd[5] )
      {
        SourceError(source, "value %d out of range [%f, %f]", intval, *(float *)&fd[4], *(float *)&fd[5]);
        return 0;
      }
    }
  }
  if ( (v8 & 0xFF) == 1 )
  {
    *(_BYTE *)p = intval;
  }
  else if ( (v8 & 0xFF) == 2 )
  {
    *(_DWORD *)p = intval;
    return 1;
  }
  else
  {
    if ( (v8 & 0xFF) == 3 )
      *p = (float)intval;
    return 1;
  }
  return 1;
}

//----- (10040990) --------------------------------------------------------
int __cdecl ReadChar(source_t *source, char **fd, float *p)
{
  int result; // eax
  token_t token;

  result = PC_ExpectAnyToken(source, token.string);
  if ( !result )
    return result;
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

//----- (10040A50) --------------------------------------------------------
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

//----- (10040AD0) --------------------------------------------------------
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
    fd = (fielddef_t *)FindField(def->fields, token.string);
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
          if ( !ReadChar(source, (char **)fd, (float *)p) )
            return 0;
          p += sizeof(char);
          break;
        case 2:
          if ( !ReadNumber(source, (char **)fd, (float *)p) )
            return 0;
          p += sizeof(int);
          break;
        case 3:
          if ( !ReadNumber(source, (char **)fd, (float *)p) )
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

//----- (10040E30) --------------------------------------------------------
int __cdecl WriteIndent(FILE *fp, int indent)
{
  while ( indent-- > 0 )
  {
    if ( fprintf(fp, "\t") < 0 )
      return 0;
  }
  return 1;
}

//----- (10040E80) --------------------------------------------------------
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

//----- (10040F20) --------------------------------------------------------
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
  /* A plain `return 0` guard, not `result = WriteIndent(...); … return result;`:
   * the original's tail exit has no `xor eax,eax` because the fall-through
   * already proves eax == 0 from WriteIndent's return.  A trailing
   * `return result;` makes cl.exe canonicalise every `return 0` into one block
   * at the textual end, un-pinning the shared exit the original keeps inline at
   * the first guard. */
  if ( !WriteIndent(fp, indent - 1) )
    return 0;
  if ( fprintf(fp, "}\r\n") < 0 )
    return 0;
  return 1;
}

//----- (10041210) --------------------------------------------------------
int __cdecl WriteStructure(FILE *fp, int def, int structure)
{
  /* Thin entry point: WriteStructWithIndent with indent = 0. */
  return WriteStructWithIndent(fp, (structdef_t *)def, structure, 0);
}
