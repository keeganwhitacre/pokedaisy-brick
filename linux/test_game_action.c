/* Explicit controller model tests: failure, timeout and cancellation boundaries. */
#include "game_action.h"
#include "polished_field.h"
#include "polished_action_data.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char ram[0x20000];
static bool fail;
static bool read_mem(void *ctx,uint32_t addr,void *out,size_t n) {
  (void)ctx;if(fail || addr>=sizeof ram || n>sizeof ram-addr)return false;
  memcpy(out,ram+addr,n);return true;
}
int main(void) {
  struct pocket_action a={0};ram[PD_PC_MAP]=2;
  assert(!pocket_action_begin(&a,"wrong ROM",read_mem,NULL));
  assert(pocket_action_tick(&a,read_mem,NULL)==0);
  ram[PD_PC_BATTLE]=1;assert(!pocket_action_begin(&a,PD_PC_ACTION_SHA,read_mem,NULL));ram[PD_PC_BATTLE]=0;
  ram[PD_PC_STATE]=0x40;assert(!pocket_action_begin(&a,PD_PC_ACTION_SHA,read_mem,NULL));ram[PD_PC_STATE]=0;
  fail=true;assert(!pocket_action_begin(&a,PD_PC_ACTION_SHA,read_mem,NULL));fail=false;
  assert(pocket_action_begin(&a,PD_PC_ACTION_SHA,read_mem,NULL));
  assert(pocket_action_tick(&a,read_mem,NULL)==8);
  assert(!pocket_action_begin(&a,PD_PC_ACTION_SHA,read_mem,NULL));
  pocket_action_cancel(&a);assert(a.result==PA_CANCELLED && !pocket_action_tick(&a,read_mem,NULL));
  assert(pocket_action_begin(&a,PD_PC_ACTION_SHA,read_mem,NULL));
  for(int i=0;i<362;i++)pocket_action_tick(&a,read_mem,NULL);
  assert(a.result==PA_TIMEOUT && !pocket_action_tick(&a,read_mem,NULL));
  assert(pocket_action_begin(&a,PD_PC_ACTION_SHA,read_mem,NULL));
  pocket_action_tick(&a,read_mem,NULL);pocket_action_tick(&a,read_mem,NULL);
  ram[PD_PC_POINTER]=PD_PC_START_MENU&255;ram[PD_PC_POINTER+1]=PD_PC_START_MENU>>8;
  ram[PD_PC_STATE]=0x40;ram[PD_PC_SCRIPT]=1;
  ram[PD_PC_LIST]=2;ram[PD_PC_LIST+1]=2;ram[PD_PC_LIST+2]=2;ram[PD_PC_LIST+3]=255;ram[PD_PC_CURSOR]=1;
  pocket_action_tick(&a,read_mem,NULL);
  assert(a.result==PA_REJECTED); /* duplicate menu IDs must not receive A */
  memset(ram,0,sizeof ram);ram[PD_PC_MAP]=2;
  for(unsigned p=0;p<sizeof pd_pc_item_pockets/sizeof *pd_pc_item_pockets;p++)ram[pd_pc_item_pockets[p].addr+1]=255;
  unsigned med=pd_pc_item_pockets[1].addr;
  ram[med]=1;ram[med+1]=25;ram[med+2]=2;ram[med+3]=255;
  assert(pocket_action_begin_item(&a,PD_PC_ACTION_SHA,25,read_mem,NULL));
  assert(a.item==25 && a.pocket==1 && a.slot==0 && a.quantity==2);
  pocket_action_cancel(&a);
  assert(!pocket_action_begin_item(&a,PD_PC_ACTION_SHA,0x201,read_mem,NULL));
  assert(pocket_action_begin_item(&a,PD_PC_ACTION_SHA,25,read_mem,NULL));
  a.phase=3;ram[med+2]=3; /* inventory changed: no A may be emitted */
  assert(!pocket_action_tick(&a,read_mem,NULL) && a.result==PA_REJECTED);
  ram[med+2]=2;
  unsigned items=pd_pc_item_pockets[0].addr;
  ram[items]=1;ram[items+1]=25;ram[items+2]=1;ram[items+3]=255;
  assert(!pocket_action_begin_item(&a,PD_PC_ACTION_SHA,25,read_mem,NULL));
  memset(ram,0,sizeof ram);ram[PD_PC_MAP]=2;
  assert(pocket_action_begin(&a,PD_PC_ACTION_SHA,read_mem,NULL));
  a.phase=2;ram[PD_PC_STATE]=0x40;ram[PD_PC_SCRIPT]=1;
  ram[PD_PC_POCKET]=3;ram[PD_PC_PACK_STATE]=2;ram[PD_PC_IN_MENU]=1;
  const unsigned char geometry[]={2,7,1,1,0x0c,0,0x20,0xfb};
  memcpy(ram+PD_PC_MENU2D,geometry,sizeof geometry);
  ram[PD_PC_CURSOR]=ram[PD_PC_CURSOR+1]=1;
  for(int i=0;i<10;i++)assert(!pocket_action_tick(&a,read_mem,NULL));
  assert(a.result==PA_RUNNING); /* stale geometry without visible cursor is insufficient */
  ram[PD_PC_TILEMAP+2*20+7]=PD_PC_CURSOR_TILE;
  for(int i=0;i<6;i++)pocket_action_tick(&a,read_mem,NULL);
  assert(a.result==PA_DONE);
  puts("PASS TM pocket requires live waiting cursor as well as menu geometry");
  puts("PASS selected ordinary item lookup, special-namespace rejection, duplicate rejection and changed-quantity guard");
  puts("PASS Bag controller rejects wrong ROM/battle/text/read failures and malformed menus; timeout/cancel release input");
  memset(ram,0,sizeof ram);ram[PD_PC_MAP]=2;
  ram[PD_PC_PARTY_COUNT]=1;ram[PD_PC_DIRECTION]=4;
  assert(!polished_field_supported("other ROM"));
  assert(polished_pc_reason("other ROM",read_mem,NULL));
  assert(!polished_pc_reason(PD_PC_ACTION_SHA,read_mem,NULL));
  const unsigned blocked[]={PD_PC_SCRIPT,PD_PC_BATTLE,PD_PC_SCRIPT_RUNNING,
      PD_PC_MAP_EVENTS,PD_PC_QUEUE,PD_PC_IN_MENU};
  for(unsigned i=0;i<sizeof blocked/sizeof *blocked;i++) {
    ram[blocked[i]]=1;assert(polished_pc_reason(PD_PC_ACTION_SHA,read_mem,NULL));
    ram[blocked[i]]=0;
  }
  ram[PD_PC_STATE]=0x40;assert(polished_pc_reason(PD_PC_ACTION_SHA,read_mem,NULL));ram[PD_PC_STATE]=0;
  ram[PD_PC_PARTY_COUNT]=0;assert(polished_pc_reason(PD_PC_ACTION_SHA,read_mem,NULL));ram[PD_PC_PARTY_COUNT]=1;
  ram[PD_PC_DIRECTION]=0;assert(polished_pc_reason(PD_PC_ACTION_SHA,read_mem,NULL));ram[PD_PC_DIRECTION]=4;
  fail=true;assert(polished_pc_reason(PD_PC_ACTION_SHA,read_mem,NULL));fail=false;
  assert(!polished_pc_open(NULL,PD_PC_ACTION_SHA,read_mem,NULL) && !ram[PD_PC_QUEUE]);
  puts("PASS PC rejects wrong ROM, busy field/menu/queue, no party, wrong facing and failed reads/writes");
  assert(!polished_wild_hold(NULL,"wrong ROM",read_mem,NULL));
  fail=true;assert(!polished_wild_hold(NULL,PD_PC_ACTION_SHA,read_mem,NULL));fail=false;
  ram[PD_PC_BATTLE]=1;assert(polished_wild_hold(NULL,PD_PC_ACTION_SHA,read_mem,NULL));ram[PD_PC_BATTLE]=0;
  ram[PD_PC_IN_MENU]=1;assert(polished_wild_hold(NULL,PD_PC_ACTION_SHA,read_mem,NULL));ram[PD_PC_IN_MENU]=0;
  ram[PD_PC_MAP]=0;assert(polished_wild_hold(NULL,PD_PC_ACTION_SHA,read_mem,NULL));ram[PD_PC_MAP]=2;
  ram[PD_PC_WILD_COOLDOWN]=5;assert(polished_wild_hold(NULL,PD_PC_ACTION_SHA,read_mem,NULL));
  ram[PD_PC_WILD_COOLDOWN]=2;assert(!polished_wild_hold(NULL,PD_PC_ACTION_SHA,read_mem,NULL));
  puts("PASS WILD rejects unsupported ROM/read/write failures and skips menu/battle/title or an adequate cooldown");
}
