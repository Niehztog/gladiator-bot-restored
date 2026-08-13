/*
 * be_ai_weight.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x10035700..0x10037020; the boundary evidence -- per-object .text and
 * .data link order in the DLL, cross-checked against the Linux gladi386.so's
 * F-number runs and its unscrambled data-symbol names -- is recorded in
 * .claude/memory/tu_partition.md.
 *
 * Its own interface is in the matching .h; botlib_local.h, which that header
 * pulls in, carries the shared compilation environment (includes, CRT and
 * POSIX shims, forward typedefs, the side-band scheme and the externs for
 * botlib.c's remaining globals).
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
#include "be_ai_weight.h"
#include "be_interface.h"
#include "l_memory.h"
#include "l_precomp.h"
#include "l_script.h"
#include "l_struct.h"
#include "l_utils.h"


//----- (10035700) --------------------------------------------------------
int __cdecl ReadValue(source_t *source, float *value)
{
  token_t token;

  if ( !PC_ExpectAnyToken(source, token.string) )
    return 0;
  if ( !strcmp(token.string, "-") )
  {
    SourceWarning(source, "negative value set to zero\n");
    if ( !PC_ExpectTokenType(source, 3, 0, token.string) )
      return 0;
  }
  if ( token.type != 3 )
  {
    SourceError(source, "invalid return value %s\n", token.string);
    return 0;
  }
  *value = (float)token.floatvalue;
  return 1;
}
//----- (10035820) --------------------------------------------------------
// Parse one `weight`/`minweight`/`maxweight` clause from a bot's _i.c/_w.c
// file into a fuzzyseperator_t leaf. LIVE: called during config load.
// Part of the runtime config reader chain.
int __cdecl ReadFuzzyWeight(source_t *source, fuzzyseperator_t *fs)
{
  int *v3;
  int v4;

  if ( PC_CheckTokenString(source, "balance") )
  {
    fs->type = 1;
    if ( !PC_ExpectTokenString(source, "(") )
      return 0;
    if ( !ReadValue(source, &fs->weight) )
      return 0;
    if ( !PC_ExpectTokenString(source, ",") )
      return 0;
    if ( !ReadValue(source, &fs->minweight) )
      return 0;
    if ( !PC_ExpectTokenString(source, ",") )
      return 0;
    if ( !ReadValue(source, &fs->maxweight) )
      return 0;
    if ( !PC_ExpectTokenString(source, ")") )
      return 0;
    return PC_ExpectTokenString(source, ";") != 0;
  }
  v3 = (int *)&fs->weight;
  fs->type = 0;
  if ( !ReadValue(source, &fs->weight) )
    return 0;
  v4 = *v3;
  *(int *)&fs->minweight = v4;
  fs->maxweight = fs->minweight;
  return PC_ExpectTokenString(source, ";") != 0;
}
//----- (10035960) --------------------------------------------------------
// Recursively free a fuzzy decision-tree subtree (child + next siblings +
// own node). LIVE: paired with FreeWeightConfig2 on bot teardown.
void __cdecl FreeFuzzySeperators_r(fuzzyseperator_t *fs)
{
  fuzzyseperator_t *v2; // edi

  if ( fs )
  {
    if ( fs->child )
      FreeFuzzySeperators_r(fs->child);
    v2 = fs->next;
    FreeMemory(fs);
    if ( v2 )
      FreeFuzzySeperators_r(v2);
  }
}
//----- (100359B0) --------------------------------------------------------
// Free a full weightconfig_t (all weight_t entries + their trees + name
// strings + the container). LIVE: called when releasing a bot.
void __cdecl FreeWeightConfig2(weightconfig_t *config)
{
  int i;

  for ( i = 0; i < config->numweights; ++i )
  {
    FreeFuzzySeperators_r(config->weights[i].firstseperator);
    if ( config->weights[i].name )
      FreeMemory(config->weights[i].name);
  }
  FreeMemory(config);
}
//----- (10035A20) --------------------------------------------------------
// Recursive parser for one decision-tree subtree: reads `switch`/`case`
// blocks from the config and builds linked fuzzyseperator_t nodes
// (child = yes-branch, next = sibling case). LIVE.
fuzzyseperator_t *__cdecl ReadFuzzySeperators_r(source_t *source)
{
  int def; // edi
  int newindent; // edi
  fuzzyseperator_t *v7; // eax
  fuzzyseperator_t *firstfs; // ebp
  int index;
  int founddefault;
  fuzzyseperator_t *fs;
  fuzzyseperator_t *lastfs;
  token_t token;

  founddefault = 0;
  firstfs = 0;
  lastfs = 0;
  if ( !PC_ExpectTokenString(source, "(") )
    return 0;
  if ( !PC_ExpectTokenType(source, 3, 4096, token.string) )
    return 0;
  index = token.intvalue;
  if ( !PC_ExpectTokenString(source, ")") )
    return 0;
  if ( !PC_ExpectTokenString(source, "{") )
    return 0;
  if ( !PC_ExpectAnyToken(source, token.string) )
    return 0;
  do
  {
    def = strcmp(token.string, "default") == 0;
    if ( def || !strcmp(token.string, "case") )
    {
      fs = (fuzzyseperator_t *)GetClearedMemory(sizeof(fuzzyseperator_t));
      fs->index = index;
      if ( lastfs )
        lastfs->next = fs;
      else
        firstfs = fs;
      lastfs = fs;
      if ( def )
      {
        if ( founddefault )
        {
          SourceError(source, "switch already has a default\n");
          FreeFuzzySeperators_r(firstfs);
          return 0;
        }
        fs->value = 999999;
        founddefault = 1;
      }
      else
      {
        if ( !PC_ExpectTokenType(source, 3, 4096, token.string) )
        {
          FreeFuzzySeperators_r(firstfs);
          return 0;
        }
        fs->value = token.intvalue;
      }
      if ( !PC_ExpectTokenString(source, ":") || !PC_ExpectAnyToken(source, token.string) )
      {
        FreeFuzzySeperators_r(firstfs);
        return 0;
      }
      newindent = 0;
      if ( !strcmp(token.string, "{") )
      {
        newindent = 1;
        if ( !PC_ExpectAnyToken(source, token.string) )
        {
          FreeFuzzySeperators_r(firstfs);
          return 0;
        }
      }
      if ( !strcmp(token.string, "return") )
      {
        if ( !ReadFuzzyWeight(source, fs) )
        {
          FreeFuzzySeperators_r(firstfs);
          return 0;
        }
      }
      else if ( !strcmp(token.string, "switch") )
      {
        v7 = ReadFuzzySeperators_r(source);
        fs->child = v7;
        if ( !v7 )
        {
          FreeFuzzySeperators_r(firstfs);
          return 0;
        }
      }
      else
      {
        SourceError(source, "invalid name %s\n", token.string);
        return 0;
      }
      if ( newindent )
      {
        if ( !PC_ExpectTokenString(source, "}") )
        {
          FreeFuzzySeperators_r(firstfs);
          return 0;
        }
      }
    }
    else
    {
      FreeFuzzySeperators_r(firstfs);
      SourceError(source, "invalid name %s\n", token.string);
      return 0;
    }
    if ( !PC_ExpectAnyToken(source, token.string) )
    {
      FreeFuzzySeperators_r(firstfs);
      return 0;
    }
  }
  while ( strcmp(token.string, "}") );
  if ( !founddefault )
  {
    SourceWarning(source, "switch without default\n");
    fs = (fuzzyseperator_t *)GetClearedMemory(sizeof(fuzzyseperator_t));
    fs->index = index;
    fs->value = 999999;
    fs->weight = 0;
    fs->next = 0;
    fs->child = 0;
    if ( lastfs )
      lastfs->next = fs;
    else
      firstfs = fs;
    lastfs = fs;
  }
  return firstfs;
}
//----- (10035FA0) --------------------------------------------------------
// Top-level loader: opens a bot's _i.c (items) or _w.c (weapons) config,
// parses each `weight "name" { ... }` block, returns a populated
// weightconfig_t. LIVE: entry point for item & weapon priority data.
weightconfig_t *__cdecl ReadWeightConfig(char *filename)
{
  source_t *src;
  weightconfig_t *cfg;
  fuzzyseperator_t *sep;
  int has_balance;
  bot_fileref_t file_ref; /* original bot_fileref_t local */
  char Destination[144]; // [esp+B0h] [ebp-4C0h] BYREF
  token_t token; // [esp+140h] [ebp-430h] BYREF

  memset(&file_ref, 0, sizeof(file_ref));
  strncpy(Destination, filename, 0x90u);
  if ( !sub_10041F60(Destination, &file_ref) )
  {
    botimport.Print(PRT_ERROR, "couldn't find %s\n", Destination);
    return 0;
  }
  src = LoadSourceFile(file_ref.path, file_ref.fileofs, file_ref.filelen);
  if ( !src )
  {
    botimport.Print(PRT_ERROR, "counldn't load %s\n", Destination);
    return 0;
  }
 cfg = (weightconfig_t *)GetClearedMemory(sizeof(weightconfig_t));
 cfg->numweights = 0;
 while ( PC_ReadTokenHandle(src, token.string) )
 {
   if ( !strcmp(token.string, "weight") )
   {
     if ( cfg->numweights >= MAX_FUZZY_WEIGHTS )
     {
       SourceWarning(src, "too many fuzzy weights\n");
       break;
     }
     if ( !PC_ExpectTokenType(src, 1, 0, token.string) )
     {
       FreeWeightConfig2(cfg);
       FreeSource(src);
       return 0;
     }
     StripDoubleQuotes(token.string);
     cfg->weights[cfg->numweights].name = (char *)GetMemory(strlen(token.string) + 1);
     strcpy(cfg->weights[cfg->numweights].name, token.string);
     if ( !PC_ExpectAnyToken(src, token.string) )
     {
       FreeWeightConfig2(cfg);
       FreeSource(src);
       return 0;
     }
     has_balance = 0;
     if ( !strcmp(token.string, "{") )
     {
       has_balance = 1;
       if ( !PC_ExpectAnyToken(src, token.string) )
       {
         FreeWeightConfig2(cfg);
         FreeSource(src);
         return 0;
       }
     }
     if ( !strcmp(token.string, "switch") )
     {
       sep = ReadFuzzySeperators_r(src);
       if ( !sep )
       {
         FreeWeightConfig2(cfg);
         FreeSource(src);
         return 0;
       }
       cfg->weights[cfg->numweights].firstseperator = sep;
     }
     else if ( !strcmp(token.string, "return") )
     {
       sep = (fuzzyseperator_t *)GetClearedMemory(sizeof(fuzzyseperator_t));
       sep->index = 0;
       sep->value = 999999;
       sep->next  = 0;
       sep->child = 0;
       if ( !ReadFuzzyWeight(src, sep) )
       {
         FreeMemory(sep);
         FreeWeightConfig2(cfg);
         FreeSource(src);
         return 0;
       }
       cfg->weights[cfg->numweights].firstseperator = sep;
     }
     else
     {
       SourceError(src, "invalid name %s\n", token.string);
       FreeWeightConfig2(cfg);
       FreeSource(src);
       return 0;
     }
     if ( has_balance && !PC_ExpectTokenString(src, "}") )
     {
       FreeWeightConfig2(cfg);
       FreeSource(src);
       return 0;
     }
     ++cfg->numweights;
   }
   else
   {
     SourceError(src, "invalid name %s\n", token.string);
     FreeWeightConfig2(cfg);
     FreeSource(src);
     return 0;
   }
 }
 FreeSource(src);
 if ( file_ref.filelen )
   botimport.Print(PRT_MESSAGE, "loaded %s\\%s\n", file_ref.path, Destination);
 else
   botimport.Print(PRT_MESSAGE, "loaded %s\n", Destination);
  return cfg;
}
//----- (10036570) --------------------------------------------------------
// Serialise one leaf weight (constant or min/max fuzzy range) back to a
// config file. DEAD in Gladiator (no callers); part of the GA-tuning
// pipeline that writes evolved weights back to bots/*_i.c / *_w.c.
// Live in Q3 via WriteWeightConfig wrapper.
qboolean __cdecl WriteFuzzyWeight(FILE *fp, fuzzyseperator_t *fs)
{
  if ( fs->type == 1 )
  {
    if ( fprintf(fp, " return balance(") < 0) return 0;
    if ( !WriteFloat(fp, fs->weight)) return 0;
    if ( fprintf(fp, ",") < 0) return 0;
    if ( !WriteFloat(fp, fs->minweight)) return 0;
    if ( fprintf(fp, ",") < 0) return 0;
    if ( !WriteFloat(fp, fs->maxweight) ) return 0;
    if (fprintf(fp, ");\n") < 0) return 0;
  }
  else
  {
    if ( fprintf(fp, " return ") < 0 ) return 0;
    if ( !WriteFloat(fp, fs->weight) ) return 0;
    if ( fprintf(fp, ";\n") < 0) return 0;
  }
  return 1;
}
//----- (10036690) --------------------------------------------------------
// Recursively serialise a decision-tree subtree as nested `switch`/`case`
// blocks. DEAD in Gladiator (only self-recursive callers); GA-pipeline
// counterpart of ReadFuzzySeperators_r. Live in Q3.
qboolean __cdecl WriteFuzzySeperators_r(FILE *fp, fuzzyseperator_t *fs, int indent)
{
  if ( !WriteIndent(fp, indent) )
    return 0;
  if ( fprintf(fp, "switch(%d)\n", fs->index) < 0 )
    return 0;
  if ( !WriteIndent(fp, indent) )
    return 0;
  if ( fprintf(fp, "{\n") < 0 )
    return 0;
  ++indent;
  do
  {
    if ( !WriteIndent(fp, indent) )
      return 0;
    if ( fs->next )
    {
      if ( fprintf(fp, "case %d:", fs->value) < 0 )
        return 0;
    }
    else if ( fprintf(fp, "default:") < 0 )
    {
      return 0;
    }
    if ( fs->child )
    {
      if ( fprintf(fp, "\n") < 0 )
        return 0;
      if ( !WriteIndent(fp, indent) )
        return 0;
      if ( fprintf(fp, "{\n") < 0 )
        return 0;
      if ( !WriteFuzzySeperators_r(fp, fs->child, indent + 1) )
        return 0;
      if ( !WriteIndent(fp, indent) )
        return 0;
      if ( fs->next )
      {
        if ( fprintf(fp, "} //end case\n") < 0 )
          return 0;
      }
      else if ( fprintf(fp, "} //end default\n") < 0 )
      {
        return 0;
      }
    }
    else if ( !WriteFuzzyWeight(fp, fs) )
    {
      return 0;
    }
    fs = fs->next;
  }
  while ( fs );
  --indent;
  if ( !WriteIndent(fp, indent) )
    return 0;
  if ( fprintf(fp, "} //end switch\n") < 0 )
    return 0;
  return 1;
}
//----- (100368B0) --------------------------------------------------------
// WriteWeightConfig: serialise a weightconfig_t back to disk as a
// weights-file ('wb' fopen mode, .rdata 0x1005b32c).  For each weight
// entry write a banner via fprintf(fp, '\nweight "%s"\n', name),
// then '{\n', then the body — recursive WriteFuzzySeperators_r(fp,
// fs, indent=1) when fs->type > 0, else WriteIndent(fp, 1) followed
// by WriteFuzzyWeight(fp, fs) for a leaf — and close with
// '} //end itemweight\n' (.rdata 0x1005e5cc).  Returns 1 on success,
// 0 on any fopen/fprintf/recursion failure (note: failure path does
// not close fp — minor leak in the dead code, preserved).  DEAD in
// Gladiator — /INCREMENTAL; live in Q3 as WriteWeightConfig.

int __cdecl WriteWeightConfig(const char *filename, weightconfig_t *config)
{
  FILE *fp;
  int i;
  weight_t *w;

  fp = fopen(filename, "wb");
  if ( !fp )
    return 0;
  for ( i = 0; i < config->numweights; i++ )
  {
    w = &config->weights[i];
    if ( fprintf(fp, "\nweight \"%s\"\n", w->name) < 0 )
      return 0;
    if ( fprintf(fp, "{\n") < 0 )
      return 0;
    if ( w->firstseperator->index > 0 )
    {
      if ( !WriteFuzzySeperators_r(fp, w->firstseperator, 1) )
        return 0;
    }
    else
    {
      if ( !WriteIndent(fp, 1) )
        return 0;
      if ( !WriteFuzzyWeight(fp, w->firstseperator) )
        return 0;
    }
    if ( fprintf(fp, "} //end itemweight\n") < 0 )
      return 0;
  }
  fclose(fp);
  return 1;
}
//----- (100369C0) --------------------------------------------------------
// Linear lookup of a weight_t* in a weightconfig_t by item/weapon name.
// LIVE: called by the goal AI before each FuzzyWeight evaluation.
int __cdecl FindFuzzyWeight(weightconfig_t *wc, const char *name)
{
  int i;

  for ( i = 0; i < wc->numweights; ++i )
  {
    if ( !strcmp(wc->weights[i].name, name) )
      return i;
  }
  return -1;
}
//----- (10036A40) --------------------------------------------------------
// Recursive tree walk for the discrete case: tests `facts[index]` against
// each seperator's threshold, descends the matching branch, returns the
// leaf weight. LIVE: core of FuzzyWeight.
float __cdecl FuzzyWeight_r(int *inventory, fuzzyseperator_t *fs)
{
  float scale, w1, w2;

  if ( inventory[fs->index] < fs->value )
  {
    if ( fs->child )
      return FuzzyWeight_r(inventory, fs->child);
    else
      return fs->weight;
  }
  else if ( fs->next )
  {
    if ( inventory[fs->index] < fs->next->value )
    {
      if ( fs->child )
        w1 = FuzzyWeight_r(inventory, fs->child);
      else
        w1 = fs->weight;
      if ( fs->next->child )
        w2 = FuzzyWeight_r(inventory, fs->next->child);
      else
        w2 = fs->next->weight;
      scale = (inventory[fs->index] - fs->value) / (fs->next->value - fs->value);
      return scale * w1 + (1.0f - scale) * w2;
    }
    return FuzzyWeight_r(inventory, fs->next);
  }
  return fs->weight;
}
//----- (10036B10) --------------------------------------------------------
// Recursive tree walk with fuzzy interpolation: when a fact straddles a
// seperator threshold, blends the two branch weights instead of snapping.
// LIVE: core of FuzzyWeightUndecided (used for smooth priority changes).
float __cdecl FuzzyWeightUndecided_r(int *inventory, fuzzyseperator_t *fs)
{
  float scale, w1, w2;

  if ( inventory[fs->index] < fs->value )
  {
    if ( fs->child )
      return FuzzyWeightUndecided_r(inventory, fs->child);
    else
      return fs->minweight + ((rand() & 0x7FFF) * 0.000030518509f) * (fs->maxweight - fs->minweight);
  }
  else if ( fs->next )
  {
    if ( inventory[fs->index] < fs->next->value )
    {
      if ( fs->child )
        w1 = FuzzyWeightUndecided_r(inventory, fs->child);
      else
        w1 = fs->minweight + ((rand() & 0x7FFF) * 0.000030518509f) * (fs->maxweight - fs->minweight);
      if ( fs->next->child )
        w2 = FuzzyWeight_r(inventory, fs->next->child);
      else
        w2 = fs->next->minweight + ((rand() & 0x7FFF) * 0.000030518509f) * (fs->next->maxweight - fs->next->minweight);
      scale = (inventory[fs->index] - fs->value) / (fs->next->value - fs->value);
      return scale * w1 + (1.0f - scale) * w2;
    }
    return FuzzyWeightUndecided_r(inventory, fs->next);
  }
  return fs->weight;
}
//----- (10036C70) --------------------------------------------------------
// Public entry: discrete fuzzy weight for one weight_t (one item/weapon).
// Returns the leaf value of the decision tree given current bot facts.
// LIVE: called by the item/weapon goal scorers every think.
float __cdecl FuzzyWeight(int *facts, weight_t *w)
{
  /* Thin wrapper over FuzzyWeight_r, which returns a float in ST(0). */
  return FuzzyWeight_r(facts, w->firstseperator);
}
//----- (10036CA0) --------------------------------------------------------
// Public entry: fuzzy-interpolated weight for one weight_t. Smooth
// variant of FuzzyWeight; avoids snap behaviour when a fact value
// crosses a threshold. LIVE.
float __cdecl FuzzyWeightUndecided(int *facts, weight_t *w)
{
  /* Binary at 0x10036CA0 is a thin wrapper around FuzzyWeightUndecided_r (returns float). */
  return FuzzyWeightUndecided_r(facts, w->firstseperator);
}
//----- (10036CD0) --------------------------------------------------------
// GA mutation operator: walks a tree and randomly perturbs seperator
// thresholds and leaf weights. DEAD in Gladiator (no callers); intended
// for offline auto-tuning of bot personalities. Live in Q3 via
// EvolveWeightConfig -> BotMutateGoalFuzzyLogic.
void __cdecl EvolveFuzzySeperator_r(fuzzyseperator_t *fs)
{
  do
  {
    if ( fs->child )
    {
      EvolveFuzzySeperator_r(fs->child);
    }
    else if ( fs->type == 1 )
    {
      /* crandom() = 2.0 * (random() - 0.5), with random() = (rand()&0x7FFF)*c.
       * Keep the crandom() value as its own subexpression so the doubling (which
       * MSVC strength-reduces to `fadd st(0),st`) happens BEFORE the
       * (maxweight-minweight) multiply, as in the original. */
      if ( (float)(rand() & 0x7FFF) * 0.000030518509f < 0.01 )
      {
        fs->weight += (2.0 * ((float)(rand() & 0x7FFF) * 0.000030518509f - 0.5))
                    * (fs->maxweight - fs->minweight);
      }
      else
      {
        fs->weight += (2.0 * ((float)(rand() & 0x7FFF) * 0.000030518509f - 0.5))
                    * (fs->maxweight - fs->minweight)
                    * 0.5;
      }
      if ( fs->weight < fs->minweight )
        fs->weight = fs->minweight;
      else if ( fs->weight > fs->maxweight )
        fs->weight = fs->maxweight;
    }
    fs = fs->next;
  }
  while ( fs );
}
//----- (10036DF0) --------------------------------------------------------
/* Run EvolveFuzzySeperator_r over every weight in the config — the outer driver
 * of the GA pipeline, matching Q3's EvolveFuzzyNetwork.  DEAD in Gladiator. */
void __cdecl EvolveWeightConfig(int *config)
{
  int i;
  int off;

  for ( i = 0; i < config[0]; ++i )
  {
    off = i * 8;
    EvolveFuzzySeperator_r((fuzzyseperator_t *)*(int *)((char *)config + off + 8));
  }
}
//----- (10036E30) --------------------------------------------------------
// Uniformly rescale all weights in a subtree by a scalar factor.
// DEAD in Gladiator. Used by the GA pipeline to normalise mutated trees
// and as a primitive for crossover blending. Live in Q3.
void __cdecl ScaleFuzzySeperator_r(fuzzyseperator_t *fs, float scale)
{
  /* Q3's body verbatim, including the recursion on `next` rather than a loop
   * over it.  The loop form kept `scale` live in the FPU across the call, so
   * gcc spilled it as an 80-bit `fstp TBYTE`; the real image just re-pushes the
   * incoming 4-byte parameter slot.  `maxweight + minweight` in that order for
   * the same reason -- gcc evaluates a binary operator left to right, so the
   * real's `fld [+0x14]; fadd [+0x10]` names the operands and their order.
   * Pass `scale` directly: routing it through LODWORD makes GCC insert an
   * int->float conversion that destroys the bit pattern. */
  if ( fs->child )
  {
    ScaleFuzzySeperator_r(fs->child, scale);
  }
  else if ( fs->type == 1 )
  {
    fs->weight = (fs->maxweight + fs->minweight) * scale;
    if ( fs->weight < fs->minweight )
      fs->weight = fs->minweight;
    else if ( fs->weight > fs->maxweight )
      fs->weight = fs->maxweight;
  }
  if ( fs->next )
    ScaleFuzzySeperator_r(fs->next, scale);
}
//----- (10036EB0) --------------------------------------------------------
// Q3 be_ai_weight.c's ScaleWeight, verbatim.  The earlier reading of this as an
// anonymous (count, {name, fs}[]) table with a hand-rolled strcmp was the
// hand-rolled part wrong: gladi386.so's F39 CALLS strcmp, and the "table" is a
// weightconfig_t -- numweights at +0, weights[] at +4, stride 8.
//
// It was also `static`, which is why the ELF audit read it as MISSING: it has no
// callers, and gcc 2.7.2.3 does not merely inline an unreferenced static, it
// drops it.  Same class as be_aas_file.c's max_aaslights initialiser.  The real
// image's F39 is GLOBAL, so the linkage is matched here.
// DEAD in Gladiator — part of the offline GA tuning pipeline.
void __cdecl ScaleWeight(weightconfig_t *config, char *name, float scale)
{
  int i;

  if ( scale < 0 )
    scale = 0;
  else if ( scale > 1 )
    scale = 1;
  for ( i = 0; i < config->numweights; i++ )
  {
    if ( !strcmp(name, config->weights[i].name) )
    {
      ScaleFuzzySeperator_r(config->weights[i].firstseperator, scale);
      break;
    }
  }
}
//----- (10036F90) --------------------------------------------------------
// GA crossover operator: blends two parent trees into a child tree by
// recursively averaging seperator thresholds and leaf weights.
// DEAD in Gladiator. Live in Q3 via InterbreedingGoalFuzzyLogic.
// Together with Evolve* and Write* forms the offline bot-tuning pipeline.
int __cdecl InterbreedFuzzySeperator_r(fuzzyseperator_t *fs1, fuzzyseperator_t *fs2)
{
  int result; // eax
  fuzzyseperator_t *v5; // eax

  while ( 1 )
  {
    result = (int)fs1->child;
    if ( result )
    {
      v5 = fs2->child;
      if ( !v5 )
        return botimport.Print(PRT_ERROR, "can't merge weight configs\n");
      result = InterbreedFuzzySeperator_r(v5, fs2->child);
    }
    else if ( fs1->type == 1 )
    {
      if ( fs2->type != 1 )
        return botimport.Print(PRT_ERROR, "can't merge weight configs\n");
      fs1->weight = (fs2->weight + fs1->weight) * 0.5f;
    }
    fs1 = fs1->next;
    if ( !fs1 )
      return result;
    if ( fs2->next )
      break;
    fs2 = 0;
  }
  return botimport.Print(PRT_ERROR, "can't merge weight configs\n");
}
//----- (10037020) --------------------------------------------------------
// Top-level pair-wise interbreed of two
// weightconfig_t's.  Bails with "can't merge weight configs\n" if the
// configs' numweights disagree; otherwise calls InterbreedFuzzySeperator_r
// on every (a->weights[i].firstseperator, b->weights[i].firstseperator)
// pair.  This is the public entry to the GA crossover used by the offline
// bot-tuning pipeline — dead in Gladiator (live in Q3 InterbreedGoalFuzzyLogic).
void __cdecl InterbreedWeightConfigs(weightconfig_t *a, weightconfig_t *b)
{
  int i;

  if ( a->numweights != b->numweights )
  {
    botimport.Print(PRT_ERROR, "can't merge weight configs\n");
    return;
  }
  for ( i = 0; i < a->numweights; ++i )
    InterbreedFuzzySeperator_r(a->weights[i].firstseperator, b->weights[i].firstseperator);
}
