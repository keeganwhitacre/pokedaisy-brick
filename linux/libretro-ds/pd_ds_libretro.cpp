// SPDX-License-Identifier: GPL-3.0-or-later
// Keep DSperate's libretro lifecycle, SRAM and state format. Only presentation
// and input are adapted; there is no second launcher or save writer.
#define retro_set_environment ds_set_environment
#define retro_set_video_refresh ds_set_video_refresh
#define retro_set_input_state ds_set_input_state
#define retro_get_system_info ds_get_system_info
#define retro_get_system_av_info ds_get_system_av_info
#define retro_load_game ds_load_game
#define retro_unload_game ds_unload_game
#define retro_run ds_run
#define retro_reset ds_reset
#define retro_unserialize ds_unserialize
#include "src/libretro.cpp"
#undef retro_set_environment
#undef retro_set_video_refresh
#undef retro_set_input_state
#undef retro_get_system_info
#undef retro_get_system_av_info
#undef retro_load_game
#undef retro_unload_game
#undef retro_run
#undef retro_reset
#undef retro_unserialize
#include "companion.h"
#include "party_view.h"
#include "bag_view.h"
#include "dex_view.h"
#include "map_view.h"
#include "browser.h"
#include "data_gen.h"
#include "pk_nds_field.h"
#include "core/crypto/sha1.h"
#include <fstream>
extern "C" {
#include "pd_style.h"
#include "pd_ui.h"
#include "../hud.h"
#include "../quick_menu.h"
#include "pd_tables.h"
}
namespace pd_ds {
retro_environment_t environment;
retro_video_refresh_t video;
retro_input_state_t input;
const PkNdsFieldGame* game;
DsCompanion snapshot;
DsMapArt map;
std::vector<std::vector<uint32_t>> icons;
bool supported=false, companion=false, summary=false, details=false;
bool select_used=false, select_pulse=false;
unsigned old_buttons=0, buttons=0, tick=0, width=256, height=192;
int tab=0, selected=0, page=0, scroll=0, pocket=0, filter=0;
int screen_override=0, cursor_x=128, cursor_y=96;
int layout=0; // 0 top, 1 bottom, 2 side by side, 3 stacked
int map_x=0,map_y=0;
uint16_t pixels[512*384];
uint32_t ui_pixels[341*256];

uint32_t read32(uint32_t a) {
  if (!nds || a<0x02000000 || a>0x023ffffc) return 0;
  return nds->bus.dma_read32(ds::Cpu::ARM9,a);
}
void clear() {
  supported=companion=summary=details=select_used=select_pulse=false; game=nullptr; snapshot={};map={};icons.clear();
  old_buttons=buttons=tick=0;tab=selected=page=scroll=pocket=filter=0;
  screen_override=0;cursor_x=128;cursor_y=96;layout=0;map_x=map_y=0;
}
bool env(unsigned cmd,void* data) {
  if(cmd==RETRO_ENVIRONMENT_SET_VARIABLES) {
    auto vars=static_cast<const retro_variable*>(data);
    std::vector<retro_variable> all;
    for(;vars && vars->key;++vars)all.push_back(*vars);
    all.push_back({"pokedaisy_ds_layout","DS layout; auto|top|bottom|stacked"});
    all.push_back({"pokedaisy_ds_hud","Party HUD; disabled|enabled"});
    all.push_back({nullptr,nullptr});
    return environment(cmd,all.data());
  }
  if(cmd==RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) {
    retro_pixel_format f=RETRO_PIXEL_FORMAT_RGB565;
    return environment(cmd,&f);
  }
  return environment(cmd,data);
}
void geometry(unsigned w,unsigned h) {
  if(width==w && height==h)return;
  width=w;height=h;
  retro_game_geometry g{w,h,512,384,float(w)/h};
  environment(RETRO_ENVIRONMENT_SET_GEOMETRY,&g);
}
uint16_t rgb565(uint32_t p) {return ((p>>8)&0xf800)|((p>>5)&0x7e0)|((p>>3)&31);}
void refresh() {
  snapshot=supported?ds_read_companion(nds->bus.main_ram.get(),nds->bus.main_ram_size()):DsCompanion{};
}
void choose_layout() {
  const auto option=opt("pokedaisy_ds_layout");
  if(screen_override)layout=screen_override==1?1:0;
  else if(option=="top")layout=0;
  else if(option=="bottom")layout=1;
  else if(option=="stacked")layout=3;
  else {
    int bits=game?pk_nds_field_state(game,read32,nullptr,0):0;
    // Upstream pk_nds.cpp's blank-screen fallback covers CONTINUE and intro
    // questions before the field process exists. No game addresses are added.
    auto one_colour=[](const uint32_t* px){
      uint32_t first=px[0]&0xffffff;
      for(int gy=0;gy<12;++gy)for(int gx=0;gx<16;++gx)
        if((px[(gy*16+8)*256+gx*16+8]&0xffffff)!=first)return false;
      return true;
    };
    bool top_blank=one_colour(nds->gpu.framebuffer(0));
    bool bottom_blank=one_colour(nds->gpu.framebuffer(1));
    if(top_blank && bottom_blank)return; // retain layout through a scene fade
    if(top_blank && !bottom_blank){layout=1;return;}
    layout=(bits&PK_NDS_FIELD_TOP_ONLY)?0:
      (bits&(PK_NDS_FIELD_TOUCH_ONLY|PK_NDS_FIELD_TOUCH_BATTLE_MENU))?1:
      (bits&PK_NDS_FIELD_TOUCH)?2:0;
  }
}
void draw_companion() {
  pd_canvas c;pd_canvas_init(&c,341,256,ui_pixels);pd_backdrop(&c);
  pd_title_box(&c,5,5,331,25,0xffffff);
  pd_text(&c,12,10,"PokeDaisy DS",PD_TITLE_TEXT,PD_TITLE_SHADOW);
  const char* names[]={"PARTY","BAG","DEX","MAP"};
  pd_canvas nav;pd_canvas_init(&nav,341,26,ui_pixels+32*341);
  pd_ui u{};u.pressedId=-1;pd_ui_navigation(&u,&nav,names,4,tab);
  pd_list_box(&c,5,61,331,171);
  pd_canvas body;pd_canvas_init(&body,341,171,ui_pixels+61*341);
  if(!supported)pd_text(&body,13,12,"Companion: Platinum USA rev 1 only",0x303030,PD_NO_SHADOW);
  else if(!snapshot.valid)pd_text(&body,13,12,"Return to game to load save data",0x303030,PD_NO_SHADOW);
  else if(tab==0)ds_party_view(&body,snapshot,icons,selected,summary,page);
  else if(tab==1)ds_bag_view(&body,snapshot,pocket,scroll,details);
  else if(tab==2)ds_dex_view(&body,snapshot,filter,scroll);
  else ds_map_view(&body,snapshot,map,map_x,map_y);
  const char* hints[]={"D-pad Pokemon  A summary  B back","Up/Down item  Left/Right pocket  A info","SELECT filter  Left/Right page","D-pad map  A player"};
  pd_text_fit(&c,9,236,323,hints[tab],0xffffff,PD_NO_SHADOW);
  pd_text(&c,9,247,"L/R tabs  L2 game",0xffffff,PD_NO_SHADOW);
  geometry(341,256);
  for(unsigned i=0;i<341*256;++i)pixels[i]=rgb565(ui_pixels[i]);
  video(pixels,width,height,width*2);
}
void present(const void* data,unsigned,unsigned,size_t) {
  if(companion){draw_companion();return;}
  // Refresh composition even when DSperate skipped rasterization: switching
  // screens must still display the latest retained framebuffer.
  (void)data;
  choose_layout();
  unsigned w=layout==2?512:256,h=layout==3?384:192;
  geometry(w,h);
  const uint32_t* top=nds->gpu.framebuffer(0);
  const uint32_t* bottom=nds->gpu.framebuffer(1);
  for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x) {
    const uint32_t* src=layout==1?bottom:(layout==2 && x>=256)||(layout==3 && y>=192)?bottom:top;
    pixels[y*w+x]=rgb565(src[(y%192)*256+(x%256)]);
  }
  if(buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_SELECT)) {
    int ox=layout==2?256:0,oy=layout==3?192:0;
    if(layout!=0)for(int d=-4;d<=4;++d){
      int x=ox+cursor_x+d,y=oy+cursor_y;
      if(x>=ox && x<ox+256)pixels[y*w+x]=0xffff;
      x=ox+cursor_x;y=oy+cursor_y+d;
      if(y>=oy && y<oy+192)pixels[y*w+x]=0xffff;
    }
  }
  if(supported && opt("pokedaisy_ds_hud")=="enabled" && layout==0) {
    if(tick%12==0 || !snapshot.valid)refresh();
    pd_canvas c;pd_canvas_init(&c,256,192,ui_pixels);
    std::fill(ui_pixels,ui_pixels+256*192,QUICK_CLEAR);
    pd_snapshot party{};party.valid=snapshot.valid;party.partyCount=std::min(6,int(snapshot.party.size()));
    for(int i=0;i<party.partyCount;++i){
      const auto& m=snapshot.party[i];auto& d=party.party[i];
      d.species=m.egg?PD_SPECIES_EGG:1;d.level=m.level;d.hp=m.hp;d.maxHp=m.maxHp;d.status=m.status;
    }
    hud_party_draw(&c,&party,&party,[](pd_canvas* c,void* ctx,const pd_mon* m,int x,int y,bool){
      auto* p=static_cast<pd_snapshot*>(ctx);
      const auto& mon=snapshot.party[m-p->party];
      int id=mon.egg?pd_ds_data::iconEgg:mon.species;
      if(id<0 || size_t(id)>=icons.size())return;
      const auto& art=icons[id];if(art.size()!=1024)return;
      for(int j=0;j<32;++j)for(int i=0;i<32;++i){auto p=art[j*32+i];if(p>>24)pd_fill(c,x+i,y+j,1,1,p&0xffffff);}
    });
    // Composite the shared HUD glass over the native game frame.
    for(unsigned i=0;i<256*192;++i){
      uint32_t p=ui_pixels[i];if(p==QUICK_CLEAR)continue;
      unsigned a=p>>24;if(!a)a=255;
      uint16_t old=pixels[i];unsigned r=(old>>11&31)*255/31,g=(old>>5&63)*255/63,b=(old&31)*255/31;
      r=(((p>>16)&255)*a+r*(255-a))/255;
      g=(((p>>8)&255)*a+g*(255-a))/255;b=((p&255)*a+b*(255-a))/255;
      pixels[i]=rgb565((r<<16)|(g<<8)|b);
    }
  }
  video(pixels,w,h,w*2);
}
int16_t filtered_input(unsigned p,unsigned d,unsigned i,unsigned id) {
  if(d==RETRO_DEVICE_JOYPAD) {
    if(id==RETRO_DEVICE_ID_JOYPAD_SELECT && select_pulse)return 1;
    if(companion || (buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_SELECT)))return 0;
    if(id==RETRO_DEVICE_ID_JOYPAD_L2 || id==RETRO_DEVICE_ID_JOYPAD_R2)return 0;
  }
  // DSperate expects a pointer over stacked screens. Reproject the current
  // layout into that convention before its input::apply receives it.
  if(d==RETRO_DEVICE_POINTER) {
    bool stylus=buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_SELECT);
    if(stylus) {
      if(id==RETRO_DEVICE_ID_POINTER_PRESSED)return !!(buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_A));
      if(id==RETRO_DEVICE_ID_POINTER_X)return cursor_x*65535/256-32767;
      if(id==RETRO_DEVICE_ID_POINTER_Y)return cursor_y*32768/192;
    }
    int x=input(p,d,i,RETRO_DEVICE_ID_POINTER_X),y=input(p,d,i,RETRO_DEVICE_ID_POINTER_Y);
    if(layout==0)return 0;
    if(layout==1 && id==RETRO_DEVICE_ID_POINTER_Y)return (y+32767)/2;
    if(layout==2) {
      if(id==RETRO_DEVICE_ID_POINTER_PRESSED)return x>=0?input(p,d,i,id):0;
      if(id==RETRO_DEVICE_ID_POINTER_X)return std::clamp(x*2-32767,-32767,32767);
      if(id==RETRO_DEVICE_ID_POINTER_Y)return (y+32767)/2;
    }
  }
  return input(p,d,i,id);
}
void controls() {
  buttons=0;for(unsigned id=0;id<16;++id)if(input(0,RETRO_DEVICE_JOYPAD,0,id))buttons|=1u<<id;
  select_pulse=!(buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_SELECT)) &&
    (old_buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_SELECT)) && !select_used;
  if(!(buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_SELECT)))select_used=false;
  unsigned edge=buttons&~old_buttons;old_buttons=buttons;
  auto hit=[&](unsigned id){return !!(edge&(1u<<id));};
  if(hit(RETRO_DEVICE_ID_JOYPAD_L2)) {
    companion=!companion;
    if(companion){refresh();map_x=snapshot.mapX;map_y=snapshot.mapY;}
  }
  if(!companion) {
    if(hit(RETRO_DEVICE_ID_JOYPAD_R2))screen_override=(screen_override+1)%3;
    if(buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_SELECT)) {
      if(buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_UP))cursor_y-=3;
      if(buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_DOWN))cursor_y+=3;
      if(buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_LEFT))cursor_x-=3;
      if(buttons&(1u<<RETRO_DEVICE_ID_JOYPAD_RIGHT))cursor_x+=3;
      cursor_x=std::clamp(cursor_x,0,255);cursor_y=std::clamp(cursor_y,0,191);
      // A SELECT tap is forwarded on release; SELECT plus pen controls is consumed.
      unsigned pen_keys=(1u<<RETRO_DEVICE_ID_JOYPAD_UP)|(1u<<RETRO_DEVICE_ID_JOYPAD_DOWN)|
        (1u<<RETRO_DEVICE_ID_JOYPAD_LEFT)|(1u<<RETRO_DEVICE_ID_JOYPAD_RIGHT)|(1u<<RETRO_DEVICE_ID_JOYPAD_A);
      if(buttons&pen_keys){select_used=true;if(layout==0)screen_override=1;}
    }
    return;
  }
  if(hit(RETRO_DEVICE_ID_JOYPAD_L) || hit(RETRO_DEVICE_ID_JOYPAD_R)) {
    tab=(tab+(hit(RETRO_DEVICE_ID_JOYPAD_R)?1:3))%4;scroll=0;summary=details=false;
  }
  int dy=hit(RETRO_DEVICE_ID_JOYPAD_DOWN)-hit(RETRO_DEVICE_ID_JOYPAD_UP);
  int dx=hit(RETRO_DEVICE_ID_JOYPAD_RIGHT)-hit(RETRO_DEVICE_ID_JOYPAD_LEFT);
  if(tab==0) {
    selected=std::clamp(selected+dy+(summary?0:dx),0,std::max(0,int(snapshot.party.size())-1));
    if(summary && dx)page=1-page;
    if(hit(RETRO_DEVICE_ID_JOYPAD_A))summary=!summary;
    if(hit(RETRO_DEVICE_ID_JOYPAD_B)){if(summary)summary=false;else companion=false;}
  } else if(tab==1) {
    if(dx){pocket=(pocket+dx+9)%9;scroll=0;}
    scroll=std::clamp(scroll+dy,0,std::max(0,ds_bag_count(snapshot,pocket)-1));
    if(hit(RETRO_DEVICE_ID_JOYPAD_A))details=!details;
  } else if(tab==2) {
    if(hit(RETRO_DEVICE_ID_JOYPAD_SELECT)){filter=(filter+1)%3;scroll=0;}
    scroll=std::clamp(scroll+dy+dx*8,0,std::max(0,ds_dex_count(snapshot,filter)-1));
  } else {
    map_x=std::clamp(map_x+dx,0,29);map_y=std::clamp(map_y+dy,0,29);
    if(hit(RETRO_DEVICE_ID_JOYPAD_A)){map_x=snapshot.mapX;map_y=snapshot.mapY;}
  }
  if(tab && hit(RETRO_DEVICE_ID_JOYPAD_B))companion=false;
}
}
extern "C" {
RETRO_API void retro_set_environment(retro_environment_t cb){pd_ds::environment=cb;ds_set_environment(pd_ds::env);
  static const retro_input_descriptor d[]={
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_UP,"Up"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_DOWN,"Down"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_LEFT,"Left"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_RIGHT,"Right"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_A,"DS A"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_B,"DS B"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_X,"DS X"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_Y,"DS Y"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_START,"DS Start"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_SELECT,"DS Select / Stylus"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_L,"DS L"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_R,"DS R"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_L2,"Companion"},
    {0,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_R2,"Screen Auto Bottom Top"},
    {0,0,0,0,nullptr}};
  cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS,const_cast<retro_input_descriptor*>(d));}
RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb){pd_ds::video=cb;ds_set_video_refresh(pd_ds::present);}
RETRO_API void retro_set_input_state(retro_input_state_t cb){pd_ds::input=cb;ds_set_input_state(pd_ds::filtered_input);}
RETRO_API void retro_get_system_info(retro_system_info* info){ds_get_system_info(info);info->library_name="PokeDaisy DS";info->library_version="0.1-experimental";}
RETRO_API void retro_get_system_av_info(retro_system_av_info* info){
  ds_get_system_av_info(info);info->geometry={256,192,512,384,4.f/3.f};
}
RETRO_API bool retro_load_game(const retro_game_info* info){
  pd_ds::clear();pd_ds::width=256;pd_ds::height=192;
  if(!ds_load_game(info))return false;
  std::ifstream f(info->path,std::ios::binary);
  std::vector<uint8_t> rom{std::istreambuf_iterator<char>(f),{}};
  if(rom.size()>=512) {
    // Preserve the existing exact-ROM guard: offsets and art are rev-specific.
    uint8_t hash[20];ds::crypto::sha1(rom.data(),rom.size(),hash);
    char hex[41];for(int i=0;i<20;++i)std::sprintf(hex+2*i,"%02x",hash[i]);
    pd_ds::supported=!std::strcmp(hex,pd_ds_data::sha1);
    if(pd_ds::supported){
      pd_ds::game=pk_nds_field_game(reinterpret_cast<const char*>(rom.data()+12),rom[30]);
      pd_ds::map=ds_map_art(rom);pd_ds::icons=ds_party_icons(rom);
    }
  }
  log(RETRO_LOG_INFO,"PokeDaisy DS: Platinum companion %s\n",pd_ds::supported?"enabled":"unavailable for this ROM");
  return true;
}
RETRO_API void retro_reset(){
  ds_reset();pd_ds::companion=false;pd_ds::screen_override=0;pd_ds::snapshot={};pd_ds::tick=0;
}
RETRO_API bool retro_unserialize(const void* data,size_t n){
  bool ok=ds_unserialize(data,n);
  if(ok){pd_ds::companion=false;pd_ds::screen_override=0;pd_ds::snapshot={};pd_ds::tick=0;}
  return ok;
}
RETRO_API void retro_unload_game(){ds_unload_game();pd_ds::clear();}
RETRO_API void retro_run(){
  if(!nds)return;
  input_poll_cb();pd_ds::controls();
  if(pd_ds::companion){
    pd_ds::draw_companion();
    static const int16_t silence[548*2]={};audio_batch_cb(silence,547);
  }else {ds_run();++pd_ds::tick;}
}
}
