"""Generate the recovered reader's constants from current app-owned Gen 2 data."""
from pathlib import Path
import re
import argparse
root = Path(__file__).resolve().parents[1]
base = root / 'app/src/main/kotlin/com/pokedaisy/app/companion/data'
s = (base / 'Gen2Reader.kt').read_text().split('val GEN2_POLISHED_CRYSTAL = GEN2_CRYSTAL.copy(', 1)[1].split('\n)', 1)[0]
ram = {k: (int(b) << 16) | int(a, 16) for k,b,a in re.findall(r'(\w+) = wramx\((\d+), (0x[0-9A-Fa-f]+)\)', s)}
ram.update({k:int(a,16) for k,a in re.findall(r'(\w+) = (0x[0-9A-Fa-f]+)L', s)})
out = ['/* Generated from Gen2Reader.kt and Gen2SpeciesPolishedCrystal.kt. Do not edit. */']
out += ['enum {' + ','.join('PC_RAM_'+k+'='+hex(v) for k,v in sorted(ram.items())) + '};']
s = (base / 'Gen2SpeciesPolishedCrystal.kt').read_text()
for name, dest in [('polishedVariantForms','variants'), ('gen2GenderRatiosPolishedCrystal','genders'), ('gen2TmItemsPolishedCrystal','tmItems')]:
    a = re.search(r'val '+name+r' = intArrayOf\((.*?)\)', s, re.S)
    if not a: raise ValueError('Upstream array changed: '+name)
    out.append('static const int '+dest+'[]={'+a[1]+'};')
text = '\n'.join(out)+'\n'
p = Path(__file__).with_name('polished_reader_data.h')
a = argparse.ArgumentParser(); a.add_argument('--check', action='store_true'); args=a.parse_args()
if args.check:
    if not p.exists() or p.read_text()!=text: raise SystemExit('Regenerate with python3 linux/gen_polished_reader.py')
else: p.write_text(text)
