// GPL-3.0. DS data/choices rendered by the established Settings components.
#include "options_view.h"
#include <string>
namespace {
void draw(pd_canvas* c,const char* title,const pd_option_row* rows,int count,int selected){
  pd_option_source source{title,rows,count};pd_ui ui{};ui.optionSource=&source;
  ui.optionSelected=selected;ui.pressedId=-1;pd_game game{};pd_snapshot snapshot{};
  pd_ui_options_view(&ui,c,&game,&snapshot);
}
}
void ds_settings_view(pd_canvas* c,const pd_settings& s,bool bottom,int battle,int selected){
  const pd_option_row rows[]={
    {"SCREEN",bottom?"BOTTOM":"TOP"},{"BATTLES",battle?"AUTO":"OFF"},
    {"QUICK RESUME",s.quickResume?"ON":"OFF"},{"SAVE ON QUIT",s.saveOnQuit?"ON":"OFF"},
    {"HUD",s.hud?"ON":"OFF"},{"QUICK MENU",s.quickGrid?"GRID":"RING"},{"FILTER",s.lcdFilter?"LCD":"OFF"}};
  draw(c,"SETTINGS",rows,7,selected);
}
void ds_status_view(pd_canvas* c,const DsCompanion& s){
  int badges=0;for(int i=0;i<8;i++)badges+=(s.badges>>i)&1;
  const std::string money=std::to_string(s.money),badgeText=std::to_string(badges)+" / 8";
  const std::string seen=s.dexValid?std::to_string(s.seenCount):"UNAVAILABLE",caught=s.dexValid?std::to_string(s.caughtCount):"UNAVAILABLE";
  const pd_option_row rows[]={{"TRAINER",s.trainer.c_str()},{"MONEY",money.c_str()},
    {"PLACE",ds_place(s.map)},{"BADGES",badgeText.c_str()},{"SEEN",seen.c_str()},{"CAUGHT",caught.c_str()},
    {"NATIONAL DEX",!s.dexValid?"UNAVAILABLE":s.nationalDex?"UNLOCKED":"LOCKED"}};
  draw(c,"STATUS",rows,7,-1);
}
