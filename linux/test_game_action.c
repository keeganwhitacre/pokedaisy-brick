/* Explicit controller model tests: failure, timeout and cancellation boundaries. */
#include "game_action.h"
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
  puts("PASS Bag controller rejects wrong ROM/battle/text/read failures and malformed menus; timeout/cancel release input");
}
