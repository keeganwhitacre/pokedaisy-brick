/* Optional personal-ROM integration runner. Uses the actual host and guest controls only. */
#define POCKET_TEST
#define main linux_app_main
#include "main.c"
#undef main
#include <assert.h>
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
static void test_frame(struct mCore *c,bool companion,int frame,struct pd_ui *ui,struct pd_snapshot *s) {
  (void)ui;
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
  }
  if(frame==5) {fprintf(stderr,"LIVE party=%d valid=%d money=%ld battle=%d scene=%d\n",s->partyCount,s->valid,s->money,s->inBattle,pk_wide_scene());shot("01-overworld");tap(SDLK_f);}
  if(frame==8) {shot("02-quick-menu");tap(SDLK_f);}
  if(frame==11) tap(SDLK_TAB); /* host X opens companion */
  if(frame==14) {assert(companion && canvasW==512 && canvasH==384); assert(s->partyCount==1 && s->party[0].species==155); fprintf(stderr,"COMPANION=%d\n",companion);shot("03-party");tap(SDLK_z);}
  if(frame==17) {shot("04-summary");tap(SDLK_x);}
  if(frame==20) tap(SDLK_e);
  if(frame==23) {shot("05-bag");tap(SDLK_e);}
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
