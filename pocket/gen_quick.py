"""Adapt original Android quick-menu pixel art; no ROM assets."""
from pathlib import Path
import re,json,hashlib,sys
base=Path(__file__).resolve().parents[1]/'app/src/main/kotlin/com/pokedaisy/app/companion/ui'
files={n:(base/n).read_text() for n in ['QuickMenu.kt','FieldHud.kt']}
rows=[]
for name,file in [('SAVE_STATE_ICON','QuickMenu.kt'),('LOAD_STATE_ICON','QuickMenu.kt'),('FF_ICON','QuickMenu.kt'),('COMPANION_ICON','FieldHud.kt')]:
 block=re.search(r'val '+name+r' = listOf\((.*?)\n\)',files[file],re.S).group(1)
 art=re.findall(r'"([^"]*)"',block)
 assert art and len(set(map(len,art)))==1 and len(art)<=16 and len(art[0])<=16
 assert set("".join(art)) <= set(".AKDL1"), "Upstream icon palette changed"
 rows.append('{'+str(len(art[0]))+','+str(len(art))+',{'+','.join(json.dumps(s) for s in art)+'}}')
colors=re.findall(r"'([KDLA])' to Color\(0xFF([0-9A-Fa-f]{6})\)",files['QuickMenu.kt'].split('private val STATE_COLORS = mapOf(',1)[1].split('\n)',1)[0]);assert len(colors)==4
out='/* Generated from upstream QuickMenu.kt and FieldHud.kt. GPL-3.0. */\n'
out+='static const struct {int w,h;const char *rows[16];} quickArt[] = {'+','.join(rows)+'};\n'
out+='static const unsigned quickColors[128] = {'+','.join("['"+c+"']=0x"+v for c,v in colors)+",['1']=0x202838};\n"
out+='/* Input SHA-256: '+json.dumps({k:hashlib.sha256(v.encode()).hexdigest() for k,v in files.items()},sort_keys=True)+' */\n'
p=Path(__file__).with_name('quick_icons.h')
if '--check' in sys.argv:assert p.read_text()==out,'quick_icons.h stale; run gen_quick.py'
else:p.write_text(out)
print('Quick-menu assets verified' if '--check' in sys.argv else 'Quick-menu assets generated')
