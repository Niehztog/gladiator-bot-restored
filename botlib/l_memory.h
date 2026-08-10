/*
 * l_memory.h — interface of l_memory.c, one of the original Gladiator Bot v0.96
 * translation units (Mr. Elusive, 1999); see .claude/memory/tu_partition.md.
 *
 * Includes nothing, exactly as Q3 botlib's own be_aas_reach.h / l_libvar.h /
 * be_interface.h do: the .c establishes the environment (botlib_local.h) first,
 * then pulls in the interfaces it calls into.  A per-TU header that included
 * the shared header instead would form a cycle with it, because the shared
 * header needs types these files declare against.
 */
#ifndef BOTLIB_L_MEMORY_H
#define BOTLIB_L_MEMORY_H

typedef struct memoryblock_s {
  unsigned long int       id;        /* +0  MEM_ID sentinel               */
  void                   *ptr;       /* +4  user pointer (= block + 1)    */
  int                     size;      /* +8  total alloc incl. header      */
  struct memoryblock_s   *prev;      /* +12                               */
  struct memoryblock_s   *next;      /* +16  sizeof = 20 on 32-bit        */
} memoryblock_t;

static memoryblock_t *BlockFromPointer(void *ptr, const char *str);
void DumpMemory(void);
int __cdecl FreeMemory(void *ptr);
void *__cdecl GetClearedMemory(unsigned int size);
void *__cdecl GetMemory(int size);
void LinkMemoryBlock(memoryblock_t *block);
int __cdecl MemoryByteSize(void *ptr);
void PrintMemoryLabels(void);
void PrintUsedMemorySize(void);
void UnlinkMemoryBlock(memoryblock_t *block);

#endif /* BOTLIB_L_MEMORY_H */
