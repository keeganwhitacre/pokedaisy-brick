// GPL-3.0. Platinum map assets decoded from the owner's cartridge, as NdsArt.kt does.
#pragma once
#include <cstdint>
#include <vector>
struct DsMapArt {
  std::vector<uint32_t> background;
  std::vector<std::vector<uint32_t>> blocks,heads;
  int width=0,height=0;
};
DsMapArt ds_map_art(const std::vector<uint8_t>& rom);
int ds_map_section(int x,int y);
const char* ds_map_name(int section);
