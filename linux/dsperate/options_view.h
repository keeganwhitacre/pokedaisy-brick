#pragma once
#include "party_view.h"
extern "C" {
#include "pd_ui.h"
}
void ds_settings_view(pd_canvas*,const pd_settings&,bool,int,int);
void ds_status_view(pd_canvas*,const DsCompanion&);
