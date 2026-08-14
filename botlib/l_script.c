/*
 * l_script.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x1003E120..0x10040470; the boundary evidence -- per-object .text and
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
#include "l_script.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_memory.h"
#include "l_utils.h"

/* unk_10060418 — 72-byte blob; &[3] is the "You are not allowed to…" text. */
char unk_10060418[72] = {
    0x00, 0x00, 0x00, 0x59, 0x6F, 0x75, 0x20, 0x61, 0x72, 0x65, 0x20, 0x6E, 0x6F, 0x74, 0x20, 0x61,
    0x6C, 0x6C, 0x6F, 0x77, 0x65, 0x64, 0x20, 0x74, 0x6F, 0x0A, 0x6D, 0x6F, 0x64, 0x69, 0x66, 0x79,
    0x20, 0x74, 0x68, 0x65, 0x20, 0x62, 0x6F, 0x74, 0x20, 0x63, 0x68, 0x61, 0x72, 0x61, 0x63, 0x74,
    0x65, 0x72, 0x73, 0x20, 0x69, 0x6E, 0x0A, 0x69, 0x6E, 0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x76,
    0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x2E, 0x00,
};

/* The punctuation_t[] array at VA 0x1005FE00 (52 entries + NULL), identical to
 * Q3 l_script.c::default_punctuations.  PS_CreatePunctuationTable walks it with
 * a 3-slot stride and fills each `next` at runtime.  String literals are const
 * while punctuation_t.p is char* — BOTCFLAGS suppresses that warning. */
punctuation_t default_punctuations[] = {
    /* multi-char — longest first */
    {">>=", 1,  NULL}, {"<<=", 2,  NULL},
    {"...", 3,  NULL}, {"##",  4,  NULL},
    {"&&",  5,  NULL}, {"||",  6,  NULL},
    {">=",  7,  NULL}, {"<=",  8,  NULL},
    {"==",  9,  NULL}, {"!=",  10, NULL},
    {"*=",  11, NULL}, {"/=",  12, NULL},
    {"%=",  13, NULL}, {"+=",  14, NULL},
    {"-=",  15, NULL}, {"++",  16, NULL},
    {"--",  17, NULL}, {"&=",  18, NULL},
    {"|=",  19, NULL}, {"^=",  20, NULL},
    {">>",  21, NULL}, {"<<",  22, NULL},
    {"->",  23, NULL}, {"::",  24, NULL},
    {".*",  25, NULL},
    /* single-char */
    {"*",   26, NULL}, {"/",   27, NULL},
    {"%",   28, NULL}, {"+",   29, NULL},
    {"-",   30, NULL}, {"=",   31, NULL},
    {"&",   32, NULL}, {"|",   33, NULL},
    {"^",   34, NULL}, {"~",   35, NULL},
    {"!",   36, NULL}, {">",   37, NULL},
    {"<",   38, NULL}, {".",   39, NULL},
    {",",   40, NULL}, {";",   41, NULL},
    {":",   42, NULL}, {"?",   43, NULL},
    {"(",   44, NULL}, {")",   45, NULL},
    {"{",   46, NULL}, {"}",   47, NULL},
    {"[",   48, NULL}, {"]",   49, NULL},
    {"\\",  50, NULL}, {"#",   51, NULL},
    {"$",   52, NULL},
    {NULL,  0,  NULL}  /* sentinel */
};

//----- (1003E120) --------------------------------------------------------
/* Build a 256-entry perfect-hash table indexed by each punctuation's first
 * character, with chains sorted longer-first. */
void __cdecl PS_CreatePunctuationTable(script_t *script, punctuation_t *punctuations)
{
  int i;
  punctuation_t *p, *lastp, *newp;

  if ( !script->punctuationtable )
    script->punctuationtable = (punctuation_t **)GetMemory(256 * sizeof(punctuation_t *));
  memset(script->punctuationtable, 0, 256 * sizeof(punctuation_t *));
  for ( i = 0; punctuations[i].p; i++ )
  {
    newp  = &punctuations[i];
    lastp = NULL;
    for ( p = script->punctuationtable[(unsigned int)newp->p[0]]; p; p = p->next )
    {
      if ( strlen(p->p) < strlen(newp->p) )
      {
        newp->next = p;
        if ( lastp ) lastp->next = newp;
        else         script->punctuationtable[(unsigned int)newp->p[0]] = newp;
        break;
      }
      lastp = p;
    }
    if ( !p )
    {
      newp->next = NULL;
      if ( lastp ) lastp->next = newp;
      else         script->punctuationtable[(unsigned int)newp->p[0]] = newp;
    }
  }
}
// 1003E1C8: conditional instruction was optimized away because edx.4!=0
//----- (1003E250) --------------------------------------------------------
/* Linear scan of script->punctuations (a contiguous array terminated by a NULL
 * `p`) for the record whose .n matches, returning its string or the original's
 * typo'd "unkown punctuation" default.  DEAD in Gladiator. */
char *__cdecl PunctuationFromNum(script_t *script, int num)
{
  int i;
  for ( i = 0; script->punctuations[i].p; i++ )
  {
    if ( script->punctuations[i].n == num )
      return script->punctuations[i].p;
  }
  return "unkown punctuation";
}
//----- (1003E2C0) --------------------------------------------------------
void ScriptError(int script, char *Format, ...)
{
  char Buffer[1024]; // [esp+4h] [ebp-400h] BYREF
  va_list va; // [esp+410h] [ebp+Ch] BYREF

  if ( *(unsigned char *)&((script_t *)script)->flags & 1 )
    return;
  va_start(va, Format);
  vsprintf(Buffer, Format, va);
  botimport.Print(PRT_ERROR, "file %s, line %d: %s\n", (const char *)script, ((script_t *)script)->line, Buffer);
}
//----- (1003E340) --------------------------------------------------------
void ScriptWarning(int script, char *Format, ...)
{
  char Buffer[1024]; // [esp+4h] [ebp-400h] BYREF
  va_list va; // [esp+410h] [ebp+Ch] BYREF

  if ( *(unsigned char *)&((script_t *)script)->flags & 2 )
    return;
  va_start(va, Format);
  vsprintf(Buffer, Format, va);
  botimport.Print(PRT_WARNING, "file %s, line %d: %s\n", (const char *)script, ((script_t *)script)->line, Buffer);
}
//----- (1003E3C0) --------------------------------------------------------
//----- (1003E3C0) --------------------------------------------------------
/* Install the given punctuation list, or the default table when NULL. */
void __cdecl SetScriptPunctuations(script_t *script, punctuation_t *p)
{
  if ( p )
    PS_CreatePunctuationTable(script, p);
  else
    PS_CreatePunctuationTable(script, default_punctuations);
  if ( p )
    script->punctuations = p;
  else
    script->punctuations = default_punctuations;
}
//----- (1003E410) --------------------------------------------------------
/* PS_ReadWhiteSpace — takes a real script_t*, so PS_ReadToken's pointer is not
 * truncated across the call. */
int __cdecl PS_ReadWhiteSpace(script_t *script)
{
  while ( 1 )
  {
    while ( *script->script_p <= ' ' )
    {
      if ( !*script->script_p ) return 0;
      if ( *script->script_p == '\n' ) script->line++;
      script->script_p++;
    }
    if ( *script->script_p == '/' )
    {
      if ( *(script->script_p + 1) == '/' )
      {
        script->script_p++;
        do
        {
          script->script_p++;
          if ( !*script->script_p ) return 0;
        }
        while ( *script->script_p != '\n' );
        script->line++;
        script->script_p++;
        if ( !*script->script_p ) return 0;
        continue;
      }
      else if ( *(script->script_p + 1) == '*' )
      {
        script->script_p++;
        do
        {
          script->script_p++;
          if ( !*script->script_p ) return 0;
          if ( *script->script_p == '\n' ) script->line++;
        }
        while ( !(*script->script_p == '*' && *(script->script_p + 1) == '/') );
        script->script_p++;
        if ( !*script->script_p ) return 0;
        script->script_p++;
        if ( !*script->script_p ) return 0;
        continue;
      }
    }
    break;
  }
  return 1;
}
//----- (1003E520) --------------------------------------------------------
int __cdecl PS_ReadEscapeCharacter(script_t *script, _BYTE *ch)
{
  int c, val, i;

  script->script_p++;
  switch ( *script->script_p )
  {
    case '\\':
      c = '\\';
      break;
    case 'n':
      c = '\n';
      break;
    case 'r':
      c = '\r';
      break;
    case 't':
      c = '\t';
      break;
    case 'v':
      c = '\v';
      break;
    case 'b':
      c = '\b';
      break;
    case 'f':
      c = '\f';
      break;
    case 'a':
      c = '\a';
      break;
    case '\'':
      c = '\'';
      break;
    case '"':
      c = '"';
      break;
    case '?':
      c = '?';
      break;
    case 'x':
    {
      script->script_p++;
      for ( i = 0, val = 0; ; i++, script->script_p++ )
      {
        c = *script->script_p;
        if ( c >= '0' && c <= '9' ) c = c - '0';
        else if ( c >= 'A' && c <= 'Z' ) c = c - 'A' + 10;
        else if ( c >= 'a' && c <= 'z' ) c = c - 'a' + 10;
        else break;
        val = (val << 4) + c;
      }
      script->script_p--;
      if ( val > 0xFF )
      {
        ScriptWarning(script, "too large value in escape character");
        val = 0xFF;
      }
      c = val;
      break;
    }
    default:
    {
      if ( *script->script_p < '0' || *script->script_p > '9' )
        ScriptError(script, "unknown escape char");
      for ( i = 0, val = 0; ; i++, script->script_p++ )
      {
        c = *script->script_p;
        if ( c >= '0' && c <= '9' ) c = c - '0';
        else break;
        val = val * 10 + c;
      }
      script->script_p--;
      if ( val > 0xFF )
      {
        ScriptWarning(script, "too large value in escape character");
        val = 0xFF;
      }
      c = val;
      break;
    }
  }
  script->script_p++;
  *ch = c;
  return 1;
}
//----- (1003E7F0) --------------------------------------------------------
int __cdecl PS_ReadString(script_t *script, token_t *token, int quote)
{
  int len;
  int tmpline;
  char *tmpscript_p;

  if ( quote == 34 )
    token->type = 1;
  else
    token->type = 2;
  len = 0;
  token->string[len++] = *script->script_p++;
  while ( 1 )
  {
    if ( len >= 1022 )
    {
      ScriptError(script, "string longer than MAX_TOKEN = %d", 1024);
      return 0;
    }
    if ( *script->script_p == 92 && (script->flags & 8) == 0 )
    {
      if ( !PS_ReadEscapeCharacter(script, &token->string[len]) )
      {
        token->string[len] = 0;
        return 0;
      }
      len++;
    }
    else if ( *script->script_p == quote )
    {
      ++script->script_p;
      if ( (script->flags & 4) != 0 )
        break;
      tmpscript_p = script->script_p;
      tmpline = script->line;
      if ( !PS_ReadWhiteSpace(script) )
      {
        script->script_p = tmpscript_p;
        script->line = tmpline;
        break;
      }
      if ( *script->script_p != quote )
      {
        script->script_p = tmpscript_p;
        script->line = tmpline;
        break;
      }
      ++script->script_p;
    }
    else
    {
      if ( !*script->script_p )
      {
        token->string[len] = 0;
        ScriptError(script, "missing trailing quote");
        return 0;
      }
      if ( *script->script_p == 10 )
      {
        token->string[len] = 0;
        ScriptError(script, "newline inside string %s", token->string);
        return 0;
      }
      token->string[len++] = *script->script_p++;
    }
  }
  token->string[len++] = quote;
  token->string[len] = 0;
  token->subtype = len;
  return 1;
}
//----- (1003E9F0) --------------------------------------------------------
int __cdecl PS_ReadName(script_t *script, intptr_t a2)
{
  token_t *token = (token_t *)a2;
  int len = 0;
  char c; // al

  token->type = 4;
  do
  {
    token->string[len++] = *script->script_p++;
    if ( len >= 1024 )
    {
      ScriptError(script, "name longer than MAX_TOKEN = %d", 1024);
      return 0;
    }
    c = *script->script_p;
  }
  while ( (c >= 97 && c <= 122) || (c >= 65 && c <= 90) || (c >= 48 && c <= 57) || c == 95 );
  token->string[len] = 0;
  token->subtype = len;
  return 1;
}
//----- (1003EAB0) --------------------------------------------------------
/* Walks `string` through a plain char* cursor.  Subtype flags: 0x800 = float,
 * 0x8 = decimal, 0x100 = hex, 0x200 = octal, 0x400 = binary. */
void __cdecl NumberValue(char *string, int subtype, int *intvalue, long double *floatvalue)
{
  char *p;
  unsigned int dotfound = 0;
  char i;

  *intvalue = 0;
  *floatvalue = 0.0;
  if ( (subtype & 0x800) != 0 )
  {
    for ( p = string; (i = *p) != 0; ++p )
    {
      if ( i == 46 )
      {
        if ( dotfound )
          return;
        dotfound = 10;
        ++p;
      }
      if ( dotfound )
      {
        *floatvalue = (double)((*p) - 48) / (double)dotfound + *floatvalue;
        dotfound *= 10;
      }
      else
      {
        *floatvalue = (double)(i - 48) + *floatvalue * 10.0;
      }
    }
    *intvalue = (int)*floatvalue;
  }
  else if ( (subtype & 8) != 0 )
  {
    for ( p = string; *p; ++p )
      *intvalue = *p + 10 * *intvalue - 48;
    *floatvalue = (double)(unsigned int)*intvalue;
  }
  else if ( (subtype & 0x100) != 0 )
  {
    string += 2;
    while ( *string )
    {
      *intvalue <<= 4;
      if ( *string >= 'a' && *string <= 'f' )
        *intvalue += *string - 'a' + 10;
      else if ( *string >= 'A' && *string <= 'F' )
        *intvalue += *string - 'A' + 10;
      else
        *intvalue += *string - '0';
      ++string;
    }
    *floatvalue = (double)(unsigned int)*intvalue;
  }
  else if ( (subtype & 0x200) != 0 )
  {
    ++string;
    while ( *string )
      *intvalue = (*intvalue << 3) + (*string++ - '0');
    *floatvalue = (double)(unsigned int)*intvalue;
  }
  else if ( (subtype & 0x400) != 0 )
  {
    for ( p = string + 2; *p; ++p )
      *intvalue = *p + 2 * *intvalue - 48;
    *floatvalue = (double)(unsigned int)*intvalue;
  }
}
//----- (1003ECD0) --------------------------------------------------------
int __cdecl PS_ReadNumber(script_t *script, token_t *token)
{
  int len = 0;
  char c;
  int octal, dot;
  int i;

  token->type = 3;
  if ( *script->script_p == 48 && (script->script_p[1] == 120 || script->script_p[1] == 88) )
  {
    token->string[len++] = *script->script_p++;
    token->string[len++] = *script->script_p++;
    c = *script->script_p;
    /* hexadecimal — NB the faithful Gladiator quirk: uppercase digits admit only 'A' (65..65) */
    while ( (c >= 48 && c <= 57) || (c >= 97 && c <= 102) || (c >= 65 && c <= 65) )
    {
      token->string[len++] = *script->script_p++;
      if ( len >= 1024 )
      {
        ScriptError(script, "hexadecimal number longer than MAX_TOKEN = %d",
                    1024);
        return 0;
      }
      c = *script->script_p;
    }
    token->subtype |= 0x100;
  }
  else if ( *script->script_p == 48 && (script->script_p[1] == 98 || script->script_p[1] == 66) )
  {
    token->string[len++] = *script->script_p++;
    token->string[len++] = *script->script_p++;
    c = *script->script_p;
    while ( c == 48 || c == 49 )
    {
      token->string[len++] = *script->script_p++;
      if ( len >= 1024 )
      {
        ScriptError(script, "binary number longer than MAX_TOKEN = %d", 1024);
        return 0;
      }
      c = *script->script_p;
    }
    token->subtype |= 0x400;
  }
  else
  {
    octal = 0;
    dot = 0;
    if ( *script->script_p == 48 )
      octal = 1;
    while ( 1 )
    {
      token->string[len++] = *script->script_p++;
      if ( len >= 1024 )
      {
        ScriptError(script, "number longer than MAX_TOKEN = %d", 1024);
        return 0;
      }
      c = *script->script_p;
      if ( c == 46 )
        dot = 1;
      else if ( c == 56 || c == 57 )
        octal = 0;
      else if ( c < 48 || c > 57 )
        break;
    }
    if ( octal )
      token->subtype |= 0x200;
    else
      token->subtype |= 8;
    if ( dot )
      token->subtype |= 0x800;
  }
  for ( i = 0; i < 2; i++ )
  {
    c = *script->script_p;
    if ( c == 108 || c == 76 && (token->subtype & 0x2000) == 0 )
    {
      script->script_p++;
      token->subtype |= 0x2000;
    }
    else if ( c == 117 || c == 85 && (token->subtype & 0x4800) == 0 )
    {
      script->script_p++;
      token->subtype |= 0x4000;
    }
  }
  token->string[len] = 0;
  NumberValue(token->string, token->subtype, (int *)&token->intvalue, &token->floatvalue);
  if ( (token->subtype & 0x800) == 0 )
    token->subtype |= 0x1000;
  return 1;
}
// 1003ED96: conditional instruction was optimized away because dl.1==30
//----- (1003F020) --------------------------------------------------------
/* Q3's PS_ReadCharacterLiteral — read a 'x' or '\\n'-style character constant
 * and store its value in token.subtype.  DEAD in Gladiator. */
int __cdecl PS_ReadLiteral(script_t *script, token_t *token)
{
  token->type = 2; /* TT_LITERAL */
  token->string[0] = *script->script_p++;
  if ( !*script->script_p )
  {
    ScriptError((int)script, "end of file before trailing \'");
    return 0;
  }
  if ( *script->script_p == '\\' )
  {
    if ( !PS_ReadEscapeCharacter(script, (_BYTE *)&token->string[1]) )
      return 0;
  }
  else
  {
    token->string[1] = *script->script_p++;
  }
  if ( *script->script_p != '\'' )
  {
    ScriptWarning((int)script, "too many characters in literal, ignored");
    while ( *script->script_p && *script->script_p != '\'' && *script->script_p != '\n' )
      ++script->script_p;
    if ( *script->script_p == '\'' )
      ++script->script_p;
  }
  token->string[2] = *script->script_p++;
  token->string[3] = 0;
  token->subtype = (signed char)token->string[1];
  return 1;
}
//----- (1003F160) --------------------------------------------------------
/* Try to read a punctuation token at the script's current position.
 * NB the table index `*script->script_p` is a SIGNED char on purpose — the
 * original sign-extends it.  Q3 later changed this to unsigned; do NOT "fix"
 * the -Wchar-subscripts warning here. */
int __cdecl PS_ReadPunctuation(script_t *script, char *token)
{
  punctuation_t *punc;
  char *p;
  int len;

  for ( punc = script->punctuationtable[*script->script_p]; punc; punc = punc->next )
  {
    p = punc->p;
    len = strlen(p);
    if ( script->script_p + len <= script->end_p )
    {
      if ( !strncmp(script->script_p, p, len) )
      {
        strncpy(token, p, 0x400u);
        script->script_p += len;
        *((_DWORD *)token + 256) = 5;
        *((_DWORD *)token + 257) = punc->n;
        return 1;
      }
    }
  }
  return 0;
}
//----- (1003F230) --------------------------------------------------------
int __cdecl PS_ReadPrimitive(script_t *script, token_t *token)
{
  int len; // ecx
  char v3; // dl

  len = 0;
  while ( *(script)->script_p > 32 )
  {
    v3 = *(_BYTE *)(script)->script_p;
    if ( v3 == 59 )
      break;
    if ( len >= 1024 )
    {
      ScriptError(script, "primitive token longer than MAX_TOKEN = %d", 0x400);
      return 0;
    }
    token->string[len++] = *(script)->script_p++;
  }
  token->string[len] = 0;
  memcpy(&(script)->token, token, sizeof(token_t));
  return 1;
}
//----- (1003F2D0) --------------------------------------------------------
/* Q3's PS_ReadToken(script_t *, token_t *).  Only the first parameter is typed
 * so far; the body still reaches its sub-helpers through (int)script casts. */
int __cdecl PS_ReadToken(script_t *script, char *Destination)
{
  token_t *token = (token_t *)Destination;

  if ( script->tokenavailable )
  {
    script->tokenavailable = 0;
    memcpy(token, &script->token, sizeof(token_t));
    return 1;
  }
  script->lastscript_p = script->script_p;
  script->lastline = script->line;
  memset(token, 0, sizeof(token_t));
  script->whitespace_p = script->script_p;
  token->whitespace_p = script->script_p;
  if ( !PS_ReadWhiteSpace(script) ) return 0;
  script->endwhitespace_p = script->script_p;
  token->endwhitespace_p = script->script_p;
  token->line = script->line;
  token->linescrossed = script->line - script->lastline;
  if ( *script->script_p == '\"' )
  {
    if ( !PS_ReadString(script, token, '\"') ) return 0;
  }
  else if ( *script->script_p == '\'' )
  {
    if ( !PS_ReadString(script, token, '\'') ) return 0;
  }
  else if ( (*script->script_p >= '0' && *script->script_p <= '9')
       || (*script->script_p == '.' && (script->script_p[1] >= '0' && script->script_p[1] <= '9')) )
  {
    if ( !PS_ReadNumber(script, token) ) return 0;
  }
  else if ( script->flags & 0x10 )
  {
    return PS_ReadPrimitive(script, token);
  }
  else if ( (*script->script_p >= 'a' && *script->script_p <= 'z')
       || (*script->script_p >= 'A' && *script->script_p <= 'Z')
       || *script->script_p == '_' )
  {
    if ( !PS_ReadName(script, (intptr_t)token) ) return 0;
  }
  else if ( !PS_ReadPunctuation(script, Destination) )
  {
    ScriptError((int)script, "can't read token");
    return 0;
  }
  memcpy(&script->token, token, sizeof(token_t));
  return 1;
}
//----- (1003F4D0) --------------------------------------------------------
/* Read the next token and ScriptError unless its string equals `string`.
 * DEAD in Gladiator. */
int __cdecl PS_ExpectTokenString(script_t *script, const char *string)
{
  token_t token;
  if ( !PS_ReadToken(script, (char *)&token) )
  {
    ScriptError((int)script, "couldn't find expected %s", string);
    return 0;
  }
  if ( strcmp(token.string, string) != 0 )
  {
    ScriptError((int)script, "expected %s, found %s", string, token.string);
    return 0;
  }
  return 1;
}
//----- (1003F5C0) --------------------------------------------------------
int __cdecl PS_ExpectTokenType(script_t *script, int type, int subtype, token_t *token)
{
  int v6; // eax
  char str[1024]; // [esp+10h] [ebp-400h] BYREF

  if ( !PS_ReadToken(script, (char *)token) )
  {
    ScriptError(script, "couldn't read expected token");
    return 0;
  }
  v6 = token->type;
  if ( v6 != type )
  {
    if ( type == 1 )
      strcpy(str, "string");
    else if ( type == 2 )
      strcpy(str, "literal");
    else if ( type == 3 )
      strcpy(str, "number");
    else if ( type == 4 )
      strcpy(str, "name");
    else if ( type == 5 )
      strcpy(str, "punctuation");
    ScriptError(script, "expected a %s, found %s", str, token);
    return 0;
  }
  if ( v6 == 3 )
  {
    if ( (token->subtype & subtype) != subtype )
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
        strcat(str, " long");
      if ( (subtype & 0x4000) != 0 )
        strcat(str, " unsigned");
      if ( (subtype & 0x800) != 0 )
        strcat(str, " float");
      if ( (subtype & 0x1000) != 0 )
        strcat(str, " integer");
      ScriptError(script, "expected %s, found %s", str, token);
      return 0;
    }
  }
  else if ( v6 == 5 )
  {
    if ( subtype < 0 )
    {
      ScriptError(script, "BUG: wrong punctuation subtype");
      return 0;
    }
    if ( token->subtype != subtype )
    {
      ScriptError(script, "expected %s, found %s",
                  (script->punctuations)[subtype], token);
      return 0;
    }
  }
  return 1;
}
//----- (1003F9B0) --------------------------------------------------------
int __cdecl PS_ExpectAnyToken(int script, int token)
{
  if ( !PS_ReadToken(script, token) )
  {
    ScriptError(script, "couldn't read expected token");
    return 0;
  }
  return 1;
}
//----- (1003F9F0) --------------------------------------------------------
/* Peek the next token: 1 if its string matches, else rewind script_p from
 * lastscript_p and return 0.  Sibling of PS_CheckTokenType.
 * DEAD in Gladiator. */
int __cdecl PS_CheckTokenString(script_t *script, const char *string)
{
  token_t token;
  if ( !PS_ReadToken(script, (char *)&token) )
    return 0;
  if ( strcmp(token.string, string) == 0 )
    return 1;
  script->script_p = script->lastscript_p;
  return 0;
}
//----- (1003FAB0) --------------------------------------------------------
/* Peek the next token: on (type == expected && (subtype & mask) == mask) copy
 * it out and return 1, else rewind script_p and return 0.  Sibling of
 * PS_ExpectTokenType.  DEAD in Gladiator. */
int __cdecl PS_CheckTokenType(script_t *script, int type, int subtype, token_t *out)
{
  token_t token;
  if ( !PS_ReadToken(script, (char *)&token) )
    return 0;
  if ( token.type == type && (token.subtype & subtype) == subtype )
  {
    memcpy(out, &token, sizeof(token_t));
    return 1;
  }
  script->script_p = script->lastscript_p;
  return 0;
}
//----- (1003FB50) --------------------------------------------------------
/* Read tokens until one equals `string` (1) or the stream ends (0).  Sibling of
 * PC_SkipUntilString.  DEAD in Gladiator. */
int __cdecl PS_SkipUntilString(script_t *script, const char *string)
{
  token_t token;
  while ( PS_ReadToken(script, (char *)&token) )
  {
    if ( strcmp(token.string, string) == 0 )
      return 1;
  }
  return 0;
}
//----- (1003FC10) --------------------------------------------------------
/* Sets script->tokenavailable = 1.  DEAD in Gladiator. */
void __cdecl PS_UnreadLastToken(script_t *script)
{
  script->tokenavailable = 1;
}
//----- (1003FC70) --------------------------------------------------------
/* getc over the script's whitespace span: read one byte, advance the cursor,
 * return 0 at the end.  DEAD in Gladiator. */
char PS_NextWhiteSpaceChar(script_t *script)
{
  if (script->whitespace_p != script->endwhitespace_p)
    return *script->whitespace_p++;
  return 0;
}
//----- (1003FCB0) --------------------------------------------------------
void __cdecl StripDoubleQuotes(char *string)
{
  /* The original uses strcpy() with OVERLAPPING src/dst — a byte copy under
   * 32-bit MSVC and under the vintage i386 glibc/libc5 the 1999 Linux build
   * shipped with, undefined with a modern glibc's SIMD strcpy — so keep
   * strcpy only for the two vintage-i386 oracles (MSVC6 and gcc 2.7.2.3;
   * same `__i386__ && !__SSE_MATH__` predicate as be_aas_reach.c's x87-temp
   * gate above, for the same reason: it is FLT_EVAL_METHOD/instruction-set
   * vintage, not compiler brand) and memmove elsewhere. The outer `while`
   * (not Q3's single `if`) matches the original's loopback. */
  while ( *string == '"' )
#if defined(_MSC_VER) || (defined(__i386__) && !defined(__SSE_MATH__))
    strcpy(string, string + 1);
#else
    memmove(string, string + 1, strlen(string));
#endif
  while ( string[strlen(string) - 1] == '"' )
    string[strlen(string) - 1] = '\0';
}
//----- (1003FD40) --------------------------------------------------------
void __cdecl StripSingleQuotes(char *string)
{
  while ( *string == '\'' )
#if defined(_MSC_VER) || (defined(__i386__) && !defined(__SSE_MATH__))
    strcpy(string, string + 1);
#else
    memmove(string, string + 1, strlen(string));
#endif
  while ( string[strlen(string) - 1] == '\'' )
    string[strlen(string) - 1] = '\0';
}
//----- (1003FDD0) --------------------------------------------------------
// Signed float reader: PS_ExpectAnyToken; if token == "-" set sign=-1.0
// and read another token (PS_ExpectTokenType with type=TT_NUMBER, mask=0);
// else require token.type == TT_NUMBER, ScriptError'ing on mismatch
// with .rdata 0x100603c8 "expected float value, found %s\n".  Returns
// token.floatvalue * sign as a double.
// DEAD in Gladiator — preserved by /INCREMENTAL.  Sibling of ReadSignedInt
// (integer variant below).  The sign is
// constructed as a double via two int half-writes (0|0x3ff00000 for +1.0,
// 0|0xbff00000 for -1.0), exactly as the MSVC frontend would emit.
long double __cdecl ReadSignedFloat(int script)
{
  long double sign;
  token_t token;

  sign = 1.0;
  PS_ExpectAnyToken(script, (int)&token);
  if ( !strcmp(token.string, "-") )
  {
    sign = -1.0;
    PS_ExpectTokenType(script, 3, 0, &token);
  }
  else if ( token.type != 3 )
  {
    ScriptError(script, "expected float value, found %s\n", token.string);
  }
  return sign * token.floatvalue;
}
//----- (1003FEC0) --------------------------------------------------------
// Signed integer reader: PS_ExpectAnyToken; if token == "-" set sign=-1
// and read another token (PS_ExpectTokenType with type=TT_NUMBER,
// mask=0x1000); else require token.type == TT_NUMBER and reject the
// float subtype (0x800), ScriptError'ing on mismatch with .rdata
// 0x100603f0 "expected integer value, found %s\n".  Returns
// token.intvalue * sign as int.
// DEAD in Gladiator — preserved by /INCREMENTAL.  Sibling of ReadSignedFloat
// (float variant above).
int __cdecl ReadSignedInt(int script)
{
  int sign;
  token_t token;

  sign = 1;
  PS_ExpectAnyToken(script, (int)&token);
  if ( !strcmp(token.string, "-") )
  {
    sign = -1;
    PS_ExpectTokenType(script, 3, 0x1000, &token);
  }
  else if ( token.type != 3 || token.subtype == 0x800 )
  {
    ScriptError(script, "expected integer value, found %s\n", token.string);
  }
  return (int)token.intvalue * sign;
}
//----- (1003FFB0) --------------------------------------------------------
void __cdecl SetScriptFlags(script_t *script, int flags)
{
  script->flags = flags;
}
//----- (1003FFD0) --------------------------------------------------------
/* Returns script->flags.  DEAD in Gladiator — the live API uses the dedicated
 * accessors instead. */
int __cdecl GetScriptFlags(script_t *script)
{
  return script->flags;
}
//----- (10040060) --------------------------------------------------------
/* Q3 l_script.c:1242: int EndOfScript(script_t *script). */
BOOL __cdecl EndOfScript(script_t *script)
{
  return script->script_p >= script->end_p;
}
//----- (10040090) --------------------------------------------------------
/* Returns script->line - script->lastline.  DEAD in Gladiator. */
int __cdecl NumLinesCrossed(script_t *script)
{
  return script->line - script->lastline;
}
//----- (100400C0) --------------------------------------------------------
/* Character-level companion to the token-based PS_SkipUntilString: scan the raw
 * script stream, skipping whitespace between probes, for an occurrence of
 * `value` anchored on its first byte.  No Q3 counterpart.
 *
 * Keep the single `while (PS_ReadWhiteSpace(...))`, which lets MSVC rotate the
 * loop as the original does; a leading guard plus while(1) emits an extra jmp.
 *
 * DEAD in Gladiator. */
int __cdecl ScriptSkipTo(script_t *script, char *value)
{
  int len;
  char firstchar;

  firstchar = *value;
  len = strlen(value);
  do
  {
    if (!PS_ReadWhiteSpace(script)) return 0;
    if (*script->script_p == firstchar)
    {
      if (!strncmp(script->script_p, value, len))
      {
        return 1;
      }
    }
    script->script_p++;
  } while(1);
}
//----- (10040150) --------------------------------------------------------
int __cdecl FileLength(FILE *fp)
{
  int v1; // edi
  int v2; // ebx

  v1 = ftell(fp);
  fseek(fp, 0, 2);
  v2 = ftell(fp);
  fseek(fp, v1, 0);
  return v2;
}
//----- (100401A0) --------------------------------------------------------
/* Raw script file loader: create a script_t (1392-byte header + file data) in
 * one allocation.  Does NOT set up a source_t or scriptstack — LoadSourceFile
 * does that around this. */
script_t *__cdecl LoadScriptFile(char *FileName, int Offset, size_t ElementSize)
{
  FILE *fp;
  size_t length;
  script_t *script;
  char v10[144]; // [esp+Ch] [ebp-90h] BYREF

  fp = fopen(FileName, "rb");
  if ( !fp )
    return NULL;
  if ( Offset )
    fseek(fp, Offset, 0);
  length = ElementSize;
  if ( !ElementSize )
    length = FileLength(fp) - Offset;
  script = (script_t *)GetClearedMemory(length + sizeof(script_t) + 1);
  memset(script, 0, sizeof(script_t));
  strcpy(script->filename, FileName);
  script->buffer       = (char *)script + sizeof(script_t);
  script->buffer[length] = 0;
  script->script_p     = script->buffer;
  script->lastscript_p = script->buffer;
  script->length       = length;
  script->end_p        = script->buffer + length;
  script->tokenavailable = 0;
  script->line         = 1;
  script->lastline     = 1;
  SetScriptPunctuations(script, NULL);
  if ( fread(script->buffer, length, 1u, fp) != 1 )
  {
    FreeMemory(script);
    script = NULL;
  }
  fclose(fp);
  {
    const unsigned char *buffer;

    /* The buffer fetch is the FIRST statement of this block: the original loads
     * script->buffer before the memcpy and only materialises the memset's zero
     * afterwards. */
    buffer = (const unsigned char *)script->buffer;
    memcpy(v10, &unk_10060418, 0x48u);
    memset(&v10[72], 0, 0x48u);
    if ( !sub_10037850(FileName, buffer, script->length) )
    {
      LibVar("__squatt", (char *)"1");
      botimport.Print(PRT_EXIT, &v10[3]);
    }
  }
  return script;
}
//----- (10040380) --------------------------------------------------------
script_t *__cdecl LoadScriptMemory(const void *ptr, unsigned int length, const char *name)
{
  script_t *script;

  script = (script_t *)GetClearedMemory(length + sizeof(script_t) + 1);
  memset(script, 0, sizeof(script_t));
  strcpy(script->filename, name);
  script->buffer       = (char *)script + sizeof(script_t);
  script->buffer[length] = 0;
  script->length       = length;
  script->script_p     = script->buffer;
  script->lastscript_p = script->buffer;
  script->end_p        = script->buffer + length;
  script->tokenavailable = 0;
  script->line         = 1;
  script->lastline     = 1;
  SetScriptPunctuations(script, NULL);
  memcpy(script->buffer, ptr, length);
  return script;
}
//----- (10040470) --------------------------------------------------------
void __cdecl FreeScript(script_t *script)
{
  if ( script->punctuationtable )
    FreeMemory(script->punctuationtable);
  FreeMemory(script);
}

/* The nine structure read/write functions live in their own TU:
 * botlib/l_struct.c (l_struct.obj, DLL 0x100404B0..0x1004123F -- see
 * .claude/memory/tu_partition.md). */

/* Both of the following ARE in gladiator.dll -- verified by disassembling the
 * spans our funcmap left unclaimed in l_script's range: 0x1003FC30 is the
 * `rep movsd` + `tokenavailable = 1` of PS_UnreadToken and 0x1003FFF0 is
 * ResetScript's seven field writes plus `rep stos`.  They were briefly gated
 * `#ifndef _WIN32` on the mistaken assumption that anything the ELF audit
 * reported MISSING was Linux-only; the ELF audit says nothing about the DLL,
 * and the check that does is "is there unclaimed .text in this TU's range". */

//----- (1003FC30) --------------------------------------------------------
/* F379 @ 0x00052700 (37 B ELF) / 0x1003FC30 (DLL).  Q3 botlib l_script.c has
 * this verbatim.  The two builds copy a different count -- 0x10b dwords in the
 * ELF against 0x10c in the DLL -- because sizeof(token_t) differs by 4 between
 * them; both are `sizeof(token_t)` in source. */
void __cdecl PS_UnreadToken(script_t *script, token_t *token)
{
  memcpy(&script->token, token, sizeof(token_t));
  script->tokenavailable = 1;
} //end of the function PS_UnreadToken

//----- (1003FFF0) --------------------------------------------------------
/* F387 @ 0x00052a14 (114 B ELF) / 0x1003FFF0 (DLL).  Q3 botlib l_script.c,
 * field for field. */
void __cdecl ResetScript(script_t *script)
{
  script->script_p = script->buffer;
  script->lastscript_p = script->buffer;
  script->whitespace_p = NULL;
  script->endwhitespace_p = NULL;
  script->tokenavailable = 0;
  script->line = 1;
  script->lastline = 1;
  memset(&script->token, 0, sizeof(token_t));
} //end of the function ResetScript
