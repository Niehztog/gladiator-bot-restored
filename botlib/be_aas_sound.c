/*
 * be_aas_sound.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x1001C6F0..0x1001D290; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * botlib_local.h carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs and every prototype), so this file compiles in
 * exactly the environment these functions had before the split.
 */

#include "botlib_local.h"

//----- (1001C6F0) --------------------------------------------------------
/* Dumps every loaded soundinfo_t to the bot debug log via the generic
 * WriteStructure pretty-printer and the soundinfo structdef at .data
 * 0x1005C138.  Companion to Q3's AAS_DumpAreas / AAS_DumpReachabilities.
 * DEAD in Gladiator. */
void __cdecl sub_1001C6F0(void)
{
  int   i;
  FILE *fp;
  for ( i = 0; i < aasworld.numsoundinfo; ++i )
  {
    fp = Log_FilePointer();
    if ( !fp )
      return;
    WriteStructure(fp, (int)&soundinfo_struct, (char *)&aasworld.soundinfo[i]);
    Log_Flush();
  }
}
//----- (1001C760) --------------------------------------------------------
int sub_1001C760(char *Source)
{
  int v2; // ebx
  source_t *v5;
  bot_fileref_t file_ref; /* original bot_fileref_t local */
  char Destination[144]; // [esp+A8h] [ebp-4C0h] BYREF
  char ArgList[sizeof(token_t)] __attribute__((aligned(8))); // [esp+138h] [ebp-430h] BYREF

  v2 = (int)LibVarValue("max_soundinfo", (char *)"256");
  if ( v2 < 0 || v2 > 0xFFFF )
  {
    botimport.Print(PRT_ERROR, "max_soundinfo out of range [0, 65535]");
    v2 = 256;
    LibVarSet("max_soundinfo", (char *)"256");
  }
  if ( aasworld.soundinfo )
    FreeMemory(aasworld.soundinfo);
  aasworld.soundinfo = (soundinfo_t *)GetClearedMemory(sizeof(soundinfo_t) * v2);
  memset(&file_ref, 0, sizeof(file_ref));
  strncpy(Destination, Source, 0x90u);
  if ( !sub_10041F60(Destination, &file_ref) )
  {
    botimport.Print(PRT_ERROR, "couldn't find %s\n", Destination);
    return 0;
  }
  v5 = LoadSourceFile(file_ref.path, file_ref.fileofs, file_ref.filelen);
  if ( !v5 )
  {
    botimport.Print(PRT_ERROR, "counldn't load %s\n", Destination);
    return 0;
  }
  aasworld.numsoundinfo = 0;
  while ( PC_ReadTokenHandle(v5, ArgList) )
  {
    if ( !strcmp(ArgList, "soundinfo") )
    {
      if ( aasworld.numsoundinfo >= v2 )
      {
        SourceError(v5, "more than %d sound infos defined\n", v2);
        FreeSource(v5);
        return 0;
      }
      memset(&aasworld.soundinfo[aasworld.numsoundinfo], 0, sizeof(soundinfo_t));
      if ( !ReadStructure(v5, &soundinfo_struct, (char *)&aasworld.soundinfo[aasworld.numsoundinfo]) )
      {
        FreeSource(v5);
        return 0;
      }
      ++aasworld.numsoundinfo;
    }
    else
    {
      SourceError(v5, "unknown definition %s\n", ArgList);
      FreeSource(v5);
      return 0;
    }
  }
  FreeSource(v5);
  if ( file_ref.filelen )
  {
    botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", file_ref.path, Source);
    return 1;
  }
  botimport.Print(PRT_MESSAGE, "loaded %s\n", Destination);
  return 1;
}
//----- (1001CAB0) --------------------------------------------------------
/* Initialise the aas-sound node free pool: clamp the cvar, allocate
 * max_aas_sounds nodes and thread them into a doubly-linked free list with the
 * pool base as the head. */
void sub_1001CAB0()
{
  int v1;
  int i;

  v1 = (int)LibVarValue("max_aassounds", (char *)"256");
  if ( v1 < 0 || v1 > 0x10000 )
  {
    botimport.Print(PRT_ERROR, "max_aassounds out of range [0, 65536]");
    v1 = 256;
    LibVarSet("max_aassounds", (char *)"256");
  }
  if ( aasworld.d_100669C4 )
    FreeMemory(aasworld.d_100669C4);
  aasworld.d_100669C4 = (aas_soundpool_t *)GetMemory(sizeof(aas_soundpool_t) * v1);
  aasworld.d_100669C4[0].prev = NULL;
  aasworld.d_100669C4[0].next = &aasworld.d_100669C4[1];
  if ( v1 - 1 > 1 )
  {
    for ( i = 1; i < v1 - 1; ++i )
    {
      aasworld.d_100669C4[i].prev = &aasworld.d_100669C4[i - 1];
      aasworld.d_100669C4[i].next = &aasworld.d_100669C4[i + 1];
    }
  }
  aasworld.d_100669C4[v1 - 1].prev = &aasworld.d_100669C4[v1 - 2];
  aasworld.d_100669C4[v1 - 1].next = NULL;
  aasworld.d_100669C8 = aasworld.d_100669C4;
  { (void)((int)(intptr_t)aasworld.d_100669C4); return; }
}
//----- (1001CBE0) --------------------------------------------------------
/* Pop next free aas_soundpool_t off the free-list. */
aas_soundpool_t *sub_1001CBE0()
{
  aas_soundpool_t *result = aasworld.d_100669C8;
  if ( result )
  {
    aasworld.d_100669C8 = result->next;
    if ( aasworld.d_100669C8 )
      aasworld.d_100669C8->prev = NULL;
  }
  return result;
}
//----- (1001CC10) --------------------------------------------------------
/* Push an aas_soundpool_t back onto the free-list. */
aas_soundpool_t *sub_1001CC10(aas_soundpool_t *a1)
{
  if ( aasworld.d_100669C8 )
    aasworld.d_100669C8->prev = a1;
  a1->prev = NULL;
  a1->next = aasworld.d_100669C8;
  aasworld.d_100669C8 = a1;
  return a1;
}
//----- (1001CC50) --------------------------------------------------------
/* Insert into the d_100669CC/D0 sorted active list (descending by float at
 * payload offset +4).  Original gladiator at 0x1001CC50. */
void sub_1001CC50(aas_soundpool_t *a1)
{
  aas_soundpool_t *v1;
  aas_soundpool_t *i;

  v1 = aasworld.d_100669D0;
  for ( i = NULL; v1; v1 = v1->prev )
  {
    if ( v1->endtime < a1->endtime )
      break;
    i = v1;
  }
  a1->next = i;
  a1->prev = v1;
  if ( i )
    i->prev = a1;
  else
    aasworld.d_100669D0 = a1;
  if ( v1 )
    v1->next = a1;
  else
    aasworld.d_100669CC = a1;
}
//----- (1001CCC0) --------------------------------------------------------
/* Unlink from the d_100669CC/D0 sorted list. */
aas_soundpool_t *sub_1001CCC0(aas_soundpool_t *a1)
{
  aas_soundpool_t *result = a1;
  aas_soundpool_t *v2;
  aas_soundpool_t *v3;

  v2 = a1->prev;
  if ( v2 )
    v2->next = a1->next;
  else
    aasworld.d_100669CC = a1->next;
  v3 = a1->next;
  if ( v3 )
  {
    v3->prev = a1->prev;
  }
  else
  {
    result = a1->prev;
    aasworld.d_100669D0 = result;
  }
  return result;
}
//----- (1001CD10) --------------------------------------------------------
/* Insert into the d_100669D4/D8 sorted active list (descending by float at
 * payload offset +0).  Original gladiator at 0x1001CD10. */
void sub_1001CD10(aas_soundpool_t *a1)
{
  aas_soundpool_t *v1;
  aas_soundpool_t *i;

  v1 = aasworld.d_100669D8;
  for ( i = NULL; v1; v1 = v1->prev )
  {
    if ( v1->starttime < a1->starttime )
      break;
    i = v1;
  }
  a1->next = i;
  a1->prev = v1;
  if ( i )
    i->prev = a1;
  else
    aasworld.d_100669D8 = a1;
  if ( v1 )
    v1->next = a1;
  else
    aasworld.d_100669D4 = a1;
}
//----- (1001CD80) --------------------------------------------------------
/* Unlink from the d_100669D4/D8 sorted list. */
aas_soundpool_t *sub_1001CD80(aas_soundpool_t *a1)
{
  aas_soundpool_t *result = a1;
  aas_soundpool_t *v2;
  aas_soundpool_t *v3;

  v2 = a1->prev;
  if ( v2 )
    v2->next = a1->next;
  else
    aasworld.d_100669D4 = a1->next;
  v3 = a1->next;
  if ( v3 )
  {
    v3->prev = a1->prev;
  }
  else
  {
    result = a1->prev;
    aasworld.d_100669D8 = result;
  }
  return result;
}
//----- (1001CDD0) --------------------------------------------------------
/* Search the d_100669CC list for the node whose entnum and soundindex match
 * a1/a2, then unlink and free it.  Genuinely void — neither exit sets a return
 * value, and all three callers ignore it. */
void __cdecl sub_1001CDD0(int a1, int a2)
{
  aas_soundpool_t *v2;

  v2 = aasworld.d_100669CC;
  if ( v2 )
  {
    for ( ; v2; v2 = v2->next )
    {
      if ( v2->entnum == a1 && v2->soundindex == a2 )
      {
        sub_1001CCC0(v2);
        sub_1001CC10(v2);
        return;
      }
    }
  }
}
//----- (1001CE20) --------------------------------------------------------
int __cdecl sub_1001CE20(float *a1, int a2, int a3, int a4, int a5, int a6, float a7)
{
  soundinfo_t *v8; // ebx
  aas_soundpool_t *i;
  aas_soundpool_t *v10;

  if ( a4 < 0 || a4 >= aasworld.soundindex_table->numindexes )
  {
    botimport.Print(PRT_FATAL, "sound index %d out of range [0, %d]\n", a4, aasworld.soundindex_table->numindexes);
    return BLERR_INVALIDSOUNDINDEX;
  }
  else
  {
    if ( !aasworld.d_100669C0 )
    {
      botimport.Print(PRT_MESSAGE, "no soundindex to soundinfo table\n");
      return 0;
    }
    if ( a4 < aasworld.d_100669BC )
    {
      v8 = (soundinfo_t *)aasworld.d_100669C0[a4];
      if ( !v8 )
        return 0;
      for ( i = aasworld.d_100669CC; i; i = i->next )
        ;
      if ( a7 == 0.0f )
        sub_1001CDD0(a2, a4);
      v10 = sub_1001CBE0();
      if ( !v10 )
      {
        botimport.Print(PRT_ERROR, "empty sound heap\n");
        return 0;
      }
      v10->starttime = AAS_Time() + a7;
      v10->endtime = AAS_Time() + v8->duration + a7;
      VectorCopy(a1, v10->origin);
      v10->_reserved20 = 0;
      v10->entnum = a2;
      v10->channel = a3;
      v10->soundindex = a4;
      *(int *)&v10->volume = a5;   /* bit-pattern store: the arg is int-declared but the field is float */
      v10->unknown40 = a6;
      sub_1001CD10(v10);
    }
    return 0;
  }
}
//----- (1001CFA0) --------------------------------------------------------
/* Time-tick: expire nodes whose +4 float (end-time) is past, and promote
 * nodes from the d_100669D4/D8 list to the d_100669CC/D0 list when their
 * +0 float (start-time) has elapsed.  Original gladiator at 0x1001CFA0. */
void __cdecl sub_1001CFA0(float a1)
{
  aas_soundpool_t *v1;
  aas_soundpool_t *v2;
  aas_soundpool_t *v3;
  aas_soundpool_t *v4;

  v1 = aasworld.d_100669CC;
  while ( v1 && v1->endtime <= a1 )
  {
    v2 = v1->next;
    sub_1001CCC0(v1);
    sub_1001CC10(v1);
    v1 = v2;
  }
  v3 = aasworld.d_100669D4;
  if ( v3 )
  {
    do
    {
      v4 = v3->next;
      if ( v3->starttime < a1 )
      {
        sub_1001CD80(v3);
        sub_1001CDD0(v3->entnum, v3->soundindex);
        sub_1001CC50(v3);
      }
      v3 = v4;
    }
    while ( v4 );
  }
}
//----- (1001D040) --------------------------------------------------------
/* Sound-pool list cursor: the active list head for a NULL argument, else the
 * node's next link.  DEAD in Gladiator — sub_1001CFA0 walks the list inline. */
int __cdecl sub_1001D040(aas_soundpool_t *p)
{
  if ( !p )
    return (int)(intptr_t)aasworld.d_100669CC;
  return (int)(intptr_t)p->next;
}
//----- (1001D070) --------------------------------------------------------
/* Returns aasworld.d_100669C0[node->soundindex] — the sound's payload
 * pointer.  DEAD in Gladiator. */
int __cdecl sub_1001D070(aas_soundpool_t *p)
{
  return (int)(intptr_t)aasworld.d_100669C0[p->soundindex];
}
//----- (1001D0A0) --------------------------------------------------------
// Inverse-square sound audibility on a moving source.  Args:
//   arg1 (edi) — listener origin (vec3 at +0)
//   arg2 (esi) — sound emitter: an aas_soundpool_t node (see its struct
//                doc comment in gladiator.dll.h for the full field map)
// Walks aasworld.d_100669C0[emitter->soundindex] to obtain the per-sound
// soundinfo_t whose ->volume holds the sound's range/strength constant.
// First gates on AAS_InPVS(listener, &emitter->origin, default-flags)
// via thunk @0x10001fc8 → sub_10005C90: returns 0.0f (.rdata
// 0x10058000) when the emitter isn't in PVS or the soundindex maps
// to a NULL soundinfo.  Otherwise returns
//   (soundinfo->volume * emitter->volume) / dot(delta, delta)
// where delta = emitter->origin - listener.  DEAD in Gladiator —
// /INCREMENTAL; the live sound code uses a per-cluster path instead.

float __cdecl sub_1001D0A0(float *listener, aas_soundpool_t *emitter)
{
  float *eorigin;
  soundinfo_t *info;
  float dx, dy, dz;

  eorigin = emitter->origin;
  if ( !sub_10005C90(listener, eorigin) )
    return 0.0f;
  info = (soundinfo_t *)aasworld.d_100669C0[emitter->soundindex];
  if ( !info )
    return 0.0f;
  dx = eorigin[0] - listener[0];
  dy = emitter->origin[1] - listener[1];
  dz = emitter->origin[2] - listener[2];
  return (info->volume * emitter->volume)
       / (dx*dx + dy*dy + dz*dz);
}
//----- (1001D140) --------------------------------------------------------
void sub_1001D140()
{
  int i;
  int j;

  if ( aasworld.d_100669C0 )
    FreeMemory(aasworld.d_100669C0);
  aasworld.d_100669C0 = (void **)GetMemory(sizeof(void *) * aasworld.soundindex_table->numindexes);
  memset(aasworld.d_100669C0, 0, sizeof(void *) * aasworld.soundindex_table->numindexes);
  for ( i = 0; i < aasworld.soundindex_table->numindexes; i++ )
  {
    if ( aasworld.soundindex_table->indexes[i] )
    {
      for ( j = 0; j < aasworld.numsoundinfo; j++ )
      {
        if ( !Q_stricmp(aasworld.soundinfo[j].name, aasworld.soundindex_table->indexes[i]) )
        {
          aasworld.d_100669C0[i] = &aasworld.soundinfo[j];
          break;
        }
      }
    }
  }
  aasworld.d_100669BC = aasworld.soundindex_table->numindexes;
}
//----- (1001D260) --------------------------------------------------------
int sub_1001D260()
{
  char *v1; // eax

  sub_1001CAB0();
  v1 = LibVarString("soundconfig", (char *)"sounds.c");
  return sub_1001C760(v1);
}
//----- (1001D290) --------------------------------------------------------
/* Empty in the original — an AAS_Shutdown post-cleanup step, called after
 * AAS_FreeRoutingCaches. */
void sub_1001D290(void) { /* empty body */ }
