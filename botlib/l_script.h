/*
 * l_script.h — interface of l_script.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_L_SCRIPT_H
#define BOTLIB_L_SCRIPT_H

/* token_t: the script token this TU produces. */
/* token_t — script token (Q3's l_script.h::token_t with NUMBERVALUE).  Q3's
 * float floatvalue is a double here, so with its alignment pad whitespace_p
 * lands at +0x418 and next at +0x428 (Q3: +0x420). */
typedef struct token_s {
    char string[1024];                   /* +0x000: token text (MAX_TOKEN chars)     */
    int type;                            /* +0x400: TT_STRING=1 NUMBER=3 NAME=4 PUNCT=5 */
    int subtype;                         /* +0x404: punctuation id / number subtype  */
    unsigned int intvalue;               /* +0x408: `unsigned long` in the 32-bit
                                          *         original; must stay 4 bytes    */
    int _floatvalue_pad;                 /* +0x40C: alignment padding before double  */
    double floatvalue;                   /* +0x410: floating-point value (8 bytes)   */
    char *whitespace_p;                  /* +0x418 */
    char *endwhitespace_p;
    int line;
    int linescrossed;
    struct token_s *next;
    int padding;
} token_t;

/* punctuation_t and script_t belong next to token_t: Q3 botlib keeps all
 * three in l_script.h, and splitting them left l_script.h and be_ai_def.h
 * needing each other. */
/* punctuation_t — layout as Q3. */
typedef struct punctuation_s {
    char                  *p;       /* +0  punctuation literal text   */
    int                    n;       /* +4  punctuation type id        */
    struct punctuation_s  *next;    /* +8  chain                      */
} punctuation_t;                    /* sizeof = 12 */

/* script_t — 1392-byte header followed by the file data inline.  Differs from
 * Q3: filename buffer trimmed from 1024 to 260 bytes, and the embedded token
 * is token_t (1072 B, double floatvalue) rather than Q3's float variant. */
typedef struct script_s {
    char                   filename[260];      /* +0    file path (strcpy at sub_100401A0) */
    char                  *buffer;             /* +260  start of file data buffer           */
    char                  *script_p;           /* +264  current parse pointer               */
    char                  *end_p;              /* +268  one-past-end of buffer              */
    char                  *lastscript_p;       /* +272  start of last token                 */
    char                  *whitespace_p;       /* +276  start of preceding whitespace       */
    char                  *endwhitespace_p;    /* +280  end of preceding whitespace         */
    int                    length;             /* +284  buffer length in bytes              */
    int                    line;               /* +288  current source line (1-based)       */
    int                    lastline;           /* +292  line of last token                  */
    int                    tokenavailable;     /* +296  pushed-back token flag              */
    int                    flags;              /* +300  script flags                        */
    punctuation_t         *punctuations;       /* +304  per-script punctuation list head    */
    punctuation_t        **punctuationtable;   /* +308  perfect-hash table (FreeScript frees)*/
    struct token_s token;            /* +312..+1383  embedded last token          */
    struct script_s       *next;               /* +1384 next in scriptstack chain           */
    int                    _trail;             /* +1388 trailing padding (memset clears 1392)*/
    /* +1392 onwards: file data lives inline after the header */
} script_t;                                    /* sizeof = 1392 (header only) */



/* 1072 (0x430) bytes on 32-bit, 1088 on 64-bit where the three pointers
 * widen — so copy/alloc sites must use sizeof(token_t), never the literal.
 * Fields up through `floatvalue` keep their original offsets. */



/* Declarations for what this TU defines, from the retired
 * botlib_local.h.  At the end of the file so the types above are
 * already in scope. */
extern char unk_10060418[72]; /* 72-byte blob; &[3]="You are not allowed to..." — botlib_structdefs.c */

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
