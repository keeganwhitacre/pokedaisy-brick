#pragma once
#include "party_view.h"
struct pd_bag_source;
pd_bag_source ds_bag_source(const DsCompanion&);
struct DsBagPockets { int active; unsigned occupied; };
DsBagPockets ds_bag_view(pd_canvas*,const DsCompanion&,int pocket,int selected,bool details,int sort=0,int tabKey=-1);
