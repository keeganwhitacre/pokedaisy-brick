/* Private-ROM runtime captures. Normal input only; never edits game RAM. */
#define POCKET_TEST
#define main pocket_app_main
#include "main.c"
#undef main
#include <assert.h>
#include "../3ds/core/pd_ui_internal.h"
static unsigned framesMatched;
static const char *outDir;
static void drive(struct mCore *c, unsigned keys, unsigned n) {
  c->setKeys(c, keys);
  for (unsigned i=0;i<n;i++) {
    c->runFrame(c); wide_tick(c, false);
    framesMatched += wideReady;
  }
  c->setKeys(c, 0);
}
static void shot(const char *name) {
  char path[1200]; snprintf(path,sizeof path,"%s/%s.png",outDir,name);
  assert(capture_frame(path,NULL));
}
static void activate(struct pd_ui* ui, int id) {
  for(int i=0;i<ui->hitCount;i++)if(ui->hits[i].id==id) {
    struct pd_hit h=ui->hits[i];
    pd_ui_touch(ui,h.x+h.w/2,h.y+h.h/2,true);
    pd_ui_touch(ui,h.x+h.w/2,h.y+h.h/2,false);return;
  }
  assert(!"Missing real Summary hit target");
}
static void test_frame(struct mCore *c,int *pc,bool companion,int frame,
                       struct pd_ui *ui,struct pd_snapshot *s,const struct pc_ui *p) {
  (void)pc;(void)companion;(void)p; assert(frame==0 && wideGba && wideSupported);
  wideMode=1;
  for(unsigned i=0;i<160 && !wideReady;i++) {
    drive(c,1,8); drive(c,0,40);
  }
  assert(wideReady);
  drive(c,0,180); // Let normal palette fades and the map-name popup finish.
  pd_snapshot_read(s,&(struct pd_game){0},read_memory,c);
  wideMode=0; wideReady=false; render_game(240,160); shot("emerald-original");
  wideMode=1; drive(c,0,45); assert(wideReady);
  render_game(240,160); shot("emerald-expanded");
  assert(state(c,"/tmp/pokedaisy-emerald-test.ss",true));
  wideMode=2; drive(c,0,30); assert(wideReady);
  render_game(240,160); shot("emerald-zoom");
  assert(state(c,"/tmp/pokedaisy-emerald-test.ss",false));
  wideMode=1; drive(c,0,45); assert(wideReady);
  render_game(240,160); shot("emerald-reloaded");
  displayOptions.autoZoom=true;
  drive(c,8,8); drive(c,0,120); // START overlay is TEXT in upstream's classifier.
  fprintf(stderr,"START scene=%d mode=%d close=%d ready=%d\n",pk_wide_scene(),pk_wide_last_mode(),pk_wide_wants_close(),wideReady);
  assert(!wideReady); render_game(240,160); shot("emerald-game-menu");
  drive(c,2,8); drive(c,0,120); assert(wideReady);
  displayOptions.autoZoom=false;
  drive(c,16,16); drive(c,0,30); // Ordinary walking, with PPU checks each frame.
  render_game(240,160); shot("emerald-walking");
  unsigned char *rom;size_t len;
  FILE *f=fopen(getenv("EMERALD_ROM"),"rb");assert(f);
  fseek(f,0,SEEK_END);len=ftell(f);rewind(f);rom=malloc(len);assert(rom&&fread(rom,1,len,f)==len);fclose(f);
  struct pd_game game;pd_game_detect(&game,rom,len);game.rom=rom;game.romSize=len;pd_romart_scan(&game);
  pd_snapshot_read(s,&game,read_memory,c);assert(s->valid&&s->partyCount>0);
  struct pd_host_info host={.hideScreenMode=true};
  int tabs[]={PD_TAB_PARTY,PD_TAB_BAG,PD_TAB_MAP,PD_TAB_DEX,PD_TAB_SETTINGS};
  const char *names[]={"emerald-party","emerald-bag","emerald-map","emerald-dex","emerald-settings"};
  for(unsigned i=0;i<5;i++){ui->tab=tabs[i];pd_ui_update(ui,s);pd_ui_draw(ui,&canvas,&game,s,&host);present_ui(true);shot(names[i]);}
  ui->tab=PD_TAB_PARTY;ui->summarySlot=0;
  pd_ui_draw(ui,&canvas,&game,s,&host);present_ui(true);shot("emerald-summary");
  assert(s->party[0].stats.valid);
  bool statsHit=false;
  for(int i=0;i<ui->hitCount;i++)if(ui->hits[i].id==HIT_SUMMARY_STATS) {
    struct pd_hit h=ui->hits[i];
    pd_ui_touch(ui,h.x+h.w/2,h.y+h.h/2,true);
    pd_ui_touch(ui,h.x+h.w/2,h.y+h.h/2,false);statsHit=true;break;
  }
  assert(statsHit && ui->summaryStats);
  pd_ui_draw(ui,&canvas,&game,s,&host);present_ui(true);shot("emerald-stats");
  ui->summaryStats=false;
  // Only actual party data. No memory injection or substitute display values.
  for(int i=0;i<s->partyCount;i++) {
    const struct pd_mon* mon=&s->party[i];
    assert(mon->hasExperience && mon->hasHeldItem);
    fprintf(stderr,"LIVE party %d species=%d Lv%d EXP=%lu held=%d\n",i,mon->species,mon->level,(unsigned long)mon->experience,mon->heldItem);
    ui->summarySlot=i;ui->summaryTypes=ui->summaryItem=false;
    pd_ui_draw(ui,&canvas,&game,s,&host);
    if(mon->level==100) { present_ui(true);shot("emerald-summary-max"); }
    activate(ui,HIT_SUMMARY_TYPES);assert(ui->summaryTypes);
    pd_ui_draw(ui,&canvas,&game,s,&host);present_ui(true);
    if(i==0)shot("emerald-summary-types");
    activate(ui,HIT_SUMMARY_TYPES);assert(!ui->summaryTypes);
    pd_ui_draw(ui,&canvas,&game,s,&host);
    if(mon->heldItem) {
      activate(ui,HIT_SUMMARY_ITEM);assert(ui->summaryItem);
      pd_ui_draw(ui,&canvas,&game,s,&host);present_ui(true);shot("emerald-held-item");
      activate(ui,HIT_SUMMARY_STATS);assert(ui->summaryStats && !ui->summaryItem);
      pd_ui_draw(ui,&canvas,&game,s,&host);
      activate(ui,HIT_SUMMARY_ITEM);assert(ui->summaryItem);
      assert(pd_ui_back(ui) && !ui->summaryItem && ui->summarySlot==i);
      ui->summaryStats=false;
    }
    if(s->partyCount>1) {
      pd_ui_draw(ui,&canvas,&game,s,&host);
      activate(ui,HIT_SLOT+(i+1)%s->partyCount);
      assert(ui->summarySlot==(i+1)%s->partyCount && !ui->summaryStats && !ui->summaryTypes && !ui->summaryItem);
    }
  }
  ui->summarySlot=0;
  pd_ui_draw(ui,&canvas,&game,s,&host);present_ui(true);shot("emerald-summary-exp");
  assert(pd_ui_back(ui) && ui->summarySlot==-1);
  ui->summarySlot=-1;ui->tab=PD_TAB_BAG;ui->bagSelected=0;
  pd_ui_draw(ui,&canvas,&game,s,&host);present_ui(true);shot("emerald-item-description");
  free(rom);running=0;
  printf("PASS actual Emerald save: %u matched frames, original PPU equality, expansion/zoom, state reload and companion captures; party=%d\n",framesMatched,s->partyCount);
}
int main(int argc,char **argv){assert(argc==4);outDir=argv[3];setenv("EMERALD_ROM",argv[1],1);forceFresh=true;
 char *args[]={"pokedaisy-pocket","--data",argv[2],"--frames","1",argv[1],NULL};return pocket_app_main(6,args);}
