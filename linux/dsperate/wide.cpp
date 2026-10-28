// GPL-3.0. Genuine auxiliary geometry renders; the centre is the real screen.
#include "wide.h"
#include <algorithm>
#include "core/gpu/kernels.h"

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
  // Use the emulator's real RGB666 output stage, including BG0 fades and
  // master brightness. Scaling six-bit channels by 255/63 rounded differently
  // from the hardware path and made a rectangular colour boundary.
  std::array<std::vector<uint32_t>,9> converted;
  alignas(16) uint32_t row[256];
  std::array<uint8_t,256> ids,win;ids.fill(1);win.fill(0x20);
  auto& engine=nds_.gpu.engine[0];
  for(int i=0;i<9;i++)if(views_[i]){
    converted[i].resize(256*192);
    for(int y=0;y<192;y++){
      auto raw=views_[i]->line(frames[i],y);
      int effect=(engine.bldcnt()>>6)&3;
      if(effect>=2){ds::gpu::kern::active::composite_line_fade(engine.bldcnt(),engine.evy(),raw,ids.data(),win.data(),row);raw=row;}
      ds::gpu::kern::active::output_line(raw,engine.master_bright(),converted[i].data()+y*256);
    }
  }
  out.assign(width_*height_,0xff000000);int mx=(width_-256)/2,my=(height_-192)/2;
  for(int y=0;y<height_;y++)for(int x=0;x<width_;x++) {
    int gx=x-mx,gy=y-my;
    if(gx>=0 && gx<256 && gy>=0 && gy<192){out[y*width_+x]=top[gy*256+gx];continue;}
    int dx=gx<0?-1:gx>=256?1:0,dy=gy<0?-1:gy>=192?1:0,i=(dy+1)*3+dx+1;
    if(!views_[i])continue;
    int vx=gx-dx*252,vy=gy-dy*188;
    if(vx<0 || vx>=256 || vy<0 || vy>=192)continue;
    out[y*width_+x]=converted[i][vy*256+vx];
  }
  return true;
}
