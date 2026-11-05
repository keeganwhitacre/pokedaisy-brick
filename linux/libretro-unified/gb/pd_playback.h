/* GPL-3.0. Libretro capability adapter; keeps the pinned core source unchanged. */
#ifndef PD_PLAYBACK_H
#define PD_PLAYBACK_H
#include "libretro.h"
struct pd_playback { bool fast_forward, rewinding; };
static inline struct pd_playback pd_playback_query(retro_environment_t env) {
  struct pd_playback result={false,false};
  if(!env)return result;
  struct retro_throttle_state state={0,0.0f};
  if(env(RETRO_ENVIRONMENT_GET_THROTTLE_STATE,&state)){
    result.fast_forward=state.mode==RETRO_THROTTLE_FAST_FORWARD;
    result.rewinding=state.mode==RETRO_THROTTLE_REWINDING;
  }else{
    bool fast=false;
    if(env(RETRO_ENVIRONMENT_GET_FASTFORWARDING,&fast))result.fast_forward=fast;
  }
  return result;
}
#endif
