// GPL-3.0. Experimental DSperate implementation of PokéDaisy's slice views.
#pragma once
#include "core/nds.h"
#include <memory>
#include <array>
#include <vector>

class DsWide {
 public:
  explicit DsWide(ds::NDS& nds);
  ~DsWide();
  void enable(bool on, int width=340, int height=256);
  bool compose(const uint32_t* top, std::vector<uint32_t>& out);
  int width() const { return width_; }
  int height() const { return height_; }
  unsigned polygons() const { return polygons_; }
 private:
  ds::NDS& nds_;
  std::array<std::unique_ptr<ds::gpu::Gpu3D>,9> views_;
  bool enabled_=false, swapped_=false;
  int width_=340, height_=256;
  unsigned polygons_=0;
  static void polygon(void*,const ds::gpu::Gpu3D&,const ds::gpu::Vertex* const*,int);
  static void swap(void*,const ds::gpu::Gpu3D&);
};
