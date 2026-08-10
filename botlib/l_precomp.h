/*
 * l_precomp.h — interface of l_precomp.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_L_PRECOMP_H
#define BOTLIB_L_PRECOMP_H

/* The preprocessor's own types, next to its interface -- Q3 keeps them in l_precomp.h */
/* indent_t — layout as Q3. */
typedef struct indent_s {
    int                    type;    /* +0  INDENT_IF / IFDEF / ELSE / ELIF / ... */
    int                    skip;    /* +4  > 0: skipping until matching #endif   */
    struct script_s       *script;  /* +8  script that opened this indent        */
    struct indent_s       *next;    /* +12 next on indent stack                  */
} indent_t;                         /* sizeof = 16 */

/* define_t — layout as Q3 l_precomp.h::define_t. */
typedef struct define_s {
    char                  *name;    /* +0  macro name                              */
    int                    flags;   /* +4  DEFINE_FIXED etc.                       */
    int                    builtin; /* +8  > 0 for built-in defines (__LINE__ etc.) */
    int                    numparms;/* +12 number of parameter tokens              */
    struct token_s *parms;/* +16 parameter token list                    */
    struct token_s *tokens;/* +20 macro body token list                   */
    struct define_s       *next;    /* +24 hash-chain next                          */
    struct define_s       *hashnext;/* +28                                        */
} define_t;                         /* sizeof = 32 */

/* PC_EvaluateTokens / PC_DollarEvaluate value- and operator-cell lists.
 * Q3's l_precomp.c shape but with `double floatvalue`.  Allocate with
 * sizeof() — the original's fixed 32/24 bytes only hold on 32-bit. */
typedef struct value_s {
    int intvalue;                /* +0  */
    int _pad0;                   /* +4  alignment for double */
    double floatvalue;           /* +8  */
    int parentheses;             /* +16 */
    /* Compiler auto-pads 4 bytes here on 64-bit ABIs to align prev/next. */
    struct value_s *prev;        /* +20 on 32-bit, +24 on 64-bit */
    struct value_s *next;        /* +24 on 32-bit, +32 on 64-bit */
} value_t;

typedef struct operator_s {
    int op;
    int priority;
    int parentheses;
    struct operator_s *prev;
    struct operator_s *next;
} operator_t;

/* source_t — 1624 bytes.  Q2 trims Q3's 1024-byte path buffers and adds the
 * separately-allocated 1024-byte definebuffer scratchpad at +308. */
typedef struct source_s {
    char                   filename[260];      /* +0    source filename                     */
    char                   includepath[48];    /* +260  base path for #include resolution   */
    char                  *definebuffer;       /* +308  scratch buffer (sub_1003E120)  */
    char                   _pad_1[212];        /* +312..+523 reserved/unknown               */
    script_t              *scriptstack;        /* +524  current script-include stack head   */
    struct token_s *tokens;          /* +528  pushed-back token list              */
    define_t              *defines;            /* +532  define list head (linear)           */
    define_t             **definehash;         /* +536  define hash table (4096 B)          */
    indent_t              *indentstack;        /* +540  conditional-compile indent stack    */
    int                    skip;               /* +544  > 0 skipping #if/#else block        */
    int                    _pad_after_skip;    /* +548  unused / padding                    */
    struct token_s         cachedtoken;        /* +552  last-read token (memcpy'd by PC_ReadTokenHandle) */
} source_t;                                    /* sizeof = 1624 on 32-bit */



/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
/* Forward declarations for the functions defined below. */
int __cdecl PC_DollarEvaluate(source_t *source, int *intvalue, double *floatvalue, int integer); /* l_precomp.c: evaluates #if expression tokens */
int __cdecl PC_ReadLine(source_t *source, token_t *token);                       /* 2-param line reader */
int __cdecl PC_ReadSourceToken(source_t *source, token_t *token); /* l_precomp.c: reads one token from source, handling pushed-back tokens */
int __cdecl PC_Directive_line(source_t * source); /* #line handler */
/* `crctable` and the five CRC_* functions live in their own TU: botlib/l_crc.c
 * (l_crc.obj, DLL 0x100385B0..0x1003874F -- see .claude/memory/tu_partition.md). */
extern char word_1005F588[];
extern define_t *globaldefines;

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
