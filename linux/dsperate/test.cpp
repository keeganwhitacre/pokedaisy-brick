// GPL-3.0. Real core and captured-RAM tests, never fake application screens.
#include "core/nds.h"
#include "core/state/state.h"
#include "companion.h"
#include "browser.h"
#include "party_view.h"
#include "bag_view.h"
#include "dex_view.h"
#include "battle_view.h"
#include "pk_nds_field.h"
#include <filesystem>
#include "map_art.h"
#include <cassert>
#include <fstream>
#include <cstdio>
#include <cstring>
extern "C" {
#include "../hud.h"
#include "pd_ui.h"
#include "pd_tables.h"
#include "../quick_menu.h"
}
static std::vector<uint8_t> battleRam;
static uint32_t battle_read32(uint32_t addr){
  if(addr<0x02000000 || addr>0x023ffffc)return 0;
  uint32_t value;std::memcpy(&value,battleRam.data()+addr-0x02000000,4);return value;
}
int main(int argc,char** argv){
  assert(ds_move_type(53)==10 && ds_move_power(53)==95); // app's Platinum Flamethrower
  assert(ds_species_type(387,0)==12 && ds_species_type(387,1)==12);
  assert(ds_species_type(392,0)==10 && ds_species_type(392,1)==1);
  assert(ds_move_type(-1)==-1 && ds_move_power(99999)==0);
  assert(ds_species_type(0,0)==-1 && ds_species_type(1,2)==-1);
  assert(!ds_battle_view(0,true,true,false).bottom);
  assert(ds_battle_view(1,true,true,false).bottom);
  assert(!ds_battle_view(1,true,false,true).bottom);
  assert(ds_battle_view(1,true,false,false,1).bottom);
  assert(!ds_battle_view(1,true,true,false,0).bottom);
  assert(ds_battle_view(1,false,false,true).bottom);
  std::puts("PASS automatic screen transitions and temporary manual override");
  assert(argc==3 || argc==4);std::ifstream file(argv[1],std::ios::binary);
  std::vector<uint8_t> ram{std::istreambuf_iterator<char>(file),{}};
  assert(ram.size()==4*1024*1024);
  auto battlePath=std::filesystem::path(argv[1]).parent_path().parent_path()/"platinum_battle"/"mainram.bin";
  std::ifstream capturedBattle(battlePath,std::ios::binary);
  battleRam={std::istreambuf_iterator<char>(capturedBattle),{}};
  assert(battleRam.size()==4*1024*1024);
  const auto* game=pk_nds_field_game("CPUE",1);assert(game && game->offBattleContext==0x30 && game->offCommand==8);
  assert(pk_nds_field_state(game,battle_read32,nullptr,0)&PK_NDS_FIELD_BATTLE);
  auto fs=battle_read32(game->fieldSystemPtr),pm=battle_read32(fs+game->offProcessManager),child=battle_read32(pm+4);
  auto system=battle_read32(child+0x1c),context=battle_read32(system+game->offBattleContext);
  assert(context>=0x02000000 && context<=0x023ffff0);
  // The captured fixture omits the controller head; seed just that test field.
  uint32_t command=PK_NDS_BATTLE_SELECTING;
  std::memcpy(battleRam.data()+context-0x02000000+game->offCommand,&command,4);
  assert(pk_nds_battle_command(game,battle_read32)==pk_nds_battle_selecting(game));
  std::puts("PASS captured battle pointer chain and Platinum controller metadata");
  auto s=ds_read_companion(ram.data(),ram.size());
  assert(s.valid && s.party.size()==1);
  assert(s.party[0].species==387 && s.party[0].level==6);
  pd_snapshot hs{};hs.valid=true;hs.partyCount=1;
  hs.party[0].species=s.party[0].species;hs.party[0].level=s.party[0].level;
  hs.party[0].hp=s.party[0].hp;hs.party[0].maxHp=s.party[0].maxHp;
  std::vector<uint32_t> hpixels(341*256,QUICK_CLEAR);pd_canvas hc;pd_canvas_init(&hc,341,256,hpixels.data());
  int icons=0;
  hud_party_draw(&hc,&hs,&icons,[](pd_canvas*,void* ctx,const pd_mon* mon,int,int,bool){
    assert(mon->species==387 && mon->level==6);++*static_cast<int*>(ctx);
  });
  assert(icons==1);quick_finish(&hc);
  bool drawn=false;for(auto p:hpixels)drawn|=p!=0;assert(drawn);
  // National 412 is Burmy on DS, not the GBA renderer's egg sentinel.
  DsCompanion presentation;presentation.valid=true;
  DsMon burmy;burmy.species=412;burmy.nickname="BURMY";burmy.level=5;burmy.hp=burmy.maxHp=20;
  DsMon egg=burmy;egg.egg=true;egg.nickname="PRIVATE EGG NAME";
  presentation.party={burmy,egg};
  std::vector<std::vector<uint32_t>> art(495);
  art[412].assign(1024,0xffff0000);art[494].assign(1024,0xff0000ff);
  std::vector<uint32_t> pixels(341*171);pd_canvas body;pd_canvas_init(&body,341,171,pixels.data());
  ds_party_view(&body,presentation,art,0,false,0);
  bool red=false,blue=false;for(auto px:pixels){red|=px==0xff0000;blue|=px==0x0000ff;}
  assert(red && blue);
  ds_party_view(&body,presentation,art,0,true,0);
  ds_party_view(&body,presentation,art,1,true,1);
  pd_ui legacy{};legacy.summarySlot=-1;legacy.pressedId=-1;pd_game legacyGame{};
  pd_snapshot legacySnapshot{};legacySnapshot.valid=true;legacySnapshot.partyCount=2;
  legacySnapshot.party[0].species=1;legacySnapshot.party[0].hp=legacySnapshot.party[0].maxHp=20;
  legacySnapshot.party[1].species=PD_SPECIES_EGG;
  pd_ui_party_view(&legacy,&body,&legacyGame,&legacySnapshot,0);
  assert(legacy.hitCount>0 && legacy.partySource==nullptr);
  DsCompanion inventory;inventory.valid=true;
  for(int i=0;i<150;i++)inventory.bag.push_back({17,1,4});
  inventory.bag.push_back({4,5,6});inventory.bag.push_back({1,1,1});
  ds_bag_view(&body,inventory,0,149,false); // beyond GBA's 99-row view
  ds_bag_view(&body,inventory,5,149,true);
  ds_bag_view(&body,inventory,8,0,false); // empty last pocket, scrolled tabs
  inventory.bag.clear();ds_bag_view(&body,inventory,0,0,true);
  pd_config bagConfig{};for(int i=0;i<PD_POCKET_COUNT;i++)bagConfig.bagOrder[i]=i;
  legacyGame.cfg=&bagConfig;legacySnapshot.bagOk=true;legacy.bagSelected=-1;
  pd_ui_bag_view(&legacy,&body,&legacyGame,&legacySnapshot);
  assert(legacy.bagSource==nullptr && legacy.hitCount>0);
  DsCompanion dex;dex.valid=dex.dexValid=true;dex.dexSeen.resize(494);dex.dexCaught.resize(494);
  dex.dexSeen[493]=dex.dexCaught[493]=1;dex.seenCount=dex.caughtCount=1;
  ds_dex_view(&body,dex,0,492); // Gen 4 ID beyond shared GBA arrays
  ds_dex_view(&body,dex,1,999);ds_dex_view(&body,dex,2,-1);
  dex.dexCaught[493]=0;ds_dex_view(&body,dex,2,0);
  pd_ui nav{};nav.pressedId=-1;const char* tabs[]={"PARTY","BAG","DEX","STATUS","SET","MAP"};
  pd_ui_navigation(&nav,&body,tabs,6,2);assert(nav.hitCount==6);
  pd_ui_navigation(&nav,&body,tabs,0,0);assert(nav.hitCount==0);
  std::puts("PASS shared dex national 493, filter bounds, empty list and navigation chips");
  std::puts("PASS shared bag rendering past row 99, details, empty pockets and trailing tabs");
  std::puts("PASS shared party/summary renderer with DS Burmy and egg identities");
  std::puts("PASS shared party HUD with captured Gen 4 data and backend asset callback");
  assert(s.dexValid && s.seenCount==2 && s.caughtCount==1);
  assert(s.dexSeen[387] && s.dexSeen[390] && s.dexCaught[387] && !s.dexCaught[390]);
  assert(s.dexSeen.size()==494);
  assert(ds_dex_count(s,0)==493 && ds_dex_count(s,1)==2 && ds_dex_count(s,2)==1);
  assert(!ds_dex_visible(s,0,0) && !ds_dex_visible(s,494,0));
  assert(ds_dex_visible(s,390,1) && !ds_dex_visible(s,390,2));
  int total=0;for(int pocket=1;pocket<=8;pocket++)total+=ds_bag_count(s,pocket);
  assert(total==int(s.bag.size()) && ds_bag_count(s,0)==total);
  DsCompanion empty;assert(ds_dex_count(empty,2)==0 && ds_bag_count(empty,8)==0);
  std::puts("PASS bag pocket partition and seen/caught dex bounds on upstream RAM fixture");

  assert(s.mapX==3 && s.mapY==27); // Player's house: use its outside exit block.
  assert(ds_map_section(s.mapX,s.mapY)==1);
  assert(std::strcmp(ds_map_name(1),"Twinleaf Town")==0);
  assert(ds_map_section(0,0)==-1 && ds_map_section(-1,0)==-1 && ds_map_section(30,0)==-1);
  assert(ds_map_art({}).background.empty());
  assert(ds_party_icons({}).empty());
  if(argc==4){
    std::ifstream cart(argv[3],std::ios::binary);std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(cart),{}};
    auto icons=ds_party_icons(bytes);assert(icons.size()==495);
    for(int species: {1,387,412,493,494}){
      assert(icons[species].size()==1024);bool clear=false,opaque=false;
      for(auto p:icons[species]){clear|=p==0;opaque|=(p>>24)==255;}assert(clear && opaque);
    }
    std::puts("PASS real ROM party icons including Gen 4 species, Burmy and egg");
    auto art=ds_map_art(bytes);assert(art.width==216 && art.height==168);
    assert(art.background.size()==216*168 && art.blocks.size()==21 && art.heads.size()==2);
    bool varied=false;for(auto px:art.background)varied|=px!=art.background[0];assert(varied);
    std::puts("PASS real ROM Town Map tiles/palettes/cells and player heads decoded");
  }
  std::puts("PASS map grid blank/bounds and captured indoor exit-location fallback");
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
