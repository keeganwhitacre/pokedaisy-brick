"""Reuse PartyScreen's pixel HP lettering; no game ROM assets."""
from pathlib import Path
import re,hashlib,sys
p=Path(__file__).resolve().parents[1]/'app/src/main/kotlin/com/pokedaisy/app/companion/ui/PartyScreen.kt'
s=p.read_text();block=s.split('internal val HpLetters = listOf(',1)[1].split('\n)',1)[0]
rows=re.findall(r'"([01.]+)"',block)
assert len(rows)==5 and all(len(r)==9 for r in rows)
out='/* Generated from upstream PartyScreen.kt; GPL-3.0. SHA256 '+hashlib.sha256(p.read_bytes()).hexdigest()+' */\n'
out+='static const char *partyHpLetters[]={'+','.join('"'+r+'"' for r in rows)+'};\n'
p=Path(__file__).with_name('party_art.h')
if '--check' in sys.argv:assert p.read_text()==out,'Run gen_party.py'
else:p.write_text(out)
print('Party art verified' if '--check' in sys.argv else 'Party art generated')
