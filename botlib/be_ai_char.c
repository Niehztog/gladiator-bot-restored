/*
 * be_ai_char.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10029E10..0x1002A810; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * botlib_local.h carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs and every prototype), so this file compiles in
 * exactly the environment these functions had before the split.
 */

#include "botlib_local.h"

//----- (10029E10) --------------------------------------------------------
/* Print each characteristic with its index and a type-specific format
 * (tag 1 = int, 2 = float, 3 = string).  Companion to the named-value list
 * dumpers at 1002B070 / 1002B900.  DEAD in Gladiator. */
void __cdecl BotDumpCharacter(bot_character_t *ch)
{
  int   i;
  char *p;
  Log_Write("{");
  i = 0;
  if ( ch->numcharacteristics > 0 )
  {
    /* The walk is biased to the VALUE field, reading the type at [edi-4], which is
     * what makes three of the four accesses short `[edi]` forms; a struct-pointer
     * walk over p->type / p->value biases to the pair base instead and costs
     * ~124 bytes.  Bias and stride come from the struct, not literals —
     * bot_characteristic_t is 16 bytes on 64-bit. */
    p = (char *)&BC_PAIRS(ch)[0].value;
    do
    {
      int type = (signed char)p[-(int)offsetof(bot_characteristic_t, value)];
      switch ( type )
      {
        case 1:
          Log_Write(" %4d %d", i, *(int *)p);
          break;
        case 2:
          Log_Write(" %4d %f", i, *(float *)p);
          break;
        case 3:
          Log_Write(" %4d %s", i, *(char **)p);
          break;
      }
      ++i;
      p += sizeof(bot_characteristic_t);
    }
    while ( i < ch->numcharacteristics );
  }
  Log_Write("}");
}
//----- (10029EB0) --------------------------------------------------------
bot_character_t *__cdecl BotLoadCharacter(char *charfile, const char *a2)
{
  bot_character_t *ch; // ebx
  int index;
  int indent;
  source_t *source; // [esp+10h] [ebp-5E4h]
  int numchars; // [esp+14h] [ebp-5E0h]
  int stringbytes; // [esp+18h] [ebp-5DCh]
  int pass; // [esp+1Ch] [ebp-5D8h]
  int foundcharacter; // [esp+20h] [ebp-5D4h]
  char *strptr; // [esp+24h] [ebp-5D0h]
  bot_fileref_t file_ref;
  token_t token;
  char Destination[260];

  ch = 0;
  strncpy(Destination, charfile, 0x104u);
  if ( !sub_10041F60(Destination, &file_ref) )
  {
    botimport.Print(PRT_ERROR, "couldn't find %s\n", Destination);
    return 0;
  }
  for ( pass = 0; pass < 2; pass++ )
  {
    source = LoadSourceFile(file_ref.path, file_ref.fileofs, file_ref.filelen);
    if ( !source )
    {
      botimport.Print(PRT_ERROR, "counldn't load %s\n", file_ref.path);
      return 0;
    }
    stringbytes = 0;
    numchars = 0;
    foundcharacter = 0;
    while ( PC_ReadTokenHandle(source, token.string) )
    {
      if ( !strcmp(token.string, "character") )
      {
        if ( !PC_ExpectTokenType(source, 1, 0, token.string) )
        {
          FreeSource(source);
          return 0;
        }
        StripDoubleQuotes(token.string);
        if ( !PC_ExpectTokenString(source, "{") )
        {
          FreeSource(source);
          return 0;
        }
        if ( !strcmp(token.string, a2) )
        {
          foundcharacter = 1;
          while ( PC_ExpectAnyToken(source, token.string) )
          {
            if ( !strcmp(token.string, "}") )
              break;
            if ( token.type != 3 || (token.subtype & 0x1000) == 0 )
            {
              SourceError(source, "expected integer index, found %s\n", token.string);
              FreeSource(source);
              return 0;
            }
            index = token.intvalue;
            if ( index > numchars )
              numchars = index;
            if ( pass && BC_PAIRS(ch)[token.intvalue].type )
            {
              SourceError(source, "characteristic %d already initialized\n", token.intvalue);
              FreeSource(source);
              return 0;
            }
            if ( !PC_ExpectAnyToken(source, token.string) )
            {
              FreeSource(source);
              return 0;
            }
            if ( token.type == 3 )
            {
              if ( pass )
              {
                if ( (token.subtype & 0x800) != 0 )
                {
                  *(float *)&BC_PAIRS(ch)[index].value = token.floatvalue;
                  BC_PAIRS(ch)[index].type = 2;
                }
                else
                {
                  BC_PAIRS(ch)[index].value = (intptr_t)token.intvalue;
                  BC_PAIRS(ch)[index].type = 1;
                }
              }
            }
            else
            {
              if ( token.type != 1 )
              {
                SourceError(source,
                            "expected integer, float or string, found %s\n",
                            token.string);
                FreeSource(source);
                return 0;
              }
              StripDoubleQuotes(token.string);
              if ( pass )
              {
                strcpy(strptr, token.string);
                BC_PAIRS(ch)[index].value = (intptr_t)strptr;
                BC_PAIRS(ch)[index].type = 3;
                strptr += strlen(token.string) + 1;
              }
              else
              {
                stringbytes += strlen(token.string) + 1;
              }
            }
          }
        }
        else
        {
          indent = 1;
          while ( 1 )
          {
            if ( !PC_ExpectAnyToken(source, token.string) )
            {
              FreeSource(source);
              return 0;
            }
            if ( !strcmp(token.string, "{") )
              ++indent;
            else if ( !strcmp(token.string, "}") )
              --indent;
            if ( !indent )
              break;
          }
        }
      }
      else
      {
        SourceError(source, "unknown definition %s\n", token.string);
        FreeSource(source);
        return 0;
      }
    }
    FreeSource(source);
    if ( !foundcharacter )
    {
      botimport.Print(PRT_ERROR, "couldn't find character %s in %s\n", a2, file_ref.path);
      return 0;
    }
    if ( !pass )
    {
      /* The original allocates 12 + 8*numchars + strings: a 4-byte count, one
       * extra pair slot, then the string tail. */
      ch = (bot_character_t *)GetClearedMemory(
        stringbytes
        + sizeof(bot_characteristic_t) * numchars
        + ((sizeof(bot_character_t) + sizeof(intptr_t) - 1) & ~(sizeof(intptr_t) - 1))
        + sizeof(bot_characteristic_t));
      ch->numcharacteristics = numchars;
      strptr = (char *)&BC_PAIRS(ch)[numchars + 1];
    }
  }
  if ( file_ref.filelen )
    botimport.Print(PRT_MESSAGE, "loaded %s from %s\\%s\n", a2, file_ref.path, Destination);
  else
    botimport.Print(PRT_MESSAGE, "loaded %s from %s\n", a2, Destination);
  return ch;
}
//----- (1002A590) --------------------------------------------------------
void __cdecl sub_1002A590(int a1)
{
  FreeMemory(a1);
}
//----- (1002A5B0) --------------------------------------------------------
int __cdecl CheckCharacteristicIndex(bot_character_t *character, int index)
{
  if ( index >= 0 && index < character->numcharacteristics )
  {
    if ( !(unsigned char)BC_PAIRS(character)[index].type )
    {
      botimport.Print(PRT_ERROR, "characteristic %d is not initialized\n", index);
      return 0;
    }
    return 1;
  }
  botimport.Print(PRT_ERROR, "characteristic %d does not exist\n", index);
  return 0;
}
//----- (1002A620) --------------------------------------------------------
float __cdecl Characteristic_Float(bot_character_t *character, int index)
{
  char v2; // al

  if ( CheckCharacteristicIndex(character, index) )
  {
    v2 = (char)BC_PAIRS(character)[index].type;
    if ( v2 == 1 )
      return (float)(int)BC_PAIRS(character)[index].value;
    if ( v2 == 2 )
      return *(float *)&BC_PAIRS(character)[index].value;
    botimport.Print(PRT_ERROR, "characteristic %d is not a float\n", index);
  }
  return 0.0f;
}
//----- (1002A690) --------------------------------------------------------
float __cdecl Characteristic_BFloat(bot_character_t *character, int index, float min, float max)
{
  float result; // st7 — returns float (disasm loads fld DWORD / fcom DWORD,
                // not the double-promotion sequence); Q3's is float too.

  if ( min > (float)max )
  {
    botimport.Print(PRT_ERROR, "cannot bound characteristic %d between %f and %f\n", index, min, max);
    return 0.0f;
  }
  result = Characteristic_Float(character, index);
  if ( result < min )
    return min;
  if ( result > max )
    return max;
  return result;
}
//----- (1002A730) --------------------------------------------------------
int __cdecl Characteristic_Integer(bot_character_t *character, int index)
{
  char v2; // al

  if ( CheckCharacteristicIndex(character, index) )
  {
    v2 = (char)BC_PAIRS(character)[index].type;
    if ( v2 == 1 )
      return (int)BC_PAIRS(character)[index].value;
    else if ( v2 == 2 )
      return (__int64)*(float *)&BC_PAIRS(character)[index].value;
    else
      botimport.Print(PRT_ERROR, "characteristic %d is not a integer\n", index);
  }
  return 0;
}
//----- (1002A7A0) --------------------------------------------------------
int __cdecl Characteristic_BInteger(bot_character_t *character, int index, int min, int max)
{
  int result; // eax

  if ( min > max )
  {
    botimport.Print(PRT_ERROR, "cannot bound characteristic %d between %d and %d\n", index, min, max);
    return 0;
  }
  result = Characteristic_Integer(character, index);
  if ( result < min )
    return min;
  if ( result > max )
    return max;
  return result;
}
//----- (1002A810) --------------------------------------------------------
char *__cdecl Characteristic_String(bot_character_t *character, int index)
{
  if ( !CheckCharacteristicIndex(character, index) )
    return &byte_1006294C;
  if ( (unsigned char)BC_PAIRS(character)[index].type == 3 )
    return (char *)BC_PAIRS(character)[index].value;
  botimport.Print(PRT_ERROR, "characteristic %d is not a string\n", index);
  return 0;
}
