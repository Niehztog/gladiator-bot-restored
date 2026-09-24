/*
 * l_precomp.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10039200..0x1003E000.
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
#include "l_precomp.h"
#include "be_ai_weight.h"
#include "be_interface.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_utils.h"

define_t *globaldefines;

/* Preprocessor directive table at VA 0x1005F260 — a {char*, int(*)(int)} array in
 * .data.  #ifdef/#ifndef are 1-arg wrappers over PC_Directive_if_def(src,
 * INDENT_IFDEF/INDENT_IFNDEF); Q3 has exactly this trio and the same INDENT_*
 * values, and gladi386.so exports all three, so the wrappers are the original's own
 * functions.  `directive_t` / `directives` / `dollardirectives` are the original
 * names, from gladi386.so's .dynsym (both tables 160 B = Q3's `directive_t
 * directives[20]`).
 *
 * Deliberately NOT `static`: .dynsym lists both tables as exported `D` symbols,
 * which a file-static array can never be.  That also drives PC_ReadDirective's
 * codegen — gcc -fPIC addresses a non-static (potentially-interposable) global
 * through a real GOT pointer slot, where a `static` array gets a direct GOT-relative
 * address with no extra indirection. */
typedef struct { const char *name; int (__cdecl *handler)(intptr_t); } directive_t;
/* [20], not []: gladi386.so's .dynsym gives `directives` st_size 160 = 20*8,
 * and Q3 `l_precomp.c:2490` declares exactly `directive_t directives[20]` with
 * only 15 initialisers.  Found by dataaudit.py 2026-08-16 (we had 120 B). */
directive_t directives[20] = {
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
/* IDA named the table's first two FIELDS as if they were separate objects
 * (`off_1005F260` = directives[0].name, `off_1005F264` = directives[0].handler).
 * They are not: gladi386.so has no such symbols, and nothing here referenced
 * the aliases once `directives` itself was recovered.  Removed 2026-08-17. */
/* $-directive dispatch table at VA 0x1005F300: 2 entries + NULL, walked by
 * PC_ReadDollarDirective as a stride-2 pointer array.  Same directive_t element type
 * as `directives` above. */
/* [20] for the same reason as `directives` above — st_size 160 = 20*8, and
 * Q3 `l_precomp.c:2603` is `directive_t dollardirectives[20]` with three
 * initialisers.  We had 24 B. */
directive_t dollardirectives[20] = {
    {"evalint",   PC_DollarDirective_evalint},   /* 0x100011D6 thunk → PC_DollarDirective_evalint */
    {"evalfloat", PC_DollarDirective_evalfloat},        /* 0x10001B0E thunk → PC_DollarDirective_evalfloat     */
    {NULL, NULL}
};
/* Same IDA field-as-object artifact as off_1005F260/64 above; removed. */

// gladiator.dll: 10039200..1003924B
// gladi386.so:   0004ADDC..0004AE42
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

// gladiator.dll: 10039270..100392BB
// gladi386.so:   0004AE44..0004AEAA
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

// gladiator.dll: 100392E0..1003932B
// gladi386.so:   0004AEAC..0004AF06
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

// gladiator.dll: 10039350..100393B8
// gladi386.so:   0004AF08..0004AF76
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

// gladiator.dll: 100393E0..10039436
// gladi386.so:   0004AF78..0004AFE1
void __cdecl PC_PushScript(source_t *source, script_t *script)
{
  script_t *s;

  for ( s = source->scriptstack; s; s = s->next )
  {
    /* `stricmp`, NOT `Q_stricmp`: the .so calls `strcasecmp` here and the DLL
     * calls the MSVC CRT `_stricmp` (0x10045cb0, reached directly at
     * 0x100393f7 -- not through an /INCREMENTAL thunk, so it is CRT, not bot
     * code).  Both 1999 makefiles carry `BASE_CFLAGS=-Dstricmp=strcasecmp`,
     * which only makes sense if the source spells it `stricmp`.  The other
     * five botlib call sites really are `Q_stricmp` -- contentsweep confirms
     * their rows' tokens match.  (contentseq, 2026-08-16) */
    if ( !stricmp(s->filename, script->filename) )
    {
      SourceError(source, "%s recursively included", script->filename);
      return;
    }
  }
  script->next = source->scriptstack;
  source->scriptstack = script;
}

// gladiator.dll: 10039460..10039489
// gladi386.so:   0004AFE4..0004B01C
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

// gladiator.dll: 100394A0..100394AC
// gladi386.so:   0004B01C..0004B038
void __cdecl PC_FreeToken(token_t *token)
{
  FreeMemory(token);
}

// gladiator.dll: 100394C0..100395AE
// gladi386.so:   0004B038..0004B18C
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

// gladiator.dll: 100395F0..10039619
// gladi386.so:   0004B18C..0004B1E1
int __cdecl PC_UnreadSourceToken(source_t *source, const void *token)
{
  struct token_s *copy;

  copy = (struct token_s *)PC_CopyToken(token);  /* allocate 1072-byte copy */
  copy->next = source->tokens;
  source->tokens = copy;
  return 1;
}

// gladiator.dll: 10039630..10039990
// gladi386.so:   0004B1E4..0004B524
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

// gladiator.dll: 10039A70..10039B1E
// gladi386.so:   0004B524..0004B5EF
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

// gladiator.dll: 10039B50..10039BF9
// gladi386.so:   0004B5F0..0004B66A
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

// gladiator.dll: 10039C30..10039C8F
// gladi386.so:   0004B66C..0004B6E6
unsigned int __cdecl PC_NameHash(const char *name)
{
  unsigned int v2; // ecx
  int v4 = 0; // [esp+4h] [ebp-4h] BYREF

  /* A redundant `v4 = 0` on the len == 0 arm, and the abs() INSIDE the
   * `if (name)` block.  Both 1999 images skip the abs on the NULL-name path --
   * the DLL's `je` lands on the trailing `and eax,0x3ff`, the .so's on the
   * final reload -- so it was never the function's last statement.  Written
   * there, the one source gives each compiler its own original: cl.exe /O2
   * expands abs() to `cdq; xor eax,edx; sub eax,edx` and duplicates it into
   * both length arms, while gcc 2.7 expands it to `test; jge; neg` and, v4
   * being address-taken, stores the result back and re-reads it for the mask.
   * (The previous `if (v4 < 0) v4 = -v4;` after the block matched the .so
   * only; `abs()` after the block matched neither.)  gcc inlines PC_NameHash
   * into seven callers, so its shape decides those rows too. */
  if ( name )
  {
    v2 = strlen(name);
    if ( (int)v2 > 4 )
      v2 = 4;
    if ( v2 )
      memcpy(&v4, name, v2);
    else
      v4 = 0;
    v4 = abs(v4);
  }
  return v4 & 0x3FF;
}

// gladiator.dll: 10039CB0..10039CCF
// gladi386.so:   0004B6E8..0004B775
/* Prepend `define` to its bucket in the source's definehash table, keyed by
 * PC_NameHash of the define's name. */
void __cdecl PC_AddDefineToHash(define_t *define, define_t **definehash)
{
  unsigned int hash;

  hash = PC_NameHash(define->name);
  define->hashnext = definehash[hash];
  definehash[hash] = define;
}

// gladiator.dll: 10039CE0..10039D46
// gladi386.so:   0004B778..0004B829
/* Hash-bucket lookup of a define_t by name: `definehash` is the bucket array
 * indexed by PC_NameHash, each bucket chained via define_t.hashnext. */
define_t *__cdecl PC_FindHashedDefine(define_t **definehash, const char *name)
{
  define_t *d;
  int hash;

  hash = PC_NameHash(name);
  for ( d = definehash[hash]; d; d = d->hashnext )
  {
    if ( !strcmp(d->name, name) )
      return d;
  }
  return NULL;
}

// gladiator.dll: 10039D70..10039DCC
// gladi386.so:   0004B82C..0004B86D
/* Linear search of a define_t list.  The only caller is
 * PC_RemoveGlobalDefine, walking globaldefines. */
define_t *__cdecl PC_FindDefine(define_t *defines, const char *name)
{
  define_t *v2;

  for ( v2 = defines; v2; v2 = v2->next )
  {
    if ( !strcmp(v2->name, name) )
      return v2;
  }
  return 0;
}

// gladiator.dll: 10039DF0..10039E50
// gladi386.so:   0004B870..0004B8C1
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

// gladiator.dll: 10039E70..10039EBA
// gladi386.so:   0004B8C4..0004B91E
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

// gladiator.dll: 10039EE0..10039FB4
// gladi386.so:   0004B920..0004BA58
/* Q3's PC_AddBuiltinDefines — walk a local {name, value} table of __LINE__, __FILE__,
 * __DATE__, __TIME__ and add each to source->definehash as a built-in define, with
 * the name stored inline after the struct.  DEAD in Gladiator. */
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

// gladiator.dll: 1003A000..1003A240
// gladi386.so:   0004BA58..0004BCB2
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

// gladiator.dll: 1003A2D0..1003A5C4
// gladi386.so:   0004BCB4..0004C23E
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

// gladiator.dll: 1003A690..1003A6E2
// gladi386.so:   0004C240..0004C2B4
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

// gladiator.dll: 1003A710..1003A77E
// gladi386.so:   0004C2B4..0004C31C
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
      /* Two-week source drift (see AAS_Trace in be_aas_bspq2.c): the Windows DLL
       * normalizes to backslash, but gladi386.so writes forward slash here — Quake's
       * own file/VFS layer is always '/'-separated regardless of host OS. */
#ifdef _WIN32
      *ptr = '\\';
#else
      *ptr = '/';
#endif
    ptr++;
  }
}

// gladiator.dll: 1003A7A0..1003AA5D
// gladi386.so:   0004C31C..0004C77E
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
    if ( FindQuakeFile(path, &file) )
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

// gladiator.dll: 1003AB10..1003AB9D
// gladi386.so:   0004C780..0004C82C
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

// gladiator.dll: 1003ABD0..1003ABEA
// gladi386.so:   0004C82C..0004C849
/* Returns (token->endwhitespace_p > token->whitespace_p). */
BOOL __cdecl PC_WhiteSpaceBeforeToken(token_t *token)
{
  int diff;

  diff = token->endwhitespace_p - token->whitespace_p;
  return diff > 0;
}

// gladiator.dll: 1003AC00..1003AC19
// gladi386.so:   0004C84C..0004C86F
/* Zeroes whitespace_p, endwhitespace_p and linescrossed. */
token_t *__cdecl PC_ClearTokenWhiteSpace(token_t *token)
{
  token->whitespace_p = NULL;
  token->endwhitespace_p = NULL;
  token->linescrossed = 0;
  return token;
}

// gladiator.dll: 1003AC30..1003AD7D
// gladi386.so:   0004C870..0004CB2D
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

// gladiator.dll: 1003ADE0..1003B20A
// gladi386.so:   0004CB30..0004D4DC
/* Struct-field access throughout rather than the original's literal offsets, so this
 * stays correct as define_t grows 32 -> 56 bytes and token_t 1072 -> 1088 on 64-bit.
 * Variable names follow Q3's PC_Directive_define. */
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

// gladiator.dll: 1003B320..1003B41F
// gladi386.so:   0004D4DC..0004D633
/* Allocate a stub source_t on the stack, parse `string` through PC_Directive_define,
 * copy out the single resulting define from the local definehash, free the scratch
 * buffers and return it. */
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
  /* Q3 advances through `src.tokens` itself, not through the `t` copy: the
   * original re-reads the field for the `->next` even though `t` holds the same
   * pointer in a live register. */
  for ( t = src.tokens; src.tokens; t = src.tokens )
  {
    src.tokens = src.tokens->next;
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

// gladiator.dll: 1003B460..1003B48C
// gladi386.so:   0004D634..0004D6EE
/* Q3's PC_AddDefine — parse `string` into a fresh define_t and link it into
 * source->definehash; 1 on success, 0 if parsing failed.  DEAD in Gladiator. */
int __cdecl PC_AddDefine(source_t *source, const char *string)
{
  define_t *def;

  def = PC_DefineFromString(string);
  if ( !def )
    return 0;
  PC_AddDefineToHash(def, source->definehash);
  return 1;
}

// gladiator.dll: 1003B4A0..1003B4C6
// gladi386.so:   0004D6F0..0004D730
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

// gladiator.dll: 1003B4E0..1003B50A
// gladi386.so:   0004D730..0004D7C2
/* Remove a #define from globaldefines by name; line-for-line Q3's
 * PC_RemoveGlobalDefine.  No reachable caller — only its own unused thunk. */
int __cdecl PC_RemoveGlobalDefine(const char *name)
{
  define_t *define;

  define = PC_FindDefine(globaldefines, name);
  if ( define )
  {
    PC_FreeDefine(define);
    return 1;
  }
  return 0;
}

// gladiator.dll: 1003B520..1003B545
// gladi386.so:   0004D7C4..0004D836
// Drain the globaldefines list by repeatedly popping the head, advancing
// globaldefines to head->next, and calling PC_FreeDefine on the popped node — the
// shutdown path for the preprocessor's global #define table.  Line-for-line Q3's
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

// gladiator.dll: 1003B560..1003B63A
// gladi386.so:   0004D838..0004D9D7
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

// gladiator.dll: 1003B680..1003B6B0
// gladi386.so:   0004D9D8..0004DA9C
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

// gladiator.dll: 1003B6C0..1003B774
// gladi386.so:   0004DA9C..0004DCDF
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

// gladiator.dll: 1003B7B0..1003B7C0
// gladi386.so:   0004DCE0..0004DCFE
int __cdecl PC_Directive_ifdef(source_t *src)
{
  return PC_Directive_if_def(src, INDENT_IFDEF);
} //end of the function PC_Directive_ifdef

// gladiator.dll: 1003B7D0..1003B7E0
// gladi386.so:   0004DD00..0004DD1E
int __cdecl PC_Directive_ifndef(source_t *src)
{
  return PC_Directive_if_def(src, INDENT_IFNDEF);
} //end of the function PC_Directive_ifndef

// gladiator.dll: 1003B7F0..1003B85B
// gladi386.so:   0004DD20..0004DE0F
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

// gladiator.dll: 1003B880..1003B8BC
// gladi386.so:   0004DE10..0004DEA3
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

// gladiator.dll: 1003B8D0..1003B998
// gladi386.so:   0004DEA4..0004DFCC
int __cdecl PC_OperatorPriority(int op)
{
  switch ( op )
  {
    case 5: return 7;     /* P_LOGIC_AND */
    case 6: return 6;     /* P_LOGIC_OR */
    case 7: return 12;    /* P_LOGIC_GEQ */
    case 8: return 12;    /* P_LOGIC_LEQ */
    case 9: return 11;    /* P_LOGIC_EQ */
    case 10: return 11;   /* P_LOGIC_UNEQ */

    case 36: return 16;   /* P_LOGIC_NOT */
    case 37: return 12;   /* P_LOGIC_GREATER */
    case 38: return 12;   /* P_LOGIC_LESS */

    case 21: return 13;   /* P_RSHIFT */
    case 22: return 13;   /* P_LSHIFT */

    case 26: return 15;   /* P_MUL */
    case 27: return 15;   /* P_DIV */
    case 28: return 15;   /* P_MOD */
    case 29: return 14;   /* P_ADD */
    case 30: return 14;   /* P_SUB */

    case 32: return 10;   /* P_BIN_AND */
    case 33: return 8;    /* P_BIN_OR */
    case 34: return 9;    /* P_BIN_XOR */
    case 35: return 16;   /* P_BIN_NOT */

    case 42: return 5;    /* P_COLON */
    case 43: return 5;    /* P_QUESTIONMARK */
  }
  return 0;
}

// gladiator.dll: 1003B9E0..1003C307
// gladi386.so:   0004DFCC..0004EC03
int __cdecl PC_EvaluateTokens(source_t *source, token_t *tokens, int *intvalue, double *floatvalue, int integer)
{
  operator_t *o, *firstoperator, *lastoperator;
  value_t *v, *firstvalue, *lastvalue, *v1, *v2;
  token_t *t;
  int brace = 0;
  int parentheses = 0;
  int error = 0;
  int lastwasvalue = 0;
  int negativevalue = 0;
  int questmarkintvalue = 0;
  double questmarkfloatvalue = 0;
  int gotquestmarkvalue = 0;

  firstoperator = lastoperator = NULL;
  firstvalue = lastvalue = NULL;
  if ( intvalue ) *intvalue = 0;
  if ( floatvalue ) *floatvalue = 0;
  for ( t = tokens; t; t = t->next )
  {
    switch ( t->type )
    {
      case 4:   /* TT_NAME */
      {
        if ( lastwasvalue || negativevalue )
        {
          SourceError(source, "syntax error in #if/#elif");
          error = 1;
          break;
        }
        if ( strcmp(t->string, "defined") )
        {
          SourceError(source, "undefined name %s in #if/#elif", t->string);
          error = 1;
          break;
        }
        t = t->next;
        if ( !strcmp(t->string, "(") )
        {
          brace = 1;
          t = t->next;
        }
        if ( !t || t->type != 4 )
        {
          SourceError(source, "defined without name in #if/#elif");
          error = 1;
          break;
        }
        v = (value_t *) GetClearedMemory(sizeof(value_t));
        if ( PC_FindHashedDefine(source->definehash, t->string) )
        {
          v->intvalue = 1;
          v->floatvalue = 1;
        }
        else
        {
          v->intvalue = 0;
          v->floatvalue = 0;
        }
        v->parentheses = parentheses;
        v->next = NULL;
        v->prev = lastvalue;
        if ( lastvalue ) lastvalue->next = v;
        else firstvalue = v;
        lastvalue = v;
        if ( brace )
        {
          t = t->next;
          if ( !t || strcmp(t->string, ")") )
          {
            SourceError(source, "defined without ) in #if/#elif");
            error = 1;
            break;
          }
        }
        brace = 0;
        /* defined() creates a value */
        lastwasvalue = 1;
        break;
      }
      case 3:   /* TT_NUMBER */
      {
        if ( lastwasvalue )
        {
          SourceError(source, "syntax error in #if/#elif");
          error = 1;
          break;
        }
        v = (value_t *) GetClearedMemory(sizeof(value_t));
        if ( negativevalue )
        {
          v->intvalue = - (signed int) t->intvalue;
          v->floatvalue = - t->floatvalue;
        }
        else
        {
          v->intvalue = t->intvalue;
          v->floatvalue = t->floatvalue;
        }
        v->parentheses = parentheses;
        v->next = NULL;
        v->prev = lastvalue;
        if ( lastvalue ) lastvalue->next = v;
        else firstvalue = v;
        lastvalue = v;
        /* last token was a value */
        lastwasvalue = 1;
        negativevalue = 0;
        break;
      }
      case 5:   /* TT_PUNCTUATION */
      {
        if ( negativevalue )
        {
          SourceError(source, "misplaced minus sign in #if/#elif");
          error = 1;
          break;
        }
        if ( t->subtype == 44 )         /* P_PARENTHESESOPEN */
        {
          parentheses++;
          break;
        }
        else if ( t->subtype == 45 )    /* P_PARENTHESESCLOSE */
        {
          parentheses--;
          if ( parentheses < 0 )
          {
            SourceError(source, "too many ) in #if/#elsif");
            error = 1;
          }
          break;
        }
        /* check for invalid operators on floating point values */
        if ( !integer )
        {
          if ( t->subtype == 35 || t->subtype == 28 ||
               t->subtype == 21 || t->subtype == 22 ||
               t->subtype == 32 || t->subtype == 33 ||
               t->subtype == 34 )
          {
            SourceError(source, "illigal operator %s on floating point operands\n", t->string);
            error = 1;
            break;
          }
        }
        switch ( t->subtype )
        {
          case 36:  /* P_LOGIC_NOT */
          case 35:  /* P_BIN_NOT */
          {
            if ( lastwasvalue )
            {
              SourceError(source, "! or ~ after value in #if/#elif");
              error = 1;
              break;
            }
            break;
          }
          case 30:  /* P_SUB */
          {
            if ( !lastwasvalue )
            {
              negativevalue = 1;
              break;
            }
          }
          case 26:  /* P_MUL */
          case 27:  /* P_DIV */
          case 28:  /* P_MOD */
          case 29:  /* P_ADD */

          case 5:   /* P_LOGIC_AND */
          case 6:   /* P_LOGIC_OR */
          case 7:   /* P_LOGIC_GEQ */
          case 8:   /* P_LOGIC_LEQ */
          case 9:   /* P_LOGIC_EQ */
          case 10:  /* P_LOGIC_UNEQ */

          case 37:  /* P_LOGIC_GREATER */
          case 38:  /* P_LOGIC_LESS */

          case 21:  /* P_RSHIFT */
          case 22:  /* P_LSHIFT */

          case 32:  /* P_BIN_AND */
          case 33:  /* P_BIN_OR */
          case 34:  /* P_BIN_XOR */

          case 42:  /* P_COLON */
          case 43:  /* P_QUESTIONMARK */
          {
            if ( !lastwasvalue )
            {
              SourceError(source, "operator %s after operator in #if/#elif", t->string);
              error = 1;
              break;
            }
            break;
          }
          default:
          {
            SourceError(source, "invalid operator %s in #if/#elif", t->string);
            error = 1;
            break;
          }
        }
        if ( !error && !negativevalue )
        {
          o = (operator_t *) GetClearedMemory(sizeof(operator_t));
          o->op = t->subtype;
          o->priority = PC_OperatorPriority(t->subtype);
          o->parentheses = parentheses;
          o->next = NULL;
          o->prev = lastoperator;
          if ( lastoperator ) lastoperator->next = o;
          else firstoperator = o;
          lastoperator = o;
          lastwasvalue = 0;
        }
        break;
      }
      default:
      {
        SourceError(source, "unknown %s in #if/#elif", t->string);
        error = 1;
        break;
      }
    }
    if ( error ) break;
  }
  if ( !error )
  {
    if ( !lastwasvalue )
    {
      SourceError(source, "trailing operator in #if/#elif");
      error = 1;
    }
    else if ( parentheses )
    {
      SourceError(source, "too many ( in #if/#elif");
      error = 1;
    }
  }
  gotquestmarkvalue = 0;
  questmarkintvalue = 0;
  questmarkfloatvalue = 0;
  /* while there are operators */
  while ( !error && firstoperator )
  {
    v = firstvalue;
    for ( o = firstoperator; o->next; o = o->next )
    {
      /* if the current operator is nested deeper in parentheses
       * than the next operator */
      if ( o->parentheses > o->next->parentheses ) break;
      /* if the current and next operator are nested equally deep in parentheses */
      if ( o->parentheses == o->next->parentheses )
      {
        /* if the priority of the current operator is equal or higher
         * than the priority of the next operator */
        if ( o->priority >= o->next->priority ) break;
      }
      /* if the arity of the operator isn't equal to 1 */
      if ( o->op != 36 && o->op != 35 ) v = v->next;
      /* if there's no value or no next value */
      if ( !v )
      {
        SourceError(source, "mising values in #if/#elif");
        error = 1;
        break;
      }
    }
    if ( error ) break;
    v1 = v;
    v2 = v->next;
    switch ( o->op )
    {
      case 36: v1->intvalue = !v1->intvalue;
               v1->floatvalue = !v1->floatvalue; break;
      case 35: v1->intvalue = ~v1->intvalue;
               break;
      case 26: v1->intvalue *= v2->intvalue;
               v1->floatvalue *= v2->floatvalue; break;
      case 27: v1->intvalue /= v2->intvalue;
               v1->floatvalue /= v2->floatvalue; break;
      case 28: v1->intvalue %= v2->intvalue; break;
      case 29: v1->intvalue += v2->intvalue;
               v1->floatvalue += v2->floatvalue; break;
      case 30: v1->intvalue -= v2->intvalue;
               v1->floatvalue -= v2->floatvalue; break;
      case 5:  v1->intvalue = v1->intvalue && v2->intvalue;
               v1->floatvalue = v1->floatvalue && v2->floatvalue; break;
      case 6:  v1->intvalue = v1->intvalue || v2->intvalue;
               v1->floatvalue = v1->floatvalue || v2->floatvalue; break;
      case 7:  v1->intvalue = v1->intvalue >= v2->intvalue;
               v1->floatvalue = v1->floatvalue >= v2->floatvalue; break;
      case 8:  v1->intvalue = v1->intvalue <= v2->intvalue;
               v1->floatvalue = v1->floatvalue <= v2->floatvalue; break;
      case 9:  v1->intvalue = v1->intvalue == v2->intvalue;
               v1->floatvalue = v1->floatvalue == v2->floatvalue; break;
      case 10: v1->intvalue = v1->intvalue != v2->intvalue;
               v1->floatvalue = v1->floatvalue != v2->floatvalue; break;
      case 37: v1->intvalue = v1->intvalue > v2->intvalue;
               v1->floatvalue = v1->floatvalue > v2->floatvalue; break;
      case 38: v1->intvalue = v1->intvalue < v2->intvalue;
               v1->floatvalue = v1->floatvalue < v2->floatvalue; break;
      case 21: v1->intvalue >>= v2->intvalue;
               break;
      case 22: v1->intvalue <<= v2->intvalue;
               break;
      case 32: v1->intvalue &= v2->intvalue;
               break;
      case 33: v1->intvalue |= v2->intvalue;
               break;
      case 34: v1->intvalue ^= v2->intvalue;
               break;
      case 42:  /* P_COLON */
      {
        if ( !gotquestmarkvalue )
        {
          SourceError(source, ": without ? in #if/#elif");
          error = 1;
          break;
        }
        if ( integer )
        {
          if ( !questmarkintvalue ) v1->intvalue = v2->intvalue;
        }
        else
        {
          if ( !questmarkfloatvalue ) v1->floatvalue = v2->floatvalue;
        }
        gotquestmarkvalue = 0;
        break;
      }
      case 43:  /* P_QUESTIONMARK */
      {
        if ( gotquestmarkvalue )
        {
          SourceError(source, "? after ? in #if/#elif");
          error = 1;
          break;
        }
        questmarkintvalue = v1->intvalue;
        questmarkfloatvalue = v1->floatvalue;
        gotquestmarkvalue = 1;
        break;
      }
    }
    if ( error ) break;
    /* if not an operator with arity 1 */
    if ( o->op != 36 && o->op != 35 )
    {
      /* remove the second value if not question mark operator */
      if ( o->op != 43 ) v = v->next;
      if ( v->prev ) v->prev->next = v->next;
      else firstvalue = v->next;
      if ( v->next ) v->next->prev = v->prev;
      FreeMemory(v);
    }
    /* remove the operator */
    if ( o->prev ) o->prev->next = o->next;
    else firstoperator = o->next;
    if ( o->next ) o->next->prev = o->prev;
    FreeMemory(o);
  }
  if ( firstvalue )
  {
    if ( intvalue ) *intvalue = firstvalue->intvalue;
    if ( floatvalue ) *floatvalue = firstvalue->floatvalue;
  }
  for ( o = firstoperator; o; o = lastoperator )
  {
    lastoperator = o->next;
    FreeMemory(o);
  }
  for ( v = firstvalue; v; v = lastvalue )
  {
    lastvalue = v->next;
    FreeMemory(v);
  }
  if ( !error ) return 1;
  if ( intvalue ) *intvalue = 0;
  if ( floatvalue ) *floatvalue = 0;
  return 0;
}

// gladiator.dll: 1003C650..1003C861
// gladi386.so:   0004EC04..0004F03D
int __cdecl PC_Evaluate(source_t *source, int *intvalue, double *floatvalue, int integer)
{
  token_t token, *firsttoken, *lasttoken;
  token_t *t, *nexttoken;
  define_t *define;
  int defined = 0;

  if ( intvalue ) *intvalue = 0;
  if ( floatvalue ) *floatvalue = 0;
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
        if ( lasttoken ) lasttoken->next = t;
        else firsttoken = t;
        lasttoken = t;
      }
      else if ( !strcmp(token.string, "defined") )
      {
        defined = 1;
        t = PC_CopyToken(&token);
        t->next = NULL;
        if ( lasttoken ) lasttoken->next = t;
        else firsttoken = t;
        lasttoken = t;
      }
      else
      {
        define = PC_FindHashedDefine(source->definehash, token.string);
        if ( !define )
        {
          SourceError(source, "can't evaluate %s, not defined", token.string);
          return 0;
        }
        if ( !PC_ExpandDefineIntoSource(source, define) ) return 0;
      }
    }
    else if ( token.type == 3 || token.type == 5 )
    {
      t = PC_CopyToken(&token);
      t->next = NULL;
      if ( lasttoken ) lasttoken->next = t;
      else firsttoken = t;
      lasttoken = t;
    }
    else
    {
      SourceError(source, "can't evaluate %s", token.string);
      return 0;
    }
  } while ( PC_ReadLine(source, &token) );
  if ( !PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer) ) return 0;
  for ( t = firsttoken; t; t = nexttoken )
  {
    nexttoken = t->next;
    PC_FreeToken(t);
  }
  return 1;
}

// gladiator.dll: 1003C900..1003CB6E
// gladi386.so:   0004F040..0004F3CD
int __cdecl PC_DollarEvaluate(source_t *source, int *intvalue, double *floatvalue, int integer)
{
  int indent, defined = 0;
  token_t token, *firsttoken, *lasttoken;
  token_t *t, *nexttoken;
  define_t *define;

  if ( intvalue ) *intvalue = 0;
  if ( floatvalue ) *floatvalue = 0;
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
        if ( lasttoken ) lasttoken->next = t;
        else firsttoken = t;
        lasttoken = t;
      }
      else if ( !strcmp(token.string, "defined") )
      {
        defined = 1;
        t = PC_CopyToken(&token);
        t->next = NULL;
        if ( lasttoken ) lasttoken->next = t;
        else firsttoken = t;
        lasttoken = t;
      }
      else
      {
        define = PC_FindHashedDefine(source->definehash, token.string);
        if ( !define )
        {
          SourceError(source, "can't evaluate %s, not defined", token.string);
          return 0;
        }
        if ( !PC_ExpandDefineIntoSource(source, define) ) return 0;
      }
    }
    else if ( token.type == 3 || token.type == 5 )
    {
      if ( *token.string == '(' ) indent++;
      else if ( *token.string == ')' ) indent--;
      if ( indent <= 0 ) break;
      t = PC_CopyToken(&token);
      t->next = NULL;
      if ( lasttoken ) lasttoken->next = t;
      else firsttoken = t;
      lasttoken = t;
    }
    else
    {
      SourceError(source, "can't evaluate %s", token.string);
      return 0;
    }
  } while ( PC_ReadSourceToken(source, &token) );
  if ( !PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer) ) return 0;
  for ( t = firsttoken; t; t = nexttoken )
  {
    nexttoken = t->next;
    PC_FreeToken(t);
  }
  return 1;
}

// gladiator.dll: 1003CC10..1003CC88
// gladi386.so:   0004F3D0..0004F4D9
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

// gladiator.dll: 1003CCB0..1003CCEB
// gladi386.so:   0004F4DC..0004F565
int __cdecl PC_Directive_if(source_t *source)
{
  int value;

  if ( !PC_Evaluate(source, &value, 0, 1) )
    return 0;
  PC_PushIndent(source, 1, value == 0);
  return 1;
}

// gladiator.dll: 1003CD00..1003CD15
// gladi386.so:   0004F568..0004F58D
/* As Q3's PC_Directive_line: report "#line directive not supported" through the
 * source error reporter and return 0. */
int __cdecl PC_Directive_line(source_t *source)
{
  SourceError(source, "#line directive not supported");
  return 0;
}

// gladiator.dll: 1003CD30..1003CD6F
// gladi386.so:   0004F590..0004F5DE
int __cdecl PC_Directive_error(source_t *source)
{
  token_t token; // [esp+4h] [ebp-430h] BYREF

  strcpy(token.string, "");
  PC_ReadSourceToken(source, token.string);
  SourceError(source, "#error directive: %s", token.string);
  return 0;
}

// gladiator.dll: 1003CD80..1003CDCA
// gladi386.so:   0004F5E0..0004F6AC
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

// gladiator.dll: 1003CDF0..1003CE69
// gladi386.so:   0004F6AC..0004F778
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

// gladiator.dll: 1003CE90..1003CF4F
// gladi386.so:   0004F778..0004F92D
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

// gladiator.dll: 1003CF80..1003D04A
// gladi386.so:   0004F930..0004FAF1
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

// gladiator.dll: 1003D090..1003D18B
// gladi386.so:   0004FAF4..0004FC02
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
     * (`directives[i].func`), so MSVC keeps both the strength-reduced name pointer and
     * the counter. */
    for ( i = 0; directives[i].name; i++ )
    {
      if ( !strcmp(directives[i].name, token.string) )
        return directives[i].handler(source);
    }
  }
  SourceError(source, "unknown precompiler directive %s", token.string);
  return 0;
}

// gladiator.dll: 1003D1D0..1003D2A5
// gladi386.so:   0004FC04..0004FDD5
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

// gladiator.dll: 1003D2F0..1003D3D3
// gladi386.so:   0004FDD8..0004FFDD
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

// gladiator.dll: 1003D420..1003D526
// gladi386.so:   0004FFE0..00050127
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

// gladiator.dll: 1003D580..1003D61B
// gladi386.so:   00050128..000502AD
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

// gladiator.dll: 1003D650..1003D701
// gladi386.so:   000502B0..00050344
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

// gladiator.dll: 1003D740..1003DA19
// gladi386.so:   00050344..000505D4
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

// gladiator.dll: 1003DAE0..1003DB10
// gladi386.so:   000505D4..00050618
int __cdecl PC_ExpectAnyToken(source_t *source, intptr_t token)
{
  if ( !PC_ReadTokenHandle(source, token) )
  {
    SourceError(source, "couldn't read expected token");
    return 0;
  }
  return 1;
}

// gladiator.dll: 1003DB20..1003DBA5
// gladi386.so:   00050618..000506BC
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

// gladiator.dll: 1003DBE0..1003DC5F
// gladi386.so:   000506BC..00050786
/* Read one token and keep it only if its type matches and every bit of `subtype` is
 * set in the token's subtype; otherwise unread it and fail.  DEAD in Gladiator. */
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

// gladiator.dll: 1003DC80..1003DD10
// gladi386.so:   00050788..000507EA
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

// gladiator.dll: 1003DD40..1003DD55
// gladi386.so:   000507EC..00050846
void __cdecl PC_UnreadLastToken(source_t *source)
{
  PC_UnreadSourceToken(source, &source->cachedtoken);
}

// gladiator.dll: 1003DD70..1003DD83
// gladi386.so:   00050848..00050898
/* Pass-through wrapper around PC_UnreadSourceToken — the external PC_UnreadToken
 * entry point paralleling PC_UnreadLastToken.  DEAD in Gladiator. */
void __cdecl PC_UnreadToken(source_t *source, token_t *token)
{
  PC_UnreadSourceToken(source, token);
}

// gladiator.dll: 1003DDA0..1003DE1A
// gladi386.so:   00050898..0005091E
/* Copy `path` into source->includepath and ensure it ends with a separator.  The
 * fixed 0x104-byte memcpy (rather than strncpy) is verbatim from the original — it
 * reads past the source string's NUL, but the function never runs.  DEAD. */
void __cdecl PC_SetIncludePath(source_t *source, char *path)
{
  strncpy(source->includepath, path, MAX_PATH);
  if ( source->includepath[strlen(source->includepath) - 1] != '\\'
    && source->includepath[strlen(source->includepath) - 1] != '/' )
  {
    strcat(source->includepath, "\\");
  }
}

// gladiator.dll: 1003DE40..1003DE4F
// gladi386.so:   00050920..0005092F
/* Q3's PC_SetPunctuations verbatim.  The write lands at +0x208 in the DLL, which is
 * `punctuations` once includepath is the full MAX_PATH buffer.  DEAD. */
void __cdecl PC_SetPunctuations(source_t *source, punctuation_t *p)
{
  source->punctuations = p;
}

// gladiator.dll: 1003DE60..1003DEF3
// gladi386.so:   00050930..00050AB4
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

// gladiator.dll: 1003DF30..1003DFC3
// gladi386.so:   00050AB4..00050C38
/* The memory-buffer twin of LoadSourceFile above: same structure, but wraps an
 * already-in-memory script buffer via LoadScriptMemory.  DEAD in Gladiator. */
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

// gladiator.dll: 1003E000..1003E0D8
// gladi386.so:   00050C38..00050DE9
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
