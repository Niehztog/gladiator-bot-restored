/*
 * be_ai_chat.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x1002A880..0x1002EC80.
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
#include "be_ai_chat.h"
#include "be_aas_main.h"
#include "be_ea.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_precomp.h"
#include "l_script.h"
#include "l_utils.h"

bot_consolemessage_t *freeconsolemessages; // 0x10064364 free-list head (be_ai_chat.c; was dword_10064364)
bot_consolemessage_t *consolemessageheap; // pool base (initial bulk allocation)
bot_matchtemplate_t *matchtemplates; // weak
bot_randomlist_t *randomstrings; // weak
bot_replychat_t *replychats; // weak
bot_synonymlist_t *synonyms; /* synonyms head, set by BotLoadSynonyms */

// gladiator.dll: 1002A880..1002A95A
// gladi386.so:   00038EC8..00039148
// Allocate a fixed-size freelist pool of bot_consolemessage_t and chain every
// node by prev/next.  Struct-typed rather than the original's hardcoded
// 168-byte stride and +160/+164 offsets, so the right stride and offsets are
// emitted under both ABIs.
void InitConsoleMessageHeap()
{
  int i;
  int v1;

  if ( consolemessageheap )
    FreeMemory(consolemessageheap);
  v1 = (int)LibVarValue("max_messages", (char *)"1024");
  consolemessageheap = (bot_consolemessage_t *)GetMemory(sizeof(bot_consolemessage_t) * v1);
  consolemessageheap[0].prev = NULL;
  consolemessageheap[0].next = &consolemessageheap[1];
  for ( i = 1; i < v1 - 1; ++i )
  {
    consolemessageheap[i].prev = &consolemessageheap[i - 1];
    consolemessageheap[i].next = &consolemessageheap[i + 1];
  }
  consolemessageheap[v1 - 1].prev = &consolemessageheap[v1 - 2];
  consolemessageheap[v1 - 1].next = NULL;
  freeconsolemessages = consolemessageheap;
}

// gladiator.dll: 1002A9A0..1002A9C4
// gladi386.so:   00039148..00039179
bot_consolemessage_t *AllocConsoleMessage()
{
  bot_consolemessage_t *result;

  result = freeconsolemessages;
  if ( result )
  {
    freeconsolemessages = result->next;
    if ( freeconsolemessages )
      freeconsolemessages->prev = NULL;
  }
  return result;
}

// gladiator.dll: 1002A9E0..1002AA10
// gladi386.so:   0003917C..000391C1
void __cdecl FreeConsoleMessage(bot_consolemessage_t *message)
{
  if ( freeconsolemessages )
    freeconsolemessages->prev = message;
  message->prev = NULL;
  message->next = freeconsolemessages;
  freeconsolemessages = message;
}

// gladiator.dll: 1002AA20..1002AA89
// gladi386.so:   000391C4..0003925F
void __cdecl BotRemoveConsoleMessage(bot_chatstate_t *chatstate, bot_consolemessage_t *msg)
{
  /* The original tests `next` FIRST and `prev` second.  Swapping them leaves
   * links->first pointing at a freed message and BotCheckConsoleMessages
   * busy-loops on that node forever. */
  if ( msg->next )
    msg->next->prev = msg->prev;
  else
    BotChatMsgLinksCS(chatstate).last = msg->prev;
  if ( msg->prev )
    msg->prev->next = msg->next;
  else
    BotChatMsgLinksCS(chatstate).first = msg->next;
  FreeConsoleMessage(msg);
  BotChatMsgLinksCS(chatstate).count--;
}

// gladiator.dll: 1002AAB0..1002AB56
// gladi386.so:   00039260..0003932D
void __cdecl BotQueueConsoleMessage(bot_chatstate_t *chatstate, int type, char *message)
{
  bot_consolemessage_t *msg;

  msg = AllocConsoleMessage();
  if ( !msg )
  {
    botimport.Print(PRT_ERROR, "empty console message heap\n");
    return;
  }
  msg->time = AAS_Time();
  msg->type = type;
  strncpy(msg->message, message, 0x96u);
  msg->next = NULL;
  if ( BotChatMsgLinksCS(chatstate).last )
  {
    BotChatMsgLinksCS(chatstate).last->next = msg;
    msg->prev = BotChatMsgLinksCS(chatstate).last;
    BotChatMsgLinksCS(chatstate).last = msg;
  }
  else
  {
    BotChatMsgLinksCS(chatstate).last  = msg;
    BotChatMsgLinksCS(chatstate).first = msg;
    msg->prev    = NULL;
  }
  ++BotChatMsgLinksCS(chatstate).count;
}

// gladiator.dll: 1002AB90..1002AB9B
// gladi386.so:   00039330..0003933B
bot_consolemessage_t *__cdecl BotNextConsoleMessage(bot_chatstate_t *cs)
{
  return BotChatMsgLinksCS(cs).first;
}

// gladiator.dll: 1002ABB0..1002ABBB
// gladi386.so:   0003933C..00039347
int __cdecl BotNumConsoleMessages(bot_chatstate_t *chatstate)
{
  return BotChatMsgLinksCS(chatstate).count;
}

// gladiator.dll: 1002ABD0..1002AC21
// gladi386.so:   00039348..000393AA
BOOL __cdecl IsWhiteSpace(char c)
{
  if ( (c >= 'a' && c <= 'z')
    || (c >= 'A' && c <= 'Z')
    || (c >= '0' && c <= '9')
    || c == '(' || c == ')'
    || c == '?'
    || c == '\''
    || c == ':'
    || c == '['
    || c == ']'
    || c == '\''
    || c == '-'
    || c == '_'
    || c == '+'
    || c == '=' )
    return 0;
  return 1;
}

// gladiator.dll: 1002AC50..1002ACD0
// gladi386.so:   000393AC..000394D5
void __cdecl UnifyWhiteSpaces(void *string)
{
  char *ptr, *oldptr;

  for ( ptr = oldptr = (char *)string; *ptr; oldptr = ptr )
  {
    while ( *ptr && IsWhiteSpace(*ptr) ) ptr++;
    if ( ptr > oldptr )
    {
      if ( oldptr > (char *)string && *ptr )
        *oldptr++ = ' ';
      if ( ptr > oldptr )
        memmove(oldptr, ptr, strlen(ptr) + 1);
    }
    while ( *ptr && !IsWhiteSpace(*ptr) ) ptr++;
  }
}

// gladiator.dll: 1002ACF0..1002ADB1
// gladi386.so:   000394D8..000396AE
const char *__cdecl StringContains(const char *str1, const char *str2, int casesensitive)
{
  int len;
  int i;
  int j;

  len = strlen(str1) - strlen(str2);
  for ( i = 0; i <= len; i++, str1++ )
  {
    for ( j = 0; str2[j]; j++ )
    {
      if ( casesensitive )
      {
        if ( str1[j] != str2[j] )
          break;
      }
      else if ( toupper(str1[j]) != toupper(str2[j]) )
      {
        break;
      }
    }
    if ( !str2[j] )
      return str1;
  }
  return 0;
}

// gladiator.dll: 1002AE00..1002AEEE
// gladi386.so:   000396B0..000397E4
const char *__cdecl StringContainsWord(const char *str1, const char *str2, int casesensitive)
{
  int len;
  int i;
  int j;

  len = strlen(str1) - strlen(str2);
  for ( i = 0; i <= len; i++, str1++ )
  {
    if ( i )
    {
      while ( *str1 && *str1 != ' ' )
        str1++;
      if ( !*str1 )
        break;
      str1++;
    }
    for ( j = 0; str2[j]; j++ )
    {
      if ( casesensitive )
      {
        if ( str1[j] != str2[j] )
          break;
      }
      else if ( toupper(str1[j]) != toupper(str2[j]) )
      {
        break;
      }
    }
    if ( !str2[j] && (!str1[j] || str1[j] == ' ') )
      return str1;
  }
  return 0;
}

// gladiator.dll: 1002AF30..1002B021
// gladi386.so:   000397E4..00039C5F
void __cdecl StringReplaceWords(const char *string, const char *synonym, const char *replacement)
{
  char *str;
  char *str2;

  str = (char *)StringContainsWord(string, synonym, 0);
  while ( str )
  {
str2 = (char *)StringContainsWord(string, replacement, 0);
while ( str2 )
{
  if ( str2 <= str && str < str2 + strlen(replacement) )
    break;
  str2 = (char *)StringContainsWord(str2 + 1, replacement, 0);
}
if ( !str2 )
{
  memmove(str + strlen(replacement), str + strlen(synonym), strlen(str + strlen(synonym)));
  memcpy(str, replacement, strlen(replacement));
}
str = (char *)StringContainsWord(str + strlen(replacement), synonym, 0);
  }
}

// gladiator.dll: 1002B070..1002B0ED
// gladi386.so:   00039C60..00039CF5
/* Print each synonym list as `<key> : [("name", weight), ...]`.  Companion to the
 * named-list dumper at 1002B900, whose inner records are %s-only.  DEAD. */
void __cdecl BotDumpSynonymList(int *synlist)
{
  /* Inner item struct: { char *name; float val; struct item *next; } stride 12.
   * Outer list:        { int key; pad; struct item *items; struct list *next; } stride 16. */
  typedef struct namelist_item_s {
    char *name;
    float val;
    struct namelist_item_s *next;
  } namelist_item_t;
  FILE *fp;
  int  *outer;
  namelist_item_t *item;
  fp = Log_FilePointer();
  /* Only `fp` is tested here — the walk's own guard covers a NULL list.  IDA's
   * `if (!fp || !synlist)` makes gcc load the list pointer before the Log call and
   * then copy it into the walk register; the original loads it straight into that
   * register at the loop guard.  Same in BotDumpRandomStringList. */
  if ( !fp )
    return;
  for ( outer = synlist; outer; outer = (int *)outer[3] )
  {
    fprintf(fp, "%d : [", outer[0]);
    for ( item = (namelist_item_t *)outer[2]; item; item = item->next )
    {
      fprintf(fp, "(\"%s\", %1.2f)", item->name, (float)item->val);
      if ( item->next )
        fprintf(fp, ", ");
    }
    fprintf(fp, "]\n");
  }
}

// gladiator.dll: 1002B110..1002B655
// gladi386.so:   00039CF8..0003A23A
/* The syn.c synonym-config loader: a two-pass parser where pass 0 sums the required
 * scratch-buffer size and pass 1 allocates it and fills it with packed
 * bot_synonymlist_t / bot_synonym_t records.  Sizes come from sizeof() and writes go
 * through struct fields, since the original's +16 / +13 literal strides only hold on
 * 32-bit. */
bot_synonymlist_t *__cdecl BotLoadSynonyms(char *filename)
{
  int pass, size, level, numsynonyms;   
  int context;
  int contextstack[32];                 /* 128 bytes = 32 ints */
  /* Deliberately UNinitialised, as in the original (Q3 later added `= NULL` to hush
   * compilers): the pass-0 read is dead, since `if (pass && size)` gates the only
   * real assignment. */
  char *ptr;                            /* scratch buffer / advancing cursor */
  source_t *src;
  token_t token;
  bot_synonymlist_t *synlist, *lastsyn, *syn;
  bot_synonym_t *synonym, *lastsynonym;
  bot_fileref_t file_ref;

  if ( !sub_10041F60(filename, &file_ref) )
  {
    botimport.Print(PRT_ERROR, "couldn't find %s\n", filename);
    return 0;
  }
  size = 0;
  for ( pass = 0; pass < 2; pass++ )
  {
    if ( pass && size )
      ptr = (char *)GetClearedMemory(size);
    src = LoadSourceFile(file_ref.path, file_ref.fileofs, file_ref.filelen);
    if ( !src )
    {
      botimport.Print(PRT_ERROR, "counldn't load %s\n", file_ref.path);
      return 0;
    }
    context = 0;
    level = 0;
    synlist = NULL;
    lastsyn = NULL;
    while ( PC_ReadTokenHandle(src, token.string) )
    {
      if ( token.type == 3 )
      {
        context |= token.intvalue;
        contextstack[level] = token.intvalue;
        ++level;
        if ( level >= 32 )
        {
          SourceError(src, "more than 32 context levels");
          FreeSource(src);
          return 0;
        }
        if ( !PC_ExpectTokenString(src, "{") )
        {
          FreeSource(src);
          return 0;
        }
      }
      else if ( token.type == 5 )
      {
        if ( !strcmp(token.string, "}") )
        {
          --level;
          if ( level < 0 )
          {
            SourceError(src, "too many }");
            FreeSource(src);
            return 0;
          }
          context &= ~contextstack[level];
        }
        else if ( !strcmp(token.string, "[") )
        {
          size += sizeof(bot_synonymlist_t);
          if ( pass )
          {
            syn = (bot_synonymlist_t *)ptr;
            ptr += sizeof(bot_synonymlist_t);
            syn->context = context;
            syn->firstsynonym = NULL;
            syn->next = NULL;
            if ( lastsyn )
              lastsyn->next = syn;
            else
              synlist = syn;
            lastsyn = syn;
          }
          numsynonyms = 0;
          lastsynonym = NULL;
          while ( 1 )
          {
            if ( !PC_ExpectTokenString(src, "(") || !PC_ExpectTokenType(src, 1, 0, token.string) )
            {
              FreeSource(src);
              return 0;
            }
            StripDoubleQuotes(token.string);
            if ( !strlen(token.string) )
            {
              SourceError(src, "empty string", token.string);
              FreeSource(src);
              return 0;
            }
            size += sizeof(bot_synonym_t) + strlen(token.string) + 1;
            if ( pass )
            {
              synonym = (bot_synonym_t *)ptr;
              ptr += sizeof(bot_synonym_t);
              synonym->string = ptr;
              ptr += strlen(token.string) + 1;
              strcpy(synonym->string, token.string);
              if ( lastsynonym )
                lastsynonym->next = synonym;
              else
                syn->firstsynonym = synonym;
              lastsynonym = synonym;
            }
            ++numsynonyms;
            if ( !PC_ExpectTokenString(src, ",")
              || !PC_ExpectTokenType(src, 3, 0, token.string)
              || !PC_ExpectTokenString(src, ")") )
            {
              FreeSource(src);
              return 0;
            }
            if ( pass )
            {
              synonym->weight = token.floatvalue;
              syn->totalweight += synonym->weight;
            }
            if ( PC_CheckTokenString(src, "]") )
              break;
            if ( !PC_ExpectTokenString(src, ",") )
            {
              FreeSource(src);
              return 0;
            }
          }
          if ( numsynonyms < 2 )
          {
            SourceError(src, "synonym must have at least to entries\n");
            FreeSource(src);
            return 0;
          }
        }
        else
        {
          SourceError(src, "unexpected %s", token.string);
          FreeSource(src);
          return 0;
        }
      }
    }
    FreeSource(src);
    if ( level > 0 )
    {
      SourceError(src, "missing }");
      return 0;
    }
  }
  if ( file_ref.filelen )
    botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", file_ref.path, filename);
  else
    botimport.Print(PRT_MESSAGE, "loaded %s\n", filename);
  return synlist;
}

// gladiator.dll: 1002B7C0..1002B809
// gladi386.so:   0003A23C..0003A295
/* Typed bot_synonymlist_t / bot_synonym_t traversal. */
void __cdecl BotReplaceSynonyms(char *string, unsigned long int context)
{
  bot_synonymlist_t *syn;
  bot_synonym_t *synonym;

  for ( syn = synonyms; syn; syn = syn->next )
  {
    if ( !(syn->context & context) ) continue;
    for ( synonym = syn->firstsynonym->next; synonym; synonym = synonym->next )
    {
      StringReplaceWords(string, synonym->string, syn->firstsynonym->string);
    }
  }
}

// gladiator.dll: 1002B830..1002B8C1
// gladi386.so:   0003A298..0003A32E
/* Selects a synonym by weighted random pick and rewrites all other
 * occurrences in the string. */
void __cdecl BotReplaceWeightedSynonyms(const char *string, int context)
{
  bot_synonymlist_t *syn;
  bot_synonym_t *synonym, *replacement;
  float weight, curweight;
  int r;

  for ( syn = synonyms; syn; syn = syn->next )
  {
    if ( !(syn->context & context) ) continue;
    r = rand() & 0x7FFF;
    weight = ((float)r * 0.000030518509f) * syn->totalweight;
    curweight = 0.0;
    for ( replacement = syn->firstsynonym; replacement; replacement = replacement->next )
    {
      curweight = curweight + replacement->weight;
      if ( weight < curweight ) break;
    }
    for ( synonym = syn->firstsynonym; synonym; synonym = synonym->next )
    {
      if ( synonym == replacement ) continue;
      StringReplaceWords(string, synonym->string, replacement->string);
    }
  }
}

// gladiator.dll: 1002B900..1002B96D
// gladi386.so:   0003A330..0003A3B7
/* Print each random-string list as `<name> = {"a", "b"}`.  The closing "}\n" is
 * emitted by the LAST inner item, so an empty inner list leaves the block
 * unterminated — an abandoned helper with no Q3 counterpart.  DEAD. */
void __cdecl BotDumpRandomStringList(int *randomlist)
{
  FILE *fp;
  bot_randomlist_t *rl;
  bot_randomstring_t *rs;
  fp = Log_FilePointer();
  if ( !fp )
    return;
  for ( rl = (bot_randomlist_t *)randomlist; rl; rl = rl->next )
  {
    fprintf(fp, "%s = {", rl->string);
    for ( rs = rl->firstrandomstring; rs; rs = rs->next )
    {
      fprintf(fp, "\"%s\"", rs->string);
      if ( rs->next )
        fprintf(fp, ", ");
      else
        fprintf(fp, "}\n");
    }
  }
}

// gladiator.dll: 1002B990..1002BCE3
// gladi386.so:   0003A3B8..0003A782
// Q3 equivalent: BotLoadRandomStrings.  Two-pass loader: pass 0 counts the
// required scratch-buffer size, pass 1 allocates a single GetClearedMemory()
// block and populates it as a packed sequence of bot_randomlist_t /
// bot_randomstring_t records, each immediately followed by its inline string.
// Sizes come from sizeof() rather than the original's hardcoded +16/+8/+12
// strides, which only hold on 32-bit.
bot_randomlist_t *__cdecl BotLoadRandomStrings(char *filename)
{
  int pass, size;
  char *ptr;
  source_t *source;
  token_t token;
  bot_randomlist_t *randomlist, *lastrandom, *random;
  bot_randomstring_t *randomstring;
  bot_fileref_t file_ref;

  if ( !sub_10041F60(filename, &file_ref) )
  {
    botimport.Print(PRT_ERROR, "couldn't find %s\n", filename);
    return NULL;
  }

  /* ptr and random are deliberately left uninitialised, as in the original: ptr is
   * read only under the `pass && size` guard and random is written before read on
   * pass 1.  Only size and randomlist (the returned head) are zeroed. */
  size = 0;
  randomlist = NULL;
  for ( pass = 0; pass < 2; ++pass )
  {
    if ( pass && size )
      ptr = (char *)GetClearedMemory(size);
    source = LoadSourceFile(file_ref.path, file_ref.fileofs, file_ref.filelen);
    if ( !source )
    {
      botimport.Print(PRT_ERROR, "counldn't load %s\n", file_ref.path);
      return NULL;
    }
    randomlist = NULL;
    lastrandom = NULL;
    while ( PC_ReadTokenHandle(source, token.string) )
    {
      if ( token.type != 4 )
      {
        SourceError(source, "unknown random %s", token.string);
        FreeSource(source);
        return NULL;
      }
      size += sizeof(bot_randomlist_t) + strlen(token.string) + 1;
      if ( pass )
      {
        random = (bot_randomlist_t *)ptr;
        ptr += sizeof(bot_randomlist_t);
        random->string = ptr;
        ptr += strlen(token.string) + 1;
        strcpy(random->string, token.string);
        random->firstrandomstring = NULL;
        random->numstrings = 0;
        if ( lastrandom )
          lastrandom->next = random;
        else
          randomlist = random;
        lastrandom = random;
      }
      if ( !PC_ExpectTokenString(source, "=") || !PC_ExpectTokenString(source, "{") )
      {
        FreeSource(source);
        return NULL;
      }
      /* Each element is a SINGLE quoted string, comma-separated and brace-terminated,
       * read with PC_ExpectTokenType(TT_STRING) + StripDoubleQuotes — NOT Q3's
       * BotLoadChatMessage, which would treat the commas as message-piece separators
       * and then fail at '}'. */
      while ( PC_ExpectTokenType(source, 1, 0, token.string) )
      {
        StripDoubleQuotes(token.string);
        size += sizeof(bot_randomstring_t) + strlen(token.string) + 1;
        if ( pass )
        {
          randomstring = (bot_randomstring_t *)ptr;
          ptr += sizeof(bot_randomstring_t);
          randomstring->string = ptr;
          ptr += strlen(token.string) + 1;
          strcpy(randomstring->string, token.string);
          ++random->numstrings;
          randomstring->next = random->firstrandomstring;
          random->firstrandomstring = randomstring;
        }
        if ( PC_CheckTokenString(source, "}") )
          break;
        if ( !PC_ExpectTokenString(source, ",") )
        {
          FreeSource(source);
          return NULL;
        }
      }
    }
    FreeSource(source);
  }
  if ( file_ref.filelen )
    botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", file_ref.path, filename);
  else
    botimport.Print(PRT_MESSAGE, "loaded %s\n", filename);
  return randomlist;
}

// gladiator.dll: 1002BDD0..1002BE63
// gladi386.so:   0003A784..0003A820
// Q3 equivalent: RandomString.  Walks the bot_randomlist_t chain for a name match,
// then picks one of its bot_randomstring_t entries weighted by numstrings.
char *__cdecl RandomString(const char *name)
{
  bot_randomlist_t *list;
  bot_randomstring_t *rs;
  int n;

  for ( list = randomstrings; list; list = list->next )
  {
    if ( !strcmp(list->string, name) )
    {
      n = (int)(((float)(rand() & 0x7FFF) * 0.000030518509f) * (float)list->numstrings);
      for ( rs = list->firstrandomstring; rs; rs = rs->next )
      {
        if ( --n < 0 )
          break;
      }
      if ( rs )
        return rs->string;
    }
  }
  return NULL;
}

// gladiator.dll: 1002BEA0..1002BF6C
// gladi386.so:   0003A820..0003A91D
// Dump a chain of bot_matchtemplate_t to Log_FilePointer() in Mr. Elusive's own
// notation:
//
//   %8d { <pieces> = (type, sub);}\n
//
// where <pieces> walks the template's bot_matchpiece_t list and per piece emits
// either a "|"-joined run of "\"%s\"" strings (type 2, matchstring chain at
// +0x04) or a bare "%d" variable index (type 1, integer at +0x08), with ", "
// separating successive pieces.
//
// Faithful Mr. Elusive bug: the leading "%8d { " fprintf pushes only the format
// string and the FILE*, so the %8d consumes whatever the next stack slot holds.
// Replicated with an explicit "garbage" int local so the call site keeps the same
// arity — and the same UB on the value.
//
// DEAD in Gladiator — /INCREMENTAL.
void __cdecl BotDumpMatchTemplates(void *matches)
{
  FILE *log;
  bot_matchtemplate_t *tmpl;
  bot_matchpiece_t *piece;
  bot_matchstring_t *str;

  log = Log_FilePointer();
  if ( !log )
    return;
  for ( tmpl = (bot_matchtemplate_t *)matches; tmpl; tmpl = tmpl->next )
  {
    /* Faithful original bug: only the format and FILE* are pushed, so %8d reads
     * whatever 4 bytes follow on the stack.  Do NOT add a third argument — that
     * would emit a push the original does not have. */
    fprintf(log, "%8d { ");
    for ( piece = tmpl->first; piece; piece = piece->next )
    {
      if ( piece->type == 2 )
      {
        for ( str = piece->firststring; str; str = str->next )
        {
          fprintf(log, "\"%s\"", str->string);
          if ( str->next )
            fprintf(log, "|");
        }
      }
      else if ( piece->type == 1 )
      {
        fprintf(log, "%d", piece->variable);
      }
      if ( piece->next )
        fprintf(log, ", ");
    }
    fprintf(log, " = (%d, %d);}\n", tmpl->type, tmpl->subtype);
  }
}

// gladiator.dll: 1002BFB0..1002BFF1
// gladi386.so:   0003A920..0003A96E
void __cdecl BotFreeMatchPieces(bot_matchpiece_t *matchpieces)
{
  bot_matchpiece_t *mp;
  bot_matchpiece_t *nextmp;
  bot_matchstring_t *ms;
  bot_matchstring_t *nextms;

  for ( mp = matchpieces; mp; mp = nextmp )
  {
    nextmp = mp->next;
    if ( mp->type == MT_STRING )
    {
      for ( ms = mp->firststring; ms; ms = nextms )
      {
        nextms = ms->next;
        FreeMemory(ms);
      }
    }
    FreeMemory(mp);
  }
}

// gladiator.dll: 1002C020..1002C30E
// gladi386.so:   0003A970..0003ADDB
// Q3 equivalent: ReadFuzzySeparators.  Parses a single match pattern body (pieces
// separated by ',' or '|') into a chain of bot_matchpiece_t, allocated with
// sizeof() rather than the original's literal 16 and int-stride indexing.
bot_matchpiece_t *__cdecl BotLoadMatchPieces(source_t *source, const char *endtoken)
{
  int lastwasvariable;
  int emptystring;
  token_t token;
  bot_matchpiece_t *matchpiece;
  bot_matchpiece_t *firstpiece;
  bot_matchpiece_t *lastpiece;
  bot_matchstring_t *matchstring;
  bot_matchstring_t *lastmatchstring;

  firstpiece = NULL;
  lastpiece = NULL;
  lastwasvariable = 0;

  while ( PC_ReadTokenHandle(source, token.string) )
  {
    if ( token.type == 3 && (token.subtype & 0x1000) != 0 )
    {
      if ( token.intvalue < 0xA )
      {
        if ( lastwasvariable )
        {
          SourceError(source, "not allowed to have adjacent variables\n");
          FreeSource(source);
          BotFreeMatchPieces(firstpiece);
          return NULL;
        }
        lastwasvariable = 1;
        matchpiece = (bot_matchpiece_t *)GetMemory(sizeof(bot_matchpiece_t));
        matchpiece->type = MT_VARIABLE;
        matchpiece->variable = token.intvalue;
        matchpiece->next = NULL;
        if ( lastpiece )
          lastpiece->next = matchpiece;
        else
          firstpiece = matchpiece;
        lastpiece = matchpiece;
      }
      else
      {
        SourceError(source, "can't have more than %d match variables\n", 10);
        FreeSource(source);
        BotFreeMatchPieces(firstpiece);
        return NULL;
      }
    }
    else if ( token.type == 1 )
    {
      matchpiece = (bot_matchpiece_t *)GetMemory(sizeof(bot_matchpiece_t));
      matchpiece->firststring = NULL;
      matchpiece->type = MT_STRING;
      matchpiece->variable = 0;
      matchpiece->next = NULL;
      if ( lastpiece )
        lastpiece->next = matchpiece;
      else
        firstpiece = matchpiece;
      lastpiece = matchpiece;
      lastmatchstring = NULL;
      emptystring = 0;
      do
      {
        if ( matchpiece->firststring )
        {
          if ( !PC_ExpectTokenType(source, 1, 0, token.string) )
          {
            FreeSource(source);
            BotFreeMatchPieces(firstpiece);
            return NULL;
          }
        }
        StripDoubleQuotes(token.string);
        matchstring = (bot_matchstring_t *)GetMemory(sizeof(bot_matchstring_t) + strlen(token.string) + 1);
        matchstring->string = (char *)(matchstring + 1);
        strcpy(matchstring->string, token.string);
        if ( !strlen(token.string) )
          emptystring = 1;
        matchstring->next = NULL;
        if ( lastmatchstring )
          lastmatchstring->next = matchstring;
        else
          matchpiece->firststring = matchstring;
        lastmatchstring = matchstring;
      }
      while ( PC_CheckTokenString(source, "|") );
      if ( !emptystring )
        lastwasvariable = 0;
    }
    else
    {
      SourceError(source, "invalid token %s\n", token.string);
      FreeSource(source);
      BotFreeMatchPieces(firstpiece);
      return NULL;
    }
    if ( PC_CheckTokenString(source, endtoken) )
      break;
    if ( !PC_ExpectTokenString(source, ",") )
    {
      FreeSource(source);
      BotFreeMatchPieces(firstpiece);
      return NULL;
    }
  }
  return firstpiece;
}

// gladiator.dll: 1002C3D0..1002C3F8
// gladi386.so:   0003ADDC..0003AE58
void __cdecl BotFreeMatchTemplates(bot_matchtemplate_t *mt)
{
  bot_matchtemplate_t *m;
  bot_matchtemplate_t *next;

  for ( m = mt; m; m = next )
  {
    next = m->next;
    BotFreeMatchPieces(m->first);
    FreeMemory(m);
  }
}

// gladiator.dll: 1002C410..1002C722
// gladi386.so:   0003AE58..0003B342
// Q3 equivalent: BotLoadMatchTemplates.  Parses match.c into a linked list of
// bot_matchtemplate_t, using the proper struct types so allocation size, field
// offsets and pointer width are correct under both ABIs.
bot_matchtemplate_t *__cdecl BotLoadMatchTemplates(char *matchfile)
{
  source_t *source;
  token_t token;
  bot_matchtemplate_t *matches;       /* head of returned list */
  bot_matchtemplate_t *match;         /* current template */
  bot_matchtemplate_t *lastmatch;     /* last appended template */
  int context;
  bot_fileref_t file_ref;

  if ( !sub_10041F60(matchfile, &file_ref) )
  {
    botimport.Print(PRT_ERROR, "couldn't find %s\n", matchfile);
    return NULL;
  }
  source = LoadSourceFile(file_ref.path, file_ref.fileofs, file_ref.filelen);
  if ( !source )
  {
    botimport.Print(PRT_ERROR, "counldn't load %s\n", file_ref.path);
    return NULL;
  }
  matches = NULL;
  matches = NULL;
  lastmatch = NULL;

  while ( PC_ReadTokenHandle(source, token.string) )
  {
    if ( token.type != 3 || (token.subtype & 0x1000) == 0 )
    {
      SourceError(source, "expected integer, found %s\n", token.string);
      BotFreeMatchTemplates(matches);
      FreeSource(source);
      return NULL;
    }
    context = token.intvalue;
    if ( !PC_ExpectTokenString(source, "{") )
    {
      BotFreeMatchTemplates(matches);
      FreeSource(source);
      return NULL;
    }
    while ( PC_ReadTokenHandle(source, token.string) )
    {
      if ( !strcmp(token.string, "}") )
        break;
      PC_UnreadLastToken(source);
      match = (bot_matchtemplate_t *)GetMemory(sizeof(bot_matchtemplate_t));
      match->context = context;
      match->next = NULL;
      match->first = BotLoadMatchPieces(source, "=");
      if ( lastmatch )
        lastmatch->next = match;
      else
        matches = match;
      lastmatch = match;
      if ( !PC_ExpectTokenString(source, "(") || !PC_ExpectTokenType(source, 3, 4096, token.string) )
      {
        BotFreeMatchTemplates(matches);
        FreeSource(source);
        return NULL;
      }
      match->type = token.intvalue;
      if ( !PC_ExpectTokenString(source, ",") || !PC_ExpectTokenType(source, 3, 4096, token.string) )
      {
        BotFreeMatchTemplates(matches);
        FreeSource(source);
        return NULL;
      }
      match->subtype = token.intvalue;
      if ( !PC_ExpectTokenString(source, ")") || !PC_ExpectTokenString(source, ";") )
      {
        BotFreeMatchTemplates(matches);
        FreeSource(source);
        return NULL;
      }
    }
  }

  FreeSource(source);
  if ( file_ref.filelen )
    botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", file_ref.path, matchfile);
  else
    botimport.Print(PRT_MESSAGE, "loaded %s\n", matchfile);
  return matches;
}

// gladiator.dll: 1002C800..1002C8E6
// gladi386.so:   0003B344..0003B63E
/* Returns true if `pieces` matches `match->string`, writing capture info into
 * match->variables[].  Q3 cognate: be_ai_chat.c StringsMatch. */
BOOL __cdecl StringsMatch(bot_matchpiece_t *pieces, bot_match_t *match)
{
  /* `lastvariable` first and `ms` last: gcc 2.7 assigns these four spill slots in
   * declaration order and the ELF original has lastvariable in the HIGHEST
   * (`mov [esp+0x3c],0xffffffff`), ms in the lowest.  Worth 42 -> 20 insn_diffs;
   * what is left is a block-placement difference, not a slot one. */
  int                lastvariable;
  char              *strptr, *newstrptr;
  bot_matchpiece_t  *mp;
  bot_matchstring_t *ms;

  lastvariable = -1;
  strptr       = match->string;
  mp           = pieces;
  if ( mp )
  {
    newstrptr = match->string;
    do
    {
      if ( mp->type == MT_STRING )
      {
        /* Walk the alternative-string list for any contained in the remaining text.
         * An empty firststring means "match anywhere" — fall through with newstrptr
         * unchanged. */
        ms = mp->firststring;
        if ( ms )
        {
          while ( 1 )
          {
            /* Thunk 0x1000119A -> StringContains, which returns a pointer into the
             * haystack, or NULL. */
            newstrptr = (char *)StringContains(strptr, ms->string, 0);
            if ( newstrptr )
              break;
            ms = ms->next;
            if ( !ms )
              return 0;
          }
        }
        else if ( !newstrptr )
        {
          return 0;
        }

        if ( lastvariable >= 0 )
        {
          match->variables[lastvariable].length = newstrptr - match->variables[lastvariable].ptr;
          lastvariable = -1;
        }
        else if ( newstrptr != strptr )
        {
          return 0;
        }

        /* Advance strptr past the matched substring, unconditionally as the original
         * does — it does not re-guard mp->firststring / ms->string, which are never
         * NULL for a real MT_STRING piece, and guarding costs three branches. */
        strptr = newstrptr + strlen(ms->string);
      }
      else if ( mp->type == MT_VARIABLE )
      {
        match->variables[mp->variable].ptr = strptr;
        lastvariable = mp->variable;
      }
      mp = mp->next;
    }
    while ( mp );
  }

  if ( !mp && (lastvariable >= 0 || !strlen(strptr)) )
  {
    if ( lastvariable >= 0 )
      match->variables[lastvariable].length = strlen(match->variables[lastvariable].ptr);
    return 1;
  }
  return 0;
}

// gladiator.dll: 1002C930..1002C9E9
// gladi386.so:   0003B640..0003B785
/* Walk the chat-pattern template list, filtered by the `context` bitmask, running
 * StringsMatch on each and filling match->type and match->subtype from the hit. */
int __cdecl BotFindMatch(char *str, bot_match_t *match, int context)
{
  bot_matchtemplate_t *ms;
  int                  i;

  strncpy(match->string, str, MAX_MESSAGE_SIZE);
  /* strip trailing '\n's */
  while ( strlen(match->string) && match->string[strlen(match->string) - 1] == '\n' )
    match->string[strlen(match->string) - 1] = 0;

  for ( ms = matchtemplates; ms; ms = ms->next )
  {
    if ( !(context & ms->context) )
      continue;
    /* Reset capture info for this template. */
    for ( i = 0; i < MAX_MATCHVARIABLES; i++ )
      match->variables[i].ptr = NULL;
    if ( StringsMatch(ms->first, match) )
    {
      match->type    = ms->type;
      match->subtype = ms->subtype;
      return 1;
    }
  }
  return 0;
}

// gladiator.dll: 1002CA20..1002CA95
// gladi386.so:   0003B788..0003B814
/* Copy a captured variable (by index) out of a match into `buf`.  Unlike Q3's
 * BotMatchVariable there is no buf-size argument — the write is unbounded, so
 * callers must supply a large enough buffer. */
char *__cdecl BotMatchVariable(bot_match_t *match, int variable, char *buf)
{
  if ( variable < 0 || variable >= MAX_MATCHVARIABLES )
  {
    botimport.Print(PRT_FATAL, "BotMatchVariable: variable out of range\n");
    strcpy(buf, "");
    return buf;
  }
  if ( match->variables[variable].ptr )
  {
    strncpy(buf, match->variables[variable].ptr, match->variables[variable].length);
    buf[match->variables[variable].length] = '\0';
  }
  else
  {
    strcpy(buf, "");
  }
  return buf;
}

// gladiator.dll: 1002CAC0..1002CB1C
// gladi386.so:   0003B814..0003B855
/* Walks a bot_stringlist_t list and returns the matching node.  Used by
 * BotCheckChatMessageIntegrety to deduplicate the missing-random-key list. */
bot_stringlist_t *__cdecl BotFindStringInList(bot_stringlist_t *list, const char *string)
{
  bot_stringlist_t *n;

  for ( n = list; n; n = n->next )
  {
    if ( !strcmp(n->string, string) )
      return n;
  }
  return NULL;
}

// gladiator.dll: 1002CB40..1002CC88
// gladi386.so:   0003B858..0003BA45
/* BotCheckChatMessageIntegrety scans a chat message for \001r<var> references; builds a linked
 * list of undefined variable nodes (accumulating into a2); returns updated list. */
bot_stringlist_t *__cdecl BotCheckChatMessageIntegrety(const char *message, bot_stringlist_t *stringlist)
{
  /* Q3's structure (while + switch over the escape char), reading the message pointer
   * as `char` so MSVC emits `mov al`/`cmp al` rather than an int-promoted `movsx`.
   * The strings and the node-alloc block are Gladiator's own. */
  int i;
  const char *msgptr;
  bot_stringlist_t *node;
  char ArgList[152]; // [esp+8h] [ebp-98h] BYREF

  msgptr = message;
  while ( *msgptr )
  {
    if ( *msgptr == 1 )
    {
      ++msgptr;
      switch ( *msgptr )
      {
        case 'v':
          ++msgptr;
          while ( *msgptr && *msgptr != 1 )
            ++msgptr;
          if ( *msgptr )
            ++msgptr;
          break;
        case 'r':
          ++msgptr;
          for ( i = 0; *msgptr && *msgptr != 1; ++i )
            ArgList[i] = *msgptr++;
          ArgList[i] = 0;
          if ( *msgptr )
            ++msgptr;
          if ( !RandomString(ArgList) && !BotFindStringInList(stringlist, ArgList) )
          {
            Log_Write("%s = {\"%s\"} //MISSING RANDOM", ArgList, ArgList); /* "%s = {\"%s\"}: no value found — both %s = var name (Q3 passes temp twice) */
            node = (bot_stringlist_t *)GetClearedMemory(sizeof(bot_stringlist_t) + strlen(ArgList) + 1);
            node->string = (char *)(node + 1);
            strcpy(node->string, ArgList);
            node->next = stringlist;
            stringlist = node;
          }
          break;
        default:
          botimport.Print(PRT_FATAL, "BotCheckChatMessageIntegrety: message \"%s\" invalid escape char\n", message);
          break;
      }
    }
    else
    {
      ++msgptr;
    }
  }
  return stringlist;
}

// gladiator.dll: 1002CCF0..1002CD36
// gladi386.so:   0003BA48..0003BA9E
// Q3 equivalent: BotCheckReplyChatIntegrity.  Walks every chat message in the
// reply-chat chain and collects offenders into a pointer linked list (each node =
// { void *something; void *next; }).  The offender list stays a raw _DWORD chain —
// it is an opaque transient allocation.
void __cdecl BotCheckReplyChatIntegrety(bot_replychat_t *replychat)
{
  bot_replychat_t *rc;
  bot_chatmessage_t *cm;
  bot_stringlist_t *result;
  bot_stringlist_t *next;

  result = NULL;
  for ( rc = replychat; rc; rc = rc->next )
  {
    for ( cm = rc->firstchatmessage; cm; cm = cm->next )
      result = BotCheckChatMessageIntegrety(cm->chatmessage, result);
  }
  for ( ; result; result = next )
  {
    next = result->next;
    FreeMemory(result);
  }
}

// gladiator.dll: 1002CD60..1002CDA8
// gladi386.so:   0003BAA0..0003BAFE
void __cdecl BotCheckInitialChatIntegrety(struct chatlist_s *chat)
{
  chattype_t        *t;
  chatline_t        *l;
  bot_stringlist_t  *stringlist, *s, *nexts;

  stringlist = NULL;
  for ( t = chat->types; t; t = t->next )
  {
    for ( l = t->firstline; l; l = l->next )
    {
      stringlist = BotCheckChatMessageIntegrety(l->string, stringlist);
    }
  }
  for ( s = stringlist; s; s = nexts )
  {
    nexts = s->next;
    FreeMemory(s);
  }
}

// gladiator.dll: 1002CDD0..1002CEEC
// gladi386.so:   0003BB00..0003BC31
int __cdecl BotLoadChatMessage(source_t *source, char *chatmessagestring)
{
  token_t token;

  *chatmessagestring = 0;
  while ( 1 )
  {
    if ( !PC_ExpectAnyToken(source, token.string) )
      return 0;
    if ( token.type == 1 )
    {
      StripDoubleQuotes(token.string);
      strcat(chatmessagestring, token.string);
    }
    else if ( token.type == 3 && (token.subtype & 0x1000) != 0 )
    {
      sprintf(&chatmessagestring[strlen(chatmessagestring)], "%cv%d%c", 1,
              token.intvalue, 1);
    }
    else if ( token.type == 4 )
    {
      sprintf(&chatmessagestring[strlen(chatmessagestring)], "%cr%s%c", 1,
              token.string, 1);
    }
    else
    {
      SourceError(source, "unknown message component %s\n", token.string);
      return 0;
    }
    if ( PC_CheckTokenString(source, ";") )
      break;
    if ( !PC_ExpectTokenString(source, ",") )
      return 0;
  }
  return 1;
}

// gladiator.dll: 1002CF40..1002D124
// gladi386.so:   0003BC34..0003BE25
/* Dump each bot_replychat_t as `[<LHS>] = <weight>\n{\n\t"chat";\n…}\n`, the same
 * weight-config syntax sub_1002E5D0 emits, with that function's LHS flag cascade
 * duplicated inline rather than shared.  DEAD. */
void __cdecl BotDumpReplyChat(bot_replychat_t *replychat)
{
  FILE             *log;
  bot_replychat_t    *esi;
  bot_replychatkey_t *ebx;
  bot_matchpiece_t   *p;
  bot_chatmessage_t *chat;
  int               flags;

  log = Log_FilePointer();
  if (!log)
    return;
  fprintf(log, "BotDumpReplyChat:\n");

  for ( esi = replychat; esi; esi = esi->next ) {
    fprintf(log, "[");

    ebx = esi->keys;
    if (ebx) {
      do {
        flags = ebx->flags;
        if (flags & 0x01)
          fprintf(log, "&");
        else if (flags & 0x02)
          fprintf(log, "!");

        flags = ebx->flags;
        if (flags & 0x04) {
          fprintf(log, "name");
        } else if (flags & 0x20) {
          fprintf(log, "female");
        } else if (flags & 0x40) {
          fprintf(log, "male");
        } else if (flags & 0x80) {
          fprintf(log, "it");
        } else if (flags & 0x10) {
          fprintf(log, "(");
          p = ebx->match;
          if (p) {
            do {
              if (p->type == 2) {
                fprintf(log, "\"%s\"", p->firststring->string);
              } else {
                fprintf(log, "%d", p->variable);
              }
              if (p->next)
                fprintf(log, ", ");
              p = p->next;
            } while (p);
          }
          fprintf(log, ")");
        } else if (flags & 0x08) {
          fprintf(log, "\"%s\"", ebx->string);
        }

        if (ebx->next) {
          fprintf(log, ", ");
        } else {
          fprintf(log, "] = %1.0f\n", esi->priority);
        }
        ebx = ebx->next;
      } while (ebx);
    }

    fprintf(log, "{\n");
    for ( chat = esi->firstchatmessage; chat; chat = chat->next ) {
      fprintf(log, "\t\"%s\";\n", chat->chatmessage);
    }
    fprintf(log, "}\n");
  }
}

// gladiator.dll: 1002D1B0..1002D236
// gladi386.so:   0003BE28..0003BECF
// Q3 equivalent: BotFreeReplyChat.  Frees the entire reply-chat chain.
void __cdecl BotFreeReplyChat(bot_replychat_t *replychat)
{
  bot_replychat_t *rc;
  bot_replychat_t *nextrc;
  bot_replychatkey_t *key;
  bot_replychatkey_t *nextkey;
  bot_matchpiece_t *mp;
  bot_matchpiece_t *nextmp;
  bot_chatmessage_t *cm;
  bot_chatmessage_t *nextcm;

  for ( rc = replychat; rc; rc = nextrc )
  {
    nextrc = rc->next;
    for ( key = rc->keys; key; key = nextkey )
    {
      nextkey = key->next;
      for ( mp = key->match; mp; mp = nextmp )
      {
        nextmp = mp->next;
        FreeMemory(mp);
      }
      if ( key->string )
        FreeMemory(key->string);
      FreeMemory(key);
    }
    for ( cm = rc->firstchatmessage; cm; cm = nextcm )
    {
      nextcm = cm->next;
      FreeMemory(cm);
    }
    FreeMemory(rc);
  }
}

// gladiator.dll: 1002D270..1002D6B8
// gladi386.so:   0003BED0..0003C7BB
// Q3 equivalent: BotLoadReplyChat.  Loads rchat.c into a linked list of
// bot_replychat_t.  Typed pointers here, not the original's int slots and
// int-stride indexing, which truncate on 64-bit because every chain link doubles
// in width.
bot_replychat_t *__cdecl BotLoadReplyChat(char *filename)
{
  char *v1;
  source_t *source;
  bot_replychat_t *replyhead;
  bot_replychat_t *rc;
  bot_replychatkey_t *key;
  bot_chatmessage_t *cm;
  char *namestr;
  char chatmessagestring[152]; // BotLoadChatMessage output buffer
  token_t token;
  bot_fileref_t file_ref;

  v1 = filename;
  if ( !sub_10041F60(filename, &file_ref) )
  {
    botimport.Print(PRT_ERROR, "couldn't find %s\n", filename);
    return NULL;
  }
  source = LoadSourceFile(file_ref.path, file_ref.fileofs, file_ref.filelen);
  if ( !source )
  {
    botimport.Print(PRT_ERROR, "counldn't load %s\n", file_ref.path);
    return NULL;
  }
  replyhead = NULL;
  while ( PC_ReadTokenHandle(source, token.string) )
  {
    if ( strcmp(token.string, "[") )
    {
      SourceError(source, "expected [, found %s", token.string);
      BotFreeReplyChat(replyhead);
      FreeSource(source);
      return NULL;
    }
    rc = (bot_replychat_t *)GetClearedMemory(sizeof(bot_replychat_t));
    rc->keys = NULL;
    rc->next = replyhead;
    replyhead = rc;
    do
    {
      key = (bot_replychatkey_t *)GetClearedMemory(sizeof(bot_replychatkey_t));
      key->flags = 0;
      key->string = NULL;
      key->match = NULL;
      key->next = rc->keys;
      rc->keys = key;
      if ( PC_CheckTokenString(source, "&") )
      {
        key->flags |= 1;
      }
      else if ( PC_CheckTokenString(source, "!") )
      {
        key->flags |= 2;
      }

      if ( PC_CheckTokenString(source, "name") )
      {
        key->flags |= 4;
      }
      else if ( PC_CheckTokenString(source, "female") )
      {
        key->flags |= 0x20;
      }
      else if ( PC_CheckTokenString(source, "male") )
      {
        key->flags |= 0x40;
      }
      else if ( PC_CheckTokenString(source, "it") )
      {
        key->flags |= 0x80;
      }
      else if ( PC_CheckTokenString(source, "(") )
      {
        key->flags |= 0x10;
        key->match = BotLoadMatchPieces(source, ")");
      }
      else
      {
        key->flags |= 8;
        if ( !PC_ExpectTokenType(source, 1, 0, token.string) )
        {
          BotFreeReplyChat(replyhead);
          FreeSource(source);
          return NULL;
        }
        StripDoubleQuotes(token.string);
        namestr = (char *)GetClearedMemory(strlen(token.string) + 1);
        key->string = namestr;
        strcpy(namestr, token.string);
      }
      PC_CheckTokenString(source, ",");
    }
    while ( !PC_CheckTokenString(source, "]") );

    if ( !PC_ExpectTokenString(source, "=")
        || !PC_ExpectTokenType(source, 3, 0, token.string) )
    {
      BotFreeReplyChat(replyhead);
      FreeSource(source);
      return NULL;
    }
    rc->priority = (float)token.intvalue;
    if ( !PC_ExpectTokenString(source, "{") )
    {
      BotFreeReplyChat(replyhead);
      FreeSource(source);
      return NULL;
    }
    rc->numchatmessages = 0;
    if ( !PC_CheckTokenString(source, "}") )
    {
      while ( 1 )
      {
        if ( !BotLoadChatMessage(source, chatmessagestring) )
        {
          BotFreeReplyChat(replyhead);
          FreeSource(source);
          return NULL;
        }
        cm = (bot_chatmessage_t *)GetClearedMemory(sizeof(bot_chatmessage_t) + strlen(chatmessagestring) + 1);
        cm->chatmessage = (char *)(cm + 1);
        strcpy(cm->chatmessage, chatmessagestring);
        cm->time = -40.0f;
        cm->next = rc->firstchatmessage;
        rc->firstchatmessage = cm;
        rc->numchatmessages++;
        if ( PC_CheckTokenString(source, "}") )
          break;
      }
    }
  }
  FreeSource(source);
  if ( file_ref.filelen )
    botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", file_ref.path, v1);
  else
    botimport.Print(PRT_MESSAGE, "loaded %s\n", v1);
  BotCheckReplyChatIntegrety(replyhead);
  if ( !replyhead )
    botimport.Print(PRT_MESSAGE, "no rchats\n");
  return replyhead;
}

#if BOTLIB_NEED_SIDEBAND
static void BotFreeChatTree(chatlist_t *list);
#endif
// gladiator.dll: 1002D7E0..1002D861
// gladi386.so:   0003C7BC..0003C85A
/* Pretty-print a loaded chat tree — walks chatlist_t->types and each type's
 * messages, in the same grammar BotLoadInitialChat parses below.  DEAD. */
/* Both walks are counted `for` loops, not `while` loops with the advance in the
 * body: gcc 2.7 only rotates the `for` shape into the original's guard + do-while,
 * and emits `jmp <bottom test>` for the `while` spelling. */
void __cdecl BotDumpInitialChat(chatlist_t *chat)
{
  chattype_t *t;
  chatline_t *msg;
  Log_Write("{");
  for ( t = chat->types; t; t = t->next )
  {
    Log_Write(" type \"%s\"", t->name);
    Log_Write(" {");
    Log_Write("  numchatmessages = %d", t->numlines);
    for ( msg = t->firstline; msg; msg = msg->next )
    {
      Log_Write("  \"%s\"", msg->string);
    }
    Log_Write(" }");
  }
  Log_Write("}");
}

// gladiator.dll: 1002D8A0..1002DE0D
// gladi386.so:   0003C85C..0003CE59
void *__cdecl BotLoadInitialChat(char *chatfile, char *chatname)
{
#if BOTLIB_NEED_SIDEBAND
  /* One struct per chat-type and per chat-line, chained with real pointers, rather
   * than the original's single contiguous buffer of 4-byte slots.  Behaviour and
   * parser-token sequence are unchanged. */
  source_t      *src;
  chatlist_t    *list;
  chattype_t    *cur_type;
  bot_fileref_t  file_ref;
  char           buf[152];
  char           token[sizeof(token_t)] __attribute__((aligned(8)));
  int            found;
  int            indent;

  if ( !sub_10041F60(chatfile, &file_ref) )
  {
    botimport.Print(PRT_ERROR, "couldn't find %s\n", chatfile);
    return 0;
  }

  src = LoadSourceFile(file_ref.path, file_ref.fileofs, file_ref.filelen);
  if ( !src )
  {
    botimport.Print(PRT_ERROR, "counldn't load %s\n", file_ref.path);
    return 0;
  }

  list     = (chatlist_t *)GetClearedMemory(sizeof(chatlist_t));
  cur_type = NULL;
  found    = 0;

  while ( PC_ReadTokenHandle(src, token) )
  {
    if ( strcmp(token, "chat") )
    {
      SourceError(src, "unknown definition %s\n", token);
      FreeSource(src);
      BotFreeChatTree(list);   /* free partial list */
      return 0;
    }
    if ( !PC_ExpectTokenType(src, 1, 0, (intptr_t)token) )
    {
      FreeSource(src);
      BotFreeChatTree(list);
      return 0;
    }
    StripDoubleQuotes(token);
    if ( !PC_ExpectTokenString(src, "{") )
    {
      FreeSource(src);
      BotFreeChatTree(list);
      return 0;
    }
    if ( !strcmp(token, chatname) )
    {
      found = 1;
      while ( PC_ExpectAnyToken(src, (intptr_t)token) )
      {
        if ( !strcmp(token, "}") )
          break;
        if ( strcmp(token, "type") )
        {
          SourceError(src, "expected type found %s\n", token);
          FreeSource(src);
          BotFreeChatTree(list);
          return 0;
        }
        if ( !PC_ExpectTokenType(src, 1, 0, (intptr_t)token) || !PC_ExpectTokenString(src, "{") )
        {
          FreeSource(src);
          BotFreeChatTree(list);
          return 0;
        }
        StripDoubleQuotes(token);
        cur_type = (chattype_t *)GetClearedMemory(sizeof(chattype_t));
        strncpy(cur_type->name, token, sizeof(cur_type->name));
        cur_type->next = list->types;
        list->types    = cur_type;
        if ( !PC_CheckTokenString(src, "}") )
        {
          while ( 1 )
          {
            chatline_t *line;
            size_t      slen;
            if ( !BotLoadChatMessage(src, buf) )
            {
              FreeSource(src);
              BotFreeChatTree(list);
              return 0;
            }
            slen = strlen(buf);
            line = (chatline_t *)GetClearedMemory(sizeof(chatline_t) + slen);
            line->string = line->buf;
            line->ltime  = -40.0f;
            line->next   = cur_type->firstline;
            strcpy(line->buf, buf);
            cur_type->firstline = line;
            ++cur_type->numlines;
            if ( PC_CheckTokenString(src, "}") )
              break;
          }
        }
      }
      FreeSource(src);
      if ( !found )
      {
        botimport.Print(PRT_ERROR, "couldn't find chat %s in %s\n", chatname, file_ref.path);
        BotFreeChatTree(list);
        return 0;
      }
      if ( file_ref.filelen )
        botimport.Print(PRT_MESSAGE, "loaded %s from %s\\%s\n", chatname, file_ref.path, chatfile);
      else
        botimport.Print(PRT_MESSAGE, "loaded %s from %s\n", chatname, chatfile);
      BotCheckInitialChatIntegrety(list);
      return (int *)list;
    }
    else
    {
      /* skip a non-matching chat block */
      indent = 1;
      while ( PC_ExpectAnyToken(src, (intptr_t)token) )
      {
        if ( !strcmp(token, "{") )
          ++indent;
        else if ( !strcmp(token, "}") )
          --indent;
        if ( !indent )
          break;
      }
      if ( indent )
      {
        /* unterminated block — bail */
        FreeSource(src);
        BotFreeChatTree(list);
        return 0;
      }
    }
  }

  /* end-of-file without finding chat */
  FreeSource(src);
  if ( !found )
  {
    botimport.Print(PRT_ERROR, "couldn't find chat %s in %s\n", chatname, file_ref.path);
    BotFreeChatTree(list);
    return 0;
  }
  BotCheckInitialChatIntegrety(list);
  return (int *)list;
#else
  /* `pass` first and `src` where `pass` used to be: the reference .so puts
   * `src` two spill slots lower than `pass`, which is only reachable by
   * swapping their declarations (gcc 2.7 fills the spilled-scalar group
   * top-down in declaration order).  Takes this row to a full MATCH. */
  int            pass;
  char           buf[152];
  char           token[sizeof(token_t)];
  int            found;
  int            size;
  chatline_t    *line;
  char          *ptr;
  source_t      *src;
  int            indent;
  chatlist_t    *list;
  chattype_t    *cur_type;
  bot_fileref_t  file_ref;

  list  = 0;
  cur_type = 0;
  if ( !sub_10041F60(chatfile, &file_ref) )
  {
    botimport.Print(PRT_ERROR, "couldn't find %s\n", chatfile);
    return 0;
  }
  found = 0;
  /* Relies on pass==0 short-circuiting before ptr/size are used. */
  for ( pass = 0; pass < 2; ++pass )
  {
    if ( pass && size )
      ptr = (char *)GetClearedMemory(size);
    src = LoadSourceFile(file_ref.path, file_ref.fileofs, file_ref.filelen);
    if ( !src )
    {
      botimport.Print(PRT_ERROR, "counldn't load %s\n", file_ref.path);
      return 0;
    }
    if ( pass )
    {
      list = (chatlist_t *)ptr;
      ptr += sizeof(chatlist_t);
    }
    size = sizeof(chatlist_t);
while ( PC_ReadTokenHandle(src, token) )
{
  if ( !strcmp(token, "chat") )
  {
    if ( !PC_ExpectTokenType(src, 1, 0, (intptr_t)token) )
    {
      FreeSource(src);
      return 0;
    }
    StripDoubleQuotes(token);
    if ( !PC_ExpectTokenString(src, "{") )
    {
      FreeSource(src);
      return 0;
    }
    if ( !strcmp(token, chatname) )
    {
      found = 1;
      while ( 1 )
      {
        if ( !PC_ExpectAnyToken(src, (intptr_t)token) )
        {
          FreeSource(src);
          return 0;
        }
        if ( !strcmp(token, "}") )
          break;
        if ( strcmp(token, "type") )
        {
          SourceError(src, "expected type found %s\n", token);
          FreeSource(src);
          return 0;
        }
        if ( !PC_ExpectTokenType(src, 1, 0, (intptr_t)token) || !PC_ExpectTokenString(src, "{") )
        {
          FreeSource(src);
          return 0;
        }
        StripDoubleQuotes(token);
        if ( pass )
        {
          cur_type = (chattype_t *)ptr;
          strncpy(cur_type->name, token, sizeof(cur_type->name));
          cur_type->firstline = 0;
          cur_type->next = list->types;
          list->types = cur_type;
          ptr += sizeof(chattype_t);
        }
        size += sizeof(chattype_t);
        while ( !PC_CheckTokenString(src, "}") )
        {
          if ( !BotLoadChatMessage(src, buf) )
          {
            FreeSource(src);
            return 0;
          }
          if ( pass )
          {
            line = (chatline_t *)ptr;
            line->ltime = -40.0f;
            line->next = cur_type->firstline;
            cur_type->firstline = line;
            ptr += sizeof(chatline_t);
            line->string = ptr;
            strcpy(line->string, buf);
            ptr += strlen(buf) + 1;
            ++cur_type->numlines;
          }
          size += sizeof(chatline_t) + (int)strlen(buf) + 1;
        }
      }
    }
    else
    {
      indent = 1;
      while ( indent )
      {
        if ( !PC_ExpectAnyToken(src, (intptr_t)token) )
        {
          FreeSource(src);
          return 0;
        }
        if ( !strcmp(token, "{") )
          ++indent;
        else if ( !strcmp(token, "}") )
          --indent;
      }
    }
  }
  else
  {
    SourceError(src, "unknown definition %s\n", token);
    FreeSource(src);
    return 0;
  }
}
    FreeSource(src);
    if ( !found )
    {
      botimport.Print(PRT_ERROR, "couldn't find chat %s in %s\n", chatname, file_ref.path);
      return 0;
    }
  }
  if ( file_ref.filelen )
    botimport.Print(PRT_MESSAGE, "loaded %s from %s\\%s\n", chatname, file_ref.path, chatfile);
  else
    botimport.Print(PRT_MESSAGE, "loaded %s from %s\n", chatname, chatfile);
  BotCheckInitialChatIntegrety(list);
  return list;
#endif
}

#if BOTLIB_NEED_SIDEBAND
/* 64-bit-only helper for the side-banded chat tree; the original freed the
 * whole chat dump with a single FreeMemory. */
// gladiator.dll: absent
// gladi386.so:   absent
static void BotFreeChatTree(chatlist_t *list)
{
  chattype_t *t, *tn;
  chatline_t *l, *ln;
  if ( !list )
    return;
  for ( t = list->types; t; t = tn )
  {
    tn = t->next;
    for ( l = t->firstline; l; l = ln )
    {
      ln = l->next;
      FreeMemory(l);
    }
    FreeMemory(t);
  }
  FreeMemory(list);
}

#endif
// gladiator.dll: 1002DF70..1002DF94
// gladi386.so:   0003CE5C..0003CE8E
int __cdecl BotFreeChatFile(bot_chatstate_t *chatstate)
{
  chatlist_t *list;
#if BOTLIB_NEED_SIDEBAND
  list = (chatlist_t *)BotChatDumpSlot(chatstate);
  if ( list )
    BotFreeChatTree(list);
  BotChatDumpSlot(chatstate) = 0;
  return 0;
#else
  int result;
  list = (chatlist_t *)BotChatDumpSlot(chatstate);
  result = (int)(intptr_t)list;
  if ( result )
    result = FreeMemory(list);
  BotChatDumpSlot(chatstate) = 0;
  return result;
#endif
}

// gladiator.dll: 1002DFB0..1002DFDE
// gladi386.so:   0003CE90..0003CF55
int __cdecl BotFreeChatState(bot_chatstate_t *cs)
{
  bot_consolemessage_t *msg;

  BotFreeChatFile(cs);
  /* The chatstate pointer is the only argument, as in the original — the per-client
   * message list lives inline at chatstate +0xac/+0xb0/+0xb4 on 32-bit, and
   * BotChatMsgLinksCS derives the client index from `cs` for the 64-bit side-band. */
  for ( msg = BotNextConsoleMessage(cs); msg; msg = BotNextConsoleMessage(cs) )
    BotRemoveConsoleMessage(cs, msg);
}

// gladiator.dll: 1002DFF0..1002E03A
// gladi386.so:   0003CF58..0003CFCF
int __cdecl BotLoadChatFile(bot_chatstate_t *chatstate, char *chatfile, char *chatname)
{
  void *v3; // eax

  BotFreeChatFile(chatstate);
  v3 = BotLoadInitialChat(chatfile, chatname);
  BotChatDumpSlot(chatstate) = v3;
  if ( !v3 )
  {
    botimport.Print(PRT_FATAL, "couldn't load chat %s from %s\n", chatname, chatfile);
    return BLERR_CANNOTLOADICHAT;
  }
  return BLERR_NOERROR;
}

// gladiator.dll: 1002E060..1002E2FD
// gladi386.so:   0003CFD0..0003D470
void __cdecl BotConstructChatMessage(bot_chatstate_t *cs, const char *message, int mcontext, bot_chatvar_t *vars, int vcontext)
{
  int num;
  int len;
  int i;
  char *outputbuf;
  char *msgptr;
  char *ptr;
  char temp[152];

  msgptr = (char *)message;
  outputbuf = cs->chatmessage;
  len = 0;
  while ( *msgptr )
  {
    if ( *msgptr == 1 )
    {
      ++msgptr;
      switch ( *msgptr )
      {
        case 'v':
          ++msgptr;
          num = 0;
          while ( *msgptr && *msgptr != 1 )
            num = num * 10 + (*msgptr++) - '0';
          if ( *msgptr )
            ++msgptr;
          if ( num > 10 )
          {
            botimport.Print(PRT_ERROR, "BotConstructChat: message %s variable %d out of range\n", message, num);
            return;
          }
          if ( (ptr = vars[num].str) != NULL )
          {
            for ( i = 0; i < vars[num].len; ++i )
              temp[i] = ptr[i];
            temp[i] = 0;
            BotReplaceSynonyms(temp, vcontext);
            if ( len + strlen(temp) >= 150 )
            {
              botimport.Print(PRT_ERROR, "BotConstructChat: message %s too long\n", message);
              return;
            }
            strcpy(&outputbuf[len], temp);
            len += strlen(temp);
          }
          break;
        case 'r':
          ++msgptr;
          for ( i = 0; *msgptr && *msgptr != 1; ++i )
            temp[i] = *msgptr++;
          temp[i] = 0;
          if ( *msgptr )
            ++msgptr;
          ptr = RandomString(temp);
          if ( !ptr )
          {
            botimport.Print(PRT_ERROR, "BotConstructChat: unknown random string %s\n", temp);
            return;
          }
          if ( len + strlen(ptr) >= 150 )
          {
            botimport.Print(PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message);
            return;
          }
          strcpy(&outputbuf[len], ptr);
          len += strlen(ptr);
          break;
        default:
          botimport.Print(PRT_FATAL, "BotConstructChat: message \"%s\" invalid escape char\n", message);
          break;
      }
    }
    else
    {
      outputbuf[len++] = *msgptr++;
      if ( len >= 150 )
      {
        botimport.Print(PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message);
        break;
      }
    }
  }
  outputbuf[len] = 0;
  BotReplaceWeightedSynonyms(outputbuf, mcontext);
}

// gladiator.dll: 1002E3B0..1002E4B3
// gladi386.so:   0003D470..0003D5C9
/* Pick a chat line from the initial-chat list for the named chat type (e.g.
 * "enter_game"), returning its chat-string pointer.  `cs` is the head slot stored at
 * chatstate+184. */
char *__cdecl BotChooseInitialChatMessage(chatlist_t *cs, char *type)
{
  chattype_t *t;
  chatline_t *l, *best;
  int         n;
  int         pick;
  float       best_ltime;

  for ( t = cs->types; t; t = t->next )
  {
    if ( !_strcmpi(t->name, type) )
    {
      n = 0;
      for ( l = t->firstline; l; l = l->next )
      {
        if ( AAS_Time() >= l->ltime )
          ++n;
      }
      if ( n <= 0 )
      {
        best_ltime = 0.0f;
        best = NULL;
        for ( l = t->firstline; l; l = l->next )
        {
          if ( best_ltime == 0.0f || l->ltime < best_ltime )
          {
            best       = l;
            best_ltime = l->ltime;
          }
        }
        if ( best )
          return best->string;
      }
      else
      {
        /* `A * C * n`, in that order: the DLL reassociates it to `fild; fimul n;
         * fmul C` no matter how it is parenthesised, and forcing `n * (A * C)`
         * only costs the ELF its MATCH (measured 2026-08-17).  The DLL's 19-byte
         * residual is that reassociation and is not source-reachable. */
        pick = (int)((float)(rand() & 0x7FFF) * 0.000030518509f * n);
        for ( l = t->firstline; l; l = l->next )
        {
          if ( AAS_Time() >= l->ltime )
          {
            if ( --pick < 0 )
            {
              l->ltime = AAS_Time() + 20.0f;
              return l->string;
            }
          }
        }
      }
      return 0;
    }
  }
  return 0;
}

// gladiator.dll: 1002E510..1002E595
// gladi386.so:   0003D5CC..0003D691
void __cdecl BotInitialChat(bot_chatstate_t *cs, char *type, ...)
{
  chatlist_t *list; // chat list
  const char *message; // ebp
  const char *v5; // edi
  int i; // ebx
  va_list ap;
  bot_chatvar_t vars[10]; // BYREF

  list = (chatlist_t *)BotChatDumpSlot(cs);
  if ( list )
  {
    message = BotChooseInitialChatMessage(list, type);
    if ( message )
    {
      memset(vars, 0, sizeof(vars));
      va_start(ap, type);
      v5 = va_arg(ap, const char *);
      i = 0;
      while ( i < 10 )
      {
        if ( !v5 )
          break;
        vars[i].str = (char *)v5;
        vars[i].len = (int)strlen(v5);
        v5 = va_arg(ap, const char *);
        ++i;
      }
      va_end(ap);
      BotConstructChatMessage(cs, message, 0, vars, 0);
    }
  }
}

// gladiator.dll: 1002E5D0..1002E752
// gladi386.so:   0003D694..0003D87F
/* Fuzzy-weight LHS / condition dumper: walk a chain of reply-chat key nodes and
 * pretty-print the pattern as `[...] = <weight>\n{\n`.  Per-node flags select prefix
 * and body:
 *   0x01 "&"        0x02 "!"        0x04 "name"     0x08 "\"<string>\""
 *   0x10 "(<inner>, …)" — each inner is "\"%s\"" for type 2, else "%d"
 *   0x20 "female"   0x40 "male"     0x80 "it"
 * Nodes are joined with ", ".  DEAD. */
void __cdecl sub_1002E5D0(bot_replychat_t *arg)
{
  bot_replychatkey_t *edi;
  bot_matchpiece_t   *esi;
  int flags;

  botimport.Print(PRT_MESSAGE, "[");

  edi = arg->keys;
  if (edi)
  {

  do {
    flags = edi->flags;
    if (flags & 0x01)
      botimport.Print(PRT_MESSAGE, "&");
    else if (flags & 0x02)
      botimport.Print(PRT_MESSAGE, "!");

    flags = edi->flags;
    if (flags & 0x04) {
      botimport.Print(PRT_MESSAGE, "name");
    } else if (flags & 0x20) {
      botimport.Print(PRT_MESSAGE, "female");
    } else if (flags & 0x40) {
      botimport.Print(PRT_MESSAGE, "male");
    } else if (flags & 0x80) {
      botimport.Print(PRT_MESSAGE, "it");
    } else if (flags & 0x10) {
      botimport.Print(PRT_MESSAGE, "(");
      esi = edi->match;
      if (esi) {
        do {
          if (esi->type == 2) {
            botimport.Print(PRT_MESSAGE, "\"%s\"", esi->firststring->string);
          } else {
            botimport.Print(PRT_MESSAGE, "%d", esi->variable);
          }
          if (esi->next)
            botimport.Print(PRT_MESSAGE, ", ");
          esi = esi->next;
        } while (esi);
      }
      botimport.Print(PRT_MESSAGE, ")");
    } else if (flags & 0x08) {
      botimport.Print(PRT_MESSAGE, "\"%s\"", edi->string);
    }

    if (edi->next) {
      botimport.Print(PRT_MESSAGE, ", ");
    } else {
      botimport.Print(PRT_MESSAGE, "] = %1.0f\n", arg->priority);
    }

    edi = edi->next;
  } while (edi);

  }
  botimport.Print(PRT_MESSAGE, "{\n");
}

// gladiator.dll: 1002E7D0..1002E9C8
// gladi386.so:   0003D880..0003DCAD
int __cdecl BotReplyChat(bot_chatstate_t *cs, const char *message)
{
 bot_replychat_t *rchat; // ebx
 bot_replychatkey_t *key; // esi
 int found; // edi
 int v5; // ecx
 BOOL res; // eax
 bot_chatmessage_t *v7; // esi
 int numchatmessages; // edi / [esp+18h] [ebp-F8h]
 int num; // rax (was __int64) / edi
 float rnd;
 bot_chatmessage_t *v10; // esi
 int v14; // [esp+14h] [ebp-FCh]
 bot_chatmessage_t *bestchatmessage; // [esp+10h] [ebp-100h]
 bot_match_t match; // [esp+20h] [ebp-F0h] BYREF

 memset(&match, 0, sizeof(match));
 rchat = replychats;
 strcpy(match.string, message);
 v14 = 0;
 bestchatmessage = 0;
 if ( !rchat )
   return 0;
 do
 {
   key = rchat->keys;
   found = 0;
   if ( !rchat->keys )
     goto LABEL_34;
   do
   {
     v5 = key->flags;
     res = 0;
     if ( (key->flags & 0x20) != 0 )
     {
       res = cs->gender == 1;
     }
     else if ( (v5 & 0x40) != 0 )
     {
       res = cs->gender == 2;
     }
     else if ( (v5 & 0x80) != 0 )
     {
       res = cs->gender == 0;
     }
     else if ( (v5 & 0x10) != 0 )
     {
       res = StringsMatch(key->match, &match);
     }
     else if ( (v5 & 8) != 0 )
     {
       res = StringContains(message, key->string, 0) != 0;
     }
     if ( (key->flags & 1) != 0 )
     {
       if ( !res )
         goto LABEL_34;
       found = 1;
     }
     else if ( (key->flags & 2) != 0 )
     {
       if ( res )
         goto LABEL_34;
     }
     else if ( res )
     {
       found = 1;
     }
     key = key->next;
   }
   while ( key );
   if ( found && (float)v14 < rchat->priority )
   {
     v7 = rchat->firstchatmessage;
     numchatmessages = 0;
     if ( v7 )
     {
       do
       {
         if ( AAS_Time() >= v7->time )
           ++numchatmessages;
         v7 = v7->next;
       }
       while ( v7 );
     }
     /* Two statements, not one expression: the original multiplies by the
        0.000030518509f constant BEFORE the v15 factor, and only a sequence point
        after the const-scaled value reproduces that order. */
     rnd = (float)(rand() & 0x7FFF) * 0.000030518509f;
     num = (int)(rnd * (float)numchatmessages);
     for ( v10 = rchat->firstchatmessage; v10; v10 = v10->next )
     {
       if ( --num < 0 )
         break;
       AAS_Time();
     }
     if ( v10 )
     {
       bestchatmessage = v10;
       v14 = (int)rchat->priority;
     }
   }
LABEL_34:
   rchat = rchat->next;
 }
 while ( rchat );
 if ( bestchatmessage )
 {
   bestchatmessage->time = AAS_Time() + 20.0f;
   BotConstructChatMessage(cs, bestchatmessage->chatmessage, 0, (bot_chatvar_t *)match.variables, 16);
   return 1;
 }
 return 0;
}

// gladiator.dll: 1002EA50..1002EA66
// gladi386.so:   0003DCB0..0003DCC9
unsigned int __cdecl BotChatLength(bot_chatstate_t *chatstate)
{
  return strlen(chatstate->chatmessage);
}

// gladiator.dll: 1002EA80..1002EACF
// gladi386.so:   0003DCCC..0003DD37
/* Genuinely void, like the Q3 cognate: the trailing byte store is the
 * strcpy(cs->chatmessage, "") clear, not a return value. */
void __cdecl BotEnterChat(bot_chatstate_t *chatstate, int clientto, int sendto)
{
  if ( strlen(chatstate->chatmessage) )
  {
    if ( sendto == 1 )
      EA_SayTeam(clientto, chatstate->chatmessage);
    else
      EA_Say(clientto, chatstate->chatmessage);
    strcpy(chatstate->chatmessage, "");
  }
}

// gladiator.dll: 1002EAF0..1002EB1B
// gladi386.so:   0003DD38..0003DD63
/* Q3's BotSetChatGender — the switch is value-for-value Q3's (1 = female, 2 = male,
 * default genderless) writing chatstate.gender.  No `client` argument: Gladiator's
 * chatstate has no such field.  DEAD. */
void __cdecl BotSetChatGender(bot_chatstate_t *chatstate, int gender)
{
  switch ( gender )
  {
    case 1: chatstate->gender = 1; break;
    case 2: chatstate->gender = 2; break;
    default: chatstate->gender = 0; break;
  }
}

// gladiator.dll: 1002EB30..1002EB5B
// gladi386.so:   0003DD64..0003DD98
/* Q3's BotSetChatName — memset 15 bytes of chatstate.name[16], then strncpy with a
 * 15-byte limit.  Unlike Q3, name[15] is left as-is and there is no `client`
 * argument.  DEAD. */
void __cdecl BotSetChatName(bot_chatstate_t *chatstate, const char *name)
{
  memset(chatstate->name, 0, 15);
  strncpy(chatstate->name, name, 15);
}

// gladiator.dll: 1002EB70..1002EB95
// gladi386.so:   0003DD98..0003DDD3
/* Clear the last-used timestamp on every chatmessage of every reply-chat
 * entry, as Q3's BotResetChatAI does.  DEAD in Gladiator. */
void __cdecl BotResetChatAI(void)
{
  bot_replychat_t   *rc;
  bot_chatmessage_t *cm;

  for ( rc = replychats; rc; rc = rc->next )
    for ( cm = rc->firstchatmessage; cm; cm = cm->next )
      cm->time = 0;
}

// gladiator.dll: 1002EBB0..1002EC42
// gladi386.so:   0003DDD4..0003DEA0
int BotSetupChatAI()
{
  char *file;

  file = LibVarString("synfile", "syn.c");
  synonyms = BotLoadSynonyms(file);
  file = LibVarString("rndfile", "rnd.c");
  randomstrings = BotLoadRandomStrings(file);
  file = LibVarString("matchfile", "match.c");
  matchtemplates = BotLoadMatchTemplates(file);
  if ( !LibVarValue("nochat", "0") )
  {
    file = LibVarString("rchatfile", "rchat.c");
    replychats = BotLoadReplyChat(file);
  }
  InitConsoleMessageHeap();
  return BLERR_NOERROR;
}

// gladiator.dll: 1002EC80..1002ECFD
// gladi386.so:   0003DEA0..0003E02F
void BotShutdownChatAI()
{
  if ( consolemessageheap )
    FreeMemory(consolemessageheap);
  consolemessageheap = 0;
  if ( matchtemplates )
    BotFreeMatchTemplates(matchtemplates);
  matchtemplates = 0;
  if ( randomstrings )
    FreeMemory(randomstrings);
  randomstrings = 0;
  if ( synonyms )
    FreeMemory(synonyms);
  synonyms = 0;
  if ( replychats )
    BotFreeReplyChat(replychats);
  replychats = 0;
}
