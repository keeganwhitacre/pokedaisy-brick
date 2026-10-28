#!/usr/bin/env python3
"""Maps a pret decomp's addresses into another build of the same game, from the ROMs the user owns.

    scripts/port_gen1.py [--loose] [--window N] [--data SYMBOL:LEN ...] <decomp.sym> <base.gb(c)> <other.gb(c)> [SYMBOL ...]

The decomp builds the base cart byte for byte (pokeyellow: English Yellow, sha1 cc7d0326...; pokecrystal11:
Crystal rev 1, sha1 f2f52230...), so its .sym names every WRAM global and ROM table. Another build of the
same code (Yellow's European carts: the same code with their own text; Crystal Clear: pokecrystal grown and
rebuilt, every bank moved) keeps the code that uses a symbol, so the symbol is found there:

  - WRAM (C000-DFFF) / HRAM: every `ld a,[nn]` / `ld [nn],a` / `ld hl,nn` / `ld de,nn` / `ld bc,nn` /
    `ld sp,nn` / `ld [nn],sp` with nn = the symbol, its surrounding code (other 16-bit operands masked,
    as they may have moved too) looked up in the other cart; the operand at the same place there votes.
    A D000-DFFF symbol is in WRAM bank 1 for Crystal, as in the base: the bank isn't mapped.
  - ROM (a bank:address in 4000-7FFF or bank 0): the same, for `ld hl/de/bc,nn`, `call` and `jp`; the bank
    comes from a `ld a,n` in the same window that loaded the base's bank.
  - `--loose`: the immediate of every `ld a,n` and the offset of every `jr` are masked too (a rebuild moves
    banks and stretches code between jumps), at the cost of windows that match more than once (those don't
    vote).
  - `--window N`: N bytes of code each side of the operand (10 by default); fewer match more often in a
    rebuild whose code changed around the use, more are needed where the same code repeats.
  - `--data SYMBOL:LEN`: data no code points at directly (a table read through another) is found by the
    base's LEN bytes there, when the other build didn't change them (every place they occur is listed).

Prints `SYMBOL base -> other (votes)`; a symbol with no votes is reported, never guessed.
"""
import re
import sys

A16_OPS = {0xFA, 0xEA, 0x21, 0x11, 0x01, 0x31, 0x08, 0xCD, 0xC3, 0xC2, 0xCA, 0xD2, 0xDA, 0xC4, 0xCC, 0xD4, 0xDC}
LOAD_OPS = {0xFA, 0xEA, 0x21, 0x11, 0x01, 0x31, 0x08}
PTR_OPS = {0x21, 0x11, 0x01, 0xCD, 0xC3, 0xC2, 0xCA, 0xD2, 0xDA, 0xC4, 0xCC, 0xD4, 0xDC}
D8_OPS = {0x3E}                                 # ld a,n: a bank (or a constant) a rebuild may change
JR_OPS = {0x18, 0x20, 0x28, 0x30, 0x38}
HRAM_OPS = {0xE0, 0xF0}                         # ldh [n],a / ldh a,[n]
BEFORE, AFTER = 10, 10


def read_sym(path):
    out = {}
    for line in open(path):
        m = re.match(r"([0-9a-f]{2}):([0-9a-f]{4}) (\S+)", line)
        if m:
            out[m.group(3)] = (int(m.group(1), 16), int(m.group(2), 16))
    return out


def file_off(bank, addr):
    return addr if addr < 0x4000 else bank * 0x4000 + (addr - 0x4000)


def masked(d, lo, hi, keep, loose=False, width=2):
    """A regex for d[lo:hi]: the bytes as they are, but a 16-bit operand after an a16 opcode is any two bytes
    (the operand at [keep], [width] bytes, is the one being mapped: it's the capture group)."""
    out, i = b"", lo
    while i < hi:
        if i == keep:
            out += b"(" + b"." * width + b")"
            i += width
            continue
        b = d[i]
        out += re.escape(bytes([b]))
        if b in A16_OPS and i + 2 < hi and keep not in (i + 1, i + 2):
            out += b".."
            i += 3
            continue
        if loose and (b in D8_OPS or b in JR_OPS) and i + 1 < hi and keep != i + 1:
            out += b"."
            i += 2
            continue
        i += 1
    return re.compile(out, re.S)


def sites(d, value, ops):
    lo, hi = value & 0xFF, value >> 8
    for i in range(1, len(d) - 2):
        if d[i] == lo and d[i + 1] == hi and d[i - 1] in ops:
            yield i


def hram_sites(d, value):
    for i in range(1, len(d) - 1):
        if d[i] == (value & 0xFF) and d[i - 1] in HRAM_OPS:
            yield i


def map_value(en, other, value, ops, bank=None, loose=False):
    """Votes for what [value] (an operand) is in [other], and, with [bank], the ROM bank loaded beside it."""
    votes = {}
    hram = value >= 0xFF80
    for i in (hram_sites(en, value) if hram else sites(en, value, ops)):
        width = 1 if hram else 2
        lo, hi = max(0, i - 1 - BEFORE), min(len(en), i + width + AFTER)
        rx = masked(en, lo, hi, i, loose, width)
        found = [m for m in rx.finditer(other)]
        if len(found) != 1:
            continue
        m = found[0]
        new = 0xFF00 | m.group(1)[0] if hram else m.group(1)[0] | m.group(1)[1] << 8
        b = None
        if bank is not None:
            # `ld a,BANK` in the window: the same byte place in the other cart.
            for j in range(lo, hi - 1):
                if en[j] == 0x3E and en[j + 1] == bank:
                    b = other[m.start() + (j + 1 - lo)]
                    break
        key = (b, new)
        votes[key] = votes.get(key, 0) + 1
    return votes


def find_data(en, other, bank, addr, length):
    o = file_off(bank, addr)
    pat = en[o:o + length]
    out, i = [], other.find(pat)
    while i >= 0:
        out.append(f"{i // 0x4000:02x}:{(i % 0x4000) + (0x4000 if i >= 0x4000 else 0):04x}")
        i = other.find(pat, i + 1)
    return out


def main():
    args = sys.argv[1:]
    global BEFORE, AFTER
    loose = "--loose" in args
    args = [a for a in args if a != "--loose"]
    if "--window" in args:
        k = args.index("--window")
        BEFORE = AFTER = int(args[k + 1])
        del args[k:k + 2]
    data = []
    while "--data" in args:
        k = args.index("--data")
        data.append(args[k + 1])
        del args[k:k + 2]
    sym_path, en_path, other_path, *names = args
    sym = read_sym(sym_path)
    en, other = open(en_path, "rb").read(), open(other_path, "rb").read()
    for name in names:
        bank, addr = sym[name]
        if 0xC000 <= addr < 0xE000 or addr >= 0xFF80:
            v = map_value(en, other, addr, LOAD_OPS, loose=loose)
        else:
            v = map_value(en, other, addr, PTR_OPS, bank=bank if addr >= 0x4000 else None, loose=loose)
        best = sorted(v.items(), key=lambda kv: -kv[1])
        shown = ", ".join(f"{'' if b is None else f'{b:02x}:'}{a:04x} x{n}" for (b, a), n in best[:3]) or "NO VOTES"
        print(f"{name:32s} {bank:02x}:{addr:04x} -> {shown}")
    for spec in data:
        name, length = spec.rsplit(":", 1)
        bank, addr = sym[name]
        found = find_data(en, other, bank, addr, int(length, 0))
        print(f"{name:32s} {bank:02x}:{addr:04x} -> data {', '.join(found) or 'NOT FOUND'}")


if __name__ == "__main__":
    main()
