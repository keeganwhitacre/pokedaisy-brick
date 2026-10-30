/* GPL-3.0. Profile lifecycle only; no ROM or hardware required. */
#include "../app/src/main/cpp/pk_wide_gb.c"
#include <assert.h>
#include <stdio.h>

static int decode(const unsigned char *rom, size_t size, size_t at,
                  unsigned char *out, size_t cap) {
    (void)rom; (void)size; (void)at; (void)out; (void)cap;
    return -1;
}
int main(void) {
    static const uint32_t profile[GB_CONFIG] = {0};
    assert(!pk_wide_gb_configure(0));
    assert(!gbPolished && !gbCfg && !gbDecode);
    assert(!pk_wide_gb_configure_polished(profile, NULL));
    assert(!gbCfg);
    assert(!pk_wide_gb_configure_polished(NULL, decode));
    assert(!gbCfg);
    assert(pk_wide_gb_configure_polished(profile, decode));
    assert(gbPolished && gbGen == 2 && gbCfg == profile && gbDecode == decode);
    gbMix = 256;
    gbGood.ok = true;
    pcWideSprites[1].state = 1;
    assert(pk_wide_gb_configure(GB_ROMS[0].crc));
    assert(!gbPolished && !gbDecode && gbCfg == GB_ROMS[0].a);
    assert(gbMix == 0 && !gbGood.ok && !pcWideSprites[1].state);
    assert(pk_wide_gb_knows(GB_ROMS[0].crc));
    assert(pk_wide_gb_configure_polished(profile, decode));
    assert(!pk_wide_gb_configure(0));
    assert(!gbPolished && !gbCfg && !gbDecode);
    puts("PASS shared GB profile configure, reset, retail fallback and null rejection");
    return 0;
}
