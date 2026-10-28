// GPL-3.0. Display policy; the game owns every menu and battle action.
#pragma once
#include <cstdint>
#include <vector>
struct DsBattleView { bool bottom=false,panel=false; };
inline DsBattleView ds_battle_view(int mode,bool battle,bool selecting,bool normalBottom,int overrideScreen=-1){
  if(!mode || !battle)return {normalBottom,false};
  if(overrideScreen>=0)return {overrideScreen==1,false};
  if(!selecting)return {false,false};
  return {mode==1,mode==2};
}
inline void ds_battle_panel(const uint32_t* top,const uint32_t* bottom,std::vector<uint32_t>& out){
  constexpr int width=341,height=256;
  out.assign(width*height,0x303838);
  for(int y=0;y<192;y++)for(int x=0;x<256;x++)out[y*width+x]=top[y*256+x]&0xffffff;
  // Keep the complete native command UI at native pixel size, over the scene.
  for(int y=0;y<192;y++)for(int x=0;x<256;x++)out[(y+64)*width+x+85]=bottom[y*256+x]&0xffffff;
}
