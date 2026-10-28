// GPL-3.0. Genuine auxiliary geometry renders; the centre is the real screen.
#include "wide.h"
#include <algorithm>

DsWide::DsWide(ds::NDS& nds):nds_(nds) {}
DsWide::~DsWide(){enable(false);}
void DsWide::enable(bool on,int w,int h) {
  w=std::clamp(w,256,504);h=std::clamp(h,192,376);
  if(on==enabled_ && w==width_ && h==height_)return;
  nds_.gpu3d.sync_raster();enabled_=on;width_=w;height_=h;swapped_=false;polygons_=0;
  for(int i=0;i<9;i++)if(i!=4) {
    bool need=on && ((i%3==1 || w>256) && (i/3==1 || h>192));
    if(need && !views_[i])views_[i]=std::make_unique<ds::gpu::Gpu3D>(nds_);
    if(!need)views_[i].reset();
  }
  nds_.gpu3d.poke_user=this;
  nds_.gpu3d.poke_submit=on?polygon:nullptr;
  nds_.gpu3d.poke_swap=on?swap:nullptr;
  nds_.gpu3d.poke_width=w;nds_.gpu3d.poke_height=h;
}
void DsWide::polygon(void* p,const ds::gpu::Gpu3D& source,const ds::gpu::Vertex* const* vertices,int n) {
  auto& self=*static_cast<DsWide*>(p);
  for(int i=0;i<9;i++)if(self.views_[i])self.views_[i]->poke_polygon(source,vertices,n,(i%3-1)*252,(i/3-1)*188);
  self.polygons_++;
}
void DsWide::swap(void* p,const ds::gpu::Gpu3D& source) {
  auto& self=*static_cast<DsWide*>(p);
  for(auto& view:self.views_)if(view)view->poke_swap_list(source);
  self.swapped_=true;
}
bool DsWide::compose(const uint32_t* top,std::vector<uint32_t>& out) {
  if(!enabled_ || !swapped_)return false;
  nds_.gpu.quiesce();nds_.gpu3d.sync_raster();
  std::array<ds::gpu::Renderer3D::FrameRef,9> frames;
  for(int i=0;i<9;i++)if(views_[i]){views_[i]->poke_render(nds_.gpu3d);frames[i]=views_[i]->frame_ref(false);}
  out.assign(width_*height_,0xff000000);int mx=(width_-256)/2,my=(height_-192)/2;
  for(int y=0;y<height_;y++)for(int x=0;x<width_;x++) {
    int gx=x-mx,gy=y-my;
    if(gx>=0 && gx<256 && gy>=0 && gy<192){out[y*width_+x]=top[gy*256+gx];continue;}
    int dx=gx<0?-1:gx>=256?1:0,dy=gy<0?-1:gy>=192?1:0,i=(dy+1)*3+dx+1;
    if(!views_[i])continue;
    int vx=gx-dx*252,vy=gy-dy*188;
    if(vx<0 || vx>=256 || vy<0 || vy>=192)continue;
    uint32_t p=views_[i]->line(frames[i],vy)[vx];
    out[y*width_+x]=0xff000000u | ((p&63)*255/63)<<16 | (((p>>8)&63)*255/63)<<8 | (((p>>16)&63)*255/63);
  }
  return true;
}
