/* GPL-3.0. Captured upstream RAM fixtures; no ROM required. */
#include "polished_reader.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char ram[32768];
static bool memory(void *ctx, uint32_t a, void *out, size_t n) {
    (void)ctx;
    unsigned bank=a>>16, addr=a&65535;
    size_t at;
    if (!bank && addr>=0xc000 && addr<0xd000) at=addr-0xc000;
    else if (bank<8 && addr>=0xd000 && addr<0xe000) at=(bank?bank:1)*4096+addr-0xd000;
    else return false;
    if (n>sizeof ram-at || n>0x1000-(addr&4095)) return false;
    memcpy(out,ram+at,n); return true;
}
static void load(const char *fixture, const char *file, size_t at, size_t n) {
    char path[1024];
    snprintf(path,sizeof path,"%s/%s/%s",FIXTURES,fixture,file);
    FILE *f=fopen(path,"rb"); assert(f); assert(fread(ram+at,1,n,f)==n); fclose(f);
}
int main(void) {
    const char *fixtures[]={"polished_crystal","polished_crystal_3_2_0","polished_crystal_3_2_1","polished_crystal_3_2_2","polished_crystal_faithful"};
    for(unsigned i=0;i<sizeof fixtures/sizeof *fixtures;i++) {
        memset(ram,0,sizeof ram);
        load(fixtures[i],"wramx.bin",0,32768); load(fixtures[i],"wram.bin",0,4096);
        struct pd_snapshot s;
        assert(pc_snapshot_read(&s,memory,NULL) && s.valid);
        assert(s.partyCount==2);
        assert(s.party[0].species==155 && s.party[0].level==5);
        assert(s.party[0].hp==13 && s.party[0].maxHp==19);
        assert(s.party[1].species==16 && s.party[1].level==3);
        assert(s.money==3000 && s.bagOk);
        puts(fixtures[i]);
    }
    puts("PASS recovered Polished party and bag against current upstream fixtures");
}
