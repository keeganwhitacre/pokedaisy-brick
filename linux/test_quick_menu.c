/* GPL-3.0: layout reachability, actual settings hit and persistence. */
#include "quick_menu.h"
#include "display_filter.h"
#include "pd_settings.h"
#include "pd_ui_internal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static uint32_t px[512*384];
int main(void) {
  for(int n=1;n<=QUICK_COUNT;n++)for(int width=384;width<=1024;width+=128) {
    bool seen[QUICK_COUNT]={true};int queue[QUICK_COUNT]={0},head=0,tail=1;
    const int dx[]={-1,1,0,0},dy[]={0,0,-1,1};
    while(head<tail) {
      int at=queue[head++];
      for(int d=0;d<4;d++) {
        int next=quick_step_layout(n,at,dx[d],dy[d],width,384,true);
        assert(next>=0 && next<n);
        if(!seen[next]){seen[next]=true;queue[tail++]=next;}
      }
    }
    assert(tail==n);
  }
  assert(quick_step_layout(9,0,-1,0,512,384,true)==0);
  assert(quick_step_layout(9,0,1,0,512,384,true)==1);
  assert(quick_step_layout(9,0,0,1,512,384,true)==5);
  assert(quick_step_layout(9,4,1,0,512,384,true)==4);
  assert(quick_step_layout(9,8,0,1,512,384,true)==8);
  assert(quick_step_layout(9,8,1,0,512,384,true)==8);
  assert(quick_step_layout(9,5,-1,0,512,384,true)==5);
  assert(pocket_lcd_pixel(0,0)==pocket_lcd_pixel(3,3));
  assert(pocket_lcd_pixel(0,0)!=pocket_lcd_pixel(1,0));
  assert((pocket_lcd_pixel(0,2)&255)<(pocket_lcd_pixel(0,0)&255));
  struct pd_settings settings;pd_settings_defaults(&settings);assert(!settings.quickGrid);
  struct pd_ui ui;pd_ui_init(&ui,&settings);ui.tab=PD_TAB_SETTINGS;
  struct pd_game game={0};struct pd_snapshot snap={0};struct pd_host_info host={0};
  struct pd_canvas canvas;pd_canvas_init(&canvas,512,384,px);
  pd_ui_draw(&ui,&canvas,&game,&snap,&host);
  for(int i=0;i<ui.hitCount;i++)assert(ui.hits[i].id!=HIT_SETTING+SETTING_QUICK_MENU);
  host.hasQuickMenu=true;pd_ui_draw(&ui,&canvas,&game,&snap,&host);
  bool found=false;
  for(int i=0;i<ui.hitCount;i++)if(ui.hits[i].id==HIT_SETTING+SETTING_QUICK_MENU) {
    struct pd_hit h=ui.hits[i];pd_ui_touch(&ui,h.x+1,h.y+1,true);pd_ui_touch(&ui,h.x+1,h.y+1,false);found=true;break;
  }
  assert(found && settings.quickGrid && pd_ui_take_action(&ui)==PD_ACTION_SETTINGS_CHANGED);
  char path[128];snprintf(path,sizeof path,"/tmp/pokedaisy-grid-%ld.ini",(long)getpid());
  settings.lcdFilter=true;
  assert(pd_settings_save(&settings,path));
  struct pd_settings loaded;assert(pd_settings_load(&loaded,path) && loaded.quickGrid && loaded.lcdFilter);
  FILE *f=fopen(path,"w");assert(f);fputs("quickgrid=9\n",f);assert(!fclose(f));
  assert(pd_settings_load(&loaded,path) && !loaded.quickGrid);unlink(path);
  struct quick_item items[QUICK_COUNT]={0};for(int i=0;i<QUICK_COUNT;i++)items[i].action=i;
  for(int grid=0;grid<2;grid++) {
    assert(quick_draw(&canvas,&game,items,QUICK_COUNT,0,-1,grid));
    quick_finish(&canvas);bool opaque=false;for(int i=0;i<512*384;i++)opaque|=!!(px[i]>>24);assert(opaque);
  }
  puts("PASS Ring/Grid draw, grid reachability and edges, optional Settings control and saved style/default compatibility");
}
