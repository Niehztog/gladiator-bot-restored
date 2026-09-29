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

/* The punctuation_t[] array at VA 0x1005FE00, Q3's default_punctuations entry for
 * entry, so its text is Q3's; the '$' entry is unconditional, as Gladiator has no
 * DOLLAR switch.  PS_CreatePunctuationTable walks it with a 3-slot stride and fills
 * each `next` at runtime.  String literals are const while punctuation_t.p is
 * char* — BOTCFLAGS suppresses that warning. */
punctuation_t default_punctuations[] =
{
	//binary operators
	{">>=",P_RSHIFT_ASSIGN, NULL},
	{"<<=",P_LSHIFT_ASSIGN, NULL},
	//
	{"...",P_PARMS, NULL},
	//define merge operator
	{"##",P_PRECOMPMERGE, NULL},
	//logic operators
	{"&&",P_LOGIC_AND, NULL},
	{"||",P_LOGIC_OR, NULL},
	{">=",P_LOGIC_GEQ, NULL},
	{"<=",P_LOGIC_LEQ, NULL},
	{"==",P_LOGIC_EQ, NULL},
	{"!=",P_LOGIC_UNEQ, NULL},
	//arithmatic operators
	{"*=",P_MUL_ASSIGN, NULL},
	{"/=",P_DIV_ASSIGN, NULL},
	{"%=",P_MOD_ASSIGN, NULL},
	{"+=",P_ADD_ASSIGN, NULL},
	{"-=",P_SUB_ASSIGN, NULL},
	{"++",P_INC, NULL},
	{"--",P_DEC, NULL},
	//binary operators
	{"&=",P_BIN_AND_ASSIGN, NULL},
	{"|=",P_BIN_OR_ASSIGN, NULL},
	{"^=",P_BIN_XOR_ASSIGN, NULL},
	{">>",P_RSHIFT, NULL},
	{"<<",P_LSHIFT, NULL},
	//reference operators
	{"->",P_POINTERREF, NULL},
	//C++
	{"::",P_CPP1, NULL},
	{".*",P_CPP2, NULL},
	//arithmatic operators
	{"*",P_MUL, NULL},
	{"/",P_DIV, NULL},
	{"%",P_MOD, NULL},
	{"+",P_ADD, NULL},
	{"-",P_SUB, NULL},
	{"=",P_ASSIGN, NULL},
	//binary operators
	{"&",P_BIN_AND, NULL},
	{"|",P_BIN_OR, NULL},
	{"^",P_BIN_XOR, NULL},
	{"~",P_BIN_NOT, NULL},
	//logic operators
	{"!",P_LOGIC_NOT, NULL},
	{">",P_LOGIC_GREATER, NULL},
	{"<",P_LOGIC_LESS, NULL},
	//reference operator
	{".",P_REF, NULL},
	//seperators
	{",",P_COMMA, NULL},
	{";",P_SEMICOLON, NULL},
	//label indication
	{":",P_COLON, NULL},
	//if statement
	{"?",P_QUESTIONMARK, NULL},
	//embracements
	{"(",P_PARENTHESESOPEN, NULL},
	{")",P_PARENTHESESCLOSE, NULL},
	{"{",P_BRACEOPEN, NULL},
	{"}",P_BRACECLOSE, NULL},
	{"[",P_SQBRACKETOPEN, NULL},
	{"]",P_SQBRACKETCLOSE, NULL},
	//
	{"\\",P_BACKSLASH, NULL},
	//precompiler operator
	{"#",P_PRECOMP, NULL},
	{"$",P_DOLLAR, NULL},
	{NULL, 0}
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
void ScriptError(script_t *script, char *Format, ...)
{
  char Buffer[1024]; // [esp+4h] [ebp-400h] BYREF
  va_list va; // [esp+410h] [ebp+Ch] BYREF

  if ( script->flags & SCFL_NOERRORS )
    return;
  va_start(va, Format);
  vsprintf(Buffer, Format, va);
  botimport.Print(PRT_ERROR, "file %s, line %d: %s\n", script->filename, script->line, Buffer);
}

// gladiator.dll: 1003E340..1003E396
// gladi386.so:   00050FCC..00051035
void ScriptWarning(script_t *script, char *Format, ...)
{
  char Buffer[1024]; // [esp+4h] [ebp-400h] BYREF
  va_list va; // [esp+410h] [ebp+Ch] BYREF

  if ( script->flags & SCFL_NOWARNINGS )
    return;
  va_start(va, Format);
  vsprintf(Buffer, Format, va);
  botimport.Print(PRT_WARNING, "file %s, line %d: %s\n", script->filename, script->line, Buffer);
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
    token->type = TT_STRING;
  else
    token->type = TT_LITERAL;
  len = 0;
  token->string[len++] = *script->script_p++;
  while ( 1 )
  {
    if ( len >= MAX_TOKEN - 2 )
    {
      ScriptError(script, "string longer than MAX_TOKEN = %d", MAX_TOKEN);
      return 0;
    }
    if ( *script->script_p == 92 && (script->flags & SCFL_NOSTRINGESCAPECHARS) == 0 )
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
      if ( (script->flags & SCFL_NOSTRINGWHITESPACES) != 0 )
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

  token->type = TT_NAME;
  do
  {
    token->string[len++] = *script->script_p++;
    if ( len >= MAX_TOKEN )
    {
      ScriptError(script, "name longer than MAX_TOKEN = %d", MAX_TOKEN);
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
/* Walks `string` through a plain char* cursor. */
void __cdecl NumberValue(char *string, int subtype, unsigned int *intvalue, long double *floatvalue)
{
  unsigned int dotfound = 0;

  *intvalue = 0;
  *floatvalue = 0.0;
  if ( (subtype & TT_FLOAT) != 0 )
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
  else if ( (subtype & TT_DECIMAL) != 0 )
  {
    while ( *string )
      *intvalue = *intvalue * 10 + (*string++ - 48);
    *floatvalue = *intvalue;
  }
  else if ( (subtype & TT_HEX) != 0 )
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
  else if ( (subtype & TT_OCTAL) != 0 )
  {
    ++string;
    while ( *string )
      *intvalue = (*intvalue << 3) + (*string++ - '0');
    *floatvalue = *intvalue;
  }
  else if ( (subtype & TT_BINARY) != 0 )
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

  token->type = TT_NUMBER;
  if ( *script->script_p == 48 && (script->script_p[1] == 120 || script->script_p[1] == 88) )
  {
    token->string[len++] = *script->script_p++;
    token->string[len++] = *script->script_p++;
    c = *script->script_p;
    /* hexadecimal — NB the faithful Gladiator quirk: uppercase digits admit only 'A' (65..65) */
    while ( (c >= 48 && c <= 57) || (c >= 97 && c <= 102) || (c >= 65 && c <= 65) )
    {
      token->string[len++] = *script->script_p++;
      if ( len >= MAX_TOKEN )
      {
        ScriptError(script, "hexadecimal number longer than MAX_TOKEN = %d",
                    MAX_TOKEN);
        return 0;
      }
      c = *script->script_p;
    }
    token->subtype |= TT_HEX;
  }
  else if ( *script->script_p == 48 && (script->script_p[1] == 98 || script->script_p[1] == 66) )
  {
    token->string[len++] = *script->script_p++;
    token->string[len++] = *script->script_p++;
    c = *script->script_p;
    while ( c == 48 || c == 49 )
    {
      token->string[len++] = *script->script_p++;
      if ( len >= MAX_TOKEN )
      {
        ScriptError(script, "binary number longer than MAX_TOKEN = %d", MAX_TOKEN);
        return 0;
      }
      c = *script->script_p;
    }
    token->subtype |= TT_BINARY;
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
      if ( len >= MAX_TOKEN )
      {
        ScriptError(script, "number longer than MAX_TOKEN = %d", MAX_TOKEN);
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
      token->subtype |= TT_OCTAL;
    else
      token->subtype |= TT_DECIMAL;
    if ( dot )
      token->subtype |= TT_FLOAT;
  }
  for ( i = 0; i < 2; i++ )
  {
    c = *script->script_p;
    /* Faithful precedence bug: C binds && tighter than ||, so lowercase 'l'/'u'
     * are consumed unguarded while only 'L'/'U' test the already-set flag.  The
     * asymmetry is in both 1999 binaries (the 'l' path at 1003eed4 jumps straight
     * to the consume-and-set block; the 'L' path at 1003ed9..1003eee4 inserts the
     * test first; same shape for 'u' at 1003eeeb vs 'U' at 1003eef2..1003eefb).
     * Q3's l_script.c:709 parenthesises it; GLAD_SERVERFIX(script-lu-suffix-guard)
     * builds that form. */
#if GLAD_SERVERFIX /* GLAD_SERVERFIX(script-lu-suffix-guard) */
    if ( (c == 108 || c == 76) && (token->subtype & TT_LONG) == 0 )
#else
    if ( c == 108 || c == 76 && (token->subtype & TT_LONG) == 0 )
#endif
    {
      script->script_p++;
      token->subtype |= TT_LONG;
    }
#if GLAD_SERVERFIX /* GLAD_SERVERFIX(script-lu-suffix-guard) */
    else if ( (c == 117 || c == 85) && (token->subtype & (TT_UNSIGNED | TT_FLOAT)) == 0 )
#else
    else if ( c == 117 || c == 85 && (token->subtype & (TT_UNSIGNED | TT_FLOAT)) == 0 )
#endif
    {
      script->script_p++;
      token->subtype |= TT_UNSIGNED;
    }
  }
  token->string[len] = 0;
  NumberValue(token->string, token->subtype, &token->intvalue, &token->floatvalue);
  if ( (token->subtype & TT_FLOAT) == 0 )
    token->subtype |= TT_INTEGER;
  return 1;
}

// gladiator.dll: 1003F020..1003F112
// gladi386.so:   00051BB0..00051CA9
/* Q3's PS_ReadCharacterLiteral — read a 'x' or '\\n'-style character constant
 * and store its value in token.subtype.  DEAD in Gladiator. */
int __cdecl PS_ReadLiteral(script_t *script, token_t *token)
{
  token->type = TT_LITERAL;
  token->string[0] = *script->script_p++;
  if ( !*script->script_p )
  {
    ScriptError(script, "end of file before trailing \'");
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
    ScriptWarning(script, "too many characters in literal, ignored");
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
int __cdecl PS_ReadPunctuation(script_t *script, token_t *token)
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
        strncpy(token->string, p, MAX_TOKEN);
        script->script_p += len;
        token->type = TT_PUNCTUATION;
        //sub type is the number of the punctuation
        token->subtype = punc->n;
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
    if ( len >= MAX_TOKEN )
    {
      ScriptError(script, "primitive token longer than MAX_TOKEN = %d", MAX_TOKEN);
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
  else if ( script->flags & SCFL_PRIMITIVE )
  {
    return PS_ReadPrimitive(script, token);
  }
  else if ( (*script->script_p >= 'a' && *script->script_p <= 'z')
       || (*script->script_p >= 'A' && *script->script_p <= 'Z')
       || *script->script_p == '_' )
  {
    if ( !PS_ReadName(script, token) ) return 0;
  }
  else if ( !PS_ReadPunctuation(script, token) )
  {
    ScriptError(script, "can't read token");
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
    ScriptError(script, "couldn't find expected %s", string);
    return 0;
  }
  if ( strcmp(token.string, string) != 0 )
  {
    ScriptError(script, "expected %s, found %s", string, token.string);
    return 0;
  }
  return 1;
}

// gladiator.dll: 1003F5C0..1003F8DF
// gladi386.so:   0005226C..0005252C
int __cdecl PS_ExpectTokenType(script_t *script, int type, int subtype, token_t *token)
{
  char str[MAX_TOKEN]; // [esp+10h] [ebp-400h] BYREF

  if ( !PS_ReadToken(script, token) )
  {
    ScriptError(script, "couldn't read expected token");
    return 0;
  }
  if ( token->type != type )
  {
    if ( type == TT_STRING )
      strcpy(str, "string");
    if ( type == TT_LITERAL )
      strcpy(str, "literal");
    if ( type == TT_NUMBER )
      strcpy(str, "number");
    if ( type == TT_NAME )
      strcpy(str, "name");
    if ( type == TT_PUNCTUATION )
      strcpy(str, "punctuation");
    ScriptError(script, "expected a %s, found %s", str, token);
    return 0;
  }
  if ( token->type == TT_NUMBER )
  {
    if ( (token->subtype & subtype) != subtype )
    {
      if ( (subtype & TT_DECIMAL) != 0 )
        strcpy(str, "decimal");
      if ( (subtype & TT_HEX) != 0 )
        strcpy(str, "hex");
      if ( (subtype & TT_OCTAL) != 0 )
        strcpy(str, "octal");
      if ( (subtype & TT_BINARY) != 0 )
        strcpy(str, "binary");
      if ( (subtype & TT_LONG) != 0 )
        strcat(str, " long");
      if ( (subtype & TT_UNSIGNED) != 0 )
        strcat(str, " unsigned");
      if ( (subtype & TT_FLOAT) != 0 )
        strcat(str, " float");
      if ( (subtype & TT_INTEGER) != 0 )
        strcat(str, " integer");
      ScriptError(script, "expected %s, found %s", str, token);
      return 0;
    }
  }
  else if ( token->type == TT_PUNCTUATION )
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
int __cdecl PS_ExpectAnyToken(script_t *script, token_t *token)
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
long double __cdecl ReadSignedFloat(script_t *script)
{
  long double sign;
  token_t token;

  sign = 1.0;
  PS_ExpectAnyToken(script, &token);
  if ( !strcmp(token.string, "-") )
  {
    sign = -1.0;
    PS_ExpectTokenType(script, TT_NUMBER, 0, &token);
  }
  else if ( token.type != TT_NUMBER )
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
int __cdecl ReadSignedInt(script_t *script)
{
  int sign;
  token_t token;

  sign = 1;
  PS_ExpectAnyToken(script, &token);
  if ( !strcmp(token.string, "-") )
  {
    sign = -1;
    PS_ExpectTokenType(script, TT_NUMBER, TT_INTEGER, &token);
  }
  else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT )
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
script_t *__cdecl LoadScriptFile(char *FileName, int Offset, size_t length)
{
  FILE *fp;
  script_t *script;

  fp = fopen(FileName, "rb");
  if ( !fp )
    return NULL;
  if ( Offset )
    fseek(fp, Offset, 0);
  /* the PARAMETER, mutated in place (IDA's shadow `length = ElementSize` was
   * its view of that): gladi386.so copies it into edi at entry, which gcc 2.7
   * does only for a parameter the function assigns to. */
  if ( !length )
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
    /* An initialised automatic array, printed from +3 past three leading NULs
     * (AINode_Stand's "I never hacked your brain" uses the same trick).  Wrapped
     * one level deeper, [1][144], because the two 1999 compilers expand the
     * initializer differently only for an AGGREGATE: cl.exe copies the 72-byte
     * literal with `rep movsd` and zero-fills the other 72 whichever way it is
     * written, while gcc 2.7 copies a zero-padded 144-byte template in one
     * block for an aggregate but copies-then-clears for a plain `char buf[144]
     * = "..."`.  ([2][72] and a one-member struct measure the same on both.)
     * The earlier reading, that the two revisions spelled this differently,
     * only knew the plain-array and char-brace-list forms. */
    char buf[1][144] = {"\0\0\0You are not allowed to\nmodify the bot characters in\nin this version."};

    if ( !sub_10037850(FileName, script->buffer, script->length) )
    {
      LibVar("__squatt", "1");
      botimport.Print(PRT_EXIT, &buf[0][3]);
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
