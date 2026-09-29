/* l_script.h — interface of l_script.c, an original Gladiator Bot v0.96
 * translation unit (Mr. Elusive, 1999). */
#ifndef BOTLIB_L_SCRIPT_H
#define BOTLIB_L_SCRIPT_H

/* MAX_PATH sizes script_t's and source_t's path buffers, and is the single reason the
 * two 1999 builds disagree about those structs' sizes.
 *   Windows: 260, <windows.h>'s value.  gladiator.dll's LoadSourceFile does
 *            `strncpy(dst, src, 0x104)` and script_p lands at +0x108.
 *   Linux:   144, botlib's own fallback.  gladi386.so does `strncpy(dst, src, 0x90)` at
 *            that same call site and script_p lands at +0x94.
 * Spelled with the platform test rather than a bare #ifndef so both of OUR builds are
 * deterministic whether or not <windows.h> is in scope.  Note be_ai_weap.c's
 * `char path[144]` is deliberately NOT this macro: the DLL has 144 there, so that TU
 * did not see <windows.h> even in the Windows build. */
#ifndef MAX_PATH
#  ifdef _WIN32
#    define MAX_PATH 260
#  else
#    define MAX_PATH 144
#  endif
#endif

/* Q3's l_script.h macros, verbatim, as far as Gladiator's reader uses them: every
 * token type, number subtype and punctuation id the code and the punctuation table
 * spell, and the five script flags it tests.  Gladiator reads binary numbers,
 * computes number values and takes '$' as punctuation unconditionally, so the
 * BINARYNUMBERS, NUMBERVALUE and DOLLAR switches Q3 wraps them in are left out,
 * and so are the two script flags nothing tests. */
//maximum token length
#define MAX_TOKEN					1024

//script flags
#define SCFL_NOERRORS				0x0001
#define SCFL_NOWARNINGS				0x0002
#define SCFL_NOSTRINGWHITESPACES	0x0004
#define SCFL_NOSTRINGESCAPECHARS	0x0008
#define SCFL_PRIMITIVE				0x0010

//token types
#define TT_STRING						1			// string
#define TT_LITERAL					2			// literal
#define TT_NUMBER						3			// number
#define TT_NAME						4			// name
#define TT_PUNCTUATION				5			// punctuation

//string sub type
//---------------
//		the length of the string
//literal sub type
//----------------
//		the ASCII code of the literal
//number sub type
//---------------
#define TT_DECIMAL					0x0008	// decimal number
#define TT_HEX							0x0100	// hexadecimal number
#define TT_OCTAL						0x0200	// octal number
#define TT_BINARY						0x0400	// binary number
#define TT_FLOAT						0x0800	// floating point number
#define TT_INTEGER					0x1000	// integer number
#define TT_LONG						0x2000	// long number
#define TT_UNSIGNED					0x4000	// unsigned number
//punctuation sub type
//--------------------
#define P_RSHIFT_ASSIGN				1
#define P_LSHIFT_ASSIGN				2
#define P_PARMS						3
#define P_PRECOMPMERGE				4

#define P_LOGIC_AND					5
#define P_LOGIC_OR					6
#define P_LOGIC_GEQ					7
#define P_LOGIC_LEQ					8
#define P_LOGIC_EQ					9
#define P_LOGIC_UNEQ					10

#define P_MUL_ASSIGN					11
#define P_DIV_ASSIGN					12
#define P_MOD_ASSIGN					13
#define P_ADD_ASSIGN					14
#define P_SUB_ASSIGN					15
#define P_INC							16
#define P_DEC							17

#define P_BIN_AND_ASSIGN			18
#define P_BIN_OR_ASSIGN				19
#define P_BIN_XOR_ASSIGN			20
#define P_RSHIFT						21
#define P_LSHIFT						22

#define P_POINTERREF					23
#define P_CPP1							24
#define P_CPP2							25
#define P_MUL							26
#define P_DIV							27
#define P_MOD							28
#define P_ADD							29
#define P_SUB							30
#define P_ASSIGN						31

#define P_BIN_AND						32
#define P_BIN_OR						33
#define P_BIN_XOR						34
#define P_BIN_NOT						35

#define P_LOGIC_NOT					36
#define P_LOGIC_GREATER				37
#define P_LOGIC_LESS					38

#define P_REF							39
#define P_COMMA						40
#define P_SEMICOLON					41
#define P_COLON						42
#define P_QUESTIONMARK				43

#define P_PARENTHESESOPEN			44
#define P_PARENTHESESCLOSE			45
#define P_BRACEOPEN					46
#define P_BRACECLOSE					47
#define P_SQBRACKETOPEN				48
#define P_SQBRACKETCLOSE			49
#define P_BACKSLASH					50

#define P_PRECOMP						51
#define P_DOLLAR						52
//name sub type
//-------------
//		the length of the name

/* token_t: the script token this TU produces. */
/* token_t — Q3's l_script.h::token_t verbatim, `long double floatvalue` and all.  That
 * one field is why the struct has two different sizes in the two shipped 1999 builds,
 * with no #ifdef and no pad field in the source:
 *   MSVC6 aliases long double to double (8 B, 8-aligned), so it inserts 4 bytes after
 *   intvalue and 4 more at the tail -> 0x430, which gladiator.dll uses at 89 sites.
 *   gcc i386 makes it 12 B, 4-aligned, so neither pad appears -> 0x42C, which
 *   gladi386.so uses, reaching the field with `fld`/`fstp TBYTE PTR`.
 * `whitespace_p` at +0x418 and `next` at +0x428 in BOTH, which is the invariant the
 * asserts check. */
typedef struct token_s {
    char string[MAX_TOKEN];              /* +0x000: token text                       */
    int type;                            /* +0x400: TT_*                             */
    int subtype;                         /* +0x404: punctuation id / number subtype  */
    unsigned int intvalue;               /* +0x408: `unsigned long` in the 32-bit
                                          *         original; must stay 4 bytes    */
    long double floatvalue;              /* +0x40C (gcc) / +0x410 (MSVC)             */
    char *whitespace_p;                  /* +0x418 */
    char *endwhitespace_p;
    int line;
    int linescrossed;
    struct token_s *next;                /* +0x428 */
} token_t;

/* punctuation_t and script_t belong next to token_t: Q3 botlib keeps all three in
 * l_script.h, and splitting them left l_script.h and be_ai_def.h needing each other. */
/* punctuation_t — layout as Q3. */
typedef struct punctuation_s {
    char                  *p;       /* +0  punctuation literal text   */
    int                    n;       /* +4  punctuation type id        */
    struct punctuation_s  *next;    /* +8  chain                      */
} punctuation_t;                    /* sizeof = 12 */

/* script_t — Q3's script_s with its 1024-byte filename cut to MAX_PATH, so the header
 * is 1392 B in gladiator.dll and 1268 B in gladi386.so.  Both figures are read off
 * LoadScriptFile's own `memset(script, 0, sizeof)`: 0x570 there, 0x4F4 here, with
 * `buffer` at +260/+144 and `next` at +1384/+1264.  There is no trailing pad field —
 * MSVC adds 4 bytes of tail padding on its own because the embedded token_t makes the
 * struct 8-aligned, and gcc i386 does not. */
typedef struct script_s {
    char                   filename[MAX_PATH]; /* +0    file path (strcpy at sub_100401A0) */
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
    struct token_s token;            /* +312 (DLL) / +196 (ELF)  embedded last token */
    struct script_s       *next;               /* +1384 (DLL) / +1264 (ELF)                 */
    /* file data lives inline after the header */
} script_t;                                    /* 1392 (DLL) / 1268 (ELF), header only */



/* Copy/alloc sites must use sizeof(token_t), never a literal: the struct is
 * 0x430 under MSVC, 0x42C under gcc i386 and 1088 on a 64-bit host. */



/* Declarations for what this TU defines — last, so the types above are in scope. */
/* The 72-byte "not allowed to modify" blob is `static const` in l_script.c --
 * .rodata, no symbol, as in the original.  Not declared here. */

BOOL __cdecl EndOfScript(script_t *script);
int __cdecl FileLength(FILE *fp);
void      __cdecl FreeScript(script_t *script);
int __cdecl GetScriptFlags(script_t *script);
script_t *__cdecl LoadScriptFile(char *FileName, int Offset, size_t length);
script_t *__cdecl LoadScriptMemory(const void *ptr, unsigned int length, const char *name);
int __cdecl NumLinesCrossed(script_t *script);
void __cdecl NumberValue(char *string, int subtype, unsigned int *intvalue, long double *floatvalue);
int __cdecl PS_CheckTokenString(script_t *script, const char *string);
int __cdecl PS_CheckTokenType(script_t *script, int type, int subtype, token_t *out);
void __cdecl PS_CreatePunctuationTable(script_t *script, punctuation_t *punctuations);
int __cdecl PS_ExpectAnyToken(script_t *script, token_t *token);
int __cdecl PS_ExpectTokenString(script_t *script, const char *string);
int __cdecl PS_ExpectTokenType(script_t *script, int type, int subtype, token_t *token);
char PS_NextWhiteSpaceChar(script_t *script);
int __cdecl PS_ReadEscapeCharacter(script_t *script, _BYTE *ch);
int __cdecl PS_ReadLiteral(script_t *script, token_t *token);
int __cdecl PS_ReadName(script_t *script, token_t *token);
int __cdecl PS_ReadNumber(script_t *script, token_t *token);
int __cdecl PS_ReadPrimitive(script_t *script, token_t *token);
int __cdecl PS_ReadPunctuation(script_t *script, token_t *token);
int __cdecl PS_ReadString(script_t *script, token_t *token, int quote);
int __cdecl PS_ReadToken(script_t *script, token_t *token);
int __cdecl PS_ReadWhiteSpace(script_t *script);
int __cdecl PS_SkipUntilString(script_t *script, const char *string);
void __cdecl PS_UnreadLastToken(script_t *script);
char *__cdecl PunctuationFromNum(script_t *script, int num);
long double __cdecl ReadSignedFloat(script_t *script);
int __cdecl ReadSignedInt(script_t *script);
void ScriptError(script_t *script, char *Format, ...);
int __cdecl ScriptSkipTo(script_t *script, char *value);
void ScriptWarning(script_t *script, char *Format, ...);
void __cdecl SetScriptFlags(script_t *script, int flags);
void __cdecl SetScriptPunctuations(script_t *script, punctuation_t *p);
void __cdecl StripDoubleQuotes(char *string);
void __cdecl StripSingleQuotes(char *string);

void __cdecl PS_UnreadToken(script_t *script, token_t *token);
void __cdecl ResetScript(script_t *script);

#endif /* BOTLIB_L_SCRIPT_H */
