/*
 * l_precomp.h — interface of l_precomp.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes botlib_local.h so it is self-contained: the declarations below
 * reference types that live there, and a caller should never have to care in
 * which order it includes botlib headers.
 */
#ifndef BOTLIB_L_PRECOMP_H
#define BOTLIB_L_PRECOMP_H

#include "botlib_local.h"

void      __cdecl FreeSource(source_t *source);
source_t *__cdecl LoadSourceFile(char *Source, int Offset, size_t ElementSize);
source_t *__cdecl LoadSourceMemory(char *ptr, int length, char *name);
void __cdecl PC_AddBuiltinDefines(source_t *source);
int __cdecl PC_AddDefine(source_t *source, const char *string);
unsigned int __cdecl PC_AddDefineToHash(define_t *define, define_t **definehash);
int __cdecl PC_AddGlobalDefine(const char *string);
void __cdecl PC_AddGlobalDefinesToSource(source_t *source);
int __cdecl PC_CheckTokenString(source_t *source, const char *string);
int __cdecl PC_CheckTokenType(source_t *source, int type, int subtype, token_t *token);
token_t *__cdecl PC_ClearTokenWhiteSpace(token_t *token);
void __cdecl PC_ConvertPath(char *path);
define_t *__cdecl PC_CopyDefine(define_t *define);
token_t *__cdecl PC_CopyToken(const token_t *token);
define_t *__cdecl PC_DefineFromString(const char *string);
int __cdecl PC_Directive_define(source_t *source);
int __cdecl PC_Directive_elif(source_t *source);
int __cdecl PC_Directive_else(source_t *source);
int __cdecl PC_Directive_endif(source_t *source);
int __cdecl PC_Directive_error(source_t *source);
int __cdecl PC_Directive_eval(source_t *source);
int __cdecl PC_Directive_evalfloat(source_t *source);
int __cdecl PC_Directive_if(source_t *source);
int __cdecl PC_Directive_ifdef(source_t *src, int type);
int __cdecl PC_Directive_include(source_t *source);
int __cdecl PC_Directive_line(source_t *source);
int __cdecl PC_Directive_pragma(source_t *source);
int __cdecl PC_Directive_undef(source_t *source);
int __cdecl PC_DollarDirective_evalfloat(source_t *source);
int __cdecl PC_DollarDirective_evalint(source_t *source);
int __cdecl PC_DollarEvaluate(source_t *source, int *intvalue, double *floatvalue, int integer);
int __cdecl PC_Evaluate(source_t *source, int *intvalue, double *floatvalue, int integer);
int __cdecl PC_EvaluateTokens(source_t *source, token_t *tokens, int *intvalue, double *floatvalue, int integer);
int __cdecl PC_ExpandBuiltinDefine(source_t *src, define_t *define, char **a3, char **a4);
int __cdecl PC_ExpandDefine(source_t *src, define_t *define, char **firsttoken, char **lasttoken);
int __cdecl PC_ExpandDefineIntoSource(source_t *src, define_t *define);
int __cdecl PC_ExpectAnyToken(source_t *source, intptr_t token);
int __cdecl PC_ExpectTokenString(source_t *source, const char *string);
int __cdecl PC_ExpectTokenType(source_t *source, int type, int subtype, intptr_t token);
int __cdecl PC_FindDefine(define_t *defines, const char *name);
int __cdecl PC_FindDefineParm(define_t *define, const char *name);
define_t *__cdecl PC_FindHashedDefine(define_t **definehash, const char *name);
void __cdecl PC_FreeDefine(define_t *define);
void __cdecl PC_FreeToken(token_t *token);
int __cdecl PC_MergeTokens(token_t *t1, token_t *t2);
unsigned int __cdecl PC_NameHash(const char *name);
int __cdecl PC_OperatorPriority(int op);
indent_t *__cdecl PC_PopIndent(source_t *source, int *type, int *skip);
indent_t *__cdecl PC_PushIndent(source_t *source, int type, int skip);
void __cdecl PC_PushScript(source_t *source, script_t *script);
int __cdecl PC_ReadDefineParms(source_t *source, define_t *define, token_t **parms, int maxparms);
int __cdecl PC_ReadDirective(source_t *source);
int __cdecl PC_ReadDollarDirective(source_t *source);
int __cdecl PC_ReadLine(source_t *source, token_t *token);
int __cdecl PC_ReadSourceToken(source_t *source, token_t *token);
int __cdecl PC_ReadTokenHandle(source_t *source, _DWORD *pc_token);
void __cdecl PC_RemoveAllGlobalDefines(void);
int __cdecl PC_RemoveGlobalDefine(const char *name);
void __cdecl PC_SetIncludePath(source_t *source, char *path);
void __cdecl PC_SetPunctuations(void *source, int p);
int __cdecl PC_SkipUntilString(source_t *source, char *string);
int __cdecl PC_StringizeTokens(token_t *tokens, token_t *token);
int __cdecl PC_UnreadLastToken(source_t *source);
int __cdecl PC_UnreadSourceToken(source_t *source, const void *token);
void __cdecl PC_UnreadToken(source_t *source, token_t *token);
BOOL __cdecl PC_WhiteSpaceBeforeToken(token_t *token);
int SourceError(source_t *src, char *Format, ...);
int SourceWarning(source_t *src, char *Format, ...);
int __cdecl UnreadSignToken(source_t *source);

#endif /* BOTLIB_L_PRECOMP_H */
