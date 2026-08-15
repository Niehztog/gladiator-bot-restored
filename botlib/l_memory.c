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
 *
 * Every function below carries a two-line address annotation: its extent in the
 * 1999 Windows `gladiator.dll` (PE32) and in the 1999 Linux `gladi386.so`
 * (ELF32, glibc build), each start..end with the end exclusive.  `absent` means
 * the Linux image has no counterpart.  CLAUDE.md, "Function address
 * annotations", records how both ranges are derived.
 */

#include "botlib_port.h"
#include "l_libvar.h"      /* libvar_t: 24-byte botlib cvar (reconstructed) */
#undef VectorNegate
#include "be_ea.h"    /* ea_state_t: 36-byte per-client EA struct (reconstructed) */
#include "q2files.h"       /* Q2 BSP file format (+ the BSP header) */
#include "aasfile.h"       /* AAS file format: aas_lump_t, aas_header_t */
#include "be_aas_def.h"   /* aas_t, aas_area_t etc. (reconstructed from aasworld_* globals) */
#include "l_script.h"      /* token_t, script_t, punctuation_t */
#include "l_precomp.h"     /* source_t, define_t */
#include "l_struct.h"      /* structdef_t */
#include "l_utils.h"       /* bot_fileref_t + the Win32 UnZip declarations */
#include "be_ai_def.h"     /* the bot-AI structures and interfaces */
#include "be_interface.h"   /* botimport / botstate / bot_exports + libvar aliases */
#include "struct_sizes_asserts.h" /* compile-time struct-layout guard */
#include "l_memory.h"
#include "be_interface.h"

#define MEM_ID 0x12345678l

memoryblock_t *memory;          /* list head    @0x10063A30 */
int            totalmemorysize; /* bytes alloc  @0x10063A1C */
int            numblocks;       /* block count  @0x10063A2C */

// gladiator.dll: 10038F10..10038F37
// gladi386.so:   0004AA08..0004AA3C
void LinkMemoryBlock(memoryblock_t *block)
{
  block->prev = NULL;
  block->next = memory;
  if ( memory )
    memory->prev = block;
  memory = block;
}

// gladiator.dll: 10038F50..10038F7A
// gladi386.so:   0004AA3C..0004AA76
void UnlinkMemoryBlock(memoryblock_t *block)
{
  if ( block->prev )
    block->prev->next = block->next;
  else
    memory = block->next;
  if ( block->next )
    block->next->prev = block->prev;
}

// gladiator.dll: 10038F90..10038FDC
// gladi386.so:   0004AA78..0004AAEB
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

// gladiator.dll: 10039000..10039028
// gladi386.so:   0004AAEC..0004AB6F
void *__cdecl GetClearedMemory(unsigned int size)
{
  void *ptr = GetMemory((int)size);
  memset(ptr, 0, size);
  return ptr;
}

// gladiator.dll: 10039040..1003908B
// gladi386.so:   0004AB70..0004ABCE
/* Q3's BlockFromPointer.  The real list-unlink helper is sub_10038F50. */
/* Not `static`: gladi386.so exports this (it is one of the 809 .dynsym
 * FUNCs), and gcc 2.7.2.3 inlines a `static` with few call sites, which
 * both loses the symbol the audit needs and is the wrong linkage. */
memoryblock_t *BlockFromPointer(void *ptr, const char *str)
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

// gladiator.dll: 100390B0..100390F7
// gladi386.so:   0004ABD0..0004AC82
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

// gladiator.dll: 10039120..1003913B
// gladi386.so:   0004AC84..0004ACF6
int __cdecl MemoryByteSize(void *ptr)
{
  memoryblock_t *block = BlockFromPointer(ptr, "MemoryByteSize");
  if ( !block )
    return 0;
  return block->size;
}

// gladiator.dll: 10039150..1003917E
// gladi386.so:   0004ACF8..0004AD43
void PrintUsedMemorySize(void)
{
  botimport.Print(PRT_MESSAGE, "total botlib memory: %d KB\n", totalmemorysize >> 10);
  botimport.Print(PRT_MESSAGE, "total memory blocks: %d\n", numblocks);
}

// gladiator.dll: 10039190..100391A6
// gladi386.so:   0004AD44..0004ADA2
/* Q3 name: `PrintMemoryLabels`.  Gladiator stripped the MEMDEBUG payload,
 * leaving an empty for-walk over the block list. */
void PrintMemoryLabels(void)
{
  memoryblock_t *block;
  PrintUsedMemorySize();
  for ( block = memory; block; block = block->next )
    ;
}

// gladiator.dll: 100391C0..100391E9
// gladi386.so:   0004ADA4..0004ADDA
void DumpMemory(void)
{
  memoryblock_t *block;
  for ( block = memory; memory; block = memory )
    FreeMemory(block->ptr);
  totalmemorysize = 0;
}

