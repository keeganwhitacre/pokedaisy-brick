#!/usr/bin/env python3
"""Rewrites a retail Emerald save in Pokémon Recharged Emerald's save layout - how the
`recharged_emerald*` fixtures boot the user's retail Emerald save (there is no Recharged
Emerald save; the retail one loads as is, but with its flags, Key Items and the rest read
from the wrong places: no POKéMON on the START menu, and Rebalanced hangs on CONTINUE).

    scripts/convert_save_recharged_emerald.py <emerald.sav> <out.sav> [--rebalanced]

Jaizu's rebuild keeps retail's 4 KB sectors (0xF80 data bytes, footer at 0xFF4) and its
SaveBlock1 up to the bag, then (offsets from the GF ROM header at 0x08000100 and
SetBagItemsPointers, 0x0811B714 in v2.2.8; gBagPockets runs Items, Medicine, Poké Balls, TMs,
Berries, Key Items - each item's pocket field in its 48-byte gItems record, minus 1):

    0x560  Items          60 slots (retail 30)
    0x650  Key Items      50 slots (retail 30)
    0x718  Poké Balls     32 slots (retail 16)
    0x798  TMs & HMs      64
    0x898  Berries        46
    0x950  retail 0x848.. (pokeblocks; retail's seen1 copy at 0x988 is gone)
    0x1344 flags, 0x1470 vars (retail 0x1270 / 0x139C: +0xD4 from here on)
    0x366C Medicine       50 slots, a new pocket in retail's unused_3598[0x180]: +0x1C after it
           retail's seen2 copy (0x3B24) is gone too: -0x18 to the end, then 0x30 new bytes
    size 0x3DA0 (retail 0x3D88)

SaveBlock2 is retail's with 0x1C more bytes at its end (0xF48). Rebalanced v2.2.8 puts 4 more
bytes in front of both blocks (its header's offsets are all 4 higher: SaveBlock1 0x3DA4,
SaveBlock2 0xF4C). Both save slots are written from the newer one. The input is never written to.
"""
import struct
import sys

SECTOR, DATA, FOOTER, SIGNATURE = 0x1000, 0xF80, 0xFF4, 0x08012025
NEW_SB1, NEW_SB2 = 0x3DA0, 0xF48
# The items Recharged Emerald's gItems puts in its Medicine pocket (pocket field 2; the ids are
# retail's): they move out of retail's Items pocket.
MEDICINE = set(range(13, 42)) | {44, 45} | set(range(62, 72))


def slots(b, off, n):
    return [b[off + 4 * i:off + 4 * i + 4] for i in range(n)]


def pocket(items, n):
    items = [s for s in items if s[:2] != b"\0\0"]
    if len(items) > n:
        sys.exit(f"a pocket holds {len(items)} items, its new size is {n}")
    return b"".join(items) + bytes(4 * (n - len(items)))


def convert_sb1(o):
    items = slots(o, 0x560, 30)
    med = [s for s in items if struct.unpack_from("<H", s)[0] in MEDICINE]
    rest = [s for s in items if s not in med]
    n = (o[0:0x560] + pocket(rest, 60) + pocket(slots(o, 0x5D8, 30), 50) + pocket(slots(o, 0x650, 16), 32)
         + o[0x690:0x848]            # TMs & HMs, Berries
         + o[0x848:0x988]            # pokeblocks
         + o[0x9BC:0x3598]           # (seen1 dropped) .. mystery gift
         + pocket(med, 50)
         + o[0x3718:0x3B24]          # (unused_3598 dropped) trainer hill times .. record mixing gift
         + o[0x3B58:0x3D88]          # (seen2 dropped) .. Walda's phrase
         + bytes(0x30))
    assert len(n) == NEW_SB1 and n[0x1344:0x1344 + 0x12C] == o[0x1270:0x1270 + 0x12C], hex(len(n))
    return n


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) != 2:
        sys.exit(__doc__)
    rebalanced = "--rebalanced" in sys.argv
    b = bytearray(open(args[0], "rb").read())
    newest = {}
    for slot in range(2):
        for s in range(14):
            o = (slot * 14 + s) * SECTOR
            sid, _, sig, counter = struct.unpack_from("<HHII", b, o + FOOTER)
            if sig == SIGNATURE and sid <= 4:
                newest.setdefault(slot, {})[sid] = o
                newest[slot]["counter"] = counter
    best = max(newest, key=lambda s: newest[s]["counter"])
    sec = newest[best]
    sb2 = bytes(b[sec[0]:sec[0] + 0xF2C])
    sb1 = b"".join(bytes(b[sec[i]:sec[i] + DATA]) for i in range(1, 5))[:0x3D88]
    lead = bytes(4) if rebalanced else b""
    sb1 = lead + convert_sb1(sb1)
    sb2 = lead + sb2 + bytes(NEW_SB2 - 0xF2C)
    chunks = {0: sb2, **{i: sb1[(i - 1) * DATA:i * DATA] for i in range(1, 5)}}
    for slot, ids in newest.items():
        counter = sec["counter"] - (slot != best)
        for sid in range(5):
            o, data = ids[sid], chunks[sid]
            b[o:o + DATA] = data + bytes(DATA - len(data))
            total = sum(struct.unpack_from(f"<{len(data) // 4}I", data)) & 0xFFFFFFFF
            struct.pack_into("<HHII", b, o + FOOTER, sid, ((total >> 16) + total) & 0xFFFF, SIGNATURE, counter)
    open(args[1], "wb").write(b)
    print(f"wrote {args[1]}: SaveBlock1 {len(sb1):#x}, SaveBlock2 {len(sb2):#x}{' (Rebalanced)' if rebalanced else ''}")


if __name__ == "__main__":
    main()
