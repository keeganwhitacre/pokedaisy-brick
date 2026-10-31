/* GPL-3.0. Same map-reentry queue mechanism as Android GbField.Crystal.
 * Polished's addresses / script entry come from pinned release symbols. */
#include "polished_field.h"
#include "polished_action_data.h"
#include "../app/src/main/cpp/pk_gb_field.h"
#include <string.h>
bool polished_field_supported(const char *sha) {
  return sha && !strcmp(sha,PD_PC_ACTION_SHA);
}
const char *polished_field_reason(const char *sha,pocket_action_read r,void *ctx) {
  if(!polished_field_supported(sha))return "Not supported in this version";
  unsigned char state,script,map,battle,running,events,queue,inMenu;
#define READ(key,var) if(!r(ctx,PD_PC_##key,&var,1))return "Cannot read game state"
  READ(STATE,state);READ(SCRIPT,script);READ(MAP,map);READ(BATTLE,battle);
  READ(SCRIPT_RUNNING,running);READ(MAP_EVENTS,events);READ(QUEUE,queue);
  READ(IN_MENU,inMenu);
#undef READ
  if((state&0x40) || script || map!=2 || battle || running || events || queue || inMenu)
    return "Return to the overworld first";
  return NULL;
}
const char *polished_pc_reason(const char *sha,pocket_action_read r,void *ctx) {
  const char *why=polished_field_reason(sha,r,ctx);
  if(why)return why;
  unsigned char party,direction;
  if(!r(ctx,PD_PC_PARTY_COUNT,&party,1) || !r(ctx,PD_PC_DIRECTION,&direction,1))
    return "Cannot read game state";
  if(!party || party>6)return "Get a Pokemon first";
  /* PCScript checks VAR_FACING == UP before opening. Respect that check,
   * rather than writing facing or jumping into the middle of a ROM script. */
  if((direction&12)!=4)return "Face up to open the PC";
  return NULL;
}
bool polished_pc_open(struct mCore *c,const char *sha,pocket_action_read r,void *ctx) {
  if(polished_pc_reason(sha,r,ctx))return false;
  const unsigned script=PD_PC_PC_SCRIPT;
  const unsigned char payload[]={script>>16,script&255,(script>>8)&255};
  const unsigned char flag=1;
  /* Payload first, publish last, between frames. A failed payload write
   * never makes an incomplete script runnable. Never overwrite a queued job. */
  return pk_gb_write(c,PD_PC_QUEUE+1,payload,sizeof payload) &&
         pk_gb_write(c,PD_PC_QUEUE,&flag,1);
}
bool polished_pc_busy(pocket_action_read r,void *ctx) {
  unsigned char queue,running,menu;
  return !r(ctx,PD_PC_QUEUE,&queue,1) || !r(ctx,PD_PC_SCRIPT_RUNNING,&running,1) ||
         !r(ctx,PD_PC_IN_MENU,&menu,1) || queue || running || menu;
}
bool polished_wild_hold(struct mCore *c,const char *sha,pocket_action_read r,void *ctx) {
  if(!polished_field_supported(sha))return false;
  /* The cooldown is separate WRAM, not party / inventory or encounter data.
   * Map status 2 is the active map loop; don't touch title/menu/battle state. */
  unsigned char map,battle,menu,cooldown;
  if(!r(ctx,PD_PC_MAP,&map,1) || !r(ctx,PD_PC_BATTLE,&battle,1) ||
     !r(ctx,PD_PC_IN_MENU,&menu,1))return false;
  if(map!=2 || battle || menu)return true;
  if(!r(ctx,PD_PC_WILD_COOLDOWN,&cooldown,1))return false;
  if(cooldown>=3)return true;
  const unsigned char hold=5; /* Same five-step cooldown as the game's setup. */
  return pk_gb_write(c,PD_PC_WILD_COOLDOWN,&hold,1);
}
