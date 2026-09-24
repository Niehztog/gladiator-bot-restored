/*
 * be_aas_sound.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.  DLL extent 0x1001C6F0..0x1001D290.
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
#include "be_aas_sound.h"
#include "be_aas_bspq2.h"
#include "be_aas_main.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"

/* soundinfo_struct — descriptor at 0x1005C138 (176 B); field table at 0x1005C070.
 * Deliberately NOT `static`: gladi386.so exports it as a `D` symbol
 * (0005bbcc D soundinfo_fields), which a file-static array can never be. */
char *soundinfo_fields[] = {
    FE("name",        0x00, 0x004, 0, 0x00000000),
    FE("volume",      0x50, 0x203, 0, 0x42A00000),  /* 80.0f */
    FE("duration",    0x54, 0x203, 0, 0x41200000),  /* 10.0f */
    FE("type",        0x58, 0x002, 0, 0x00000000),
    FE("recognition", 0x5C, 0x003, 0, 0x3F800000),  /* 1.0f */
    FE("string",      0x60, 0x004, 0, 0x00000000),
    FE_END
};
structdef_t soundinfo_struct = { 176, soundinfo_fields };

// gladiator.dll: 1001C6F0..1001C738
// gladi386.so:   0002AA60..0002AACC
/* Dumps every loaded soundinfo_t to the bot debug log via the generic WriteStructure
 * pretty-printer and the soundinfo structdef at .data 0x1005C138.  Companion to Q3's
 * AAS_DumpAreas / AAS_DumpReachabilities.  DEAD in Gladiator. */
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

// gladiator.dll: 1001C760..1001CA00
// gladi386.so:   0002AACC..0002AD6F
int sub_1001C760(char *Source)
{
  int v2; // ebx
  source_t *v5;
  /* Declaration order reversed vs. stack-offset order (reverse-declaration-order rule —
   * see BotTravel_Jump / LoadItemConfig): the original has file_ref at the LOWEST offset
   * of these three, then Destination, then ArgList highest. */
  char ArgList[sizeof(token_t)] __attribute__((aligned(8))); // [esp+138h] [ebp-430h] BYREF
  char Destination[144]; // [esp+A8h] [ebp-4C0h] BYREF
  bot_fileref_t file_ref; /* original bot_fileref_t local */

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
  if ( !FindQuakeFile(Destination, &file_ref) )
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
    botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", file_ref.path, Source);
  else
    botimport.Print(PRT_MESSAGE, "loaded %s\n", Destination);
  return 1;
}

// gladiator.dll: 1001CAB0..1001CB9A
// gladi386.so:   0002AD70..0002B041
/* Initialise the aas-sound node free pool: clamp the cvar, allocate max_aas_sounds
 * nodes and thread them into a doubly-linked free list with the pool base as head. */
void sub_1001CAB0()
{
  int i;
  int v1;

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
  for ( i = 1; i < v1 - 1; ++i )
  {
    aasworld.d_100669C4[i].prev = &aasworld.d_100669C4[i - 1];
    aasworld.d_100669C4[i].next = &aasworld.d_100669C4[i + 1];
  }
  aasworld.d_100669C4[v1 - 1].prev = &aasworld.d_100669C4[v1 - 2];
  aasworld.d_100669C4[v1 - 1].next = NULL;
  aasworld.d_100669C8 = aasworld.d_100669C4;
  { (void)((int)(intptr_t)aasworld.d_100669C4); return; }
}

// gladiator.dll: 1001CBE0..1001CBFE
// gladi386.so:   0002B044..0002B077
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

// gladiator.dll: 1001CC10..1001CC37
// gladi386.so:   0002B078..0002B0BA
/* Push an aas_soundpool_t back onto the free-list. */
void sub_1001CC10(aas_soundpool_t *a1)
{
  if ( aasworld.d_100669C8 )
    aasworld.d_100669C8->prev = a1;
  a1->prev = NULL;
  a1->next = aasworld.d_100669C8;
  aasworld.d_100669C8 = a1;
}

// gladiator.dll: 1001CC50..1001CC9D
// gladi386.so:   0002B0BC..0002B134
/* Insert into the d_100669CC/D0 sorted active list (descending by float at
 * payload offset +4).  Original gladiator at 0x1001CC50.
 *
 * The link-and-return sits INSIDE the walk, not after a `break`: gladi386.so
 * lays it out as the fall-through of the compare with the `i = v1; v1 =
 * v1->prev` step behind it, reloads a1->endtime every pass and never rotates
 * the loop -- none of which any break-out spelling produces (five measured,
 * all identical).  cl.exe emits the same code either way.  sub_1001CD10 is
 * the same function over the start-time list. */
void sub_1001CC50(aas_soundpool_t *a1)
{
  aas_soundpool_t *v1;
  aas_soundpool_t *i;

  i = NULL;
  for ( v1 = aasworld.d_100669D0; ; v1 = v1->prev )
  {
    if ( !v1 || v1->endtime < a1->endtime )
    {
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
      return;
    }
    i = v1;
  }
}

// gladiator.dll: 1001CCC0..1001CCF3
// gladi386.so:   0002B134..0002B185
/* Unlink from the d_100669CC/D0 sorted list. */
void sub_1001CCC0(aas_soundpool_t *a1)
{
  aas_soundpool_t *v2;
  aas_soundpool_t *v3;

  v2 = a1->prev;
  if ( v2 )
    v2->next = a1->next;
  else
    aasworld.d_100669CC = a1->next;
  v3 = a1->next;
  if ( v3 )
    v3->prev = a1->prev;
  else
    aasworld.d_100669D0 = a1->prev;
}

// gladiator.dll: 1001CD10..1001CD5B
// gladi386.so:   0002B188..0002B1FC
/* Insert into the d_100669D4/D8 sorted active list (descending by float at
 * payload offset +0).  Original gladiator at 0x1001CD10. */
void sub_1001CD10(aas_soundpool_t *a1)
{
  aas_soundpool_t *v1;
  aas_soundpool_t *i;

  i = NULL;
  for ( v1 = aasworld.d_100669D8; ; v1 = v1->prev )
  {
    if ( !v1 || v1->starttime < a1->starttime )
    {
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
      return;
    }
    i = v1;
  }
}

// gladiator.dll: 1001CD80..1001CDB3
// gladi386.so:   0002B1FC..0002B24D
/* Unlink from the d_100669D4/D8 sorted list. */
void sub_1001CD80(aas_soundpool_t *a1)
{
  aas_soundpool_t *v2;
  aas_soundpool_t *v3;

  v2 = a1->prev;
  if ( v2 )
    v2->next = a1->next;
  else
    aasworld.d_100669D4 = a1->next;
  v3 = a1->next;
  if ( v3 )
    v3->prev = a1->prev;
  else
    aasworld.d_100669D8 = a1->prev;
}

// gladiator.dll: 1001CDD0..1001CE07
// gladi386.so:   0002B250..0002B30A
/* Search the d_100669CC list for the node whose entnum and soundindex match a1/a2, then
 * unlink and free it.  Genuinely void — neither exit sets a return value, and all three
 * callers ignore it. */
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

// gladiator.dll: 1001CE20..1001CF47
// gladi386.so:   0002B30C..0002B593
int __cdecl sub_1001CE20(float *a1, int a2, int a3, int a4, float a5, float a6, float a7)
{
  soundinfo_t *v8; // ebx
  aas_soundpool_t *i;

  if ( a4 < 0 || a4 >= aasworld.soundindex_table->numindexes )
  {
    botimport.Print(PRT_FATAL, "sound index %d out of range [0, %d]\n", a4, aasworld.soundindex_table->numindexes);
    return BLERR_INVALIDSOUNDINDEX;
  }
  if ( !aasworld.d_100669C0 )
  {
    botimport.Print(PRT_MESSAGE, "no soundindex to soundinfo table\n");
    return 0;
  }
  /* Flat negative guards, no `else` around the rest: gcc makes each THEN
   * the fall-through, so gladi386.so's inline `return 0` (with the `!v8`
   * one cross-jumped onto it) proves this guard is an early return.  IDA's
   * `else { ... }` wrapper was what made cl.exe cross-jump that return
   * backward onto the no-soundindex tail; flat, it joins the final
   * `return 0` as the DLL does. */
  if ( a4 < 0 || a4 >= aasworld.d_100669BC )
    return 0;
  v8 = (soundinfo_t *)aasworld.d_100669C0[a4];
  if ( !v8 )
    return 0;
  /* a dead walk to the list end, and then the SAME `i` for the new node:
   * gladi386.so keeps both in esi */
  for ( i = aasworld.d_100669CC; i; i = i->next )
    ;
  if ( a7 == 0.0f )
    sub_1001CDD0(a2, a4);
  i = sub_1001CBE0();
  if ( !i )
  {
    botimport.Print(PRT_ERROR, "empty sound heap\n");
    return 0;
  }
  i->starttime = AAS_Time() + a7;
  /* start + duration, in the .so's evaluation order; cl.exe reassociates
   * the three-term sum either way */
  i->endtime = AAS_Time() + a7 + v8->duration;
  VectorCopy(a1, i->origin);
  i->_reserved20 = 0;
  i->entnum = a2;
  i->channel = a3;
  i->soundindex = a4;
  i->volume = a5;
  i->attenuation = a6;
  sub_1001CD10(i);
  return 0;
}

// gladiator.dll: 1001CFA0..1001D011
// gladi386.so:   0002B594..0002B7BC
/* Time-tick: expire nodes whose +4 float (end-time) is past, and promote nodes from the
 * d_100669D4/D8 list to the d_100669CC/D0 list when their +0 float (start-time) has
 * elapsed. */
void __cdecl sub_1001CFA0(float a1)
{
  /* ONE cursor/next pair for both walks (IDA had four names): gladi386.so
   * keeps them in esi/ebp through both loops. */
  aas_soundpool_t *v1;
  aas_soundpool_t *v2;

  for ( v1 = aasworld.d_100669CC; v1 && v1->endtime <= a1; v1 = v2 )
  {
    v2 = v1->next;
    sub_1001CCC0(v1);
    sub_1001CC10(v1);
  }
  for ( v1 = aasworld.d_100669D4; v1; v1 = v2 )
  {
    v2 = v1->next;
    if ( v1->starttime < a1 )
    {
      sub_1001CD80(v1);
      sub_1001CDD0(v1->entnum, v1->soundindex);
      sub_1001CC50(v1);
    }
  }
}

// gladiator.dll: 1001D040..1001D052
// gladi386.so:   0002B7BC..0002B7E6
/* Sound-pool list cursor: the active list head for a NULL argument, else the
 * node's next link.  DEAD in Gladiator — sub_1001CFA0 walks the list inline. */
int __cdecl sub_1001D040(aas_soundpool_t *p)
{
  if ( !p )
    return (int)(intptr_t)aasworld.d_100669CC;
  return (int)(intptr_t)p->next;
}

// gladiator.dll: 1001D070..1001D081
// gladi386.so:   0002B7E8..0002B80D
/* Returns aasworld.d_100669C0[node->soundindex] — the sound's payload
 * pointer.  DEAD in Gladiator. */
int __cdecl sub_1001D070(aas_soundpool_t *p)
{
  return (int)(intptr_t)aasworld.d_100669C0[p->soundindex];
}

// gladiator.dll: 1001D0A0..1001D112
// gladi386.so:   0002B810..0002B8A1
// Inverse-square sound audibility on a moving source.  Args:
//   arg1 (edi) — listener origin (vec3 at +0)
//   arg2 (esi) — sound emitter: an aas_soundpool_t node
// Walks aasworld.d_100669C0[emitter->soundindex] to obtain the per-sound soundinfo_t
// whose ->volume holds the sound's range/strength constant.  First gates on
// AAS_InPVS(listener, &emitter->origin, default-flags): returns 0.0f when the emitter
// is not in PVS or the soundindex maps to a NULL soundinfo.  Otherwise returns
//   (soundinfo->volume * emitter->volume) / dot(delta, delta)
// where delta = emitter->origin - listener.
// DEAD in Gladiator — /INCREMENTAL; the live sound code uses a per-cluster path.

float __cdecl sub_1001D0A0(float *listener, aas_soundpool_t *emitter)
{
  soundinfo_t *info;
  vec3_t dir;
  float dist;

  if ( !sub_10005C90(listener, emitter->origin) )
    return 0.0f;
  info = (soundinfo_t *)aasworld.d_100669C0[emitter->soundindex];
  if ( !info )
    return 0.0f;
  VectorSubtract(emitter->origin, listener, dir);
  dist = DotProduct(dir, dir);
  return emitter->volume * info->volume / dist;
}

// gladiator.dll: 1001D140..1001D21D
// gladi386.so:   0002B8A4..0002B9D2
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

// gladiator.dll: 1001D260..1001D27E
// gladi386.so:   0002B9D4..0002BA04
int sub_1001D260()
{
  char *v1; // eax

  sub_1001CAB0();
  v1 = LibVarString("soundconfig", (char *)"sounds.c");
  return sub_1001C760(v1);
}

// gladiator.dll: 1001D290..1001D291
// gladi386.so:   0002BA04..0002BA05
/* Empty in the original — an AAS_Shutdown post-cleanup step, called after
 * AAS_FreeRoutingCaches. */
void sub_1001D290(void) { /* empty body */ }
