// GPL-3.0. Real core and captured-RAM tests, never fake application screens.
#include "core/nds.h"
#include "core/state/state.h"
#include "companion.h"
#include "wide.h"
#include <cassert>
#include <fstream>
#include <cstdio>
#include <cstring>
int main(int argc,char** argv){
  assert(argc==3);std::ifstream file(argv[1],std::ios::binary);
  std::vector<uint8_t> ram{std::istreambuf_iterator<char>(file),{}};
  assert(ram.size()==4*1024*1024);
  auto s=ds_read_companion(ram.data(),ram.size());
  assert(s.valid && s.party.size()==1);
  assert(s.party[0].species==387 && s.party[0].level==6);
  assert(s.dexValid && s.seenCount==2 && s.caughtCount==1);
  assert(s.dexSeen[387] && s.dexSeen[390] && s.dexCaught[387] && !s.dexCaught[390]);
  assert(s.dexSeen.size()==494);

  bool potion=false,balls=false;
  for(auto i:s.bag){potion|=i.id==17 && i.count==3;balls|=i.id==4 && i.count==5;}
  assert(potion && balls);
  assert(std::strcmp(ds_species(387),"TURTWIG")==0);
  std::puts("PASS captured upstream Platinum RAM fixture: party, checksum/decryption and bag");
  // Truncated/corrupt input must not become a valid companion snapshot.
  assert(!ds_read_companion(ram.data(),16).valid);
  auto invalid=ram;std::memset(invalid.data(),0,invalid.size());
  assert(!ds_read_companion(invalid.data(),invalid.size()).valid);
  auto nds=std::make_unique<ds::NDS>();
  nds->bus.dma_write32(ds::Cpu::ARM9,0x02000000,0x12345678);
  assert(nds->bus.dma_read32(ds::Cpu::ARM9,0x02000000)==0x12345678);
  nds->io.set_buttons(1);assert(!(nds->io.keyinput&1));nds->io.set_buttons(0);assert(nds->io.keyinput&1);
  DsWide wide(*nds);wide.enable(true);
  auto w32=[&](uint32_t a,uint32_t v){nds->bus.dma_write32(ds::Cpu::ARM9,a,v);};
  auto cmd=[&](int c,uint32_t v){w32(0x04000400+c*4,v);};
  nds->bus.dma_write16(ds::Cpu::ARM9,0x04000304,0x820f);
  w32(0x04000060,0);w32(0x04000350,0x001f0000);
  cmd(0x10,0);cmd(0x15,0);cmd(0x10,1);cmd(0x15,0);cmd(0x60,0xbfff0000);
  cmd(0x29,0x001f00c0);cmd(0x2a,0);cmd(0x40,1);cmd(0x20,0x001f);
  // A red quad entirely outside the original right clip plane.
  auto v=[&](int16_t x,int16_t y){cmd(0x25,(uint32_t(uint16_t(y))<<16)|uint16_t(x));};
  v(0x1100,-0x800);v(0x1500,-0x800);v(0x1500,0x800);v(0x1100,0x800);cmd(0x41,0);cmd(0x50,0);
  nds->sched.run_until(nds->sched.now()+ds::CYCLES_PER_FRAME*2);
  nds->gpu.quiesce();
  std::vector<uint32_t> out;const uint32_t* original=nds->gpu.framebuffer(0);
  assert(wide.compose(original,out));assert(wide.polygons()>0);
  int mx=(wide.width()-256)/2,my=(wide.height()-192)/2;
  for(int y=0;y<192;y++)for(int x=0;x<256;x++)assert(out[(y+my)*wide.width()+x+mx]==original[y*256+x]);
  assert(out[(96+my)*wide.width()+270+mx]==0xffff0000u);
  nds->bus.dma_write16(ds::Cpu::ARM9,0x0400006c,0x8008);
  nds->sched.run_until(nds->sched.now()+ds::CYCLES_PER_FRAME*2);
  nds->gpu.quiesce();
  assert(wide.compose(nds->gpu.framebuffer(0),out));
  assert(out[(96+my)*wide.width()+270+mx]==0xff7d0000u);
  std::puts("PASS auxiliary view uses the DS master-brightness output stage");
  std::puts("PASS DSperate memory/input and real offscreen polygon rendering; centre unchanged");
  // A world polygon across the whole viewport: hardware edge marking makes
  // an artificial outline at the old viewport perimeter. The overlapping
  // views must supply the continuous world there, without touching overlays.
  w32(0x04000000,0x00010108); // Display engine A BG0 as 3D.
  nds->bus.dma_write16(ds::Cpu::ARM9,0x0400006c,0);
  nds->bus.dma_write16(ds::Cpu::ARM9,0x04000060,1u<<5);
  nds->bus.dma_write16(ds::Cpu::ARM9,0x04000330,0x1084);
  cmd(0x29,0x011f00c0);cmd(0x40,1);cmd(0x20,0x001f);
  v(-0x1800,-0x1800);v(0x1800,-0x1800);v(0x1800,0x1800);v(-0x1800,0x1800);cmd(0x41,0);cmd(0x50,0);
  nds->sched.run_until(nds->sched.now()+ds::CYCLES_PER_FRAME*2);nds->gpu.quiesce();
  original=nds->gpu.framebuffer(0);assert(wide.compose(original,out));
  assert(original[96*256]!=0xffff0000u); // Reproduce the unwanted viewport edge.
  assert(out[(96+my)*wide.width()+mx]==0xffff0000u);
  assert(out[my*wide.width()+128+mx]==0xffff0000u);
  auto overlay=std::vector<uint32_t>(original,original+256*192);
  overlay[96*256]=0xff123456u;assert(wide.compose(overlay.data(),out));
  assert(out[(96+my)*wide.width()+mx]==0xff123456u);
  std::puts("PASS viewport outline removed with real overlapping geometry; 2D overlay retained");
  auto boot=std::make_unique<ds::NDS>();std::string error;
  assert(boot->load_bios("","","",{},&error));assert(boot->load_rom(argv[2]));
  boot->setup_direct_boot();boot->run_frame();boot->run_frame();boot->gpu.quiesce();
  assert(boot->gpu.framebuffer(0)[96*256+128]==0xff00fb00u);
  boot->bus.dma_write32(ds::Cpu::ARM9,0x02010000,0x12345678);
  ds::state::Writer writer;assert(boot->save_state(writer,error));
  boot->bus.dma_write32(ds::Cpu::ARM9,0x02010000,0);
  ds::state::Reader reader(writer.data().data(),writer.data().size());
  assert(boot->load_state(reader,error));
  assert(boot->bus.dma_read32(ds::Cpu::ARM9,0x02010000)==0x12345678);
  std::puts("PASS original DS diagnostic boot, video and state roundtrip");
}
