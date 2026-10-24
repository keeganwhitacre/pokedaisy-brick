#define POCKET_TEST
#define main pocket_app_main
#include "main.c"
#undef main
#include <assert.h>
static const char *starter;
static unsigned char *romBytes;
static size_t romSize;
static unsigned fades;
static uint32_t a[1024 * 768], b[1024 * 768];
static void read_screen(uint32_t *out, int w) {
  assert(!SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB888, out, w * 4));
}
static void drive(struct mCore *c, unsigned keys, int n) {
  c->setKeys(c, keys);
  for (int i = 0; i < n; i++) {
    c->runFrame(c); wide_tick(c, false);
    struct pd_snapshot s;
    pc_read(&s, read_memory, c); battleVisible = s.inBattle;
    present_game(160, 144);
    if (fadeActive) {
      fades++;
      if (fadeStep == 1 || fadeStep == 5 || fadeStep == 9) {
        if (fadeStep == 1)
          assert(pd_png_write("runtime-phase22-retained-world.png", fadeW, fadeH, fadeWorld));
        char name[80]; snprintf(name, sizeof name, "runtime-phase22-fade-%u.png", fadeStep);
        assert(capture_frame(name, NULL));
      }
      read_screen(a, 512);
      int z = lastGameRect.w / 160;
      for (int y = 0; y < 144; y++) for (int x = 0; x < 160; x++)
        assert((a[(lastGameRect.y+y*z)*512+lastGameRect.x+x*z]&0xffffff) == gamePixels[y*256+x]);
    }
  }
}
static void test_frame(struct mCore *c, int *pc, bool companion, int frame,
                       struct pd_ui *u, struct pd_snapshot *s, const struct pc_ui *p) {
  (void)pc; (void)companion; (void)u; (void)p;
  assert(frame == 0 && state(c, starter, false));
  wideMode = 1;
  struct pocket_action probe = {0}; bool freeField = false;
  for (int i=0; i<100 && !freeField; i++) {
    drive(c,1,8); drive(c,0,60);
    freeField = pocket_action_begin(&probe,displayKey,read_memory,c);
    pocket_action_cancel(&probe);
  }
  assert(freeField);
  uint8_t xy[2];
  for (int i=0;i<20;i++) {
    assert(read_memory(c,0x1dcae,xy,2)); if(xy[1]==4) break;
    drive(c,xy[1]>4?32:16,1); drive(c,0,17);
  }
  for (int i=0;i<20;i++) {
    assert(read_memory(c,0x1dcae,xy,2)); if(xy[0]>=6) break;
    drive(c,128,1); drive(c,0,17);
  }
  bool ready=false;
  for(int i=0;i<150;i++) {
    struct pocket_battle_menu m; pocket_battle_read(read_memory,c,&m);
    if(m.state==PB_ACTION){ready=true;break;}
    drive(c,1,3); drive(c,0,57);
  }
  assert(ready && fades);
  printf("PASS real-ROM battle fade: %u frames; original PPU pixels exact\n", fades);
  pc_read(s,read_memory,c);
  for(int width=512;width<=1024;width*=2) {
    int height=width*3/4;
    SDL_SetWindowSize(window,width,height); SDL_PumpEvents();
    int w,h; SDL_GetRendererOutputSize(renderer,&w,&h); assert(w==width&&h==height);
    for(int filter=1;filter<=3;filter+=2) {
      displayOptions.filter=filter;
      displayOptions.battleBackground=true; present_game(160,144); read_screen(a,w);
      SDL_Rect game=lastGameRect;
      displayOptions.battleBackground=false; present_game(160,144); read_screen(b,w);
      for(int y=game.y;y<game.y+game.h;y++) for(int x=game.x;x<game.x+game.w;x++)
        assert(a[y*w+x]==b[y*w+x]);
      bool light=false,dark=false;
      for(int y=0;y<h;y++) for(int x=0;x<game.x;x++) {
        light|=(a[y*w+x]&0xffffff)==0xffffff;
        dark|=(a[y*w+x]&0xffffff)<0xffffff;
      }
      assert(light&&dark);
      displayOptions.battleBackground=true; present_game(160,144);
      char name[100]; snprintf(name,sizeof name,"runtime-phase22-battle-%d-%d.png",w,filter);
      capture_frame(name,NULL);
      struct pc_ui ui={.section=PC_PARTY};
      pc_ui_draw(&ui,&canvas,s,romBytes,romSize,pc_identify(romBytes,romSize));
      displayOptions.companionShaders=false; present_ui(true); read_screen(b,w);
      displayOptions.companionShaders=true; present_ui(true); read_screen(a,w);
      unsigned changed=0; for(int i=0;i<w*h;i++) changed+=a[i]!=b[i];
      assert(changed>(unsigned)(w*h/4));
      snprintf(name,sizeof name,"runtime-phase22-companion-%d-%d.png",w,filter);
      capture_frame(name,NULL);
    }
  }
  puts("PASS 512/1024 LCD and scanlines: visible companion coverage, patterned battle margins, unchanged filtered game centre");
  running=0;
}
int main(int argc,char **argv) {
  assert(argc==3); starter=argv[2];
  FILE *f=fopen(argv[1],"rb"); assert(f); fseek(f,0,SEEK_END);romSize=ftell(f);rewind(f);
  romBytes=malloc(romSize);assert(romBytes&&fread(romBytes,1,romSize,f)==romSize);fclose(f);
  char dir[]="/tmp/pd-render-XXXXXX";assert(mkdtemp(dir));forceFresh=true;
  char *args[]={"pokedaisy-pocket","--data",dir,"--frames","2",argv[1],NULL};
  assert(!pocket_app_main(6,args));free(romBytes);return 0;
}
