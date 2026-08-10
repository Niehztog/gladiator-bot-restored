/*
 * be_ai2_dmnet.c — Gladiator Bot v0.96 botlib (Mr. Elusive, 1999), reconstructed
 * from the Windows gladiator.dll.
 *
 * One of the original translation units listed in lcc.mak / linux-i386.mak,
 * carved back out of the monolithic botlib.c.  Its extent in the shipped DLL
 * is 0x1001D2B0..0x10020B10; the boundary evidence -- per-object .text and
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
#include "be_ai2_dmnet.h"
#include "be_aas_bspq2.h"
#include "be_aas_entity.h"
#include "be_aas_main.h"
#include "be_aas_move.h"
#include "be_aas_reach.h"
#include "be_aas_sample.h"
#include "be_ai2_dmq2.h"
#include "be_ai2_main.h"
#include "be_ai_char.h"
#include "be_ai_chat.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"
#include "be_ai_weap.h"
#include "be_ea.h"
#include "be_interface.h"
#include "l_libvar.h"
#include "l_memory.h"
#include "l_utils.h"

int numnodeswitches;     // 0x100644A0 (game ai_dmnet.c; was dword_100644A0)
char nodeswitch[7344];   // 0x10064A80 nodeswitch[MAX_NODESWITCHES+1=51][144] (ai_dmnet.c; was byte_10064A80)

//----- (1001D2B0) --------------------------------------------------------
void BotResetNodeSwitches()
{
  numnodeswitches = 0;
}
//----- (1001D2D0) --------------------------------------------------------
int __cdecl BotDumpNodeSwitches(bot_state_t *bs)
{
  int i; // ebx
  const char *v3; // edx
  char Buffer[1400]; // [esp+10h] [ebp-578h] BYREF

  sprintf(Buffer, "%s at %1.1f switched more than %d AI nodes\n",
          (const char *)ClientName(bs->client), AAS_Time(), 50);
  i = numnodeswitches;
  if ( numnodeswitches > 0 )
  {
    v3 = (const char *)nodeswitch;
    do
    {
      strcat(Buffer, v3);
      v3 += 144;
      --i;
    }
    while ( i );
  }
  return botimport.Print(PRT_FATAL, Buffer);
}
//----- (1001D3A0) --------------------------------------------------------
void __cdecl BotRecordNodeSwitch(bot_state_t *bs, const char *node, const char *str)
{
  /* `node` is the state-name string, reused as the third %s — the original
   * passes its own argument, already on the stack from entry. */

  sprintf(&nodeswitch[144 * numnodeswitches], "%s at %2.1f entered %s: %s\n",
          (const char *)ClientName(bs->client), AAS_Time(), node, str);
  numnodeswitches++;
}
//----- (1001D420) --------------------------------------------------------
/* BotGetFormationGoal (was sub_1001D420) — predict and return a formation
 * follow-position "goal box" for escorting bs->formation_teammate.  Named from
 * Q3's ai_main.h, whose formation_angle/dir/origin/goal fields match this
 * function's writes field-for-field even though nothing else in the Q3 tree
 * ever touches them; this is likely the only logic that populated them.
 *
 * It looks up two names via ClientFromName + AAS_EntityInfo, then:
 *   - validates the first target's origin (AAS_PointAreaNum +
 *     AAS_AreaReachability) and saves it as the predict start;
 *   - takes (origin - old_origin) of the SECOND target's snapshot as a
 *     per-frame velocity, storing it when |v| > 0.1;
 *   - converts that to angles, biases the yaw by formationgoal_yawbias, zeroes
 *     pitch and roll, and predicts 0.1 s of motion from saved_origin + (0,0,1);
 *   - on a water/slime/lava stop (mask 0x38) writes the START position into
 *     the goal origin, otherwise leaves the (0, anglemod(yaw+bias), 0) angles
 *     there; then fills areanum, ±8 mins/maxs and entitynum.
 *
 * DEAD in shipped Gladiator.  Must NOT be static: /O2 would dead-strip it,
 * whereas /INCREMENTAL kept it in the original. */
int BotGetFormationGoal(bot_state_t *bs)
{
  aas_entityinfo_t entinfo; /* [esp+0x44] — entityinfo copy; reused for both lookups */
  aas_clientmove_t move;    /* prediction result; MSVC6 coalesces this slot with its own
                             * by-value return temp and with both AAS_EntityInfo temps */
  vec3_t angles;            /* [esp+0x10] — built (0, anglemod(yaw+bias), 0)           */
  vec3_t forward;           /* [esp+0x1C] — first delta, then AngleVectors output       */
  vec3_t start;             /* [esp+0x28] — saved_origin + (0,0,1) for prediction      */
  vec3_t scaled;            /* [esp+0x38] — VectorScale(forward, 400, ...) for predict */
  int    entnum, areanum, prevent_entnum;
  float  z;
  /* 1. Look up target name → entnum. */
  entnum = ClientFromName(bs->formationgoal_name) + 1;
  entinfo = AAS_EntityInfo(entnum);
  if ( !entinfo.valid )
    goto fail;
  /* 2. Validate the entity sits in a reachable AAS area (origin @ +0x10). */
  areanum = AAS_PointAreaNum(entinfo.origin);
  if ( !areanum )
    goto fail;
  if ( !AAS_AreaReachability(areanum) )
    goto fail;
  /* 3. Save current target origin to bs+0x1144..0x114C. */
  *(int *)&bs->formationgoal_origin[0] = *(int *)&entinfo.origin[0];
  *(int *)&bs->formationgoal_origin[1] = *(int *)&entinfo.origin[1];
  *(int *)&bs->formationgoal_origin[2] = *(int *)&entinfo.origin[2];
  /* 4. Look up the second name, keeping entnum+1 for the goal.  This
   *    AAS_EntityInfo OVERWRITES the same `entinfo` block — step 3 has already
   *    saved the origin it needed. */
  prevent_entnum = ClientFromName((const char *)bs->formation_teammate) + 1;
  entinfo = AAS_EntityInfo(prevent_entnum);
  /* 5. Velocity = origin - old_origin of the second target, BOTH from the same
   *    snapshot.  The acceptance threshold 0.1 is a double. */
  if ( entinfo.valid )
  {
    VectorSubtract(entinfo.origin, entinfo.old_origin, forward);
    if ( VectorLength(forward) > 0.1 )
    {
      *(int *)&bs->formationgoal_dir[0] = *(int *)&forward[0];
      *(int *)&bs->formationgoal_dir[1] = *(int *)&forward[1];
      *(int *)&bs->formationgoal_dir[2] = *(int *)&forward[2];
    }
  }
  /* 6. Velocity to angles: bias the yaw and wrap with anglemod, zero pitch and
   *    roll.  The explicit angles[2] = 0 is redundant after vectoangles but is
   *    in the original — preserved. */
  vectoangles(bs->formationgoal_dir, angles);
  angles[0] = 0.0f;
  angles[1] = anglemod(angles[1] + bs->formationgoal_yawbias);
  /* 7. start = saved_origin + (0,0,1); scaled = AngleVectors(angles) * 400.
   *    The first two start components are int bit-copies, the third a
   *    float add. */
  angles[2] = 0.0f;
  AngleVectors(angles, forward, NULL, NULL);
  *(int *)&start[0] = *(int *)&bs->formationgoal_origin[0];
  *(int *)&start[1] = *(int *)&bs->formationgoal_origin[1];
  start[2] = bs->formationgoal_origin[2] + 1.0f;
  VectorScale(forward, 400.0f, scaled);
  /* 8. Predict 0.1 s of motion from start at velocity `scaled`; stopevent mask
   *    0x7C catches HITGROUND/HITWATER/HITLAVA/HITSLIME. */
  move = AAS_ClientMovementPrediction(-1, start,
                                      2, 1, vec3_origin, scaled,
                                      1, 2, 0.1f, 124, 0);
  /* 9. On a water/slime/lava stop (mask 0x38) fall back to the start position;
   *    otherwise keep the angles vec built in step 6. */
  if ( (move.stopevent & 0x38) != 0 )
  {
    angles[0] = start[0];
    angles[1] = start[1];
    z = start[2];
  }
  else
  {
    z = angles[2];
  }
  bs->formationgoal.areanum = areanum;
  bs->formationgoal.origin[2] = z;
  *(int *)&bs->formationgoal.origin[1] = *(int *)&angles[1];
  bs->formationgoal.entitynum = prevent_entnum;
  bs->formationgoal.mins[0] = -8.0f;
  bs->formationgoal.mins[1] = -8.0f;
  bs->formationgoal.mins[2] = -8.0f;
  bs->formationgoal.maxs[0] = 8.0f;
  bs->formationgoal.maxs[1] = 8.0f;
  bs->formationgoal.maxs[2] = 8.0f;
  *(int *)&bs->formationgoal.origin[0] = *(int *)&angles[0];
fail:
  return (int)(intptr_t)&bs->formationgoal;
}
//----- (1001D760) --------------------------------------------------------
/* BotLongTermGoal — dispatch the bot's current long-term-goal state and return
 * the goal position to seek, or NULL if no goal is active.
 *
 *     tfl = travel flags (TFL_* mask in the low 17 bits)
 * retreat = 0 for full LTG processing (from AINode_Seek_LTG), 1 to only check
 *           whether the goal is still valid (from AINode_Battle_Retreat)
 *
 * LTG sub-types in the switch:
 *   1 = team-leader / accompany-ready      2 = accompany bs->teammate
 *   3 = defend a goal item                 4 = CTF goal (returns the flag base)
 *   6 = camp at goal                       7 = patrol checkpoints
 *
 * Q3's BotLongTermGoal fills a bot_goal_t out-param instead of returning a
 * pointer into bs. */
float *__cdecl BotLongTermGoal(bot_state_t *bs, int tfl, int retreat)
{
  int v7; // eax
  float *result; // eax
  float *v17; // edi
  float v18; // st7
  int v20; // rax (was __int64)
  int v21; // eax
  float *v26; // esi
  float v30; // st7
  float v34; // st7
  bot_waypoint_t *i; // edx
  bot_waypoint_t *v37; // eax
  int v38; // eax
  bot_waypoint_t *v39; // ecx
  bot_waypoint_t *v40; // edx
  bot_waypoint_t *v41; // edx
  bot_waypoint_t *v42; // eax
  float v43; // st7
  float *v44; // eax
  int v45; // [esp+18h] [ebp-230h]
  float v46; // [esp+18h] [ebp-230h]
  float v47; // [esp+18h] [ebp-230h]
  vec3_t dir; // [esp+1Ch] [ebp-22Ch] BYREF — direction vector (target - bot.origin) for VectorLength/vectoangles
  float croucher; // [esp+28h] [ebp-220h]
  vec3_t target; // [esp+2Ch] [ebp-21Ch] BYREF — BotRoamGoal output position
  int entinfo[31]; // [esp+38h] [ebp-210h] BYREF
  char netname[128]; // [esp+B4h] [ebp-194h] BYREF
  char buf[152]; // [esp+134h] [ebp-114h] BYREF

  if ( bs->ltgtype != 1 || retreat )
  {
    if ( bs->ltgtype == 2 && !retreat )
    {
      if ( bs->teammessage_time != 0 && AAS_Time() > bs->teammessage_time )
      {
        BotInitialChat(&bs->chatstate, "accompany_start", EasyClientName(bs->teammate - 1, netname), (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        bs->teammessage_time = 0.0f;
      }
      if ( AAS_Time() > bs->teamgoal_time )
      {
        BotInitialChat(&bs->chatstate, "accompany_stop", EasyClientName(bs->teammate - 1, netname), (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        bs->ltgtype = 0;
      }
      *(aas_entityinfo_t *)entinfo = AAS_EntityInfo(bs->teammate);
      if ( BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0, bs->teammate) )
      {
        bs->teammatevisible_time = AAS_Time();
        v17 = bs->origin;
        dir[0] = *(float *)&entinfo[4] - bs->origin[0];
        dir[1] = *(float *)&entinfo[5] - bs->origin[1];
        dir[2] = *(float *)&entinfo[6] - bs->origin[2];
        if ( VectorLength(dir) < bs->formation_dist )
        {
          v18 = AAS_Time() - 5;
          if ( v18 > bs->attackcrouch_time )
          {
            /* The BFloat result feeds both the rand check and the *15.0 jump-time
             * factor below. */
            croucher = (float)Characteristic_BFloat(BotCharacter(bs), 24, 0.0, 1.0);
            if ( (float)(rand() & 0x7FFF) * 0.000030518509f < croucher * bs->thinktime )
              bs->attackcrouch_time = AAS_Time() + croucher * 15 + 5;
          }
          if ( AAS_Swimming(bs->origin) )
            bs->attackcrouch_time = AAS_Time() - 1;
          if ( AAS_Time() - 2 > bs->arrive_time )
          {
            if ( bs->arrive_time == 0 )
            {
              sub_100371B0(bs->client, 1);
              BotInitialChat(&bs->chatstate, "accompany_arrive", EasyClientName(bs->teammate - 1, netname), (char *)0);
              BotEnterChat(&bs->chatstate, bs->client, 1);
              bs->arrive_time = AAS_Time();
            }
            else if ( AAS_Time() < bs->attackcrouch_time )
            {
              EA_Crouch(bs->client);
            }
            else if ( (float)(rand() & 0x7FFF) * 0.000030518509f < bs->thinktime * 0.3 )
            {
              v45 = rand() & 0x7FFF;
              v20 = (int)floor((float)v45 * 0.000030518509f * 2.9);
              switch ( v20 )
              {
                case 0:
                  sub_100371B0(bs->client, 0);
                  break;
                case 1:
                  sub_100371B0(bs->client, 2);
                  break;
                default:
                  sub_100371B0(bs->client, 3);
                  break;
              }
            }
          }
          if ( AAS_Time() - 2 < bs->arrive_time )
          {
            dir[0] = *(float *)&entinfo[4] - *v17;
            dir[1] = *(float *)&entinfo[5] - bs->origin[1];
            dir[2] = *(float *)&entinfo[6] - bs->origin[2];
            vectoangles(dir, bs->ideal_viewangles);
          }
          else if ( (float)(rand() & 0x7FFF) * 0.000030518509f < bs->thinktime * 0.8 )
          {
            BotRoamGoal(bs, target);
            dir[0] = target[0] - *v17;
            dir[1] = target[1] - bs->origin[1];
            dir[2] = target[2] - bs->origin[2];
            vectoangles(dir, bs->ideal_viewangles);
          }
          else
          {
            goto LABEL_RESET;
          }
          bs->ideal_viewangles[2] = bs->ideal_viewangles[2] * 0.5;
          goto LABEL_RESET;
        }
      }
      if ( entinfo[0] )
      {
        v21 = AAS_PointAreaNum(&entinfo[4]);
        if ( v21 )
        {
          if ( AAS_AreaReachability(v21) )
          {
            bs->teamgoal.origin[1] = *(float *)&entinfo[5];
            bs->teamgoal.entitynum = bs->teammate;
            bs->teamgoal.mins[0] = -8.0f;
            bs->teamgoal.mins[1] = -8.0f;
            bs->teamgoal.mins[2] = -8.0f;
            bs->teamgoal.areanum = v21;
            bs->teamgoal.origin[0] = *(float *)&entinfo[4];
            bs->teamgoal.origin[2] = *(float *)&entinfo[6];
            bs->teamgoal.maxs[0] = 8.0f;
            bs->teamgoal.maxs[1] = 8.0f;
            bs->teamgoal.maxs[2] = 8.0f;
          }
        }
      }
      v26 = bs->teamgoal.origin;
      if ( AAS_Time() - 60 > bs->teammatevisible_time )
      {
        BotInitialChat(&bs->chatstate, "accompany_cannotfind", EasyClientName(bs->teammate - 1, netname), (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        goto LABEL_55;
      }
      return v26;
    }
    if ( bs->ltgtype == 3 && AAS_Time() > bs->defendaway_time && !retreat )
    {
      if ( bs->teammessage_time != 0 && AAS_Time() > bs->teammessage_time )
      {
        BotInitialChat(&bs->chatstate, "defend_start", BotGoalName(bs->teamgoal.number), (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        bs->teammessage_time = 0.0f;
      }
      v26 = bs->teamgoal.origin;
      if ( AAS_Time() > bs->teamgoal_time )
      {
        BotInitialChat(&bs->chatstate, "defend_stop", BotGoalName(bs->teamgoal.number), (char *)0);
        BotEnterChat(&bs->chatstate, bs->client, 1);
        bs->ltgtype = 0;
      }
      VectorSubtract(v26, bs->origin, dir);
      if ( VectorLength(dir) < 70 )
      {
        BotResetAvoidReach((_DWORD *)&bs->ms);
        v46 = ((float)(rand() & 0x7FFF) * 0.000030518509f) * 10;
        v30 = AAS_Time();
        result = bs->teamgoal.origin;
        bs->defendaway_time = v30 + v46 + 5;
        return result;
      }
      return v26;
    }

    if ( bs->ltgtype == 6 )
    {
        if ( bs->teammessage_time != 0 && AAS_Time() > bs->teammessage_time )
        {
          BotInitialChat(&bs->chatstate, "camp_start", EasyClientName(bs->teammate - 1, netname), (char *)0);
          BotEnterChat(&bs->chatstate, bs->client, 1);
          bs->teammessage_time = 0.0f;
        }
        v26 = bs->teamgoal.origin;
        if ( AAS_Time() > bs->teamgoal_time )
        {
          BotInitialChat(&bs->chatstate, "camp_stop", (char *)0);
          BotEnterChat(&bs->chatstate, bs->client, 1);
          bs->ltgtype = 0;
        }
        VectorSubtract(v26, bs->origin, dir);
        if ( VectorLength(dir) < 40 )
        {
          if ( bs->arrive_time == 0 )
          {
            BotInitialChat(&bs->chatstate, "camp_arrive", EasyClientName(bs->teammate - 1, netname), (char *)0);
            BotEnterChat(&bs->chatstate, bs->client, 1);
            bs->arrive_time = AAS_Time();
          }
          if ( (float)(rand() & 0x7FFF) * 0.000030518509f < bs->thinktime * 0.8 )
          {
            BotRoamGoal(bs, target);   /* named `bs`, not `a1`: that alias collides with the global `char a1[2]` */
            VectorSubtract(target, bs->origin, dir);
            vectoangles(dir, bs->ideal_viewangles);
            bs->ideal_viewangles[2] = bs->ideal_viewangles[2] * 0.5;
          }
          v34 = AAS_Time() - 5;
          if ( v34 > bs->attackcrouch_time )
          {
            /* Same squat-jump pattern as the BFloat use above. */
            croucher = (float)Characteristic_BFloat(BotCharacter(bs), 24, 0.0, 1.0);
            if ( (float)(rand() & 0x7FFF) * 0.000030518509f < croucher * bs->thinktime )
              bs->attackcrouch_time = AAS_Time() + croucher * 15 + 5;
          }
          if ( AAS_Time() < bs->attackcrouch_time )
            EA_Crouch(bs->client);
          if ( AAS_Swimming(bs->origin) )
            bs->attackcrouch_time = AAS_Time() - 1;
          if ( (sub_10003080((float *)bs->eye) & 0x38) != 0 )
          {
            BotInitialChat(&bs->chatstate, "camp_stop", (char *)0);
            BotEnterChat(&bs->chatstate, bs->client, 1);
            bs->ltgtype = 0;
          }
LABEL_RESET:
          BotResetAvoidReach((_DWORD *)&bs->ms);
          return 0;
        }
        return v26;
    }
    if ( bs->ltgtype == 7 )
    {
        if ( bs->teammessage_time != 0 && AAS_Time() > bs->teammessage_time )
        {
          strcpy(buf, "");
          for ( i = BotPatrolpoints(bs); i; i = i->next )
          {
            strcat(buf, i->name);
            if ( i->next )
              strcat(buf, " to ");
          }
          BotInitialChat(&bs->chatstate, "patrol_start", buf, (char *)0);
          BotEnterChat(&bs->chatstate, bs->client, 1);
          bs->teammessage_time = 0.0f;
        }
        v37 = BotCurPatrolPoint(bs);
        if ( !v37 )
        {
          bs->ltgtype = 0;
          return 0;
        }
        if ( !BotTouchingGoal(bs->origin, &v37->goal) )
          goto LABEL_106;
        v38 = bs->patrolflags;
        v39 = BotCurPatrolPoint(bs);
        if ( (v38 & 4) != 0 )
        {
          v40 = v39->prev;
          if ( v40 )
          {
            BotCurPatrolPoint(bs) = v40;
LABEL_106:
            if ( AAS_Time() > bs->teamgoal_time )
            {
              BotInitialChat(&bs->chatstate, "patrol_stop", (char *)0);
              BotEnterChat(&bs->chatstate, bs->client, 1);
              bs->ltgtype = 0;
            }
            v42 = BotCurPatrolPoint(bs);
            if ( !v42 )
            {
              bs->ltgtype = 0;
              return 0;
            }
            return v42->goal.origin;
          }
          v38 &= 0xFFFFFFFB;
          BotCurPatrolPoint(bs) = v39->next;
        }
        else
        {
          v41 = v39->next;
          if ( v41 )
          {
            BotCurPatrolPoint(bs) = v41;
            goto LABEL_106;
          }
          v38 |= 4;
          BotCurPatrolPoint(bs) = v39->prev;
        }
        bs->patrolflags = v38;
        goto LABEL_106;
    }
    if ( bs->ltgtype == 4 )
    {
        if ( bs->teammessage_time != 0 && AAS_Time() > bs->teammessage_time )
        {
          BotInitialChat(&bs->chatstate, "captureflag_start", (char *)0);
          BotEnterChat(&bs->chatstate, bs->client, 1);
          bs->teammessage_time = 0.0f;
        }
        switch ( BotCTFTeam(bs) )
        {
          case 1:
            v26 = ctf_blueflag.origin;
            break;
          default:
            v26 = ctf_redflag.origin;
            break;
        }
        if ( BotTouchingGoal(bs->origin, v26) )
          bs->ltgtype = 0;
        if ( AAS_Time() > bs->teamgoal_time )
        {
          bs->ltgtype = 0;
          return v26;
        }
        return v26;
    }
    if ( bs->ltgtype == 5 && AAS_Time() > bs->rushbaseaway_time )
    {
      switch ( BotCTFTeam(bs) )
      {
        case 1:
          v26 = ctf_redflag.origin;
          break;
        default:
          v26 = ctf_blueflag.origin;
          break;
      }
      if ( AAS_Time() > bs->teamgoal_time )
        bs->ltgtype = 0;
      if ( BotTouchingGoal(bs->origin, v26) )
      {
        if ( BotCTFCarryingFlag(bs) )
        {
          BotResetAvoidReach((_DWORD *)&bs->ms);
          v47 = ((float)(rand() & 0x7FFF) * 0.000030518509f) * 10;
          v43 = AAS_Time();
          bs->rushbaseaway_time = v43 + v47 + 5;
          return v26;
        }
LABEL_55:
        bs->ltgtype = 0;
        return v26;
      }
      return v26;
    }
    v44 = (float *)BotGetTopGoal(&bs->goalstate);
    v26 = v44;
    if ( v44 )
    {
      if ( BotTouchingGoal(bs->origin, v44) )
      {
        if ( libvar_runes->value != 0.0f )
          sub_100262C0((_DWORD *)bs, v26);   /* named `bs`, not `a1`: that alias collides with the global `char a1[2]` */
        bs->ltg_time = 0.0f;
      }
      else if ( BotItemGoalInVisButNotVisible(bs->entitynum, bs->eye, bs->viewangles, (bot_goal_t *)v26) )
      {
        bs->ltg_time = 0.0f;
      }
    }
    else
    {
      bs->ltg_time = 0.0f;
    }
    if ( AAS_Time() > bs->ltg_time )
    {
      BotPopGoal(&bs->goalstate);
      if ( BotChooseLTGItem(&bs->goalstate, bs->origin, bs->inventory, tfl) )
      {
        bs->ltg_time = AAS_Time() + 20;
      }
      else
      {
        BotResetAvoidGoals(&bs->goalstate);
        BotResetAvoidReach((_DWORD *)&bs->ms);
      }
      v26 = (float *)BotGetTopGoal(&bs->goalstate);
    }
    return v26;
  }
  if ( bs->teammessage_time != 0 && AAS_Time() > bs->teammessage_time )
  {
    BotInitialChat(&bs->chatstate, "help_start", EasyClientName(bs->teammate - 1, netname), (char *)0);
    BotEnterChat(&bs->chatstate, bs->client, 1);
    bs->teammessage_time = 0.0f;
  }
  if ( AAS_Time() > bs->teamgoal_time )
    bs->ltgtype = 0;
  if ( AAS_Time() - 10 > bs->teammatevisible_time )
    bs->ltgtype = 0;
  *(aas_entityinfo_t *)entinfo = AAS_EntityInfo(bs->teammate);
  if ( BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0, bs->teammate) )
  {
    dir[0] = *(float *)&entinfo[4] - bs->origin[0];
    dir[1] = *(float *)&entinfo[5] - bs->origin[1];
    dir[2] = *(float *)&entinfo[6] - bs->origin[2];
    if ( VectorLength(dir) < 100 )
      goto LABEL_RESET;
  }
  else
  {
    bs->teammatevisible_time = AAS_Time();
  }
  v26 = bs->teamgoal.origin;
  if ( entinfo[0] )
  {
    v7 = AAS_PointAreaNum(&entinfo[4]);
    if ( v7 )
    {
      if ( AAS_AreaReachability(v7) )
      {
        bs->teamgoal.origin[0] = *(float *)&entinfo[4];
        bs->teamgoal.entitynum = bs->teammate;
        bs->teamgoal.mins[0] = -8.0f;
        bs->teamgoal.mins[1] = -8.0f;
        bs->teamgoal.mins[2] = -8.0f;
        bs->teamgoal.areanum = v7;
        bs->teamgoal.origin[1] = *(float *)&entinfo[5];
        bs->teamgoal.origin[2] = *(float *)&entinfo[6];
        bs->teamgoal.maxs[0] = 8.0f;
        bs->teamgoal.maxs[1] = 8.0f;
        bs->teamgoal.maxs[2] = 8.0f;
      }
    }
  }
  return v26;
}
//----- (1001EAE0) --------------------------------------------------------
void __cdecl AIEnter_Intermission(bot_state_t *bs)
{
  BotRecordNodeSwitch(bs, "intermission", "");
  BotResetState(bs);
  if ( BotChat_EndLevel(bs) )
    BotEnterChat(&bs->chatstate, bs->client, 0);
  BotAINode(bs) = AINode_Intermission;
}
//----- (1001EB50) --------------------------------------------------------
int __cdecl AINode_Intermission(bot_state_t *bs)
{
  float v2; // st7
  float v4; // [esp+8h] [ebp+4h]

  if ( !BotIntermission(bs) )
  {
    if ( BotChat_StartLevel(bs) )
    {
      v4 = BotChatTime(bs);
      v2 = AAS_Time() + v4;
    }
    else
    {
      v2 = AAS_Time() + 2.0f;
    }
    *(float *)((char *)bs + 2812) = v2;
    AIEnter_Stand(bs);
  }
  return 1;
}
//----- (1001EBD0) --------------------------------------------------------
void __cdecl AIEnter_Observer(bot_state_t *bs)
{
  BotRecordNodeSwitch(bs, "observer", "");
  BotResetState(bs);
  BotAINode(bs) = AINode_Observer;
}
//----- (1001EC10) --------------------------------------------------------
int __cdecl AINode_Observer(bot_state_t *bs)
{
  if ( !BotIsObserver(bs) )
    AIEnter_Stand(bs);
  return 1;
}
//----- (1001EC50) --------------------------------------------------------
void __cdecl AIEnter_Stand(bot_state_t *bs)
{
  BotRecordNodeSwitch(bs, "stand", "");
  BotAINode(bs) = AINode_Stand;
}
//----- (1001EC90) --------------------------------------------------------
int __cdecl AINode_Stand(bot_state_t *bs)
{

  if ( BotFindEnemy(bs) )
  {
    AIEnter_Battle_Fight(bs);
    return 0;
  }
  BotChangeViewAngles(bs, bs->thinktime);
  if ( AAS_Time() > bs->stand_time )
  {
    if ( LibVarGetValue("__squatt") != 0.0f )
    {
      EA_Say(bs->client, "I never hacked your brain...\n");
      EA_Command(bs->client, "removebot", ClientName(bs->client), (void *)0);
    }
    else
    {
      BotEnterChat(&bs->chatstate, bs->client, 0);
      AIEnter_Seek_LTG(bs);
      return 0;
    }
  }
  return 1;
}
//----- (1001ED80) --------------------------------------------------------
void __cdecl AIEnter_Respawn(bot_state_t *bs)
{
  double v2; // st7
  float v3; // [esp+10h] [ebp+4h]

  BotRecordNodeSwitch(bs, "respawn", "");
  BotResetMoveState(&bs->ms);
  BotResetGoalState(&bs->goalstate);
  BotResetWeaponState(BotWS(bs));
  BotResetAvoidGoals(&bs->goalstate);
  BotResetAvoidReach((_DWORD *)&bs->ms);
  if ( BotChat_Death((int *)bs) )
  {
    v3 = BotChatTime(bs);
    v2 = AAS_Time() + v3;
  }
  else
  {
    v2 = AAS_Time();
  }
  bs->respawnchat_time = v2;
  bs->respawn_wait = 0;
  BotAINode(bs) = AINode_Respawn;
}
//----- (1001EE40) --------------------------------------------------------
int __cdecl AINode_Respawn(bot_state_t *bs)
{

  int v2; // eax

  if ( bs->respawn_wait )
  {
    if ( !BotIsDead(bs) )
    {
      AIEnter_Seek_LTG(bs);
      return 1;
    }
  }
  else if ( AAS_Time() > bs->respawnchat_time )
  {
    v2 = bs->client;
    bs->respawn_wait = 1;
    EA_Respawn(v2);
    if ( bs->enemy )
    {
      BotEnterChat(&bs->chatstate, bs->client, 0);
      bs->enemy = 0;
    }
  }
  return 1;
}
//----- (1001EF00) --------------------------------------------------------
void __cdecl AIEnter_Seek_ActivateEntity(bot_state_t *bs)
{
  BotRecordNodeSwitch(bs, "activate entity", "");
  BotAINode(bs) = AINode_Seek_ActivateEntity;
}
//----- (1001EF40) --------------------------------------------------------
int __cdecl AINode_Seek_ActivateEntity(bot_state_t *bs)
{

  float *ent;             // esi — &bs->activategoal.origin[0] (embedded activate-goal struct)
  int v8;                 // [esp+0xc] — movement flags
  vec3_t target;          // [esp+0x14] BYREF — predicted move target
  vec3_t dir;              // [esp+0x20] BYREF — target - origin, fed to vectoangles
  bot_moveresult_t v15;   // [esp+0x2c] BYREF — copy of BotMoveToGoal result
  bot_moveresult_t v16;   // [esp+0x5c] BYREF — BotMoveToGoal output buffer

  if ( BotIsObserver(bs) )
  {
    AIEnter_Observer(bs);
    return 0;
  }
  if ( BotIntermission(bs) )
  {
    AIEnter_Intermission(bs);
    return 0;
  }
  if ( BotIsDead(bs) )
  {
    AIEnter_Respawn(bs);
    return 0;
  }
  v8 = 102334;
  if ( libvar_usehook->value != 0.0f )
    v8 = 118718;
  bs->enemy = 0;
  ent = bs->activategoal.origin;
  if ( !ent || BotTouchingGoal(bs->origin, ent) )
    *(int *)&bs->activategoal_time = 0;
  if ( AAS_Time() > bs->activategoal_time )
  {
    AIEnter_Seek_NBG(bs);
    return 0;
  }
  BotBattleUseItems(bs);
  BotEntityInfo(bs, (_DWORD *)&bs->ms);
  v15 = *BotMoveToGoal(&v16, (bot_movestate_t *)&bs->ms, (bot_goal_t *)(intptr_t)ent, v8);
  if ( v15.failure )
  {
    BotResetAvoidReach((_DWORD *)&bs->ms);
    bs->nbg_time = 0.0f;
  }
  BotAIBlocked(bs, &v15, 1);
  if ( (v15.flags & 3) != 0 )
  {
    VectorCopy(v15.ideal_viewangles, bs->ideal_viewangles);
  }
  else
  {
    if ( BotMovementViewTarget((bot_movestate_t *)&bs->ms, (bot_goal_t *)(intptr_t)ent, v8, (float *)(intptr_t)target) )
    {
      VectorSubtract(target, bs->origin, dir);
      vectoangles(dir, bs->ideal_viewangles);
    }
    else
    {
      vectoangles(v15.movedir, bs->ideal_viewangles);
    }
    bs->ideal_viewangles[2] = bs->ideal_viewangles[2] * 0.5;
  }
  if ( BotFindEnemy(bs) )
  {
    if ( BotWantsToRetreat((int *)bs) )
    {
      AIEnter_Battle_NBG(bs);
    }
    else
    {
      BotResetLastAvoidReach((intptr_t)&bs->ms);
      BotEmptyGoalStack(&bs->goalstate);
      AIEnter_Battle_Fight(bs);
    }
  }
  BotChangeViewAngles(bs, bs->thinktime);
  return 1;
}
//----- (1001F210) --------------------------------------------------------
void __cdecl AIEnter_Seek_NBG(bot_state_t *bs)
{
  bot_goal_t *goal; // eax
  const char *v2; // eax

  goal = (bot_goal_t *)BotGetTopGoal(&bs->goalstate);
  if ( goal )
  {
    v2 = (const char *)BotGoalName(goal->number);
    BotRecordNodeSwitch(bs, "seek NBG", v2);
  }
  else
  {
    BotRecordNodeSwitch(bs, "seek NBG", "no goal");
  }
  BotAINode(bs) = AINode_Seek_NBG;
}
//----- (1001F290) --------------------------------------------------------
int __cdecl AINode_Seek_NBG(bot_state_t *bs)
{

  /* v3/goal stay integer here — the original holds the goal pointer in esi
   * throughout, with no FPU traffic. */
  void *v3; // eax
  void *goal; // esi
  int v5; // edx
  int v6; // eax
  void *v7; // edi
  int v8; // [esp+10h] [ebp-7Ch]
  vec3_t target; // [esp+14h] [ebp-78h] BYREF — predicted/move target position
  vec3_t dir; // [esp+20h] [ebp-6Ch] BYREF — target - bot origin, fed to vectoangles
  bot_moveresult_t moveresult; // [esp+2Ch] [ebp-60h] BYREF
  bot_moveresult_t v16; // [esp+5Ch] [ebp-30h] BYREF

  if ( BotIsObserver(bs) )
  {
    AIEnter_Observer(bs);
    return 0;
  }
  if ( BotIntermission(bs) )
  {
    AIEnter_Intermission(bs);
    return 0;
  }
  if ( BotIsDead(bs) )
  {
    AIEnter_Respawn(bs);
    return 0;
  }
  v8 = 102334;
  if ( libvar_usehook->value != 0.0f )
    v8 = 118718;
  if ( libvar_rocketjump->value != 0.0f && BotCanAndWantsToRocketJump(bs) )
  {
    v8 |= 0x1000;
  }
  bs->enemy = 0;
  v3 = BotGetTopGoal(&bs->goalstate);
  goal = v3;
  if ( v3 )
  {
    if ( BotTouchingGoal(bs->origin, (float *)v3) )
    {
      if ( libvar_runes->value != 0.0f )
        sub_100262C0((_DWORD *)bs, goal);
      bs->nbg_time = 0.0f;
    }
    else if ( BotItemGoalInVisButNotVisible(bs->entitynum, bs->eye, bs->viewangles, (bot_goal_t *)goal) )
    {
      bs->nbg_time = 0.0f;
    }
  }
  else
  {
    bs->nbg_time = 0.0f;
  }
  if ( AAS_Time() > bs->nbg_time )
  {
    BotPopGoal(&bs->goalstate);
    AIEnter_Seek_LTG(bs);
    return 0;
  }
  BotBattleUseItems(bs);
  BotEntityInfo(bs, (_DWORD *)&bs->ms);
  moveresult = *BotMoveToGoal(&v16, (bot_movestate_t *)&bs->ms, (bot_goal_t *)(intptr_t)goal, v8);
  if ( moveresult.failure )
  {
    BotResetAvoidReach((_DWORD *)&bs->ms);
    bs->nbg_time = 0.0f;
  }
  BotAIBlocked(bs, &moveresult, 1);
  if ( (moveresult.flags & 3) != 0 )
  {
  v5 = LODWORD(moveresult.ideal_viewangles[1]);
  v6 = LODWORD(moveresult.ideal_viewangles[2]);
  *(int *)&bs->ideal_viewangles[0] = LODWORD(moveresult.ideal_viewangles[0]);
  *(int *)&bs->ideal_viewangles[1] = v5;
  *(int *)&bs->ideal_viewangles[2] = v6;
  }
  else if ( (moveresult.flags & 4) != 0 )
  {
    /* rand() side first, as in the original, so the double product
     * (thinktime*0.8) is not spilled to a QWORD slot across the call. */
    if ( (float)(rand() & 0x7FFF) * 0.000030518509f < bs->thinktime * 0.8 )
    {
    BotRoamGoal(bs, target);   /* aarch64: was `a1` — see note in BotLongTermGoal */
    VectorSubtract(target, bs->origin, dir);
    vectoangles(dir, bs->ideal_viewangles);
    bs->ideal_viewangles[2] = bs->ideal_viewangles[2] * 0.5;
    }
  }
  else
  {
    v7 = BotGetSecondGoal(&bs->goalstate);
    if ( !v7 )
      BotGetTopGoal(&bs->goalstate);
    if ( BotMovementViewTarget((bot_movestate_t *)&bs->ms, (bot_goal_t *)(intptr_t)v7, v8, (float *)(intptr_t)target) )
    {
      VectorSubtract(target, bs->origin, dir);
      vectoangles(dir, bs->ideal_viewangles);
      bs->ideal_viewangles[2] = bs->ideal_viewangles[2] * 0.5;
    }
    else
    {
      vectoangles(moveresult.movedir, bs->ideal_viewangles);
      bs->ideal_viewangles[2] = bs->ideal_viewangles[2] * 0.5;
    }
  }
  if ( BotFindEnemy(bs) )
  {
    if ( BotWantsToRetreat((int *)bs) )
    {
      AIEnter_Battle_NBG(bs);
    }
    else
    {
      BotResetLastAvoidReach((intptr_t)&bs->ms);
      BotEmptyGoalStack(&bs->goalstate);
      AIEnter_Battle_Fight(bs);
    }
  }
  if ( (moveresult.flags & 8) == 0 )
    BotChangeViewAngles(bs, bs->thinktime);
  return 1;
}
//----- (1001F6E0) --------------------------------------------------------
void __cdecl AIEnter_Seek_LTG(bot_state_t *bs)
{
  bot_goal_t *goal; // eax
  const char *v2; // eax

  goal = (bot_goal_t *)BotGetTopGoal(&bs->goalstate);
  if ( goal )
  {
    v2 = (const char *)BotGoalName(goal->number);
    BotRecordNodeSwitch(bs, "seek LTG", v2);
  }
  else
  {
    BotRecordNodeSwitch(bs, "seek LTG", "no goal");
  }
  BotAINode(bs) = AINode_Seek_LTG;
}
//----- (1001F760) --------------------------------------------------------
int __cdecl AINode_Seek_LTG(bot_state_t *bs)
{

  int v2; // edi
  bot_goal_t *goal; // 64-bit fix (was int) - BotLongTermGoal returns goal pointer
  int range; // [esp+0h] — the outgoing float arg slot; original int local (Q3 ai_dmnet.c AINode_Seek_LTG shape)
  float v9; // [esp+14h] [ebp-80h]
  vec3_t target; // [esp+18h] [ebp-7Ch] BYREF — predicted/move target position
  vec3_t dir; // [esp+24h] [ebp-70h] BYREF — target - bot origin, fed to vectoangles
  bot_moveresult_t moveresult; // [esp+34h] [ebp-60h] BYREF
  bot_moveresult_t v19; // [esp+64h] [ebp-30h] BYREF

  if ( BotIsObserver(bs) )
  {
    AIEnter_Observer(bs);
    return 0;
  }
  if ( BotIntermission(bs) )
  {
    AIEnter_Intermission(bs);
    return 0;
  }
  if ( BotIsDead(bs) )
  {
    AIEnter_Respawn(bs);
    return 0;
  }
  if ( BotChat_Random(bs) )
  {
    v9 = BotChatTime(bs);
    bs->stand_time = AAS_Time() + v9;
    AIEnter_Stand(bs);
    return 0;
  }
  v2 = 102334;
  if ( libvar_usehook->value != 0.0f )
  {
    v2 = 118718;
  }
  if ( libvar_rocketjump->value != 0.0f && BotCanAndWantsToRocketJump(bs) )
  {
    v2 |= 0x1000u;
  }
  bs->enemy = 0;
  if ( AAS_Time() - 5.0f < bs->killedenemy_time
    && (float)(rand() & 0x7FFF) * 0.000030518509f < bs->thinktime )
  {
    if ( (float)(rand() & 0x7FFF) * 0.000030518509f < 0.5 )
      sub_100371B0(bs->client, 0);
    else
      sub_100371B0(bs->client, 2);
  }
  if ( BotFindEnemy(bs) )
  {
    if ( BotWantsToRetreat((int *)bs) )
    {
      AIEnter_Battle_Retreat(bs);
    }
    else
    {
      BotResetLastAvoidReach((intptr_t)&bs->ms);
      BotEmptyGoalStack(&bs->goalstate);
      AIEnter_Battle_Fight(bs);
    }
    return 0;
  }
  if ( libvar_ctf->value != 0.0f )
    BotCTFSeekGoals(bs);
    goal = (bot_goal_t *)BotLongTermGoal(bs, v2, 0);
    if ( goal )
    {
    if ( AAS_Time() > bs->check_time )
    {
      bs->check_time = AAS_Time() + 0.5;
      if ( bs->ltgtype == 3 ) /* LTG_DEFENDKEYAREA */
        range = 1500;
      else
        range = 700;
      if ( BotChooseNBGItem(&bs->goalstate, bs->origin, bs->inventory, v2, goal, range) )
      {
        BotResetLastAvoidReach((intptr_t)&bs->ms);
        bs->nbg_time = AAS_Time() + 5.0f;
        AIEnter_Seek_NBG(bs);
        return 0;
      }
    }
    BotBattleUseItems(bs);
    BotEntityInfo(bs, (_DWORD *)&bs->ms);
    moveresult = *BotMoveToGoal(&v19, (bot_movestate_t *)&bs->ms, (bot_goal_t *)(intptr_t)goal, v2);
    if ( moveresult.failure )
    {
      BotResetAvoidReach((_DWORD *)&bs->ms);
      bs->ltg_time = 0.0f;
    }
    BotAIBlocked(bs, &moveresult, 1);
    if ( (moveresult.flags & 3) != 0 )
    {
      *(int *)&bs->ideal_viewangles[0] = LODWORD(moveresult.ideal_viewangles[0]);
      *(int *)&bs->ideal_viewangles[1] = LODWORD(moveresult.ideal_viewangles[1]);
      *(int *)&bs->ideal_viewangles[2] = LODWORD(moveresult.ideal_viewangles[2]);
    }
    else
    {
    if ( (moveresult.flags & 4) != 0 )
    {
      if ( (float)(rand() & 0x7FFF) * 0.000030518509f < bs->thinktime * 0.8 )
      {
      BotRoamGoal(bs, target);
      VectorSubtract(target, bs->origin, dir);
      vectoangles(dir, bs->ideal_viewangles);
      bs->ideal_viewangles[2] = bs->ideal_viewangles[2] * 0.5;
      }
    }
    else if ( BotMovementViewTarget((bot_movestate_t *)&bs->ms, (bot_goal_t *)(intptr_t)goal, v2, (float *)(intptr_t)target) )
    {
      VectorSubtract(target, bs->origin, dir);
      vectoangles(dir, bs->ideal_viewangles);
      bs->ideal_viewangles[2] = bs->ideal_viewangles[2] * 0.5;
    }
    else
    {
      vectoangles(moveresult.movedir, bs->ideal_viewangles);
      bs->ideal_viewangles[2] = bs->ideal_viewangles[2] * 0.5;
    }
    }
    if ( (moveresult.flags & 8) != 0 )
      return 1;
    }
    BotChangeViewAngles(bs, bs->thinktime);
    return 1;
}
//----- (1001FCF0) --------------------------------------------------------
void __cdecl AIEnter_Battle_Fight(bot_state_t *bs)
{
  BotRecordNodeSwitch(bs, "battle fight", "");
  BotAINode(bs) = AINode_Battle_Fight;
}
//----- (1001FD30) --------------------------------------------------------
int __cdecl AINode_Battle_Fight(bot_state_t *bs)
{

  int areanum; // esi
  int v8; // edi
  float v10; // [esp+Ch] [ebp-15Ch]
  bot_moveresult_t moveresult; // [esp+10h] [ebp-158h] BYREF (was int[12]; BotAttackMove result copy)
  int entinfo[31]; // [esp+40h] [ebp-128h] BYREF

  if ( BotIsObserver(bs) )
  {
    AIEnter_Observer(bs);
    return 0;
  }
  if ( BotIntermission(bs) )
  {
    AIEnter_Intermission(bs);
    return 0;
  }
  if ( BotIsDead(bs) )
  {
    AIEnter_Respawn(bs);
    return 0;
  }
  /* Inline, not a shared label: the original keeps a physical copy of
   * `AIEnter_Seek_LTG(bs); return 0;` as this guard's fall-through PLUS one at
   * the function tail for the two later gotos.  A single hosted label has three
   * predecessors and no fall-through, so both copies float to the tail. */
  if ( !bs->enemy )
  {
    AIEnter_Seek_LTG(bs);
    return 0;
  }
  *(aas_entityinfo_t *)entinfo = AAS_EntityInfo(bs->enemy);
  if ( sub_10021710(entinfo) )
  {
    /* Q3's shape: `if (BotChat_Kill(bs)) {stand} else {seek} return qfalse;` with
     * ONE shared return.  The original hoists the `bs` push above the branch to
     * serve both calls, which only works with both inline here. */
    if ( BotChat_Kill((int *)bs) )
    {
      v10 = BotChatTime(bs);
      bs->stand_time = AAS_Time() + v10;
      AIEnter_Stand(bs);
    }
    else
    {
      AIEnter_Seek_LTG(bs);
    }
    return 0;
  }
  else
  {
    areanum = AAS_PointAreaNum(&entinfo[4]);
    if ( areanum && AAS_AreaReachability(areanum) )
    {
      (*(int *)&bs->lastenemyorigin[0]) = entinfo[4];
      (*(int *)&bs->lastenemyorigin[1]) = entinfo[5];
      (*(int *)&bs->lastenemyorigin[2]) = entinfo[6];
      bs->lastenemyareanum = areanum;
    }
    BotUpdateBattleInventory(bs, bs->enemy);
    /* Q3's order: test not-visible FIRST and early-out to the chase/seek branch,
       leaving the attack pipeline as the fall-through. */
    if ( !BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0, bs->enemy) )
    {
      if ( BotWantsToChase((int *)bs) )
        AIEnter_Battle_Chase(bs);
      else
        AIEnter_Seek_LTG(bs);
      return 0;
    }
    v8 = 102334;
    if ( libvar_usehook->value != 0.0f )
      v8 = 118718;
    if ( libvar_rocketjump->value != 0.0f && BotCanAndWantsToRocketJump(bs) )
      v8 |= 0x1000u;
    sub_10020FE0(bs, BotWS(bs));
    BotChooseBestFightWeapon(BotWS(bs));
    sub_100215E0(bs);
    BotBattleUseItems(bs);
    moveresult = BotAttackMove(bs, v8);
    if ( moveresult.failure )
    {
      BotResetAvoidReach((_DWORD *)&bs->ms);
      bs->ltg_time = 0.0f;
    }
    BotAIBlocked(bs, &moveresult, 0);
    BotAimAtEnemy(bs);
    BotCheckAttack(bs);
    if ( BotWantsToRetreat((int *)bs) )
      AIEnter_Battle_Retreat(bs);
    return 1;
  }
}
//----- (10020050) --------------------------------------------------------
void __cdecl AIEnter_Battle_Chase(bot_state_t *bs)
{
  BotRecordNodeSwitch(bs, "battle chase", "");
  bs->chase_time = AAS_Time() + 10.0f;
  BotAINode(bs) = AINode_Battle_Chase;
}
//----- (100200A0) --------------------------------------------------------
int __cdecl AINode_Battle_Chase(bot_state_t *bs)
{

  int tfl; // esi
  vec3_t dir; // [esp+10h] [ebp-B0h] BYREF
  vec3_t target; // [esp+1Ch] [ebp-A4h] BYREF
  bot_goal_t goal; // [esp+28h] [ebp-98h] BYREF
  bot_moveresult_t moveresult; // [esp+60h] [ebp-60h] BYREF
  bot_moveresult_t v14; // [esp+90h] [ebp-30h] BYREF

  if ( BotIsObserver(bs) )
  {
    AIEnter_Observer(bs);
    return 0;
  }
  if ( BotIntermission(bs) )
  {
    AIEnter_Intermission(bs);
    return 0;
  }
  if ( BotIsDead(bs) )
  {
    AIEnter_Respawn(bs);
    return 0;
  }
  if ( !bs->enemy )
  {
    AIEnter_Seek_LTG(bs);
    return 0;
  }
  if ( BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0, bs->enemy) )
  {
    BotResetLastAvoidReach((intptr_t)&bs->ms);
    AIEnter_Battle_Fight(bs);
    return 0;
  }
  if ( BotFindEnemy(bs) )
  {
    AIEnter_Battle_Fight(bs);
    return 0;
  }
  if ( !bs->lastenemyareanum )
  {
    AIEnter_Seek_LTG(bs);
    return 0;
  }
  tfl = 102334;
  if ( libvar_usehook->value != 0.0f )
    tfl = 118718;
  if ( libvar_rocketjump->value != 0.0f && BotCanAndWantsToRocketJump(bs) )
    tfl |= 0x1000u;
  goal.entitynum = bs->enemy;
  goal.areanum = bs->lastenemyareanum;
  VectorCopy(bs->lastenemyorigin, goal.origin);
  goal.mins[0] = -8.0;
  goal.mins[1] = -8.0;
  goal.mins[2] = -8.0;
  goal.maxs[0] = 8.0;
  goal.maxs[1] = 8.0;
  goal.maxs[2] = 8.0;
  if ( BotTouchingGoal(bs->origin, goal.origin) )
    *(int *)&bs->chase_time = 0;
  // Gladiator inverts Q3's `if (chase_time expired) { seek; return }`: it tests
  // `AAS_Time() <= chase_time` and falls through to the seek-LTG exit otherwise.
  // Written as an early return so the three AIEnter_Seek_LTG exits stay flat and
  // inline — nesting them lets MSVC cross-jump-merge the three.
  if ( AAS_Time() > bs->chase_time )
  {
    AIEnter_Seek_LTG(bs);
    return 0;
  }
  if ( AAS_Time() > bs->check_time
    && (bs->check_time = AAS_Time() + 1.0f,
        BotChooseNBGItem(&bs->goalstate, bs->origin, bs->inventory, tfl, &goal, 500.0)) )
  {
    bs->nbg_time = AAS_Time() + 5.0f;
    BotResetLastAvoidReach((intptr_t)&bs->ms);
    AIEnter_Battle_NBG(bs);
    return 0;
  }
  else
  {
    BotUpdateBattleInventory(bs, bs->enemy);
    BotBattleUseItems(bs);
    BotEntityInfo(bs, (_DWORD *)&bs->ms);
    moveresult = *BotMoveToGoal(&v14, (bot_movestate_t *)&bs->ms, &goal, tfl);
    if ( moveresult.failure )
    {
      BotResetAvoidReach((_DWORD *)&bs->ms);
      bs->ltg_time = 0.0f;
    }
    BotAIBlocked(bs, &moveresult, 0);
    if ( (moveresult.flags & 3) != 0 )
    {
      *(int *)&bs->ideal_viewangles[0] = LODWORD(moveresult.ideal_viewangles[0]);
      *(int *)&bs->ideal_viewangles[1] = LODWORD(moveresult.ideal_viewangles[1]);
      *(int *)&bs->ideal_viewangles[2] = LODWORD(moveresult.ideal_viewangles[2]);
    }
    else
    {
      if ( BotMovementViewTarget((bot_movestate_t *)&bs->ms, &goal, tfl, (float *)(intptr_t)target) )
      {
        VectorSubtract(target, bs->origin, dir);
        vectoangles(dir, bs->ideal_viewangles);
      }
      else
      {
        vectoangles(moveresult.movedir, bs->ideal_viewangles);
      }
      bs->ideal_viewangles[2] = bs->ideal_viewangles[2] * 0.5;
    }
    if ( bs->ms.areanum == bs->lastenemyareanum )
      *(int *)&bs->chase_time = 0;
    if ( (moveresult.flags & 8) == 0 )
      BotChangeViewAngles(bs, bs->thinktime);
    if ( BotWantsToRetreat((int *)bs) )
      AIEnter_Battle_Retreat(bs);
    return 1;
  }
}
//----- (100205C0) --------------------------------------------------------
void __cdecl AIEnter_Battle_Retreat(bot_state_t *bs)
{
  BotRecordNodeSwitch(bs, "battle retreat", "");
  BotAINode(bs) = AINode_Battle_Retreat;
}
//----- (10020600) --------------------------------------------------------
int __cdecl AINode_Battle_Retreat(bot_state_t *bs)
{

  int v2; // edi
  bot_goal_t *goal; // esi (was int — holds BotLongTermGoal pointer)
  float v4; // st7
  float attack_skill; // captured BFloat dodge probability
  vec3_t dir; // [esp+14h] [ebp-170h] BYREF
  vec3_t target; // [esp+20h] [ebp-164h] BYREF
  bot_moveresult_t moveresult; // [esp+2Ch] [ebp-158h] BYREF
  bot_moveresult_t v11; // [esp+5Ch] [ebp-128h] BYREF
  char entinfo[124]; // [esp+8Ch] [ebp-F8h] BYREF

  if ( BotIsObserver(bs) )
  {
    AIEnter_Observer(bs);
    return 0;
  }
  if ( BotIntermission(bs) )
  {
    AIEnter_Intermission(bs);
    return 0;
  }
  if ( BotIsDead(bs) )
  {
    AIEnter_Respawn(bs);
    return 0;
  }
  if ( !bs->enemy )
  {
    AIEnter_Seek_LTG(bs);
    return 0;
  }
  *(aas_entityinfo_t *)entinfo = AAS_EntityInfo(bs->enemy);
  if ( sub_10021710(entinfo) )
  {
    AIEnter_Seek_LTG(bs);
    return 0;
  }
  v2 = 102334;
  if ( libvar_usehook->value != 0.0f )
    v2 |= 0x4000;
  BotUpdateBattleInventory(bs, bs->enemy);
  if ( BotWantsToChase((int *)bs) )
  {
    BotEmptyGoalStack(&bs->goalstate);
    AIEnter_Battle_Chase(bs);
    return 0;
  }
  else
  {
    if ( !BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0, bs->enemy) )
    {
      AIEnter_Seek_LTG(bs);
      return 0;
    }
    if ( libvar_ctf->value != 0.0f )
      BotCTFRetreatGoals(bs);
    goal = (bot_goal_t *)BotLongTermGoal(bs, v2, 1);
    if ( !goal )
    {
      BotChangeViewAngles(bs, bs->thinktime);
      return 1;
    }
    {
      v4 = AAS_Time();
      if ( v4 > bs->check_time
        && (bs->check_time = AAS_Time() + 1.0f,
            BotChooseNBGItem(&bs->goalstate, bs->origin, bs->inventory, v2, goal, 500.0)) )
      {
        BotResetLastAvoidReach((intptr_t)&bs->ms);
        bs->nbg_time = AAS_Time() + 5.0f;
        AIEnter_Battle_NBG(bs);
        return 0;
      }
      else
      {
        BotBattleUseItems(bs);
        BotEntityInfo(bs, (_DWORD *)&bs->ms);
        moveresult = *BotMoveToGoal(&v11, (bot_movestate_t *)&bs->ms, (bot_goal_t *)(intptr_t)goal, v2);
        if ( moveresult.failure )
        {
          BotResetAvoidReach((_DWORD *)&bs->ms);
          bs->ltg_time = 0.0f;
        }
        BotAIBlocked(bs, &moveresult, 0);
        sub_10020FE0(bs, BotWS(bs));
        BotChooseBestFightWeapon(BotWS(bs));
        if ( (moveresult.flags & 1) != 0 )
        {
          *(int *)&bs->ideal_viewangles[0] = LODWORD(moveresult.ideal_viewangles[0]);
          *(int *)&bs->ideal_viewangles[1] = LODWORD(moveresult.ideal_viewangles[1]);
          *(int *)&bs->ideal_viewangles[2] = LODWORD(moveresult.ideal_viewangles[2]);
        }
        else if ( (moveresult.flags & 8) == 0 )
        {
          /* Characteristic 4 is aggression: > 0.3 attack, <= 0.3 dodge. */
          attack_skill = (float)Characteristic_BFloat(BotCharacter(bs), 4, 0.0, 1.0);
          if ( attack_skill > 0.3 )
          {
            BotAimAtEnemy(bs);
          }
          else
          {
            if ( BotMovementViewTarget((bot_movestate_t *)&bs->ms, (bot_goal_t *)(intptr_t)goal, v2, (float *)(intptr_t)target) )
            {
              VectorSubtract(target, bs->origin, dir);
              vectoangles(dir, bs->ideal_viewangles);
            }
            else
            {
              vectoangles(moveresult.movedir, bs->ideal_viewangles);
            }
            bs->ideal_viewangles[2] = bs->ideal_viewangles[2] * 0.5;
            BotChangeViewAngles(bs, bs->thinktime);
          }
        }
        BotCheckAttack(bs);
        return 1;
      }
    }
  }
}
//----- (10020AD0) --------------------------------------------------------
void __cdecl AIEnter_Battle_NBG(bot_state_t *bs)
{
  BotRecordNodeSwitch(bs, "battle NBG", "");
  BotAINode(bs) = AINode_Battle_NBG;
}
//----- (10020B10) --------------------------------------------------------
int __cdecl AINode_Battle_NBG(bot_state_t *bs)
{

  int v8;                 // [esp+0x10] — movement flags
  int areanum;            // esi — AAS area number of enemy origin
  bot_goal_t *topgoal;    // esi — top goal pointer
  bot_moveresult_t v15;   // BotMoveToGoal result copy
  bot_moveresult_t v16;   // BotMoveToGoal output buffer
  int entinfo[31];        // [esp+0x4c] BYREF — copy of AAS entity info for enemy

  if ( BotIsObserver(bs) )
  {
    AIEnter_Observer(bs);
    return 0;
  }
  if ( BotIntermission(bs) )
  {
    AIEnter_Intermission(bs);
    return 0;
  }
  if ( BotIsDead(bs) )
  {
    AIEnter_Respawn(bs);
    return 0;
  }
  if ( !bs->enemy )
  {
    AIEnter_Seek_NBG(bs);
    return 0;
  }
  *(aas_entityinfo_t *)entinfo = AAS_EntityInfo(bs->enemy);
  if ( sub_10021710((_DWORD *)entinfo) )
  {
    AIEnter_Seek_NBG(bs);
    return 0;
  }
  v8 = 102334;
  if ( libvar_usehook->value != 0.0f )
    v8 = 118718;
  if ( libvar_rocketjump->value != 0.0f && BotCanAndWantsToRocketJump(bs) )
    v8 |= 0x1000u;
  areanum = AAS_PointAreaNum((float *)&entinfo[4]);
  if ( areanum && AAS_AreaReachability(areanum) )
  {
    (*(int *)&bs->lastenemyorigin[0]) = entinfo[4];
    (*(int *)&bs->lastenemyorigin[1]) = entinfo[5];
    (*(int *)&bs->lastenemyorigin[2]) = entinfo[6];
    bs->lastenemyareanum = areanum;
  }
  topgoal = (bot_goal_t *)BotGetTopGoal(&bs->goalstate);
  if ( topgoal )
  {
    if ( BotTouchingGoal(bs->origin, (float *)topgoal) )
    {
      if ( libvar_runes->value != 0.0f )
        sub_100262C0((_DWORD *)bs, topgoal);
      bs->nbg_time = 0.0f;
    }
  }
  else
  {
    bs->nbg_time = 0.0f;
  }
  if ( AAS_Time() > bs->nbg_time )
  {
    BotPopGoal(&bs->goalstate);
    if ( BotGetTopGoal(&bs->goalstate) )
      AIEnter_Battle_Retreat(bs);
    else
      AIEnter_Battle_Fight(bs);
    return 0;
  }
  BotBattleUseItems(bs);
  BotEntityInfo(bs, (_DWORD *)&bs->ms);
  v15 = *BotMoveToGoal(&v16, (bot_movestate_t *)&bs->ms, (bot_goal_t *)(intptr_t)topgoal, v8);
  if ( v15.failure )
  {
    BotResetAvoidReach((_DWORD *)&bs->ms);
    bs->nbg_time = 0.0f;
  }
  BotAIBlocked(bs, &v15, 0);
  sub_10020FE0(bs, BotWS(bs));
  BotUpdateBattleInventory(bs, bs->enemy);
  BotChooseBestFightWeapon(BotWS(bs));
  if ( (v15.flags & 1) != 0 )
  {
    VectorCopy(v15.ideal_viewangles, bs->ideal_viewangles);
  }
  else
  {
    BotAimAtEnemy(bs);
  }
  BotCheckAttack(bs);
  if ( (v15.flags & 8) == 0 )
    BotChangeViewAngles(bs, bs->thinktime);
  return 1;
}
