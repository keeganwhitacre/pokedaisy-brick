#include "game_action.h"
#include <string.h>
#include "polished_action_data.h"
/* Observation values are generated from app data and pinned official symbols. */
static bool read8(pocket_action_read r, void *c, uint32_t a, uint8_t *v) {
  return r(c, a, v, 1);
}
static bool field(pocket_action_read r, void *c) {
  uint8_t state, script, map, battle;
  return read8(r, c, PD_PC_STATE, &state) && read8(r, c, PD_PC_SCRIPT, &script) &&
         read8(r, c, PD_PC_MAP, &map) && read8(r, c, PD_PC_BATTLE, &battle) &&
         !(state & 0x40) && script == 0 && map == 2 && battle == 0;
}
/* tmhm2.asm uses DoMenuJoypadLoop rather than the scrolling-menu header.
 * Require its complete menu geometry, flags and visible waiting cursor. */
static bool tm_pocket_waiting(pocket_action_read r,void *c) {
  uint8_t pocket,phase,inMenu,geometry[8],cursor[2],tile;
  if(!read8(r,c,PD_PC_POCKET,&pocket) || pocket!=3 ||
     !read8(r,c,PD_PC_PACK_STATE,&phase) || phase!=2 ||
     !read8(r,c,PD_PC_IN_MENU,&inMenu) || inMenu!=1 ||
     !r(c,PD_PC_MENU2D,geometry,sizeof geometry) ||
     !r(c,PD_PC_CURSOR,cursor,sizeof cursor))return false;
  if(geometry[0]!=2 || geometry[1]!=7 || geometry[2]<1 || geometry[2]>5 ||
     geometry[3]!=1 || geometry[4]!=0x0c || geometry[5]!=0 ||
     geometry[6]!=0x20 || geometry[7]!=0xfb || cursor[0]<1 ||
     cursor[0]>geometry[2] || cursor[1]!=1)return false;
  unsigned row=2+2*(cursor[0]-1);
  return read8(r,c,PD_PC_TILEMAP+row*20+7,&tile) && tile==PD_PC_CURSOR_TILE;
}
bool pocket_action_begin(struct pocket_action *a, const char *sha,
                         pocket_action_read r, void *c) {
  if (a->result == PA_RUNNING)
    return false;
  *a = (struct pocket_action){.result = PA_REJECTED, .target = 2};
  if (!sha || strcmp(sha, PD_PC_ACTION_SHA) ||
      !field(r, c))
    return false;
  a->result = PA_RUNNING;
  return true;
}
int pocket_action_flyer(const char *sha,pocket_action_read r,void *c) {
  uint8_t count,mon[PD_PC_PARTY_SIZE];
  if(!sha || strcmp(sha,PD_PC_ACTION_SHA) || !read8(r,c,PD_PC_PARTY_COUNT,&count) || count>6)return -2;
  for(unsigned i=0;i<count;i++) {
    if(!r(c,PD_PC_PARTY_MON+i*PD_PC_PARTY_SIZE,mon,sizeof mon))return -2;
    uint8_t form=mon[PD_PC_PARTY_EGG_OFFSET];
    if((!mon[0] && !(form&0x20)) || (form&PD_PC_PARTY_EGG_MASK))continue;
    for(unsigned m=0;m<4;m++)if(mon[PD_PC_PARTY_MOVES_OFFSET+m]==PD_PC_FLY_MOVE)return (int)i;
  }
  return -1;
}
const char *pocket_action_fly_reason(const char *sha,pocket_action_read r,void *c) {
  if(!sha || strcmp(sha,PD_PC_ACTION_SHA))return "Fly not supported in this version";
  if(!field(r,c))return "Return to the overworld first";
  uint8_t badges;
  if(!read8(r,c,PD_PC_BADGES,&badges))return "Cannot read badges";
  if(!(badges&0x20))return "Storm Badge required";
  int flyer=pocket_action_flyer(sha,r,c);
  return flyer==-2 ? "Cannot read party" : flyer<0 ? "No one in your party knows Fly" : NULL;
}
bool pocket_action_begin_fly(struct pocket_action *a,const char *sha,const uint8_t *rom,
                             size_t size,pocket_action_read r,void *c) {
  if(!pocket_action_begin(a,sha,r,c))return false;
  a->target=1;a->result=PA_REJECTED;
  if(pocket_action_fly_reason(sha,r,c) || !rom || PD_PC_PARTY_SIZE>sizeof a->partyRecord)return false;
  /* The ROM's MonMenuOptions maps moves to menu IDs. Don't maintain a second
   * MONMENUITEM_FLY constant/table: read the actual version-matched table. */
  size_t at=(PD_PC_MON_OPTIONS>>16)*0x4000u+(PD_PC_MON_OPTIONS&0x3fffu);
  unsigned found=0;bool terminated=false;
  for(unsigned i=0;i<32;i++,at+=3) {
    if(at>=size)return false;
    if(rom[at]==255){terminated=true;break;}
    if(size-at<3)return false;
    if(rom[at]==0 && rom[at+2]==PD_PC_FLY_MOVE){found++;a->flyMenu=rom[at+1];}
  }
  if(!terminated || found!=1 || !a->flyMenu || a->flyMenu>=64)return false;
  int slot=pocket_action_flyer(sha,r,c);if(slot<0)return false;
  a->slot=(unsigned)slot;
  if(!r(c,PD_PC_PARTY_MON+a->slot*PD_PC_PARTY_SIZE,a->partyRecord,PD_PC_PARTY_SIZE))return false;
  a->result=PA_RUNNING;return true;
}
static bool menu_cursor(pocket_action_read r,void *c,unsigned top,unsigned left,
                        unsigned rows,unsigned *row) {
  uint8_t m[7],cur[2],tile;
  if(!r(c,PD_PC_MENU2D,m,sizeof m) || !r(c,PD_PC_CURSOR,cur,sizeof cur) ||
     m[0]!=top || m[1]!=left || m[2]!=rows || m[3]!=1 || m[6]!=0x20 ||
     !cur[0] || cur[0]>rows || cur[1]!=1)return false;
  unsigned y=top+2*(cur[0]-1);
  if(y>=18 || left>=20 || !read8(r,c,PD_PC_TILEMAP+y*20+left,&tile) || tile!=PD_PC_CURSOR_TILE)return false;
  *row=cur[0];return true;
}
static unsigned select_fly(struct pocket_action *a,pocket_action_read r,void *c) {
  uint8_t count,mon[PD_PC_PARTY_SIZE],curParty,kind,items[9];unsigned row=0;
  if(!read8(r,c,PD_PC_PARTY_COUNT,&count) || a->slot>=count || count>6 ||
     !r(c,PD_PC_PARTY_MON+a->slot*PD_PC_PARTY_SIZE,mon,sizeof mon) ||
     memcmp(mon,a->partyRecord,sizeof mon)) {a->result=PA_REJECTED;return 0;}
  unsigned want=0;
  if(a->phase==2) {
    uint8_t action;
    if(!read8(r,c,PD_PC_PARTY_MENU,&action) || action!=0 ||
       !menu_cursor(r,c,1,0,count+1,&row)) {a->stable=0;return 0;}
    want=a->slot+1;
  } else {
    bool waiting=read8(r,c,PD_PC_CUR_PARTY,&curParty) && curParty==a->slot &&
      read8(r,c,PD_PC_MON_COUNT,&kind) && kind>0 && kind<=8 &&
      r(c,PD_PC_MON_LIST,items,kind+1) && items[kind]==255 &&
      menu_cursor(r,c,18-2*kind,7,kind,&row);
    if(a->phase==4) {
      if(!waiting && ++a->stable>=6)a->result=PA_DONE;
      if(waiting)a->stable=0;
      return 0; /* The player, not this controller, chooses the destination. */
    }
    if(!waiting){a->stable=0;return 0;}
    uint64_t seen=0;
    for(unsigned i=0;i<kind;i++) {
      if(!items[i] || items[i]>=64 || (seen&(1ull<<items[i]))) {a->result=PA_REJECTED;return 0;}
      seen|=1ull<<items[i];if(items[i]==a->flyMenu)want=i+1;
    }
    if(!want){a->result=PA_REJECTED;return 0;}
  }
  if(++a->stable<6)return 0;
  a->stable=0;a->pulse=12;
  unsigned key=row==want ? 1 : row<want ? 128 : 64;
  a->hold=1;a->heldKey=key;
  if(row==want)a->phase++;
  return key;
}
static bool item_stack(pocket_action_read r,void *c,unsigned p,unsigned slot,
                       unsigned item,unsigned qty) {
  unsigned char raw[152];unsigned max=pd_pc_item_pockets[p].max;
  if(max>75 || !r(c,pd_pc_item_pockets[p].addr,raw,2+2*max))return false;
  unsigned n=raw[0];
  return n<=max && raw[1+2*n]==255 && slot<n &&
         raw[1+2*slot]==item && raw[2+2*slot]==qty;
}
/* Key items are a zero-terminated list, not {id,quantity} pairs. Preserve
 * their game-owned ordering and reject duplicates / missing terminators. */
static bool key_slot(pocket_action_read r,void *c,unsigned item,unsigned *slot) {
  unsigned char keys[PD_PC_KEY_MAX+1];
  if(item<=0x200 || item>0x200+PD_PC_KEY_ID_MAX || !r(c,PD_PC_KEYS,keys,sizeof keys))return false;
  bool seen[256]={0},found=false;
  for(unsigned i=0;i<sizeof keys;i++) {
    if(!keys[i])return found;
    if(i==PD_PC_KEY_MAX || keys[i]>PD_PC_KEY_ID_MAX || seen[keys[i]])return false;
    seen[keys[i]]=true;
    if(keys[i]==item-0x200){*slot=i;found=true;}
  }
  return false;
}
static bool selected_stack(const struct pocket_action *a,pocket_action_read r,void *c) {
  if(a->item>0x200) {
    unsigned slot=0;
    return key_slot(r,c,a->item,&slot) && slot==a->slot;
  }
  return item_stack(r,c,a->pocket,a->slot,a->item,a->quantity);
}
bool pocket_action_begin_item(struct pocket_action *a,const char *sha,unsigned item,
                              pocket_action_read r,void *c) {
  if(!pocket_action_begin(a,sha,r,c))return false;
  a->result=PA_REJECTED;
  if(item>0x200 && item<0x300) {
    if(!key_slot(r,c,item,&a->slot))return false;
    a->item=item;a->quantity=1;a->result=PA_RUNNING;return true;
  }
  if(!item || item>=255)return false; /* TM namespace needs its own adapter. */
  unsigned matches=0;
  for(unsigned p=0;p<sizeof pd_pc_item_pockets/sizeof *pd_pc_item_pockets;p++) {
    unsigned char raw[152];unsigned max=pd_pc_item_pockets[p].max;
    if(max>75 || !r(c,pd_pc_item_pockets[p].addr,raw,2+2*max))return false;
    unsigned n=raw[0];if(n>max || raw[1+2*n]!=255)return false;
    for(unsigned i=0;i<n;i++)if(raw[1+2*i]==item) {
      matches++;a->pocket=p;a->slot=i;a->quantity=raw[2+2*i];
    }
  }
  if(matches!=1 || !a->quantity)return false;
  a->item=item;a->result=PA_RUNNING;return true;
}
static unsigned select_item(struct pocket_action *a,pocket_action_read r,void *c,
                            unsigned menu,unsigned state,unsigned script) {
  if(!selected_stack(a,r,c)) {
    a->result=PA_REJECTED;return 0;
  }
  if(!(state&0x40) || script!=1)return 0;
  unsigned char pocket,cursor,scroll,item;
  if(!read8(r,c,PD_PC_POCKET,&pocket) || !read8(r,c,PD_PC_CURSOR,&cursor) ||
     !read8(r,c,PD_PC_SCROLL,&scroll) || !read8(r,c,PD_PC_ITEM,&item)) {
    a->result=PA_REJECTED;return 0;
  }
  if(a->phase==4) {
    bool submenu=false;
    for(unsigned i=0;i<sizeof pd_pc_item_submenus/sizeof *pd_pc_item_submenus;i++)
      submenu=submenu || menu==pd_pc_item_submenus[i];
    if(a->item>0x200)submenu=menu==pd_pc_item_submenus[2] || menu==PD_PC_KEY_USE_SEL_MENU;
    if(submenu && item==(a->item&255)) {if(++a->stable>=6)a->result=PA_DONE;}
    else a->stable=0;
    return 0;
  }
  if(pocket>5){a->result=PA_REJECTED;return 0;}
  if(++a->stable<6)return 0;
  a->stable=0;a->pulse=12;
  unsigned targetPocket=a->item>0x200 ? 5 : pd_pc_item_pockets[a->pocket].pocket;
  unsigned menuIndex=a->item>0x200 ? 4 : pd_pc_item_pockets[a->pocket].menu;
  if(pocket!=targetPocket)return 16; /* normal Right */
  if(menu!=pd_pc_pack_menus[menuIndex])return 0;
  if(cursor<1 || cursor>6){a->result=PA_REJECTED;return 0;}
  unsigned selected=scroll+cursor-1;
  if(selected==a->slot){a->phase=4;return 1;}
  return selected<a->slot?128:64;
}
unsigned pocket_action_tick(struct pocket_action *a, pocket_action_read r,
                            void *c) {
  if (a->result != PA_RUNNING)
    return 0;
  if (++a->frames > (a->item || a->target==1 ? 1800u : 360u)) {
    a->result = PA_TIMEOUT;
    return 0;
  }
  /* Two frames pressed, then release while the game's normal menu opens. */
  if (a->phase == 0) {
    if (a->frames <= 2)
      return 8;
    a->phase = 1;
  }
  if(a->hold){a->hold--;return a->heldKey;}
  if (a->pulse) {
    --a->pulse;
    return 0;
  }
  uint8_t ptr[2], state, script, battle;
  if (!r(c, PD_PC_POINTER, ptr, 2) || !read8(r, c, PD_PC_STATE, &state) ||
      !read8(r, c, PD_PC_SCRIPT, &script) || !read8(r, c, PD_PC_BATTLE, &battle)) {
    a->result = PA_REJECTED;
    return 0;
  }
  unsigned menu = ptr[0] | (ptr[1] << 8);
  if (battle) {
    a->result = PA_REJECTED;
    return 0;
  }
  if (a->target==1 && a->phase>=2)return select_fly(a,r,c);
  if (a->item && a->phase>=3) return select_item(a,r,c,menu,state,script);
  if (a->phase == 2) {
    bool pack = false;
    for (unsigned i=0;i<sizeof pd_pc_pack_menus/sizeof *pd_pc_pack_menus;i++)
      pack = pack || menu == pd_pc_pack_menus[i];
    pack = pack || tm_pocket_waiting(r,c);
    if (pack && (state & 0x40) && script == 1) {
      if (++a->stable >= 6) {
        if(a->item){a->phase=3;a->stable=0;}else a->result=PA_DONE;
      }
    } else
      a->stable = 0;
    return 0;
  }
  uint8_t list[11], cursor;
  if (menu != PD_PC_START_MENU || !(state & 0x40) || script != 1) {
    a->stable = 0;
    return 0;
  }
  if (!r(c, PD_PC_LIST, list, sizeof list) || !read8(r, c, PD_PC_CURSOR, &cursor))
    return 0;
  unsigned n = list[0];
  if (n < 1 || n > 9 || cursor < 1 || cursor > n || list[n + 1] != 255) {
    a->stable = 0;
    return 0;
  }
  unsigned seen = 0, row = 0;
  for (unsigned i = 1; i <= n; i++) {
    if (list[i] > 8 || (seen & (1u << list[i]))) {
      a->result = PA_REJECTED;
      return 0;
    }
    seen |= 1u << list[i];
    if (list[i] == a->target)
      row = i;
  }
  if (!row) {
    a->result = PA_REJECTED;
    return 0;
  }
  if (++a->stable < 6)
    return 0;
  a->stable = 0;
  a->pulse = 12;
  if (cursor == row) {
    a->phase = 2;
    return 1;
  } // Normal game A, never change cursor RAM.
  return cursor < row ? 128 : 64;
}
void pocket_action_cancel(struct pocket_action *a) {
  if (a->result == PA_RUNNING)
    a->result = PA_CANCELLED;
}
const char *pocket_action_message(const struct pocket_action *a) {
  switch (a->result) {
  case PA_DONE:
    if(a->target==1)return "Fly handed to game; choose a destination";
    return a->item ? "Selected item actions opened" : "Original game Bag opened";
  case PA_REJECTED:
    if(a->target==1)return "Fly unavailable or party changed; use game controls";
    return "Item shortcut unavailable or inventory changed";
  case PA_TIMEOUT:
    if(a->target==1)return "Fly shortcut timed out; game control restored";
    return "Bag shortcut timed out; game control restored";
  case PA_CANCELLED:
    return "Shortcut cancelled; game control restored";
  default:
    if(a->target==1)return "Opening original Fly menu - B cancels";
    return "Opening original game Bag - B cancels";
  }
}
