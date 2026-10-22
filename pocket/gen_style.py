"""Reuse upstream Polished bag/Gen2 OPTION frames and Dex marks, no ROM art."""
from pathlib import Path
import re,json,hashlib,sys
base=Path(__file__).resolve().parents[1]/'app/src/main/kotlin/com/pokedaisy/app/companion/ui'
files={n:(base/n).read_text() for n in ['ItemsScreen.kt','GbaMenu.kt','PokedexScreen.kt','PixelIcons.kt','CompanionScreen.kt','GuideScreen.kt']}
s=files['ItemsScreen.kt']
yellow=s.split('private val YellowBag = BagPalette(',1)[1].split('\n)',1)[0]
def layers(key):
    # Color(...) contains a closing paren, so delimit by the next property instead.
    block=yellow.split(key+' = listOf(',1)[1].split('),\n',1)[0]
    pairs=re.findall(r'(Color\(0xFF[0-9A-Fa-f]{6}\)|Color.White) to (\d+)',block)
    assert pairs
    return [(0xffffff if c=='Color.White' else int(c[10:-1],16),int(w)*2) for c,w in pairs]
panel=layers('panelLayers');edge=layers('descLayers')
assert panel==[(0x181818,2),(0xffffff,2),(0x181818,2),(0xffffff,2)]
assert edge==panel[:3]
polished=s.split('private val PolishedCrystalBag = YellowBag.copy(',1)[1].split('\n)',1)[0]
backdrop=re.search(r'backdrop = Color\(0xFF([0-9A-Fa-f]{6})\) to Color\(0xFF([0-9A-Fa-f]{6})\)',polished).groups()
underline=re.search(r'tabUnderline = Color\(0xFF([0-9A-Fa-f]{6})\)',polished).group(1)
frame=files['GbaMenu.kt'].split('private val G1_FRAME get() = listOf(',1)[1].split(')',1)[0]
assert re.sub(r'\s+','',frame)=='G1_PAPERto2,G1_INKto1,G1_PAPERto1,G1_INKto1,G1_PAPERto1'
assert 'listOf(G1_INK to 1, G1_PAPER to 1, G1_INK to 1) else titleLayers' in files['GbaMenu.kt']
out='/* Generated from Android UI sources by gen_style.py; GPL-3.0. */\n'
def array(name,pairs):
    return 'static const struct pd_layer '+name+'[]={'+','.join('{0x%06x,%d}'%p for p in pairs)+'};\n'
out+=array('pcBagPanel',panel)+array('pcBagEdge',edge)
out+=array('pcOptionFrame',[(0xffffff,4),(0x181818,2),(0xffffff,2),(0x181818,2),(0xffffff,2)])
out+='static const uint32_t pcBagBackdrop[]={0x'+backdrop[0]+',0x'+backdrop[1]+'};\n'
out+='static const uint32_t pcBagUnderline=0x'+underline+';\n'
block=files['PixelIcons.kt'].split('val pokeBall = listOf(',1)[1].split('\n    )',1)[0]
rows=re.findall(r'"([.KRWH]+)"',block)
assert len(rows)==12 and all(len(r)==12 for r in rows)
out+='static const char *dexBall[]={'+','.join(json.dumps(r) for r in rows)+'};\n'
for var in ['caughtBall','seenBall']:
    block=files['PokedexScreen.kt'].split('val '+var+' = mapOf(',1)[1].split('\n)',1)[0]
    pairs=re.findall(r"'([KRWH])' to (Color\(0xFF([0-9A-Fa-f]{6})\)|Color.White)",block)
    assert len(pairs)==4
    out+='static const uint32_t dex'+var[0].upper()+var[1:]+'[128]={'+','.join("['"+c+"']=0x"+(v or 'ffffff') for c,_,v in pairs)+'};\n'
# GuideScreen.herePage's exact method order/titles; fail closed if the source
# model changes rather than quietly keeping a second hand-maintained list.
g=files['GuideScreen.kt']
block=g.split('val methods = if (!hasWild)',1)[1].split('.filter { (_, l)',1)[0]
method_ids={'grass':0,'water':1,'oldRod':2,'goodRod':3,'superRod':4,'headbutt':5,'headbuttRare':6,'rockSmash':7}
method_rows=re.findall(r'tk\("([^"\n]+)"\) to it\.(\w+)',block)
assert len(method_rows)==8 and set(k for _,k in method_rows)==set(method_ids)
labels=['']*8
for label,key in method_rows:labels[method_ids[key]]=label
out+='static const int pcGuideMethodOrder[]={'+','.join(str(method_ids[k]) for _,k in method_rows)+'};\n'
out+='static const char *pcGuideMethods[]={'+','.join(json.dumps(v) for v in labels)+'};\n'
page_vars=['HERE','NEXT_BOSS','TIPS','WHERE_IS']
page_labels=[re.search(r'private val '+k+r' = tk\("([^"\n]+)"\)',g).group(1) for k in page_vars]
out+='static const char *pcGuidePages[]={'+','.join(json.dumps(v) for v in page_labels)+'};\n'
assert 'weight(0.34f)' in g and 'weight(0.66f)' in g
assert 'if (open == key) level else 0' in g and 'e.steps == 0 || showAnswer' in g
out+='/* Input SHA256: '+json.dumps({k:hashlib.sha256(v.encode()).hexdigest() for k,v in files.items()},sort_keys=True)+' */\n'
p=Path(__file__).with_name('polished_style.h')
if '--check' in sys.argv:assert p.read_text()==out,'Run gen_style.py'
else:p.write_text(out)
print('Polished UI style verified' if '--check' in sys.argv else 'Polished UI style generated')
