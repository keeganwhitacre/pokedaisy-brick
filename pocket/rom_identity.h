/* Existing Pocket SHA-1 codec, shared by ROM detection and host preferences. */
#ifndef POCKET_ROM_IDENTITY_H
#define POCKET_ROM_IDENTITY_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
static uint32_t pocket_sha1_rol(uint32_t x, int n) {
  return (x << n) | (x >> (32 - n));
}
static void pocket_rom_sha1(const unsigned char *p, size_t n, char hex[41]) {
  uint32_t h[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
  size_t blocks = (n + 9 + 63) / 64;
  for (size_t b = 0; b < blocks; b++) {
    uint32_t w[80] = {0};
    for (int j = 0; j < 64; j++) {
      size_t k = b * 64 + j;
      unsigned v = k < n ? p[k] : k == n ? 128 : 0;
      if (k >= blocks * 64 - 8)
        v = (unsigned)(((uint64_t)n * 8) >> (8 * (blocks * 64 - 1 - k))) & 255;
      w[j / 4] |= v << (24 - 8 * (j % 4));
    }
    for (int j = 16; j < 80; j++)
      w[j] = pocket_sha1_rol(w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16], 1);
    uint32_t a = h[0], c = h[2], d = h[3], e = h[4], bb = h[1];
    for (int j = 0; j < 80; j++) {
      uint32_t f, k;
      if (j < 20) {
        f = (bb & c) | (~bb & d);
        k = 0x5a827999;
      } else if (j < 40) {
        f = bb ^ c ^ d;
        k = 0x6ed9eba1;
      } else if (j < 60) {
        f = (bb & c) | (bb & d) | (c & d);
        k = 0x8f1bbcdc;
      } else {
        f = bb ^ c ^ d;
        k = 0xca62c1d6;
      }
      uint32_t t = pocket_sha1_rol(a, 5) + f + e + k + w[j];
      e = d;
      d = c;
      c = pocket_sha1_rol(bb, 30);
      bb = a;
      a = t;
    }
    h[0] += a;
    h[1] += bb;
    h[2] += c;
    h[3] += d;
    h[4] += e;
  }
  for (int j = 0; j < 5; j++)
    snprintf(hex + j * 8, 9, "%08x", h[j]);
}

#endif
