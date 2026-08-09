/*
 * l_memory.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10038F10..0x100391FF (ten functions) and it owns `memory`,
 * `totalmemorysize` and `numblocks`; both facts are recovered in
 * .claude/memory/tu_partition.md (the Linux .so's .dynsym keeps all three
 * under their real names, and this TU's F-number run there holds exactly ten
 * functions).
 *
 * The include block below is botlib.c's, verbatim, so every macro and typedef
 * this file compiles against is the environment these functions had before the
 * split.
 */

#include <math.h>
#include <stdarg.h>
#include <stddef.h>    /* offsetof, size_t */
#include <stdint.h>    /* intptr_t */
#include <stdio.h>     /* file I/O, sprintf, sscanf */
#include <string.h>    /* string and memory ops */
#include <stdlib.h>    /* malloc, atoi, rand, abs */
#include <ctype.h>     /* toupper */
#include <errno.h>     /* errno */
#include <time.h>      /* time(), ctime() */
#include <unistd.h>    /* access(), chdir(), getcwd() */

/* q_shared.h first, so its qboolean/vec3_t/cplane_t/… are the canonical types.
 * It sets Q_SHARED_H, which makes botlib.h skip its own Q2-type stubs. */
#include "../game/q_shared.h"
/* q_shared.h's VectorNegate is a 2-arg macro, but botlib has a 1-arg in-place
 * VectorNegate(v) function at 0x10043540.  Drop the macro so it stays callable. */
#undef VectorNegate
#include "../game/botlib.h"  /* bot_export_t, bot_import_t + prerequisite Q2 types */
#include "gladiator.dll.h"
#include "ea_state.h"    /* ea_state_t: 36-byte per-client EA struct (reconstructed) */
#include "aas_world.h"   /* aas_t, aas_area_t etc. (reconstructed from aasworld_* globals) */

#include "bot_state.h"   /* bot_state_t, BOT_* offset constants (reconstructed) */
#include "chat_state.h"  /* bot_match_t, bot_matchpiece_t etc. */
#include "libvar.h"      /* libvar_t: 24-byte botlib cvar (reconstructed) */
#include "botlib_state.h"   /* botimport / botstate / bot_exports + libvar aliases */
#include "botlib_structs.h" /* config, parser, weight, goal and item structs */
#include "struct_sizes_asserts.h" /* compile-time struct-layout guard */
#include "q2files.h"

/* signed high-word/dword accessors */
#define SHIDWORD(x)  (*((int *)&(x) + 1))

/* Stubs for the statically-linked MSVC CRT helpers the bot code still calls. */
static size_t fread_locked(void *buf, size_t sz, size_t n, FILE *f) { return fread(buf, sz, n, f); }
#ifdef _WIN32
/* Windows-only winbspc/zip subsystem (sub_1000E140/sub_1000E430, sub_10041FF0);
 * the Linux botlib has neither a process-spawn nor a chdir-for-bspc path. */
static int    remove_file(const char *path) { return remove(path); }
static int    getcwd_locked(char *buf, int size) { return getcwd(buf, size) ? 0 : -1; }
static intptr_t SpawnProcess(int mode, char *file, char *args, char *cmd, char *addargs) { (void)mode; (void)file; (void)args; (void)cmd; (void)addargs; return -1; }
/* _access is provided by MinGW's <io.h>; _chdir by <direct.h> — use POSIX wrappers */
static int    _chdir(const char *path) { return chdir(path); }
#endif /* _WIN32 */

#ifndef _WIN32
/* POSIX equivalents for the Windows CRT functions used below. */
#include <strings.h>    /* strcasecmp */
#define _strcmpi   strcasecmp
#define _access    access
typedef int __time32_t;   /* Windows 32-bit time type; int is 32-bit on all targets */
#endif

/* ----------------------------------------------------------------------
 * Memory-block leak tracker (10038F10..100391C0).
 *
 * A slimmed-down variant of Q3 l_memory.c's MEMORYMANEGER (sic) path: every
 * bi_GetMemory allocation is prefixed with a memoryblock_t header threaded onto
 * a doubly-linked list rooted at `memory`.  Q3 tracks both `allocatedmemory`
 * (raw user bytes) and `totalmemorysize` (bytes including headers); Gladiator
 * kept only the latter plus `numblocks`, matching PrintUsedMemorySize's two
 * lines of output.  Field names follow Q3's l_memory.c.
 * ---------------------------------------------------------------------- */
typedef struct memoryblock_s {
  unsigned long int       id;        /* +0  MEM_ID sentinel               */
  void                   *ptr;       /* +4  user pointer (= block + 1)    */
  int                     size;      /* +8  total alloc incl. header      */
  struct memoryblock_s   *prev;      /* +12                               */
  struct memoryblock_s   *next;      /* +16  sizeof = 20 on 32-bit        */
} memoryblock_t;

#define MEM_ID 0x12345678l

memoryblock_t *memory;          /* list head    @0x10063A30 */
int            totalmemorysize; /* bytes alloc  @0x10063A1C */
int            numblocks;       /* block count  @0x10063A2C */

//----- (10038F10) --------------------------------------------------------
void LinkMemoryBlock(memoryblock_t *block)
{
  block->prev = NULL;
  block->next = memory;
  if ( memory )
    memory->prev = block;
  memory = block;
}

//----- (10038F50) --------------------------------------------------------
void UnlinkMemoryBlock(memoryblock_t *block)
{
  if ( block->prev )
    block->prev->next = block->next;
  else
    memory = block->next;
  if ( block->next )
    block->next->prev = block->prev;
}

//----- (10038F90) --------------------------------------------------------
void *__cdecl GetMemory(int size)
{
  void          *ptr;
  memoryblock_t *block;

  ptr = botimport.GetMemory(size + (int)sizeof(memoryblock_t));
  block = (memoryblock_t *)ptr;
  block->id   = MEM_ID;
  block->ptr  = (char *)ptr + sizeof(memoryblock_t);
  block->size = size + (int)sizeof(memoryblock_t);
  LinkMemoryBlock(block);
  totalmemorysize += block->size;
  ++numblocks;
  return block->ptr;
}

//----- (10039000) --------------------------------------------------------
void *__cdecl GetClearedMemory(unsigned int size)
{
  void *ptr = GetMemory((int)size);
  memset(ptr, 0, size);
  return ptr;
}

//----- (10039040) --------------------------------------------------------
/* Q3's BlockFromPointer.  The real list-unlink helper is sub_10038F50. */
static memoryblock_t *BlockFromPointer(void *ptr, const char *str)
{
  memoryblock_t *block;

  if ( !ptr )
    return NULL;
  block = (memoryblock_t *)((char *)ptr - sizeof(memoryblock_t));
  if ( block->id != MEM_ID )
  {
    botimport.Print(PRT_FATAL, "%s: invalid memory block\n", str);
    return NULL;
  }
  if ( block->ptr != ptr )
  {
    botimport.Print(PRT_FATAL, "%s: memory block pointer invalid\n", str);
    return NULL;
  }
  return block;
}

//----- (100390B0) --------------------------------------------------------
/* Void in Q3; the apparent `int` return is just the
 * `totalmemorysize - block->size` the epilogue leaves in eax.  The `int`
 * signature is kept so the ~25 `return FreeMemory(x);` call sites still
 * compile, and goes away as each of those is audited. */
int __cdecl FreeMemory(void *ptr)
{
  memoryblock_t *block = BlockFromPointer(ptr, "FreeMemory");
  if ( block )
  {
    UnlinkMemoryBlock(block);
    totalmemorysize -= block->size;
    --numblocks;
    botimport.FreeMemory(block);
  }
}

//----- (10039120) --------------------------------------------------------
int __cdecl MemoryByteSize(void *ptr)
{
  memoryblock_t *block = BlockFromPointer(ptr, "MemoryByteSize");
  if ( !block )
    return 0;
  return block->size;
}

//----- (10039150) --------------------------------------------------------
void PrintUsedMemorySize(void)
{
  botimport.Print(PRT_MESSAGE, "total botlib memory: %d KB\n", totalmemorysize >> 10);
  botimport.Print(PRT_MESSAGE, "total memory blocks: %d\n", numblocks);
}

//----- (10039190) --------------------------------------------------------
/* Q3 name: `PrintMemoryLabels`.  Gladiator stripped the MEMDEBUG payload,
 * leaving an empty for-walk over the block list. */
void PrintMemoryLabels(void)
{
  memoryblock_t *block;
  PrintUsedMemorySize();
  for ( block = memory; block; block = block->next )
    ;
}

//----- (100391C0) --------------------------------------------------------
void DumpMemory(void)
{
  memoryblock_t *block;
  for ( block = memory; memory; block = memory )
    FreeMemory(block->ptr);
  totalmemorysize = 0;
}

