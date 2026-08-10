/*
 * l_script.h — interface of l_script.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_L_SCRIPT_H
#define BOTLIB_L_SCRIPT_H

#include "botlib_local.h"

BOOL __cdecl EndOfScript(script_t *script);
int __cdecl FileLength(FILE *fp);
void      __cdecl FreeScript(script_t *script);
int __cdecl GetScriptFlags(script_t *script);
script_t *__cdecl LoadScriptFile(char *FileName, int Offset, size_t ElementSize);
script_t *__cdecl LoadScriptMemory(const void *ptr, unsigned int length, const char *name);
int __cdecl NumLinesCrossed(script_t *script);
void __cdecl NumberValue(char *string, int subtype, int *intvalue, double *floatvalue);
int __cdecl PS_CheckTokenString(script_t *script, const char *string);
int __cdecl PS_CheckTokenType(script_t *script, int type, int subtype, token_t *out);
void __cdecl PS_CreatePunctuationTable(script_t *script, punctuation_t *punctuations);
int __cdecl PS_ExpectAnyToken(int script, int token);
int __cdecl PS_ExpectTokenString(script_t *script, const char *string);
int __cdecl PS_ExpectTokenType(script_t *script, int type, int subtype, token_t *token);
char PS_NextWhiteSpaceChar(script_t *script);
int __cdecl PS_ReadEscapeCharacter(script_t *script, _BYTE *ch);
int __cdecl PS_ReadLiteral(script_t *script, token_t *token);
int __cdecl PS_ReadName(script_t *script, intptr_t a2);
int __cdecl PS_ReadNumber(script_t *script, token_t *token);
int __cdecl PS_ReadPrimitive(script_t *script, intptr_t token);
int __cdecl PS_ReadPunctuation(script_t *script, char *token);
int __cdecl PS_ReadString(script_t *script, token_t *token, int quote);
int __cdecl PS_ReadToken(script_t *script, char *Destination);
int __cdecl PS_ReadWhiteSpace(script_t *script);
int __cdecl PS_SkipUntilString(script_t *script, const char *string);
void __cdecl PS_UnreadLastToken(script_t *script);
char *__cdecl PunctuationFromNum(script_t *script, int num);
double __cdecl ReadSignedFloat(int script);
int __cdecl ReadSignedInt(int script);
void ScriptError(int script, char *Format, ...);
int __cdecl ScriptSkipTo(script_t *script, char *value);
void ScriptWarning(int script, char *Format, ...);
void __cdecl SetScriptFlags(script_t *script, int flags);
void __cdecl SetScriptPunctuations(script_t *script, punctuation_t *p);
void __cdecl StripDoubleQuotes(char *string);
void __cdecl StripSingleQuotes(char *string);

#endif /* BOTLIB_L_SCRIPT_H */
