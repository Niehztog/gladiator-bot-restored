/*
 * l_precomp.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10039200..0x1003E000; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * Its own interface is in the matching .h; botlib_local.h, which that header
 * pulls in, carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs, the side-band scheme and the externs for
 * botlib.c's remaining globals).
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
#include "l_precomp.h"
#include "be_ai_weight.h"
#include "be_interface.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_utils.h"

define_t *globaldefines;

/* Preprocessor directive table at VA 0x1005F260 — a {char*, int(*)(int)} array
 * in .data.  #ifdef/#ifndef are 1-arg wrappers over PC_Directive_if_def(src,
 * INDENT_IFDEF/INDENT_IFNDEF) -- Q3 botlib has exactly this trio and the same
 * INDENT_* values, and gladi386.so exports all three (F243/F244/F245), so the
 * two wrappers are the original's own functions and not reconstruction glue.
 * `directive_t` / `directives` / `dollardirectives` are the original names,
 * recovered from the Linux gladi386.so .dynsym (both tables 160 B = Q3's
 * `directive_t directives[20]`), replacing the invented preproc_directive_t /
 * preproc_directives / eval_type_table.  Deliberately NOT `static`: `nm -D`
 * on gladi386.so shows both tables as exported `D` symbols
 * (0005c6b0 D directives / 0005c750 D dollardirectives), which a file-static
 * array can never be -- .dynsym only ever lists symbols with external
 * linkage.  This also fixes PC_ReadDirective's codegen: gcc -fPIC addresses a
 * non-static (potentially-interposable) global through a real GOT pointer
 * slot (load the slot, then dereference), where a `static` array gets a
 * direct GOT-relative address with no extra indirection -- the disassembly
 * diff against real showed exactly that extra indirection missing. */
//----- (1003B7B0) --------------------------------------------------------
int __cdecl PC_Directive_ifdef(source_t *src)
{
  return PC_Directive_if_def(src, INDENT_IFDEF);
} //end of the function PC_Directive_ifdef

//----- (1003B7D0) --------------------------------------------------------
int __cdecl PC_Directive_ifndef(source_t *src)
{
  return PC_Directive_if_def(src, INDENT_IFNDEF);
} //end of the function PC_Directive_ifndef

typedef struct { const char *name; int (__cdecl *handler)(intptr_t); } directive_t;
directive_t directives[] = {
    {"if",        PC_Directive_if},   /* 0x1003CCB0 */
    {"ifdef",     PC_Directive_ifdef},   /* 0x1003B7B0 */
    {"ifndef",    PC_Directive_ifndef},  /* 0x1003B7D0 */
    {"elif",      PC_Directive_elif},         /* 0x1003CC10 */
    {"else",      PC_Directive_else},         /* 0x1003B7F0 */
    {"endif",     PC_Directive_endif},         /* 0x1003B880 */
    {"include",   PC_Directive_include},         /* 0x1003A7A0 */
    {"define",    (int(*)(intptr_t))PC_Directive_define},            /* 0x1003ADE0 */
    {"undef",     PC_Directive_undef},             /* 0x1003AC30 */
    {"line",      (int(*)(intptr_t))PC_Directive_line}, /* 0x1003CD00 */
    {"error",     PC_Directive_error},         /* 0x1003CD30 */
    {"pragma",    PC_Directive_pragma},         /* 0x1003CD80 */
    {"eval",      PC_Directive_eval}, /* 0x1003CE90 */
    {"evalfloat", PC_Directive_evalfloat},         /* 0x1003CF80 */
    {NULL, NULL}
};
/* Preserved as aliases so existing PC_ReadDirective code referencing &off_1005F260 still compiles. */
char *off_1005F260 = "if"; // the table's first name field; see directives
int (__cdecl *off_1005F264)(intptr_t) = &PC_Directive_if; // weak — original: first handler
/* dollardirectives — $-directive dispatch table at VA 0x1005F300: 2 entries +
 * NULL, walked by PC_ReadDollarDirective as a stride-2 pointer array.  Same
 * directive_t element type as `directives` above (as in Q3 l_precomp.c). */
directive_t dollardirectives[] = {
    {"evalint",   PC_DollarDirective_evalint},   /* 0x100011D6 thunk → PC_DollarDirective_evalint */
    {"evalfloat", PC_DollarDirective_evalfloat},        /* 0x10001B0E thunk → PC_DollarDirective_evalfloat     */
    {NULL, NULL}
};
char *off_1005F300 = "evalint";                        /* alias: first name field  */
int (__cdecl *off_1005F304)(intptr_t) = &PC_DollarDirective_evalint; /* alias: first handler     */

//----- (10039200) --------------------------------------------------------
int SourceError(source_t *src, char *Format, ...)
{

  char Buffer[1024]; // [esp+0h] [ebp-400h] BYREF
  va_list va; // [esp+40Ch] [ebp+Ch] BYREF

  va_start(va, Format);
  vsprintf(Buffer, Format, va);
  return botimport.Print(
           3,
           "file %s, line %d: %s\n",
           src->scriptstack->filename,
           src->scriptstack->line,
           Buffer);
}
//----- (10039270) --------------------------------------------------------
int SourceWarning(source_t *src, char *Format, ...)
{

  char Buffer[1024]; // [esp+0h] [ebp-400h] BYREF
  va_list va; // [esp+40Ch] [ebp+Ch] BYREF

  va_start(va, Format);
  vsprintf(Buffer, Format, va);
  return botimport.Print(
           2,
           "file %s, line %d: %s\n",
           src->scriptstack->filename,
           src->scriptstack->line,
           Buffer);
}
//----- (100392E0) --------------------------------------------------------
void __cdecl PC_PushIndent(source_t *source, int type, int skip)
{
  indent_t *ind;

  ind = (indent_t *)GetMemory(sizeof(indent_t));
  ind->type   = type;
  ind->script = source->scriptstack;
  ind->skip   = (skip != 0);
  source->skip  += ind->skip;
  ind->next   = source->indentstack;
  source->indentstack = ind;
}
//----- (10039350) --------------------------------------------------------
void __cdecl PC_PopIndent(source_t *source, int *type, int *skip)
{
  indent_t *ind;

  *type = 0;
  *skip = 0;
  ind = source->indentstack;
  if ( !ind )
    return;
  if ( ind->script != source->scriptstack )
    return;
  *type = ind->type;
  *skip = ind->skip;
  source->indentstack = source->indentstack->next;
  source->skip       -= ind->skip;
  FreeMemory(ind);
}
//----- (100393E0) --------------------------------------------------------
void __cdecl PC_PushScript(source_t *source, script_t *script)
{
  script_t *s;

  for ( s = source->scriptstack; s; s = s->next )
  {
    if ( !Q_stricmp(s->filename, script->filename) )
    {
      SourceError(source, "%s recursively included", script->filename);
      return;
    }
  }
  script->next = source->scriptstack;
  source->scriptstack = script;
}
//----- (10039460) --------------------------------------------------------
/* Duplicate a token_t.  Clear `next` through the struct member — its offset
 * moves once the pointer fields widen. */
token_t *__cdecl PC_CopyToken(const token_t *token)
{
  token_t *result;

  result = (token_t *)GetMemory(sizeof(token_t));
  memcpy(result, token, sizeof(token_t));
  result->next = NULL;
  return result;
}
//----- (100394A0) --------------------------------------------------------
void __cdecl PC_FreeToken(token_t *token)
{
  FreeMemory(token);
}
//----- (100394C0) --------------------------------------------------------
int __cdecl PC_ReadSourceToken(source_t *source, token_t *token)
{
  token_t *t;
  script_t *script;
  int type, skip;

  while ( !source->tokens )
  {
    if ( PS_ReadToken(source->scriptstack, token) )
      return 1;
    if ( EndOfScript(source->scriptstack) )
    {
      while ( source->indentstack &&
              source->indentstack->script == source->scriptstack )
      {
        SourceWarning(source, "missing #endif");
        PC_PopIndent(source, &type, &skip);
      }
    }
    if ( !source->scriptstack->next )
      return 0;
    script = source->scriptstack;
    source->scriptstack = source->scriptstack->next;
    FreeScript(script);
  }
  memcpy(token, source->tokens, sizeof(token_t));
  t = source->tokens;
  source->tokens = source->tokens->next;
  PC_FreeToken(t);
  return 1;
}
//----- (100395F0) --------------------------------------------------------
int __cdecl PC_UnreadSourceToken(source_t *source, const void *token)
{
  struct token_s *copy;

  copy = (struct token_s *)PC_CopyToken(token);  /* allocate 1072-byte copy */
  copy->next = source->tokens;
  source->tokens = copy;
  return 1;
}
//----- (10039630) --------------------------------------------------------
/* Read the actual argument list passed to a function-style macro at a call
 * site.  Same signature as Q3's PC_ReadDefineParms. */
int __cdecl PC_ReadDefineParms(source_t *source, define_t *define, token_t **parms, int maxparms)
{
  int i;
  int done;
  int lastcomma;
  int numparms;
  token_t *last;
  int indent;
  token_t *t;
  token_t token __attribute__((aligned(8))); // [esp+20h] [ebp-430h] BYREF

  if ( !PC_ReadSourceToken(source, &token) )
  {
    SourceError(source, "define %s missing parms", define->name);
    return 0;
  }
  if ( define->numparms > maxparms )
  {
    SourceError(source, "define with more than %d parameters", maxparms);
    return 0;
  }
  for (i = 0; i < define->numparms; i++) parms[i] = NULL;
  if ( strcmp(token.string, "(") )
  {
    PC_UnreadSourceToken(source, &token);
    SourceError(source, "define %s missing parms", define->name);
    return 0;
  }
  for ( done = 0, numparms = 0, indent = 0; !done; )
  {
    if ( numparms >= maxparms )
    {
      SourceError(source, "define %s with too many parms", define->name);
      return 0;
    }
    if ( numparms >= define->numparms )
    {
      SourceWarning(source, "define %s has too many parms", define->name);
      return 0;
    }
    parms[numparms] = NULL;
    lastcomma = 1;
    last = NULL;
    while ( !done )
    {
      if ( !PC_ReadSourceToken(source, &token) )
      {
        SourceError(source, "define %s incomplete", define->name);
        return 0;
      }
      if ( !strcmp(token.string, ",") )
      {
        if ( indent <= 0 )
        {
          if ( lastcomma ) SourceWarning(source, "too many comma's");
          lastcomma = 1;
          break;
        }
      }
      lastcomma = 0;
      if ( !strcmp(token.string, "(") )
      {
        indent++;
        continue;
      }
      else if ( !strcmp(token.string, ")") )
      {
        if ( --indent <= 0 )
        {
          if ( !parms[define->numparms - 1] )
          {
            SourceWarning(source, "too few define parms");
          }
          done = 1;
          break;
        }
      }
      if ( numparms < define->numparms )
      {
        t = PC_CopyToken(&token);
        t->next = NULL;
        if ( last ) last->next = t;
        else parms[numparms] = t;
        last = t;
      }
    }
    numparms++;
  }
  return 1;
}
// 10039776: conditional instruction was optimized away because ecx.4==0
//----- (10039A70) --------------------------------------------------------
int __cdecl PC_StringizeTokens(token_t *tokens, token_t *token)
{
  token_t *i;

  token->type = 1;
  token->whitespace_p = NULL;
  token->endwhitespace_p = NULL;
  token->string[0] = 0;
  strcat(token->string, "\"");
  for ( i = tokens; i; i = i->next )
    strncat(token->string, i->string, 1024 - strlen(token->string));
  strncat(token->string, "\"", 1024 - strlen(token->string));
  return 1;
}
//----- (10039B50) --------------------------------------------------------
int __cdecl PC_MergeTokens(token_t *t1, token_t *t2)
{
  if ( t1->type == 4 )
  {
    if ( t2->type == 4 || t2->type == 3 )
    {
      strcat(t1->string, t2->string);
      return 1;
    }
  }
  if ( t1->type == 1 && t2->type == 1 )
  {
    t1->string[strlen(t1->string) - 1] = 0;
    strcat(t1->string, &t2->string[1]);
    return 1;
  }
  return 0;
}
//----- (10039C30) --------------------------------------------------------
unsigned int __cdecl PC_NameHash(const char *name)
{
  unsigned int v2; // ecx
  int v4 = 0; // [esp+4h] [ebp-4h] BYREF

  /* ONE trailing return, and a redundant `v4 = 0` on the len == 0 arm.  The ELF
   * dictates this literally: gladi386.so tests the clamped length and, when it
   * is zero, jumps to a second `mov [esp+0xc],0` that falls straight into the
   * shared hash tail — three separate paths converging on one epilogue rather
   * than the early `return abs(v4) & 0x3FF` this used to carry.  Shape matters
   * beyond this row because gcc inlines PC_NameHash into its seven callers. */
  if ( name )
  {
    v2 = strlen(name);
    if ( (int)v2 > 4 )
      v2 = 4;
    if ( v2 )
      memcpy(&v4, name, v2);
    else
      v4 = 0;
  }
  /* `if (v4 < 0) v4 = -v4;`, not `abs(v4)`: the original negates IN PLACE and
   * re-reads the variable (`neg eax; mov [esp+0xc],eax; mov eax,[esp+0xc]`),
   * which abs() folds away. */
  if ( v4 < 0 )
    v4 = -v4;
  return v4 & 0x3FF;
}
//----- (10039CB0) --------------------------------------------------------
/* Prepend `define` to its bucket in the source's definehash table, keyed by
 * PC_NameHash of the define's name. */
void __cdecl PC_AddDefineToHash(define_t *define, define_t **definehash)
{
  unsigned int hash;

  hash = PC_NameHash(define->name);
  define->hashnext = definehash[hash];
  definehash[hash] = define;
}
//----- (10039CE0) --------------------------------------------------------
/* Hash-bucket lookup of a define_t by name: `definehash` is the bucket array
 * indexed by PC_NameHash, each bucket chained via define_t.hashnext. */
define_t *__cdecl PC_FindHashedDefine(define_t **definehash, const char *name)
{
  define_t *v2;

  for ( v2 = definehash[PC_NameHash(name)]; v2; v2 = v2->hashnext )
  {
    if ( !strcmp(v2->name, name) )
      return v2;
  }
  return NULL;
}
//----- (10039D70) --------------------------------------------------------
/* Linear search of a define_t list.  The only caller is
 * PC_RemoveGlobalDefine, walking globaldefines. */
int __cdecl PC_FindDefine(define_t *defines, const char *name)
{
  define_t *v2;

  for ( v2 = defines; v2; v2 = v2->next )
  {
    if ( !strcmp(v2->name, name) )
      return (intptr_t)v2;
  }
  return 0;
}
//----- (10039DF0) --------------------------------------------------------
int __cdecl PC_FindDefineParm(define_t *define, const char *name)
{
  token_t *p;
  int i;

  i = 0;
  for ( p = define->parms; p; p = p->next )
  {
    if ( !strcmp(p->string, name) )
      return i;
    ++i;
  }
  return -1;
}
//----- (10039E70) --------------------------------------------------------
void __cdecl PC_FreeDefine(define_t *define)
{
  struct token_s *t, *next;

  for ( t = define->parms; t; t = next )
  {
    next = t->next;
    PC_FreeToken(t);
  }
  for ( t = define->tokens; t; t = next )
  {
    next = t->next;
    PC_FreeToken(t);
  }
  FreeMemory(define);
}
//----- (10039EE0) --------------------------------------------------------
/* Q3's PC_AddBuiltinDefines — walk a local {name, value} table of __LINE__,
 * __FILE__, __DATE__, __TIME__ and add each to source->definehash as a
 * built-in define, with the name stored inline after the struct.
 * DEAD in Gladiator. */
void __cdecl PC_AddBuiltinDefines(source_t *source)
{
  struct {
  char *name;
  int   value;
  } builtin[] = {
  { "__LINE__", 1 },
  { "__FILE__", 2 },
  { "__DATE__", 3 },
  { "__TIME__", 4 },
  { NULL,       0 },
  };
  define_t *def;
  int i;

  for ( i = 0; builtin[i].name; ++i )
  {
  def = (define_t *)GetMemory(strlen(builtin[i].name) + 0x21);
  memset(def, 0, sizeof(define_t));
  def->name = (char *)def + sizeof(define_t);
  strcpy(def->name, builtin[i].name);
  def->flags |= 1;
  def->builtin = builtin[i].value;
  PC_AddDefineToHash(def, source->definehash);
  }
}
//----- (1003A000) --------------------------------------------------------
int __cdecl PC_ExpandBuiltinDefine(source_t *src, define_t *define, char **a3, char **a4)
{
  int v4;
  char *curtime;
  char *v7;
  __time32_t t;
  token_t token __attribute__((aligned(8))); // [esp+14h] [ebp-430h] BYREF

  memcpy(&token, &src->cachedtoken, sizeof(token));
  v4 = define->builtin - 1;
  switch ( v4 )
  {
    case 0:
      sprintf(token.string, "%d", src->cachedtoken.line);
      token.intvalue = src->cachedtoken.line;
      token.floatvalue = src->cachedtoken.line;
      token.type = 3;
      token.subtype = 4104;
      *a3 = (char *)&token;
      *a4 = (char *)&token;
      break;
    case 1:
      strcpy(token.string, src->scriptstack->filename);
      token.type = 4;
      token.subtype = strlen(token.string);
      *a3 = (char *)&token;
      *a4 = (char *)&token;
      break;
    case 2:
      t = time(0);
      curtime = ctime(&t);
      strcpy(token.string, "\"");
      strncat(token.string, curtime + 4, 7u);
      strncat(&token.string[7], curtime + 20, 4u);
      strcat(token.string, "\"");
      free(curtime);
      token.type = 4;
      token.subtype = strlen(token.string);
      *a3 = (char *)&token;
      *a4 = (char *)&token;
      break;
    case 3:
      t = time(0);
      v7 = ctime(&t);
      strcpy(token.string, "\"");
      strncat(token.string, v7 + 11, 8u);
      strcat(token.string, "\"");
      free(v7);
      token.type = 4;
      token.subtype = strlen(token.string);
      *a3 = (char *)&token;
      *a4 = (char *)&token;
      break;
    case 4:
    default:
      *a3 = 0;
      *a4 = 0;
      break;
  }
  return 1;
}
//----- (1003A2D0) --------------------------------------------------------
int __cdecl PC_ExpandDefine(source_t *src, define_t *define, char **firsttoken, char **lasttoken)
{
  token_t *parms[128]; // [esp+14h] [ebp-630h] BYREF
  token_t *dt, *pt, *t;
  token_t *t1, *t2, *first, *last, *nextpt;
  token_t token; // [esp+214h] [ebp-430h] BYREF
  int parmnum, i;

  if ( define->builtin )
    return PC_ExpandBuiltinDefine(src, define, firsttoken, lasttoken);
  if ( define->numparms )
  {
    if ( !PC_ReadDefineParms(src, define, parms, 128) )
      return 0;
  }
  first = NULL;
  last = NULL;
  for ( dt = define->tokens; dt; dt = dt->next )
  {
    parmnum = -1;
    if ( dt->type == 4 )
      parmnum = PC_FindDefineParm(define, dt->string);
    if ( parmnum >= 0 )
    {
      for ( pt = parms[parmnum]; pt; pt = pt->next )
      {
        t = PC_CopyToken(pt);
        t->next = NULL;
        if ( last )
          last->next = t;
        else
          first = t;
        last = t;
      }
    }
    else
    {
      if ( !strcmp(dt->string, "#") )
      {
        if ( dt->next )
          parmnum = PC_FindDefineParm(define, dt->next->string);
        else
          parmnum = -1;
        if ( parmnum >= 0 )
        {
          dt = dt->next;
          if ( !PC_StringizeTokens((char *)parms[parmnum], (char *)&token) )
          {
            SourceError(src, "can't stringize tokens");
            return 0;
          }
          t = PC_CopyToken(&token);
        }
        else
        {
          SourceWarning(src, "stringizing operator without define parameter");
          continue;
        }
      }
      else
      {
        t = PC_CopyToken(dt);
      }
      t->next = NULL;
      if ( last )
        last->next = t;
      else
        first = t;
      last = t;
    }
  }
  for ( t = first; t; )
  {
    if ( t->next && !strcmp(t->next->string, "##") )
    {
      t1 = t;
      t2 = t->next->next;
      if ( t2 )
      {
        if ( !PC_MergeTokens(t1, t2) )
        {
          SourceError(src, "can't merge %s with %s", t1->string, t2->string);
          return 0;
        }
        PC_FreeToken(t1->next);
        t1->next = t2->next;
        if ( t2 == last )
          last = t1;
        PC_FreeToken(t2);
        continue;
      }
    }
    t = t->next;
  }
  *firsttoken = (char *)first;
  *lasttoken = (char *)last;
  for ( i = 0; i < define->numparms; i++ )
  {
    for ( pt = parms[i]; pt; pt = nextpt )
    {
      nextpt = pt->next;
      PC_FreeToken(pt);
    }
  }
  return 1;
}
// 1003A4E6: conditional instruction was optimized away because eax.4==0
//----- (1003A690) --------------------------------------------------------
int __cdecl PC_ExpandDefineIntoSource(source_t *src, define_t *define)
{
  token_t *firsttoken;
  token_t *lasttoken;

  if ( !PC_ExpandDefine(src, define, (char **)&firsttoken, (char **)&lasttoken) )
    return 0;
  if ( firsttoken && lasttoken )
  {
    lasttoken->next = src->tokens;
    src->tokens = firsttoken;
    return 1;
  }
  return 0;
}
//----- (1003A710) --------------------------------------------------------
void __cdecl PC_ConvertPath(char *path)
{
  char *ptr;

  for ( ptr = path; *ptr; )
  {
    if ( (*ptr == '\\' || *ptr == '/') &&
         (*(ptr + 1) == '\\' || *(ptr + 1) == '/') )
    {
#if defined(_MSC_VER) || (defined(__i386__) && !defined(__SSE_MATH__))
      strcpy(ptr, ptr + 1);
#else
      memmove(ptr, ptr + 1, strlen(ptr));
#endif
    }
    else
    {
      ptr++;
    }
  }
  for ( ptr = path; *ptr; )
  {
    if ( *ptr == '/' || *ptr == '\\' )
      /* Two-week source drift (see AAS_Trace in be_aas_bspq2.c for the
       * fully-documented instance): the Windows DLL (verified via IDA,
       * sub_1003A710) normalizes to backslash, but the real gladi386.so
       * writes forward slash here instead -- Quake's own file/VFS layer
       * is always '/'-separated regardless of host OS, so the Linux port
       * of this include-path normalizer plausibly diverged on purpose. */
#ifdef _WIN32
      *ptr = '\\';
#else
      *ptr = '/';
#endif
    ptr++;
  }
}
//----- (1003A7A0) --------------------------------------------------------
int __cdecl PC_Directive_include(source_t *source)
{
  char *script;
  token_t token;
  char path[MAX_PATH]; // [esp+10h] [ebp-5CCh] BYREF
  bot_fileref_t file;

  if ( source->skip > 0 )
    return 1;
  if ( !PC_ReadSourceToken(source, token.string) )
  {
    SourceError(source, "#include without file name");
    return 0;
  }
  if ( token.linescrossed > 0 )
  {
    SourceError(source, "#include without file name");
    return 0;
  }
  if ( token.type == 1 )
  {
    StripDoubleQuotes(token.string);
    PC_ConvertPath(token.string);
    script = (char *)LoadScriptFile(token.string, 0, 0);
    if ( !script )
    {
      strcpy(path, source->includepath);
      strcat(path, token.string);
      script = (char *)LoadScriptFile(path, 0, 0);
    }
  }
  else if ( token.type == 5 && token.string[0] == 60 )
  {
    strcpy(path, source->includepath);
    while ( PC_ReadSourceToken(source, token.string) )
    {
      if ( token.linescrossed > 0 )
      {
        PC_UnreadSourceToken(source, token.string);
        break;
      }
      if ( token.type == 5 && token.string[0] == 62 )
        break;
      strncat(path, token.string, MAX_PATH);
    }
    if ( token.string[0] != 62 )
      SourceWarning(source, "#include missing trailing >");
    if ( !strlen(path) )
    {
      SourceError(source, "#include without file name between < >");
      return 0;
    }
    PC_ConvertPath(path);
    script = (char *)LoadScriptFile(path, 0, 0);
  }
  else
  {
    SourceError(source, "#include without file name");
    return 0;
  }
  if ( !script )
  {
    memset(&file, 0, sizeof(file));
    if ( sub_10041F60(path, &file) )
    {
      script = (char *)LoadScriptFile(file.path, file.fileofs, file.filelen);
      if ( script )
        strncpy(script, path, MAX_PATH);
    }
  }
  if ( !script )
  {
    SourceError(source, "file %s not found", path);
    return 0;
  }
  PC_PushScript(source, script);
  return 1;
}
//----- (1003AB10) --------------------------------------------------------
/* Read tokens, skipping line-continuation backslashes; stop and unread once the
 * next token is on a new line. */
int __cdecl PC_ReadLine(source_t *source, token_t *token)
{
  int crossline; // ebp

  crossline = 0;
  do
  {
    if ( !PC_ReadSourceToken(source, token) )
      return 0;
    if ( token->linescrossed > crossline )
    {
      PC_UnreadSourceToken(source, token);
      return 0;
    }
    crossline = 1;
  }
  while ( !strcmp(token->string, "\\") );
  return 1;
}
//----- (1003ABD0) --------------------------------------------------------
/* Returns (token->endwhitespace_p > token->whitespace_p). */
BOOL __cdecl PC_WhiteSpaceBeforeToken(token_t *token)
{
  int diff;

  diff = token->endwhitespace_p - token->whitespace_p;
  return diff > 0;
}
//----- (1003AC00) --------------------------------------------------------
/* Zeroes whitespace_p, endwhitespace_p and linescrossed. */
token_t *__cdecl PC_ClearTokenWhiteSpace(token_t *token)
{
  token->whitespace_p = NULL;
  token->endwhitespace_p = NULL;
  token->linescrossed = 0;
  return token;
}
//----- (1003AC30) --------------------------------------------------------
int __cdecl PC_Directive_undef(source_t *source)
{
  unsigned int hash;
  define_t *lastdefine;
  define_t *define;
  token_t token;

  if ( source->skip > 0 )
    return 1;
  if ( !PC_ReadLine(source, token.string) )
  {
    SourceError(source, "undef without name");
    return 0;
  }
  if ( token.type != 4 )
  {
    PC_UnreadSourceToken(source, token.string);
    SourceError(source, "expected name, found %s", token.string);
    return 0;
  }
  hash = PC_NameHash(token.string);
  for ( lastdefine = NULL, define = source->definehash[hash]; define; define = define->hashnext )
  {
    if ( !strcmp(define->name, token.string) )
    {
      if ( (define->flags & 1) != 0 )
      {
        SourceWarning(source, "can't undef %s", token.string);
      }
      else
      {
        if ( lastdefine )
          lastdefine->hashnext = define->hashnext;
        else
          source->definehash[hash] = define->hashnext;
        PC_FreeDefine(define);
      }
      break;
    }
    lastdefine = define;
  }
  return 1;
}
//----- (1003ADE0) --------------------------------------------------------
//----- (1003ADE0) --------------------------------------------------------
/* Struct-field access throughout rather than the original's literal offsets, so
 * this stays correct as define_t grows 32 -> 56 bytes and token_t 1072 -> 1088
 * on 64-bit.  Variable names follow Q3's PC_Directive_define. */
int __cdecl PC_Directive_define(source_t *source)
{
  token_t token;
  token_t *t, *last;
  define_t *define;

  if ( source->skip > 0 )
    return 1;
  if ( !PC_ReadLine(source, &token) )
  {
    SourceError(source, "#define without name");
    return 0;
  }
  if ( token.type != 4 )
  {
    PC_UnreadSourceToken(source, &token);
    SourceError(source, "expected name after #define, found %s", token.string);
    return 0;
  }
  define = PC_FindHashedDefine(source->definehash, token.string);
  if ( define )
  {
    if ( define->flags & 1 )
    {
      SourceError(source, "can't redefine %s", token.string);
      return 0;
    }
    SourceWarning(source, "redefinition of %s", token.string);
    PC_UnreadSourceToken(source, &token);
    if ( !PC_Directive_undef(source) )
      return 0;
    define = PC_FindHashedDefine(source->definehash, token.string);
  }
  define = (define_t *)GetMemory(sizeof(define_t) + strlen(token.string) + 1);
  memset(define, 0, sizeof(define_t));
  define->name = (char *)define + sizeof(define_t);
  strcpy(define->name, token.string);
  PC_AddDefineToHash(define, source->definehash);
  if ( !PC_ReadLine(source, &token) )
    return 1;
  if ( !PC_WhiteSpaceBeforeToken(&token) && !strcmp(token.string, "(") )
  {
    last = NULL;
    if ( !PC_CheckTokenString(source, ")") )
    {
      while ( 1 )
      {
        if ( !PC_ReadLine(source, &token) )
        {
          SourceError(source, "expected define parameter");
          return 0;
        }
        if ( token.type != 4 )
        {
          SourceError(source, "invalid define parameter");
          return 0;
        }
        if ( PC_FindDefineParm(define, token.string) >= 0 )
        {
          SourceError(source, "two the same define parameters");
          return 0;
        }
        t = PC_CopyToken(&token);
        PC_ClearTokenWhiteSpace(t);
        t->next = NULL;
        if ( last )
          last->next = t;
        else
          define->parms = t;
        last = t;
        define->numparms++;
        if ( !PC_ReadLine(source, &token) )
        {
          SourceError(source, "define parameters not terminated");
          return 0;
        }
        if ( !strcmp(token.string, ")") )
          break;
        if ( strcmp(token.string, ",") )
        {
          SourceError(source, "define not terminated");
          return 0;
        }
      }
    }
    if ( !PC_ReadLine(source, &token) )
      return 1;
  }
  last = NULL;
  do
  {
    t = PC_CopyToken(&token);
    PC_ClearTokenWhiteSpace(t);
    t->next = NULL;
    if ( last )
      last->next = t;
    else
      define->tokens = t;
    last = t;
  }
  while ( PC_ReadLine(source, &token) );
  if ( !strcmp(define->tokens->string, "##") || !strcmp(last->string, "##") )
  {
    SourceError(source, "define with misplaced ##");
    return 0;
  }
  return 1;
}
//----- (1003B320) --------------------------------------------------------
/* Allocate a stub source_t on the stack, parse `string` through
 * PC_Directive_define, copy out the single resulting define from the local
 * definehash, free the scratch buffers and return it. */
define_t *__cdecl PC_DefineFromString(const char *string)
{
  script_t *script;
  int res;
  token_t *t;
  int i;
  define_t *def;
  source_t src; // [esp+Ch] [ebp-658h]

  script = LoadScriptMemory(string, strlen(string), "*extern");
  memset(&src, 0, sizeof(src));
  strncpy(src.filename, "*extern", MAX_PATH);
  src.scriptstack = script;
  src.definehash = (define_t **)GetClearedMemory(1024 * sizeof(define_t *));
  res = PC_Directive_define(&src);
  for ( t = src.tokens; src.tokens; t = src.tokens )
  {
    src.tokens = t->next;
    PC_FreeToken(t);
  }
  def = NULL;
  for ( i = 0; i < 1024; i++ )
  {
    if ( src.definehash[i] )
    {
      def = src.definehash[i];
      break;
    }
  }
  FreeMemory(src.definehash);
  FreeScript(script);
  if ( res > 0 )
    return def;
  if ( src.defines )
    PC_FreeDefine(def);
  return NULL;
}
//----- (1003B460) --------------------------------------------------------
/* Q3's PC_AddDefine — parse `string` into a fresh define_t and link it into
 * source->definehash; 1 on success, 0 if parsing failed.
 * DEAD in Gladiator. */
int __cdecl PC_AddDefine(source_t *source, const char *string)
{
  define_t *def;

  def = PC_DefineFromString(string);
  if ( !def )
    return 0;
  PC_AddDefineToHash(def, source->definehash);
  return 1;
}
//----- (1003B4A0) --------------------------------------------------------
/* Prepend a parsed define onto the file-static `globaldefines` list. */
int __cdecl PC_AddGlobalDefine(const char *string)
{
  define_t *define;

  define = PC_DefineFromString(string);
  if ( !define )
    return 0;
  define->next = globaldefines;
  globaldefines = define;
  return 1;
}
//----- (1003B4E0) --------------------------------------------------------
/* Remove a #define from globaldefines by name; line-for-line Q3's
 * PC_RemoveGlobalDefine.  No reachable caller — only its own unused thunk. */
int __cdecl PC_RemoveGlobalDefine(const char *name)
{
  int define;

  /* PC_FindDefine still takes int; the cast documents the truncation. */
  define = PC_FindDefine((int)globaldefines, name);
  if ( define )
  {
    PC_FreeDefine((define_t *)define);
    return 1;
  }
  return 0;
}
//----- (1003B520) --------------------------------------------------------
// Drains the globaldefines list by repeatedly
// popping the head, advancing globaldefines to head->next (define_t
// offset +0x18 = +24, matching botlib_structs.h define_s::next), and
// calling PC_FreeDefine on the popped node.  This is the shutdown path
// for the preprocessor's global #define table; line-for-line Q3's
// PC_RemoveAllGlobalDefines.
void __cdecl PC_RemoveAllGlobalDefines(void)
{
  define_t *define;

  for ( define = globaldefines; define; define = globaldefines )
  {
    globaldefines = globaldefines->next;
    PC_FreeDefine(define);
  }
}
//----- (1003B560) --------------------------------------------------------
/* Deep-copy a define_t: one allocation of sizeof(define_t) + strlen(name) + 1,
 * with the name stored inline right after the struct. */
define_t *__cdecl PC_CopyDefine(define_t *define)
{
  define_t *def;
  token_t *tok;
  token_t *prev;
  token_t *src;

  def = (define_t *)GetMemory(sizeof(define_t) + strlen(define->name) + 1);
  def->name = (char *)def + sizeof(define_t);
  strcpy(def->name, define->name);
  def->flags    = define->flags;
  def->builtin  = define->builtin;
  def->numparms = define->numparms;
  def->next     = NULL;
  def->hashnext = NULL;

  /* Copy tokens list (define_t.tokens @+20). */
  def->tokens = NULL;
  prev = NULL;
  for ( src = define->tokens; src; src = src->next )
  {
    tok = PC_CopyToken(src);
    tok->next = NULL;
    if ( prev )
      prev->next = tok;
    else
      def->tokens = tok;
    prev = tok;
  }

  /* Copy parms list (define_t.parms @+16). */
  def->parms = NULL;
  prev = NULL;
  for ( src = define->parms; src; src = src->next )
  {
    tok = PC_CopyToken(src);
    tok->next = NULL;
    if ( prev )
      prev->next = tok;
    else
      def->parms = tok;
    prev = tok;
  }
  return def;
}
//----- (1003B680) --------------------------------------------------------
/* Copy every entry of `globaldefines` into the source's definehash table via
 * PC_CopyDefine + PC_AddDefineToHash. */
void __cdecl PC_AddGlobalDefinesToSource(source_t *source)
{
  define_t *def;
  define_t *copy;

  for ( def = globaldefines; def; def = def->next )
  {
    copy = PC_CopyDefine(def);
    PC_AddDefineToHash(copy, source->definehash);
  }
}
//----- (1003B6C0) --------------------------------------------------------
int __cdecl PC_Directive_if_def(source_t *src, int type)
{
  define_t *def;
  token_t token;
  int skip;

  if ( !PC_ReadLine(src, token.string) )
  {
    SourceError(src, "#ifdef without name");
    return 0;
  }
  if ( token.type != 4 )
  {
    PC_UnreadSourceToken(src, token.string);
    SourceError(src, "expected name after #ifdef, found %s", token.string);
    return 0;
  }
  def = PC_FindHashedDefine(src->definehash, token.string);
  skip = (type == 8) == (def == NULL);
  PC_PushIndent(src, type, skip);
  return 1;
}
//----- (1003B7F0) --------------------------------------------------------
int __cdecl PC_Directive_else(source_t *source)
{
  int type; // [esp+4h] [ebp-4h] BYREF
  int skip;

  PC_PopIndent(source, &type, &skip);
  if ( !type )
  {
    SourceError(source, "misplaced #else");
    return 0;
  }
  if ( type == 2 )
  {
    SourceError(source, "#else after #else");
    return 0;
  }
  PC_PushIndent(source, 2, skip == 0);
  return 1;
}
//----- (1003B880) --------------------------------------------------------
int __cdecl PC_Directive_endif(source_t *source)
{
  int type; // BYREF
  int skip;

  PC_PopIndent(source, &type, &skip);
  if ( !type )
  {
    SourceError(source, "misplaced #endif");
    return 0;
  }
  return 1;
}
//----- (1003B8D0) --------------------------------------------------------
int __cdecl PC_OperatorPriority(int op)
{
  int result; // eax

  switch ( op )
  {
    case 5:
      result = 7;
      break;
    case 6:
      result = 6;
      break;
    case 9:
    case 10:
      result = 11;
      break;
    case 7:
    case 8:
    case 37:
    case 38:
      result = 12;
      break;
    case 21:
    case 22:
      result = 13;
      break;
    case 26:
    case 27:
    case 28:
      result = 15;
      break;
    case 29:
    case 30:
      result = 14;
      break;
    case 32:
      result = 10;
      break;
    case 33:
      result = 8;
      break;
    case 34:
      result = 9;
      break;
    case 35:
    case 36:
      result = 16;
      break;
    case 42:
    case 43:
      result = 5;
      break;
    default:
      result = 0;
      break;
  }
  return result;
}
//----- (1003B9E0) --------------------------------------------------------
int __cdecl PC_EvaluateTokens(source_t *source, token_t *tokens, int *intvalue, double *floatvalue, int integer)
{
  int brace; // ebx
  int lastwasvalue; // ecx
  int v7; // edx
  value_t *lastvalue;
  token_t *t;
  int v10; // eax
  operator_t *v11;
  value_t *v12;
  value_t *v13;
  int gotquestmarkvalue; // ebp
  int questmarkintvalue; // ebx
  operator_t *v16;
  value_t *v;
  operator_t *o;
  operator_t *v19;
  int v20; // ecx
  int v21; // edx
  value_t *v2;
  double v24; // st7
  double v25; // st7
  BOOL v26; // eax
  double v27; // st7
  BOOL v28; // eax
  double v29; // st7
  double v30; // st7
  double v31; // st7
  double v32; // st7
  double v33; // st7
  double v34; // st7
  double v35; // st7
  int v36; // eax
  operator_t *v41;
  value_t *v42;
  value_t *v43;
  int error; // [esp+10h] [ebp-1Ch]
  int parentheses; // [esp+14h] [ebp-18h]
  operator_t *firstoperator;
  value_t *firstvalue;
  int negativevalue; // [esp+20h] [ebp-Ch]
  operator_t *lastoperator;
  double questmarkfloatvalue; // [esp+24h] [ebp-8h]
  int ArgLista; // [esp+34h] [ebp+8h]
  int ArgListb; // [esp+34h] [ebp+8h]
  int ArgListc; // [esp+34h] [ebp+8h]
  int ArgListd; // [esp+34h] [ebp+8h]
  int ArgListe; // [esp+34h] [ebp+8h]
  int ArgListf; // [esp+34h] [ebp+8h]
  int ArgListg; // [esp+34h] [ebp+8h]
  int ArgListh; // [esp+34h] [ebp+8h]

  brace = 0;
  lastwasvalue = 0;
  v7 = 0;
  lastvalue = 0;
  parentheses = 0;
  error = 0;
  negativevalue = 0;
  lastoperator = 0;
  firstoperator = 0;
  firstvalue = 0;
  if ( intvalue )
    *intvalue = 0;
  if ( floatvalue )
  {
    *floatvalue = 0;
  }
  t = tokens;
  if ( !tokens )
    goto LABEL_73;
  while ( 1 )
  {
    switch ( t->type )
    {
      case 4:
        if ( lastwasvalue || v7 )
        {
LABEL_71:
          SourceError(source, "syntax error in #if/#elif");
          goto LABEL_76;
        }
        if ( strcmp((const char *)t, "defined") )
        {
          SourceError(source, "undefined name %s in #if/#elif", t);
          goto LABEL_76;
        }
        t = t->next;
        if ( !strcmp((const char *)t, "(") )
        {
          t = t->next;
          brace = 1;
        }
        if ( !t || t->type != 4 )
        {
          SourceError(source, "defined without name in #if/#elif");
          goto LABEL_76;
        }
        v12 = GetClearedMemory(sizeof(value_t));
        if ( PC_FindHashedDefine(source->definehash, (const char *)t) )
        {
                    v12->intvalue = 1;
          v12->floatvalue = 1.0;
        }
        else
        {
                    v12->intvalue = 0;
          v12->floatvalue = 0.0;
        }
        v12->next = 0;
        v12->parentheses = parentheses;
        v12->prev = lastvalue;
        if ( lastvalue )
          lastvalue->next = v12;
        else
          firstvalue = v12;
        lastvalue = v12;
        if ( brace )
        {
          t = t->next;
          if ( !t || strcmp((const char *)t, ")") )
          {
            SourceError(source, "defined without ) in #if/#elif");
            goto LABEL_76;
          }
        }
        brace = 0;
        lastwasvalue = 1;
        break;
      case 3:
        if ( lastwasvalue )
        {
          SourceError(source, "syntax error in #if/#elif");
          goto LABEL_76;
        }
        v13 = GetClearedMemory(sizeof(value_t));
        if ( negativevalue )
        {
          v13->intvalue = -(int)t->intvalue;
          v13->floatvalue = -t->floatvalue;
        }
        else
        {
          v13->intvalue = (int)t->intvalue;
          v13->floatvalue = t->floatvalue;
        }
        v13->parentheses = parentheses;
        v13->next = 0;
        v13->prev = lastvalue;
        if ( lastvalue )
          lastvalue->next = v13;
        else
          firstvalue = v13;
        lastvalue = v13;
        lastwasvalue = 1;
        negativevalue = 0;
        break;
      case 5:
        if ( v7 )
        {
          SourceError(source, "misplaced minus sign in #if/#elif");
          goto LABEL_76;
        }
        v10 = t->subtype;
        if ( v10 == 44 )
        {
          ++parentheses;
        }
        else if ( v10 == 45 )
        {
          if ( --parentheses < 0 )
          {
            SourceError(source, "too many ) in #if/#elsif");
            goto LABEL_76;
          }
        }
        else
        {
          if ( !integer && (v10 == 35 || v10 == 28 || v10 == 21 || v10 == 22 || v10 == 32 || v10 == 33 || v10 == 34) )
          {
            SourceError(source,
                        "illigal operator %s on floating point operands\n",
                        t);
            goto LABEL_76;
          }
          switch ( v10 )
          {
            case 35:
            case 36:
              if ( lastwasvalue )
              {
                SourceError(source, "! or ~ after value in #if/#elif");
                goto LABEL_76;
              }
              goto LABEL_29;
            case 30:
              if ( !lastwasvalue )
                negativevalue = 1;
              goto LABEL_29;
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 21:
            case 22:
            case 26:
            case 27:
            case 28:
            case 29:
            case 32:
            case 33:
            case 34:
            case 37:
            case 38:
            case 42:
            case 43:
              if ( lastwasvalue )
                goto LABEL_29;
              SourceError(source, "operator %s after operator in #if/#elif", t);
              goto LABEL_76;
LABEL_29:
              if ( !negativevalue )
              {
                v11 = GetClearedMemory(sizeof(operator_t));
                v11->op = t->subtype;
                v11->priority = PC_OperatorPriority(t->subtype);
                v11->parentheses = parentheses;
                v11->next = 0;
                v11->prev = lastoperator;
                if ( lastoperator )
                  lastoperator->next = v11;
                else
                  firstoperator = v11;
                lastoperator = v11;
                lastwasvalue = 0;
              }
              break;
            default:
              SourceError(source, "invalid operator %s in #if/#elif", t);
              goto LABEL_76;
          }
        }
        break;
      default:
        SourceError(source, "unknown %s in #if/#elif", t);
        goto LABEL_76;
    }
    t = t->next;
    if ( !t )
      break;
    v7 = negativevalue;
  }
  /* Negative guard: the "trailing operator" arm is the warm fall-through and the
   * lastwasvalue arm the jump target. */
  if ( !lastwasvalue )
  {
LABEL_73:
    SourceError(source, "trailing operator in #if/#elif");
  }
  else
  {
    if ( !parentheses )
      goto LABEL_77;
    SourceError(source, "too many ( in #if/#elif");
  }
LABEL_76:
  error = 1;
LABEL_77:
  gotquestmarkvalue = 0;
  questmarkintvalue = 0;
  questmarkfloatvalue = 0.0;
  if ( !error )
  {
    while ( 1 )
    {
      v16 = firstoperator;
      if ( !firstoperator )
        goto LABEL_165;
      v = firstvalue;
      o = firstoperator;
      v19 = firstoperator->next;
      if ( v19 )
        break;
LABEL_88:
      v2 = v->next;
      switch ( o->op )
      {
        /* Case bodies in the ORIGINAL source order, matching Q3's PC_EvaluateTokens:
         * LOGIC_NOT, BIN_NOT, MUL, DIV, MOD, ADD, SUB, AND, OR, GEQ, LEQ, EQ,
         * UNEQ, GREATER, LESS, RSHIFT, LSHIFT, BIN_AND, BIN_OR, BIN_XOR, COLON,
         * QUESTIONMARK.  MSVC6 emits case bodies in source order, so sorting them
         * numerically relocates every block and both jump tables. */
        case 36:
          v->intvalue = !v->intvalue;
          v->floatvalue = !v->floatvalue;
          goto LABEL_144;
        case 35:
          v->intvalue = ~v->intvalue;
          goto LABEL_144;
        case 26:
          v->intvalue *= v2->intvalue;
          v->floatvalue = v2->floatvalue * v->floatvalue;
          goto LABEL_144;
        case 27:
          v24 = v->floatvalue;
          v->intvalue /= v2->intvalue;
          v->floatvalue = v24 / v2->floatvalue;
          goto LABEL_144;
        case 28:
          v->intvalue %= v2->intvalue;
          goto LABEL_144;
        case 29:
          v->intvalue += v2->intvalue;
          v->floatvalue = v2->floatvalue + v->floatvalue;
          goto LABEL_144;
        case 30:
          v25 = v->floatvalue;
          v->intvalue -= v2->intvalue;
          v->floatvalue = v25 - v2->floatvalue;
          goto LABEL_144;
        case 5:
          v26 = v->intvalue && v2->intvalue;
          v27 = v->floatvalue;
          v->intvalue = v26;
          if ( v27 == 0.0 || (ArgLista = 1, v2->floatvalue == 0.0) )
            ArgLista = 0;
          v->floatvalue = (float)ArgLista;
          goto LABEL_144;
        case 6:
          v28 = v->intvalue || v2->intvalue;
          v29 = v->floatvalue;
          v->intvalue = v28;
          if ( v29 != 0.0 || (ArgListb = 0, v2->floatvalue != 0.0) )
            ArgListb = 1;
          v->floatvalue = (float)ArgListb;
          goto LABEL_144;
        case 7:
          ArgListc = 1;
          v30 = v->floatvalue;
          v->intvalue = v->intvalue >= v2->intvalue;
          if ( v30 < v2->floatvalue )
            ArgListc = 0;
          v->floatvalue = (float)ArgListc;
          goto LABEL_144;
        case 8:
          ArgListd = 1;
          v31 = v->floatvalue;
          v->intvalue = v->intvalue <= v2->intvalue;
          if ( v31 > v2->floatvalue )
            ArgListd = 0;
          v->floatvalue = (float)ArgListd;
          goto LABEL_144;
        case 9:
          ArgListe = 1;
          v32 = v->floatvalue;
          v->intvalue = v->intvalue == v2->intvalue;
          if ( v32 != v2->floatvalue )
            ArgListe = 0;
          v->floatvalue = (float)ArgListe;
          goto LABEL_144;
        case 10:
          ArgListf = 1;
          v33 = v->floatvalue;
          v->intvalue = v->intvalue != v2->intvalue;
          if ( v33 == v2->floatvalue )
            ArgListf = 0;
          v->floatvalue = (float)ArgListf;
          goto LABEL_144;
        case 37:
          ArgListg = 1;
          v34 = v->floatvalue;
          v->intvalue = v->intvalue > v2->intvalue;
          if ( v34 <= v2->floatvalue )
            ArgListg = 0;
          v->floatvalue = (float)ArgListg;
          goto LABEL_144;
        case 38:
          ArgListh = 1;
          v35 = v->floatvalue;
          v->intvalue = v->intvalue < v2->intvalue;
          if ( v35 >= v2->floatvalue )
            ArgListh = 0;
          v->floatvalue = (float)ArgListh;
          goto LABEL_144;
        case 21:
          v->intvalue >>= v2->intvalue;
          goto LABEL_144;
        case 22:
          v->intvalue <<= v2->intvalue;
          goto LABEL_144;
        case 32:
          v->intvalue &= v2->intvalue;
          goto LABEL_144;
        case 33:
          v->intvalue |= v2->intvalue;
          goto LABEL_144;
        case 34:
          v->intvalue ^= v2->intvalue;
          goto LABEL_144;
        case 42:
          if ( !gotquestmarkvalue )
          {
            SourceError(source, ": without ? in #if/#elif");
            goto LABEL_163;
          }
          if ( integer )
          {
            if ( !questmarkintvalue )
            {
              gotquestmarkvalue = 0;
              v->intvalue = v2->intvalue;
              goto LABEL_144;
            }
          }
          else if ( questmarkfloatvalue == 0.0 )
          {
            v->floatvalue = v2->floatvalue;
          }
          gotquestmarkvalue = 0;
          goto LABEL_144;
        case 43:
          if ( gotquestmarkvalue )
          {
            SourceError(source, "? after ? in #if/#elif");
            goto LABEL_163;
          }
          questmarkintvalue = v->intvalue;
          questmarkfloatvalue = v->floatvalue;
          gotquestmarkvalue = 1;
LABEL_144:
          v36 = o->op;
          if ( o->op != 36 && v36 != 35 )
          {
            if ( v36 != 43 )
              v = v->next;
            if ( v->prev )
              v->prev->next = v->next;
            else
              firstvalue = v->next;
            if ( v->next )
              v->next->prev = v->prev;
            FreeMemory(v);
          }
          if ( o->prev )
            o->prev->next = o->next;
          else
            firstoperator = o->next;
          if ( o->next )
            o->next->prev = o->prev;
          FreeMemory(o);
          break;
        default:
          goto LABEL_144;
      }
    }
    while ( 1 )
    {
      v20 = o->parentheses;
      v21 = v19->parentheses;
      if ( v20 > v21 || v20 == v21 && o->priority >= v19->priority )
        goto LABEL_88;
      if ( o->op != 36 && o->op != 35 )
        v = v->next;
      if ( !v )
        break;
      o = v19;
      v19 = v19->next;
      if ( !v19 )
        goto LABEL_88;
    }
    SourceError(source, "mising values in #if/#elif");
LABEL_163:
    error = 1;
  }
  v16 = firstoperator;
LABEL_165:
  if ( firstvalue )
  {
    if ( intvalue )
      *intvalue = firstvalue->intvalue;
    if ( floatvalue )
    {
      *floatvalue = firstvalue->floatvalue;
    }
  }
  if ( v16 )
  {
    do
    {
      v41 = v16->next;
      FreeMemory(v16);
      v16 = v41;
    }
    while ( v41 );
  }
  v42 = firstvalue;
  if ( firstvalue )
  {
    do
    {
      v43 = v42->next;
      FreeMemory(v42);
      v42 = v43;
    }
    while ( v43 );
  }
  if ( !error )
    return 1;
  if ( intvalue )
    *intvalue = 0;
  if ( floatvalue )
    *floatvalue = 0;
  return 0;
}
//----- (1003C650) --------------------------------------------------------
int __cdecl PC_Evaluate(source_t *source, int *intvalue, double *floatvalue, int integer)
{
  token_t *firsttoken;
  token_t *lasttoken;
  token_t *t;
  token_t *v9;
  define_t *define;
  token_t *v11;
  token_t *nexttoken;
  int defined;
  token_t token;

  defined = 0;
  if ( intvalue )
    *intvalue = 0;
  if ( floatvalue )
  {
    *floatvalue = 0;
  }
  if ( !PC_ReadLine(source, &token) )
  {
    SourceError(source, "no value after #if/#elif");
    return 0;
  }
  firsttoken = NULL;
  lasttoken = NULL;
  do
  {
    if ( token.type == 4 )
    {
      if ( defined )
      {
        defined = 0;
        t = PC_CopyToken(&token);
        t->next = NULL;
        if ( lasttoken )
          lasttoken->next = t;
        else
          firsttoken = t;
        lasttoken = t;
      }
      else if ( !strcmp(token.string, "defined") )
      {
        defined = 1;
        v9 = PC_CopyToken(&token);
        v9->next = NULL;
        if ( lasttoken )
          lasttoken->next = v9;
        else
          firsttoken = v9;
        lasttoken = v9;
      }
      else
      {
        define = PC_FindHashedDefine(source->definehash, token.string);
        if ( !define )
        {
          SourceError(source, "can't evaluate %s, not defined", token.string);
          return 0;
        }
        if ( !PC_ExpandDefineIntoSource(source, define) )
          return 0;
      }
    }
    else if ( token.type == 3 || token.type == 5 )
    {
      t = PC_CopyToken(&token);
      t->next = NULL;
      if ( lasttoken )
        lasttoken->next = t;
      else
        firsttoken = t;
      lasttoken = t;
    }
    else
    {
      SourceError(source, "can't evaluate %s", token.string);
      return 0;
    }
  }
  while ( PC_ReadLine(source, &token) );
  if ( !PC_EvaluateTokens(source, (intptr_t)firsttoken, (_DWORD *)intvalue, (_DWORD *)floatvalue, integer) )
    return 0;
  v11 = firsttoken;
  if ( firsttoken )
  {
    do
    {
      nexttoken = v11->next;
      PC_FreeToken(v11);
      v11 = nexttoken;
    }
    while ( nexttoken );
  }
  return 1;
}
//----- (1003C900) --------------------------------------------------------
int __cdecl PC_DollarEvaluate(source_t *source, int *intvalue, double *floatvalue, int integer)
{
  /* `indent`/`defined` declared (and later assigned) ahead of the token
   * pointers, matching Q3A's `int indent, defined = qfalse;` declared before
   * `firsttoken`/`lasttoken` (Quake-III-Arena l_precomp.c PC_DollarEvaluate).
   * gcc 2.7.2.3 -O6 assigns stack slots by DECLARATION order, not assignment
   * order -- reordering only the `indent = 1;`/`firsttoken = 0;` statements
   * without moving the declarations has no effect on the emitted slot
   * offsets. */
  int indent; // [esp+10h] [ebp-438h]
  int defined; // [esp+14h] [ebp-434h]
  token_t *firsttoken; // ebp (firsttoken)
  token_t *lasttoken; // edi
  token_t *t; // eax
  token_t *v9; // eax
  define_t *define; // eax
  token_t *v12; // eax
  token_t *nexttoken; // esi
  token_t token;

  defined = 0;
  if ( intvalue )
    *intvalue = 0;
  if ( floatvalue )
  {
    *floatvalue = 0;
  }
  if ( !PC_ReadSourceToken(source, &token) )
  {
    SourceError(source, "no leading ( after $evalint/$evalfloat");
    return 0;
  }
  if ( !PC_ReadSourceToken(source, &token) )
  {
    SourceError(source, "nothing to evaluate");
    return 0;
  }
  indent = 1;
  firsttoken = 0;
  lasttoken = 0;
  do
  {
    if ( token.type == 4 )
    {
      if ( defined )
      {
        defined = 0;
        t = PC_CopyToken(&token);
        t->next = 0;
        if ( lasttoken )
          lasttoken->next = t;
        else
          firsttoken = t;
        lasttoken = t;
      }
      else if ( !strcmp(token.string, "defined") )
      {
        defined = 1;
        v9 = PC_CopyToken(&token);
        v9->next = 0;
        if ( lasttoken )
          lasttoken->next = v9;
        else
          firsttoken = v9;
        lasttoken = v9;
      }
      else
      {
        define = PC_FindHashedDefine(source->definehash, token.string);
        if ( !define )
        {
          SourceError(source, "can't evaluate %s, not defined", token.string);
          return 0;
        }
        if ( !PC_ExpandDefineIntoSource(source, define) )
          return 0;
      }
    }
    else if ( token.type == 3 || token.type == 5 )
    {
      if ( token.string[0] == 40 )
        ++indent;
      else if ( token.string[0] == 41 )
        --indent;
      if ( indent <= 0 )
        break;
      t = PC_CopyToken(&token);
      t->next = 0;
      if ( lasttoken )
        lasttoken->next = t;
      else
        firsttoken = t;
      lasttoken = t;
    }
    else
    {
      SourceError(source, "can't evaluate %s", token.string);
      return 0;
    }
  }
  while ( PC_ReadSourceToken(source, &token) );
  if ( !PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer) )
    return 0;
  v12 = firsttoken;
  if ( firsttoken )
  {
    do
    {
      nexttoken = v12->next;
      PC_FreeToken(v12);
      v12 = nexttoken;
    }
    while ( nexttoken );
  }
  return 1;
}
//----- (1003CC10) --------------------------------------------------------
int __cdecl PC_Directive_elif(source_t *source)
{
  int value; // [esp+4h] [ebp-8h] BYREF
  int type; // [esp+8h] [ebp-4h] BYREF
  int skip;

  PC_PopIndent(source, &type, &skip);
  if ( !type || type == 2 )
  {
    SourceError(source, "misplaced #elif");
    return 0;
  }
  if ( !PC_Evaluate(source, &value, 0, 1) )
    return 0;
  skip = value == 0;
  PC_PushIndent(source, 4, skip);
  return 1;
}
//----- (1003CCB0) --------------------------------------------------------
int __cdecl PC_Directive_if(source_t *source)
{
  int value;

  if ( !PC_Evaluate(source, &value, 0, 1) )
    return 0;
  PC_PushIndent(source, 1, value == 0);
  return 1;
}
//----- (1003CD00) --------------------------------------------------------
/* As Q3's PC_Directive_line: report "#line directive not supported" through the
 * source error reporter and return 0. */
int __cdecl PC_Directive_line(source_t *source)
{
  SourceError(source, "#line directive not supported");
  return 0;
}
//----- (1003CD30) --------------------------------------------------------
int __cdecl PC_Directive_error(source_t *source)
{
  token_t token; // [esp+4h] [ebp-430h] BYREF

  strcpy(token.string, "");
  PC_ReadSourceToken(source, token.string);
  SourceError(source, "#error directive: %s", token.string);
  return 0;
}
//----- (1003CD80) --------------------------------------------------------
int __cdecl PC_Directive_pragma(source_t *source)
{
  /* A real token_t, not a fixed 1072-byte buffer: PC_ReadLine writes a full
   * token_t, which is 1088 bytes once the pointer fields widen. */
  token_t v3; // [esp+0h] [ebp-430h] BYREF

  SourceWarning(source, "#pragma directive not supported");
  while ( PC_ReadLine(source, &v3) )
    ;
  return 1;
}
//----- (1003CDF0) --------------------------------------------------------
void __cdecl UnreadSignToken(source_t *source)
{
  token_t token;

  token.line = source->scriptstack->line;
  token.whitespace_p = source->scriptstack->script_p;
  token.endwhitespace_p = source->scriptstack->script_p;
  token.linescrossed = 0;
  strcpy(token.string, "-");
  token.type = 5;
  token.subtype = 30;
  PC_UnreadSourceToken(source, &token);
}
//----- (1003CE90) --------------------------------------------------------
int __cdecl PC_Directive_eval(source_t *source)
{
  int result; // eax
  int value; // [esp+4h] [ebp-434h] BYREF
  token_t token;

  result = PC_Evaluate(source, &value, 0, 1);
  if ( !result )
    return result;
  /* scriptstack re-read per statement, as Q3 writes it -- caching it in a local
   * costs one register and shortens all three loads. */
  token.line = source->scriptstack->line;
  token.whitespace_p = source->scriptstack->script_p;
  token.endwhitespace_p = source->scriptstack->script_p;
  token.linescrossed = 0;
  sprintf(token.string, "%d", abs(value));
  token.type = 3;
  token.subtype = 12296;
  PC_UnreadSourceToken(source, &token);
  if ( value < 0 )
    UnreadSignToken(source);
  return 1;
}
//----- (1003CF80) --------------------------------------------------------
int __cdecl PC_Directive_evalfloat(source_t *source)
{
  int result; // eax
  double value; // [esp+Ch] [ebp-438h] BYREF
  token_t token;

  result = PC_Evaluate(source, 0, &value, 0);
  if ( !result )
    return result;
  token.line = source->scriptstack->line;
  token.whitespace_p = source->scriptstack->script_p;
  token.endwhitespace_p = source->scriptstack->script_p;
  token.linescrossed = 0;
  sprintf(token.string, "%1.2f", fabs(value));
  token.type = 3;
  token.subtype = 10248;
  PC_UnreadSourceToken(source, &token);
  if ( value < 0.0 )
    UnreadSignToken(source);
  return 1;
}
//----- (1003D090) --------------------------------------------------------
int __cdecl PC_ReadDirective(source_t *source)
{
  /* One token_t allocated as a single block, as in the original. */
  token_t token;
  int i;

  if ( !PC_ReadSourceToken(source, token.string) )
  {
    SourceError(source, "found # without name");
    return 0;
  }
  if ( token.linescrossed > 0 )
  {
    PC_UnreadSourceToken(source, token.string);
    SourceError(source, "found # at end of line");
    return 0;
  }
  if ( token.type == 4 )
  {
    /* Indexed scan, as Q3 writes it: the index is reused at the call site
     * (`directives[i].func`), so MSVC keeps both the strength-reduced name
     * pointer and the counter. */
    for ( i = 0; directives[i].name; i++ )
    {
      if ( !strcmp(directives[i].name, token.string) )
        return directives[i].handler(source);
    }
  }
  SourceError(source, "unknown precompiler directive %s", token.string);
  return 0;
}
//----- (1003D1D0) --------------------------------------------------------
int __cdecl PC_DollarDirective_evalint(source_t *source)
{
  int result; // eax
  int value; // [esp+4h] [ebp-434h] BYREF
  token_t token;

  result = PC_DollarEvaluate(source, &value, 0, 1);
  if ( !result )
    return result;
  token.line = source->scriptstack->line;
  token.whitespace_p = source->scriptstack->script_p;
  token.endwhitespace_p = source->scriptstack->script_p;
  token.linescrossed = 0;
  sprintf(token.string, "%d", abs(value));
  token.type = 3;
  token.subtype = 12296;
  token.intvalue = value;
  token.floatvalue = (float)value;
  PC_UnreadSourceToken(source, token.string);
  if ( value < 0 )
    UnreadSignToken(source);
  return 1;
}
//----- (1003D2F0) --------------------------------------------------------
int __cdecl PC_DollarDirective_evalfloat(source_t *source)
{
  int result; // eax
  double value; // [esp+Ch] [ebp-438h] BYREF
  token_t token;

  result = PC_DollarEvaluate(source, 0, &value, 0);
  if ( !result )
    return result;
  token.line = source->scriptstack->line;
  token.whitespace_p = source->scriptstack->script_p;
  token.endwhitespace_p = source->scriptstack->script_p;
  token.linescrossed = 0;
  sprintf(token.string, "%1.2f", fabs(value));
  token.type = 3;
  token.subtype = 10248;
  token.intvalue = (__int64)value;
  token.floatvalue = value;
  PC_UnreadSourceToken(source, token.string);
  if ( value < 0.0 )
    UnreadSignToken(source);
  return 1;
}
//----- (1003D420) --------------------------------------------------------
int __cdecl PC_ReadDollarDirective(source_t *source)
{
  int i;
  token_t token;

  if ( !PC_ReadSourceToken(source, token.string) )
  {
    SourceError(source, "found $ without name");
    return 0;
  }
  if ( token.linescrossed > 0 )
  {
    PC_UnreadSourceToken(source, token.string);
    SourceError(source, "found $ at end of line");
    return 0;
  }
  if ( token.type == 4 )
  {
    for ( i = 0; dollardirectives[i].name; i++ )
    {
      if ( !strcmp(dollardirectives[i].name, token.string) )
        return dollardirectives[i].handler(source);
    }
  }
  PC_UnreadSourceToken(source, token.string);
  SourceError(source, "unknown precompiler directive %s", token.string);
  return 0;
}
//----- (1003D580) --------------------------------------------------------
/* Uses the source_t->skip and ->definehash fields rather than the original's
 * width-dependent indexing. */
int __cdecl PC_ReadTokenHandle(source_t *source, _DWORD *pc_token)
{
  define_t *v3;

  while ( 1 )
  {
    if ( !PC_ReadSourceToken(source, (token_t *)pc_token) )
      return 0;
    if ( ((token_t *)pc_token)->type == 5 && ((token_t *)pc_token)->string[0] == '#' )
    {
      if ( !PC_ReadDirective(source) )
        return 0;
      continue;
    }
    if ( ((token_t *)pc_token)->type == 5 && ((token_t *)pc_token)->string[0] == '$' )
    {
      if ( !PC_ReadDollarDirective(source) )
        return 0;
      continue;
    }
    if ( source->skip )
      continue;
    if ( ((token_t *)pc_token)->type == 4 )
    {
      v3 = PC_FindHashedDefine(source->definehash, (const char *)pc_token);
      if ( v3 )
      {
        if ( !PC_ExpandDefineIntoSource(source, v3) )
          return 0;
        continue;
      }
    }
    memcpy(&source->cachedtoken, pc_token, sizeof(token_t));
    return 1;
  }
}
// 1003D5BA: conditional instruction was optimized away because eax.4==5
//----- (1003D650) --------------------------------------------------------
int __cdecl PC_ExpectTokenString(source_t *source, const char *string)
{
  char token[sizeof(token_t)] __attribute__((aligned(8))); // [esp+8h] [ebp-430h] BYREF

  if ( !PC_ReadTokenHandle(source, token) )
  {
    SourceError(source, "couldn't find expected %s", string);
    return 0;
  }
  if ( strcmp(token, string) )
  {
    SourceError(source, "expected %s, found %s", string, token);
    return 0;
  }
  return 1;
}
//----- (1003D740) --------------------------------------------------------
int __cdecl PC_ExpectTokenType(source_t *source, int type, int subtype, intptr_t token)
{
  char str[1024]; // [esp+8h] [ebp-400h] BYREF
  token_t *tok = (token_t *)token;

  if ( !PC_ReadTokenHandle(source, token) )
  {
    SourceError(source, "couldn't read expected token");
    return 0;
  }
  if ( tok->type != type )
  {
    if ( type == 1 )
      strcpy(str, "string");
    if ( type == 2 )
      strcpy(str, "literal");
    if ( type == 3 )
      strcpy(str, "number");
    if ( type == 4 )
      strcpy(str, "name");
    if ( type == 5 )
      strcpy(str, "punctuation");
    SourceError(source, "expected a %s, found %s", str, token);
    return 0;
  }
  if ( tok->type == 3 )
  {
    if ( (tok->subtype & subtype) != subtype )
    {
      if ( (subtype & 8) != 0 )
        strcpy(str, "decimal");
      if ( (subtype & 0x100) != 0 )
        strcpy(str, "hex");
      if ( (subtype & 0x200) != 0 )
        strcpy(str, "octal");
      if ( (subtype & 0x400) != 0 )
        strcpy(str, "binary");
      if ( (subtype & 0x2000) != 0 )
      {
        strcat(str, " long");
      }
      if ( (subtype & 0x4000) != 0 )
      {
        strcat(str, " unsigned");
      }
      if ( (subtype & 0x800) != 0 )
      {
        strcat(str, " float");
      }
      if ( (subtype & 0x1000) != 0 )
        strcat(str, " integer");
      SourceError(source, "expected %s, found %s", str, token);
      return 0;
    }
  }
  else if ( tok->type == 5 )
  {
    if ( tok->subtype != subtype )
    {
      SourceError(source, "found %s", token);
      return 0;
    }
  }
  return 1;
}
//----- (1003DAE0) --------------------------------------------------------
int __cdecl PC_ExpectAnyToken(source_t *source, intptr_t token)
{
  if ( !PC_ReadTokenHandle(source, token) )
  {
    SourceError(source, "couldn't read expected token");
    return 0;
  }
  return 1;
}
//----- (1003DB20) --------------------------------------------------------
int __cdecl PC_CheckTokenString(source_t *source, const char *string)
{
  char tok[sizeof(token_t)] __attribute__((aligned(8))); // [esp+4h] [ebp-430h] BYREF

  if ( !PC_ReadTokenHandle(source, tok) )
    return 0;
  if ( !strcmp(tok, string) )
    return 1;
  PC_UnreadSourceToken(source, tok);
  return 0;
}
//----- (1003DBE0) --------------------------------------------------------
/* Read one token and keep it only if its type matches and every bit of
 * `subtype` is set in the token's subtype; otherwise unread it and fail.
 * DEAD in Gladiator. */
int __cdecl PC_CheckTokenType(source_t *source, int type, int subtype, token_t *token)
{
  token_t Buffer __attribute__((aligned(8))); // [esp+0h] [ebp-430h] BYREF

  if ( !PC_ReadTokenHandle(source, &Buffer) )
    return 0;
  if ( Buffer.type == type && (Buffer.subtype & subtype) == subtype )
  {
    memcpy(token, &Buffer, sizeof(token_t));
    return 1;
  }
  PC_UnreadSourceToken(source, &Buffer);
  return 0;
}
//----- (1003DC80) --------------------------------------------------------
/* Read tokens until one equals `string` (1) or the stream ends (0).
 * DEAD in Gladiator. */
int __cdecl PC_SkipUntilString(source_t *source, char *string)
{
  token_t Buffer __attribute__((aligned(8))); // [esp+10h] [ebp-430h] BYREF

  while ( PC_ReadTokenHandle(source, &Buffer) )
  {
    if ( !strcmp(Buffer.string, string) )
      return 1;
  }
  return 0;
}
//----- (1003DD40) --------------------------------------------------------
void __cdecl PC_UnreadLastToken(source_t *source)
{
  PC_UnreadSourceToken(source, &source->cachedtoken);
}
//----- (1003DD70) --------------------------------------------------------
/* Pass-through wrapper around PC_UnreadSourceToken — the external
 * PC_UnreadToken entry point paralleling PC_UnreadLastToken.
 * DEAD in Gladiator. */
void __cdecl PC_UnreadToken(source_t *source, token_t *token)
{
  PC_UnreadSourceToken(source, token);
}
//----- (1003DDA0) --------------------------------------------------------
/* Copy `path` into source->includepath and ensure it ends with a separator.
 * The fixed 0x104-byte memcpy (rather than strncpy) is verbatim from the
 * original — it reads past the source string's NUL, but the function never
 * runs.  DEAD in Gladiator. */
void __cdecl PC_SetIncludePath(source_t *source, char *path)
{
  strncpy(source->includepath, path, MAX_PATH);
  if ( source->includepath[strlen(source->includepath) - 1] != '\\'
    && source->includepath[strlen(source->includepath) - 1] != '/' )
  {
    strcat(source->includepath, "\\");
  }
}
//----- (1003DE40) --------------------------------------------------------
/* Q3's PC_SetPunctuations verbatim.  The write lands at +0x208 in the DLL,
 * which is `punctuations` once includepath is the full MAX_PATH buffer.
 * DEAD in Gladiator. */
void __cdecl PC_SetPunctuations(source_t *source, punctuation_t *p)
{
  source->punctuations = p;
}
//----- (1003DE60) --------------------------------------------------------
source_t *__cdecl LoadSourceFile(char *Source, int Offset, size_t ElementSize)
{
  script_t *script;
  source_t *src;

  script = LoadScriptFile(Source, Offset, ElementSize);
  if ( !script )
    return NULL;
  script->next = NULL;
  src = (source_t *)GetMemory(sizeof(source_t));
  memset(src, 0, sizeof(source_t));
  strncpy(src->filename, Source, MAX_PATH);
  src->scriptstack  = script;
  src->tokens       = NULL;
  src->defines      = NULL;
  src->indentstack  = NULL;
  src->skip         = 0;
  src->definehash   = (define_t **)GetClearedMemory(1024 * sizeof(define_t *));
  PC_AddGlobalDefinesToSource(src);
  return src;
}
//----- (1003DF30) --------------------------------------------------------
/* The memory-buffer twin of LoadSourceFile above: same structure, but wraps an
 * already-in-memory script buffer via LoadScriptMemory.
 * DEAD in Gladiator. */
source_t *__cdecl LoadSourceMemory(char *ptr, int length, char *name)
{
  script_t *script;
  source_t *src;

  script = LoadScriptMemory(ptr, length, name);
  if ( !script )
    return NULL;
  script->next = NULL;
  src = (source_t *)GetMemory(sizeof(source_t));
  memset(src, 0, sizeof(source_t));
  strncpy(src->filename, name, MAX_PATH);
  src->scriptstack  = script;
  src->tokens       = NULL;
  src->defines      = NULL;
  src->indentstack  = NULL;
  src->skip         = 0;
  src->definehash   = (define_t **)GetClearedMemory(1024 * sizeof(define_t *));
  PC_AddGlobalDefinesToSource(src);
  return src;
}
//----- (1003E000) --------------------------------------------------------
void __cdecl FreeSource(source_t *source)
{
  script_t *s;
  struct token_s *tok;
  define_t *d;
  indent_t *ind;
  int k;

  while ( source->scriptstack )
  {
    s = source->scriptstack;
    source->scriptstack = s->next;
    FreeScript(s);
  }
  while ( source->tokens )
  {
    tok = source->tokens;
    source->tokens = tok->next;
    PC_FreeToken(tok);
  }
  for ( k = 0; k < 1024; k++ )
  {
    while ( source->definehash[k] )
    {
      d = source->definehash[k];
      source->definehash[k] = d->hashnext;
      PC_FreeDefine(d);
    }
  }
  while ( source->indentstack )
  {
    ind = source->indentstack;
    source->indentstack = ind->next;
    FreeMemory(ind);
  }
  if ( source->definehash )
    FreeMemory(source->definehash);
  FreeMemory(source);
}
