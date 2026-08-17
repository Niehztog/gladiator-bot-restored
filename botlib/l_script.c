/*
 * l_script.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x1003E120..0x10040470.
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
#include "l_script.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_memory.h"
#include "l_utils.h"

/* 72-byte read-only blob whose &[3] is the "You are not allowed to…" text.
 * `static const`, so it lands in .rodata with no symbol -- which is where the
 * original keeps it: gladi386.so reaches it with `lea eax,[ebx-0x23e8]`
 * (.rodata 0x5a6a0, three NULs then the message, immediately after the "rb"
 * fopen mode string at 0x5a69d), not with a GOT load.  Ours had it as a
 * writable exported `char[]` in .data, which is one of the surplus globals
 * dataaudit.py's EXPORTED-but-not-in-real check reports.  (2026-08-17.) */
static const char unk_10060418[72] = {
    0x00, 0x00, 0x00, 0x59, 0x6F, 0x75, 0x20, 0x61, 0x72, 0x65, 0x20, 0x6E, 0x6F, 0x74, 0x20, 0x61,
    0x6C, 0x6C, 0x6F, 0x77, 0x65, 0x64, 0x20, 0x74, 0x6F, 0x0A, 0x6D, 0x6F, 0x64, 0x69, 0x66, 0x79,
    0x20, 0x74, 0x68, 0x65, 0x20, 0x62, 0x6F, 0x74, 0x20, 0x63, 0x68, 0x61, 0x72, 0x61, 0x63, 0x74,
    0x65, 0x72, 0x73, 0x20, 0x69, 0x6E, 0x0A, 0x69, 0x6E, 0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x76,
    0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x2E, 0x00,
};

/* The punctuation_t[] array at VA 0x1005FE00 (52 entries + NULL), identical to Q3's
 * default_punctuations.  PS_CreatePunctuationTable walks it with a 3-slot stride and
 * fills each `next` at runtime.  String literals are const while punctuation_t.p is
 * char* — BOTCFLAGS suppresses that warning. */
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

// gladiator.dll: 1003E120..1003E201
// gladi386.so:   00050DEC..00050F18
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

// gladiator.dll: 1003E250..1003E291
// gladi386.so:   00050F18..00050F60
/* Linear scan of script->punctuations (a contiguous array terminated by a NULL `p`)
 * for the record whose .n matches, returning its string or the original's typo'd
 * "unkown punctuation" default.  DEAD in Gladiator. */
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

// gladiator.dll: 1003E2C0..1003E316
// gladi386.so:   00050F60..00050FC9
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

// gladiator.dll: 1003E340..1003E396
// gladi386.so:   00050FCC..00051035
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

// gladiator.dll: 1003E3C0..1003E3FF
// gladi386.so:   00051038..00051088
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

// gladiator.dll: 1003E410..1003E4D1
// gladi386.so:   00051088..00051197
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

// gladiator.dll: 1003E520..1003E757
// gladi386.so:   00051198..0005148F
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

// gladiator.dll: 1003E7F0..1003E97E
// gladi386.so:   00051490..00051667
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

// gladiator.dll: 1003E9F0..1003EA7E
// gladi386.so:   00051668..000516FC
int __cdecl PS_ReadName(script_t *script, token_t *token)
{
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

// gladiator.dll: 1003EAB0..1003EC5F
// gladi386.so:   000516FC..000518DE
/* Walks `string` through a plain char* cursor.  Subtype flags: 0x800 = float,
 * 0x8 = decimal, 0x100 = hex, 0x200 = octal, 0x400 = binary. */
void __cdecl NumberValue(char *string, int subtype, unsigned int *intvalue, long double *floatvalue)
{
  unsigned int dotfound = 0;

  *intvalue = 0;
  *floatvalue = 0.0;
  if ( (subtype & 0x800) != 0 )
  {
    while ( *string )
    {
      if ( *string == 46 )
      {
        if ( dotfound )
          return;
        dotfound = 10;
        ++string;
      }
      if ( dotfound )
      {
        *floatvalue = (double)(*string - 48) / (double)dotfound + *floatvalue;
        dotfound *= 10;
      }
      else
      {
        *floatvalue = *floatvalue * 10.0 + (long double)(*string - 48);
      }
      ++string;
    }
    *intvalue = (unsigned int)*floatvalue;
  }
  else if ( (subtype & 8) != 0 )
  {
    while ( *string )
      *intvalue = *intvalue * 10 + (*string++ - 48);
    *floatvalue = *intvalue;
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
    *floatvalue = *intvalue;
  }
  else if ( (subtype & 0x200) != 0 )
  {
    ++string;
    while ( *string )
      *intvalue = (*intvalue << 3) + (*string++ - '0');
    *floatvalue = *intvalue;
  }
  else if ( (subtype & 0x400) != 0 )
  {
    string += 2;
    while ( *string )
      *intvalue = (*intvalue << 1) + (*string++ - 48);
    *floatvalue = *intvalue;
  }
}

// gladiator.dll: 1003ECD0..1003EF6E
// gladi386.so:   000518E0..00051BAE
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
  NumberValue(token->string, token->subtype, &token->intvalue, &token->floatvalue);
  if ( (token->subtype & 0x800) == 0 )
    token->subtype |= 0x1000;
  return 1;
}

// gladiator.dll: 1003F020..1003F112
// gladi386.so:   00051BB0..00051CA9
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

// gladiator.dll: 1003F160..1003F1FD
// gladi386.so:   00051CAC..00051D6A
/* Try to read a punctuation token at the script's current position.
 * NB the table index `*script->script_p` is a SIGNED char on purpose — the original
 * sign-extends it.  Q3 later changed this to unsigned; do NOT "fix" the
 * -Wchar-subscripts warning here. */
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

// gladiator.dll: 1003F230..1003F2A2
// gladi386.so:   00051D6C..00051DE4
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

// gladiator.dll: 1003F2D0..1003F45F
// gladi386.so:   00051DE4..000521D7
/* Q3's PS_ReadToken(script_t *, token_t *), both parameters properly typed.  A raw
 * `char *Destination` with a local `token_t *token` alias is an extra stack local
 * caching a parameter, which gcc materialises as its own slot — 4 bytes bigger than
 * the original's frame, shifting every ESP-relative offset in the function. */
int __cdecl PS_ReadToken(script_t *script, token_t *token)
{
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
    if ( !PS_ReadName(script, token) ) return 0;
  }
  else if ( !PS_ReadPunctuation(script, (char *)token) )
  {
    ScriptError((int)script, "can't read token");
    return 0;
  }
  memcpy(&script->token, token, sizeof(token_t));
  return 1;
}

// gladiator.dll: 1003F4D0..1003F581
// gladi386.so:   000521D8..0005226C
/* Read the next token and ScriptError unless its string equals `string`.
 * DEAD in Gladiator. */
int __cdecl PS_ExpectTokenString(script_t *script, const char *string)
{
  token_t token;
  if ( !PS_ReadToken(script, &token) )
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

// gladiator.dll: 1003F5C0..1003F8DF
// gladi386.so:   0005226C..0005252C
int __cdecl PS_ExpectTokenType(script_t *script, int type, int subtype, token_t *token)
{
  char str[1024]; // [esp+10h] [ebp-400h] BYREF

  if ( !PS_ReadToken(script, token) )
  {
    ScriptError(script, "couldn't read expected token");
    return 0;
  }
  if ( token->type != type )
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
    ScriptError(script, "expected a %s, found %s", str, token);
    return 0;
  }
  if ( token->type == 3 )
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
  else if ( token->type == 5 )
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

// gladiator.dll: 1003F9B0..1003F9E0
// gladi386.so:   0005252C..00052570
int __cdecl PS_ExpectAnyToken(int script, int token)
{
  if ( !PS_ReadToken(script, token) )
  {
    ScriptError(script, "couldn't read expected token");
    return 0;
  }
  return 1;
}

// gladiator.dll: 1003F9F0..1003FA73
// gladi386.so:   00052570..000525EC
/* Peek the next token: 1 if its string matches, else rewind script_p from
 * lastscript_p and return 0.  Sibling of PS_CheckTokenType.  DEAD in Gladiator. */
int __cdecl PS_CheckTokenString(script_t *script, const char *string)
{
  token_t token;
  if ( !PS_ReadToken(script, &token) )
    return 0;
  if ( strcmp(token.string, string) == 0 )
    return 1;
  script->script_p = script->lastscript_p;
  return 0;
}

// gladiator.dll: 1003FAB0..1003FB2D
// gladi386.so:   000525EC..00052689
/* Peek the next token: on (type == expected && (subtype & mask) == mask) copy it out
 * and return 1, else rewind script_p and return 0.  Sibling of PS_ExpectTokenType.
 * DEAD in Gladiator. */
int __cdecl PS_CheckTokenType(script_t *script, int type, int subtype, token_t *out)
{
  token_t token;
  if ( !PS_ReadToken(script, &token) )
    return 0;
  if ( token.type == type && (token.subtype & subtype) == subtype )
  {
    memcpy(out, &token, sizeof(token_t));
    return 1;
  }
  script->script_p = script->lastscript_p;
  return 0;
}

// gladiator.dll: 1003FB50..1003FBE0
// gladi386.so:   0005268C..000526EE
/* Read tokens until one equals `string` (1) or the stream ends (0).  Sibling of
 * PC_SkipUntilString.  DEAD in Gladiator. */
int __cdecl PS_SkipUntilString(script_t *script, const char *string)
{
  token_t token;
  while ( PS_ReadToken(script, &token) )
  {
    if ( strcmp(token.string, string) == 0 )
      return 1;
  }
  return 0;
}

// gladiator.dll: 1003FC10..1003FC1F
// gladi386.so:   000526F0..000526FF
/* Sets script->tokenavailable = 1.  DEAD in Gladiator. */
void __cdecl PS_UnreadLastToken(script_t *script)
{
  script->tokenavailable = 1;
}

// gladiator.dll: 1003FC70..1003FC91
// gladi386.so:   00052728..0005274A
/* getc over the script's whitespace span: read one byte, advance the cursor,
 * return 0 at the end.  DEAD in Gladiator. */
char PS_NextWhiteSpaceChar(script_t *script)
{
  if (script->whitespace_p != script->endwhitespace_p)
    return *script->whitespace_p++;
  return 0;
}

// gladiator.dll: 1003FCB0..1003FD1B
// gladi386.so:   0005274C..000527C9
void __cdecl StripDoubleQuotes(char *string)
{
  /* The original uses strcpy() with OVERLAPPING src/dst — a byte copy under 32-bit
   * MSVC and under the vintage i386 glibc/libc5 the 1999 Linux build shipped with,
   * undefined with a modern glibc's SIMD strcpy — so keep strcpy only for the two
   * vintage-i386 oracles (same `__i386__ && !__SSE_MATH__` predicate as
   * be_aas_reach.c's x87-temp gate, and for the same reason: it is instruction-set
   * vintage, not compiler brand) and memmove elsewhere.  The outer `while` (not Q3's
   * single `if`) matches the original's loopback. */
  while ( *string == '"' )
#if defined(_MSC_VER) || (defined(__i386__) && !defined(__SSE_MATH__))
    strcpy(string, string + 1);
#else
    memmove(string, string + 1, strlen(string));
#endif
  while ( string[strlen(string) - 1] == '"' )
    string[strlen(string) - 1] = '\0';
}

// gladiator.dll: 1003FD40..1003FDAB
// gladi386.so:   000527CC..00052849
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

// gladiator.dll: 1003FDD0..1003FE8A
// gladi386.so:   0005284C..0005292A
// Signed float reader: PS_ExpectAnyToken; if token == "-" set sign=-1.0 and read
// another token (PS_ExpectTokenType with type=TT_NUMBER, mask=0); else require
// token.type == TT_NUMBER, ScriptError'ing with "expected float value, found %s\n".
// Returns token.floatvalue * sign as a double.  The sign is constructed as a double
// via two int half-writes (0|0x3ff00000 for +1.0, 0|0xbff00000 for -1.0), exactly as
// the MSVC frontend would emit.  DEAD in Gladiator — /INCREMENTAL.
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

// gladiator.dll: 1003FEC0..1003FF77
// gladi386.so:   0005292C..000529F6
// Signed integer reader: PS_ExpectAnyToken; if token == "-" set sign=-1 and read
// another token (PS_ExpectTokenType with type=TT_NUMBER, mask=0x1000); else require
// token.type == TT_NUMBER and reject the float subtype (0x800), ScriptError'ing with
// "expected integer value, found %s\n".  Returns token.intvalue * sign as int.
// DEAD in Gladiator — /INCREMENTAL.
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

// gladiator.dll: 1003FFB0..1003FFBF
// gladi386.so:   000529F8..00052A07
void __cdecl SetScriptFlags(script_t *script, int flags)
{
  script->flags = flags;
}

// gladiator.dll: 1003FFD0..1003FFDB
// gladi386.so:   00052A08..00052A13
/* Returns script->flags.  DEAD in Gladiator — the live API uses the dedicated
 * accessors instead. */
int __cdecl GetScriptFlags(script_t *script)
{
  return script->flags;
}

// gladiator.dll: 10040060..10040076
// gladi386.so:   00052A88..00052AA1
/* Q3 l_script.c:1242: int EndOfScript(script_t *script). */
BOOL __cdecl EndOfScript(script_t *script)
{
  return script->script_p >= script->end_p;
}

// gladiator.dll: 10040090..100400A3
// gladi386.so:   00052AA4..00052AB7
/* Returns script->line - script->lastline.  DEAD in Gladiator. */
int __cdecl NumLinesCrossed(script_t *script)
{
  return script->line - script->lastline;
}

// gladiator.dll: 100400C0..1004012E
// gladi386.so:   00052AB8..00052B3C
/* Character-level companion to the token-based PS_SkipUntilString: scan the raw
 * script stream, skipping whitespace between probes, for an occurrence of `value`
 * anchored on its first byte.  No Q3 counterpart.
 *
 * Keep the single `while (PS_ReadWhiteSpace(...))`, which lets MSVC rotate the loop as
 * the original does; a leading guard plus while(1) emits an extra jmp.
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

// gladiator.dll: 10040150..10040183
// gladi386.so:   00052B3C..00052B7D
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

// gladiator.dll: 100401A0..10040320
// gladi386.so:   00052B80..00052D97
/* Raw script file loader: create a script_t (1392-byte header + file data) in one
 * allocation.  Does NOT set up a source_t or scriptstack — LoadSourceFile does that
 * around this. */
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
  script->length       = length;
  script->script_p     = script->buffer;
  script->lastscript_p = script->buffer;
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

// gladiator.dll: 10040380..10040437
// gladi386.so:   00052D98..00052E67
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

// gladiator.dll: 10040470..10040493
// gladi386.so:   00052E68..00052E99
void __cdecl FreeScript(script_t *script)
{
  if ( script->punctuationtable )
    FreeMemory(script->punctuationtable);
  FreeMemory(script);
}

/* The nine structure read/write functions live in their own TU: botlib/l_struct.c
 * (DLL 0x100404B0..0x1004123F). */

/* Both of the following ARE in gladiator.dll — 0x1003FC30 is the `rep movsd` +
 * `tokenavailable = 1` of PS_UnreadToken and 0x1003FFF0 is ResetScript's seven field
 * writes plus `rep stos`.  The ELF audit says nothing about the DLL; the check that
 * does is "is there unclaimed .text in this TU's range". */

// gladiator.dll: 1003FC30..1003FC54
// gladi386.so:   00052700..00052725
/* F379 @ 0x00052700 (37 B ELF) / 0x1003FC30 (DLL).  Q3 has this verbatim.  The two
 * builds copy a different count — 0x10b dwords in the ELF against 0x10c in the DLL —
 * because sizeof(token_t) differs by 4 between them; both are `sizeof(token_t)` in
 * source. */
void __cdecl PS_UnreadToken(script_t *script, token_t *token)
{
  memcpy(&script->token, token, sizeof(token_t));
  script->tokenavailable = 1;
} //end of the function PS_UnreadToken

// gladiator.dll: 1003FFF0..1004003D
// gladi386.so:   00052A14..00052A86
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
