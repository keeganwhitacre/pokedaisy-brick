/* Optional personal-ROM integration runner. Uses the actual host and guest controls only. */
#define POCKET_TEST
#define main linux_app_main
#include "main.c"
#undef main
#include <assert.h>
#include "polished_action_data.h"
static void drive(struct mCore *c,unsigned keys,int n) {
  c->setKeys(c,keys); while(n-->0)c->runFrame(c); c->setKeys(c,0);
}
static void tap(SDL_Keycode key) {
  SDL_Event e={.type=SDL_KEYDOWN};e.key.keysym.sym=key;assert(SDL_PushEvent(&e)==1);
  e.type=SDL_KEYUP;assert(SDL_PushEvent(&e)==1);
}
static void shot(const char *name) {
  char path[1024];snprintf(path,sizeof path,"%s/%s.png",getenv("PC_SHOTS"),name);
  assert(capture_frame(path,NULL));
}
/* Earn damage through normal wild-battle inputs; never patch HP or inventory. */
static void earn_injury(struct mCore *c) {
  struct pd_snapshot s;
  pc_read(&s,read_memory,c);
  for(int i=0;i<30 && s.y<7;i++){drive(c,128,1);drive(c,0,17);pc_read(&s,read_memory,c);}
  for(int i=0;i<8 && s.x>3;i++){drive(c,32,1);drive(c,0,17);pc_read(&s,read_memory,c);}
  for(int i=0;i<8 && s.y<8;i++){drive(c,128,1);drive(c,0,17);pc_read(&s,read_memory,c);}
  bool route=false;
  for(int i=0;i<65 && !route;i++) {
    drive(c,32,1);drive(c,0,17);uint8_t dims[2];
    route=read_memory(c,pcWideConfig[GB_MAP_HEIGHT],dims,2) && dims[0]==9 && dims[1]==30;
  }
  assert(route);
  /* Official Route29 block/collision source: nearest grass from the east
   * connection is (53,12), reached west then south. Normal movement only. */
  pc_read(&s,read_memory,c);
  for(int i=0;i<40 && s.x>53;i++){drive(c,32,3);drive(c,0,17);pc_read(&s,read_memory,c);}
  for(int i=0;i<40 && s.y<12;i++){drive(c,128,3);drive(c,0,17);pc_read(&s,read_memory,c);if(s.inBattle)break;}
  for(int i=0;i<120 && !s.inBattle;i++) {
    drive(c,i%2?16:32,16);drive(c,0,8);pc_read(&s,read_memory,c);
  }
  fprintf(stderr,"INJURY encounter=%d location=%d,%d HP=%d/%d\n",s.inBattle,s.x,s.y,s.party[0].hp,s.party[0].maxHp);
  assert(s.inBattle && !s.isTrainer);
  struct pocket_battle_action action={0};bool damaged=false;
  for(int i=0;i<12000;i++) {
    pc_read(&s,read_memory,c);damaged=damaged || (s.party[0].hp>0 && s.party[0].hp<s.party[0].maxHp);
    if(!s.inBattle)break;
    if(action.result!=PA_RUNNING) {
      struct pocket_battle_menu menu;
      if(pocket_battle_read(read_memory,c,&menu) && (menu.state==PB_ACTION || menu.state==PB_MOVE)) {
        pocket_battle_begin(&action,PD_PC_ACTION_SHA,damaged?PB_CHOOSE_ACTION:PB_CHOOSE_MOVE,damaged?3:1,read_memory,c);
      }
    }
    unsigned keys=action.result==PA_RUNNING?pocket_battle_tick(&action,read_memory,c):(i%120<3?1:0);
    drive(c,keys,1);
  }
  drive(c,0,180);pc_read(&s,read_memory,c);
  fprintf(stderr,"INJURY final battle=%d HP=%d/%d\n",s.inBattle,s.party[0].hp,s.party[0].maxHp);
  assert(!s.inBattle && s.party[0].hp>0 && s.party[0].hp<s.party[0].maxHp);
  char path[1024];snprintf(path,sizeof path,"%s/states/injured.ss",root);assert(state(c,path,true));
}
static void test_frame(struct mCore *c,bool companion,int frame,struct pd_ui *ui,struct pd_snapshot *s) {
  (void)ui;
  if(getenv("PC_INJURE")) {
    if(frame==0){char path[1024];snprintf(path,sizeof path,"%s/states/live-test.ss",root);assert(state(c,path,false));earn_injury(c);}
    if(frame==5)shot("earned-injury");
    return;
  }
  if (getenv("PC_HANDOFF")) {
    static struct pd_snapshot beforeItem;
    static unsigned chosenItem,chosenQty;
    if(frame==0){char path[1024];snprintf(path,sizeof path,"%s/states/%s",root,getenv("PC_HEAL")?"injured.ss":"live-test.ss");assert(state(c,path,false));pc_read(&beforeItem,read_memory,c);chosenItem=beforeItem.bag[PD_POCKET_ITEMS].items[0].id;chosenQty=beforeItem.bag[PD_POCKET_ITEMS].items[0].quantity;}
    if(frame==2)tap(SDLK_TAB);
    if(frame==5)tap(SDLK_e);
    if(frame==8)tap(SDLK_2);
    if(frame==10 && getenv("PC_HANDOFF_CANCEL"))tap(SDLK_x);
    if(frame==130) {
      unsigned char raw[2];assert(read_memory(c,PD_PC_POINTER,raw,2));
      unsigned menu=raw[0]|(raw[1]<<8);bool pack=false;
      for(unsigned i=0;i<sizeof pd_pc_pack_menus/sizeof *pd_pc_pack_menus;i++)pack=pack||menu==pd_pc_pack_menus[i];
      assert(!companion);
      bool actions=false;
      for(unsigned i=0;i<sizeof pd_pc_item_submenus/sizeof *pd_pc_item_submenus;i++)actions=actions||menu==pd_pc_item_submenus[i];
      if(getenv("PC_HANDOFF_CANCEL"))assert(!actions);else {
        unsigned char item;assert(read_memory(c,PD_PC_ITEM,&item,1));
        assert(actions && item==s->bag[PD_POCKET_ITEMS].items[0].id);
      }
      shot(getenv("PC_HANDOFF_CANCEL")?"handoff-cancelled":"selected-item-actions");
      fprintf(stderr,"PASS actual companion R2 Bag %s\n",getenv("PC_HANDOFF_CANCEL")?"cancellation":"handoff confirmed");
    }
    if(frame==131 && getenv("PC_ITEM_ACTION")) {
      const char *action=getenv("PC_ITEM_ACTION");
      int row=!strcmp(action,"give")?1:!strcmp(action,"toss")?2:0;
      for(int i=0;i<row;i++){drive(c,128,3);drive(c,0,8);}
      drive(c,1,3);drive(c,0,120);
    }
    if(frame==133 && getenv("PC_ITEM_ACTION")) {
      const char *action=getenv("PC_ITEM_ACTION");
      shot(!strcmp(action,"give")?"give-native-prompt":!strcmp(action,"toss")?"toss-native-prompt":getenv("PC_HEAL")?"heal-native-prompt":"fullhp-native-prompt");
    }
    if(frame==135 && getenv("PC_ITEM_ACTION")) {
      for(int i=0;i<3;i++){drive(c,1,3);drive(c,0,120);}
      drive(c,2,3);drive(c,0,60);
    }
    if(frame==140 && getenv("PC_ITEM_ACTION")) {
      struct pd_snapshot after;assert(pc_read(&after,read_memory,c));
      unsigned qty=0;for(int p=0;p<PD_POCKET_COUNT;p++)for(int i=0;i<after.bag[p].count;i++)
        if(after.bag[p].items[i].id==chosenItem)qty+=after.bag[p].items[i].quantity;
      const char *action=getenv("PC_ITEM_ACTION");
      fprintf(stderr,"ITEM %s HP %d -> %d, quantity %u -> %u, held %d\n",action,beforeItem.party[0].hp,after.party[0].hp,chosenQty,qty,after.party[0].heldItem);
      if(!strcmp(action,"use")) {
        if(getenv("PC_HEAL")){assert(after.party[0].hp>beforeItem.party[0].hp && qty+1==chosenQty);}
        else assert(after.party[0].hp==beforeItem.party[0].hp && qty==chosenQty);
      } else if(!strcmp(action,"give"))assert(after.party[0].heldItem==(int)chosenItem && qty+1==chosenQty);
      else assert(qty+1==chosenQty && after.party[0].heldItem==beforeItem.party[0].heldItem);
      shot(!strcmp(action,"give")?"give-native-result":!strcmp(action,"toss")?"toss-native-result":getenv("PC_HEAL")?"heal-native-result":"fullhp-native-result");fprintf(stderr,"PASS native item action result\n");
    }
    if(frame==141 && getenv("PC_HEAL"))tap(SDLK_TAB);
    if(frame==143 && getenv("PC_HEAL"))tap(SDLK_q);
    if(frame==147 && getenv("PC_HEAL")){assert(companion && s->party[0].hp==s->party[0].maxHp);shot("heal-companion-after");}
    return;
  }
  if (getenv("PC_PROBE")) {
    if(frame==0){char path[1024];snprintf(path,sizeof path,"%s/states/live-test.ss",root);assert(state(c,path,false));}
    if(frame%30==20){char name[64];snprintf(name,sizeof name,"probe-%03d",frame);shot(name);fprintf(stderr,"probe%d scene%d\n",frame,pk_wide_scene());}
    if(frame==22) drive(c,32,8);
    if(frame==52) drive(c,128,100);
    if(frame==82) drive(c,16,48);
    if(frame==112) drive(c,64,48);
    if(frame==142) drive(c,32,96);
    if(frame==172) drive(c,128,80);
    return;
  }
  if(frame==0 && getenv("PC_RESUME")) {
    char path[1024];snprintf(path,sizeof path,"%s/states/live-test.ss",root);
    assert(state(c,path,false));return;
  }
  if(frame==0) {
    FILE *script=fopen(getenv("PC_BOOT"),"r"); assert(script);
    char line[256],key[24];int n;
    while(fgets(line,sizeof line,script)) {
      if(sscanf(line,"wait %d",&n)==1) drive(c,0,n);
      else if(sscanf(line,"hold %23s %d",key,&n)==2) {
        unsigned bits=!strcmp(key,"A")?1:!strcmp(key,"B")?2:!strcmp(key,"START")?8:!strcmp(key,"RIGHT")?16:!strcmp(key,"LEFT")?32:!strcmp(key,"UP")?64:!strcmp(key,"DOWN")?128:0;
        assert(bits);drive(c,bits,n);
      }
    }
    fclose(script);
    for(int i=0;i<30;i++){drive(c,1,3);drive(c,0,120);}
    for(int i=0;i<20;i++){drive(c,2,30);drive(c,0,60);}
    drive(c,128,160);drive(c,0,120);
    if (getenv("PC_OUTDOOR")) {
      bool outside=false;
      drive(c,32,16);drive(c,0,30);
      for(int i=0;i<120 && !outside;i++) {
        struct pd_snapshot now;pc_read(&now,read_memory,c);
        if (now.inBattle) {
          drive(c,2,3);drive(c,0,30);drive(c,2,3);drive(c,0,30);
          drive(c,64,3);drive(c,0,3);drive(c,32,3);drive(c,0,3);
          drive(c,1,3);drive(c,0,30);drive(c,1,3);drive(c,0,180);
        } else {
          drive(c,1,3);drive(c,0,120);drive(c,128,16);drive(c,0,30);
        }
        uint8_t dims[2];
        outside=read_memory(c,pcWideConfig[GB_MAP_HEIGHT],dims,2) && dims[0]==9 && dims[1]==10;
      }
      struct pd_snapshot position; pc_read(&position,read_memory,c);
      fprintf(stderr,"OUTDOOR=%d map=%d,%d pos=%d,%d battle=%d\n",outside,position.mapGroup,position.mapNum,position.x,position.y,position.inBattle);assert(outside);
      drive(c,0,120);
    }
  }
  if(frame==5) {fprintf(stderr,"LIVE party=%d valid=%d money=%ld battle=%d scene=%d\n",s->partyCount,s->valid,s->money,s->inBattle,pk_wide_scene());shot("01-overworld");tap(SDLK_f);}
  if(frame==8) {shot("02-quick-menu");tap(SDLK_f);}
  if(frame==11) tap(SDLK_TAB); /* host X opens companion */
  if(frame==14) {assert(companion && canvasW==512 && canvasH==384); assert(s->partyCount==1 && s->party[0].species==155); fprintf(stderr,"COMPANION=%d\n",companion);shot("03-party");tap(SDLK_z);}
  if(frame==17) {shot("04-summary");tap(SDLK_x);}
  if(frame==20) tap(SDLK_e);
  if(frame==23) {shot("05-bag");if(!getenv("PC_FAVORITES"))tap(SDLK_e);}
  if(frame==24 && getenv("PC_FAVORITES") && !getenv("PC_FAVORITES_RELOAD")) tap(SDLK_TAB);
  if(frame==25 && getenv("PC_FAVORITES")) tap(SDLK_e);
  if(frame==26 && getenv("PC_FAVORITES")) {
    struct pocket_favorites f;
    assert(pocket_favorites_load(root,"6930b48af5844d373e3c9130f26d6dd1084cf4ed",&f));
    assert(s->bag[PD_POCKET_ITEMS].count>0);
    assert(pocket_favorite_has(&f,s->bag[PD_POCKET_ITEMS].items[0].id));
    fprintf(stderr,"PASS live bag favorite %s\n",getenv("PC_FAVORITES_RELOAD")?"survives process restart":"X saves atomically");
  }
  if(frame==26) {shot("06-battle-info");tap(SDLK_e);}
  if(frame==29) {shot("07-dex");tap(SDLK_e);}
  if(frame==32) {shot("08-map");tap(SDLK_e);}
  if(frame==35) {shot("09-guide");tap(SDLK_RETURN);}
  if(frame==38) {shot("10-settings");tap(SDLK_x);}
  if(frame==41) tap(SDLK_TAB);
  if(frame==70) {
    shot("11-hud");
    char path[1024];snprintf(path,sizeof path,"%s/states/live-test.ss",root);
    uint32_t before=c->frameCounter(c);
    assert(state(c,path,true));drive(c,0,90);assert(state(c,path,false));
    assert(c->frameCounter(c)==before);
    snprintf(path,sizeof path,"%s/settings.ini",root);
    ui->settings->hud=false;assert(pd_settings_save(ui->settings,path));
    struct pd_settings loaded;pd_settings_load(&loaded,path);assert(!loaded.hud);
    ui->settings->hud=true;assert(pd_settings_save(ui->settings,path));
    pd_settings_load(&loaded,path);assert(loaded.hud && loaded.aspect==PD_ASPECT_EXPAND);
    fprintf(stderr,"PASS live state roundtrip, HUD persistence and EXPAND persistence\n");
  }
}
int main(int argc,char **argv){ if(!getenv("PC_SHOTS") || !getenv("PC_BOOT")){fprintf(stderr,"Set PC_SHOTS and PC_BOOT; use a disposable --data directory.\n");return 2;} return linux_app_main(argc,argv); }
