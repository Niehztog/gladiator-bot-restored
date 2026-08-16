/*
 * l_memory.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x10038F10..0x100391FF.
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
/* Not `static`: gladi386.so exports this (one of the 809 .dynsym FUNCs), and gcc 2.7.2.3
 * inlines a `static` with few call sites, which both loses the symbol the audit needs
 * and is the wrong linkage. */
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
/* Void in Q3; the apparent `int` return is just the `totalmemorysize - block->size` the
 * epilogue leaves in eax.  The `int` signature is kept so the ~25
 * `return FreeMemory(x);` call sites still compile. */
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

