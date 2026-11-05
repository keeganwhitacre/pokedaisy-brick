#include "pd_playback.h"
#include <assert.h>
#include <stdio.h>
static int mode,legacy_calls;
static bool env(unsigned cmd,void* p){
 if(cmd==RETRO_ENVIRONMENT_GET_THROTTLE_STATE){
  if(mode<2)return false;
  ((struct retro_throttle_state*)p)->mode=mode==2?RETRO_THROTTLE_FAST_FORWARD:mode==3?RETRO_THROTTLE_REWINDING:RETRO_THROTTLE_VSYNC;return true;
 }
 if(cmd==RETRO_ENVIRONMENT_GET_FASTFORWARDING){++legacy_calls;*(bool*)p=mode==1;return mode>=0;}
 return false;
}
int main(){
 assert(!pd_playback_query(NULL).fast_forward);
 mode=-1;assert(!pd_playback_query(env).fast_forward);
 mode=0;assert(!pd_playback_query(env).fast_forward);
 mode=1;assert(pd_playback_query(env).fast_forward);
 int before=legacy_calls;
 mode=2;assert(pd_playback_query(env).fast_forward&&!pd_playback_query(env).rewinding);
 mode=3;assert(pd_playback_query(env).rewinding&&!pd_playback_query(env).fast_forward);
 mode=4;assert(!pd_playback_query(env).fast_forward&&!pd_playback_query(env).rewinding);
 assert(legacy_calls==before);
 puts("PASS modern throttle precedence, legacy FF fallback, missing callbacks and rewind detection");
}
