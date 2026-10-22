"""Adapt upstream Android tables, preserving their IDs; no ROM assets bundled."""
from gen_polished_guide import generate, INPUTS as GUIDE_INPUTS
from pathlib import Path
import re, json, hashlib, argparse, ast
base=Path(__file__).resolve().parents[1]/'app/src/main/kotlin/com/pokedaisy/app/companion/data'
def array(file,name):
 s=(base/file).read_text(); return re.search(r'val '+name+r' = intArrayOf\((.*?)\)',s,re.S).group(1)
out=['/* Generated from upstream Android tables by gen_polished.py. */']
out+=['static const int variants[]={'+array('Gen2SpeciesPolishedCrystal.kt','polishedVariantForms')+'};']
out+=['static const int genders[]={'+array('Gen2SpeciesPolishedCrystal.kt','gen2GenderRatiosPolishedCrystal')+'};']
s=(base/'SpeciesNamesPolishedCrystal.kt').read_text()
pairs=re.findall(r'(0x[0-9A-Fa-f]+|\d+) to "([^"]+)"',s)
out+=['static const char *names[512] = {'+','.join('['+k+']='+json.dumps(v) for k,v in pairs)+'};']
s=(base/'MoveDataPolishedCrystal.kt').read_text()
normal,faithful=s.split('val moveDataPolishedCrystalFaithfulChanges',1)
for var,text in [('moves',normal),('faithfulMoves',faithful)]:
 pairs=re.findall(r'(\d+) to MoveInfo\("([^"]+)", (\d+), (\d+)\)',text)
 assert pairs
 out+=['static const struct {const char *name; int type,power;} '+var+'[256] = {'+','.join('['+k+']={'+json.dumps(v)+','+t+','+p+'}' for k,v,t,p in pairs)+'};']
s=(base/'SpeciesTypesPolishedCrystal.kt').read_text()
normal,faithful=s.split('val speciesTypeDataPolishedCrystalFaithfulChanges',1)
for var,text in [('speciesTypes',normal),('faithfulTypes',faithful)]:
 pairs=re.findall(r'(0x[0-9A-Fa-f]+|\d+) to SpeciesTypes\((\d+), (\d+)\)',text)
 out+=['static const int '+var+'[512][3]={'+','.join('['+k+']={1,'+a+','+b+'}' for k,a,b in pairs)+'};']
s=(base/'TypeChartPolishedCrystal.kt').read_text().split('val typeChart',1)[0]
pairs=re.findall(r'(\d+) to "([^"]+)"',s)
out+=['static const char *typeNames[20]={'+','.join('['+k+']='+json.dumps(v) for k,v in pairs)+'};']
s=(base/'ItemNamesPolishedCrystal.kt').read_text()
normal,faithful=s.split('val itemNamesPolishedCrystalFaithfulChanges',1)
for var,text in [('itemNames',normal),('faithfulItems',faithful)]:
 pairs=re.findall(r'(0x[0-9A-Fa-f]+|\d+) to "([^"]+)"',text)
 out+=['static const char *'+var+'[0x300]={'+','.join('['+k+']='+json.dumps(v) for k,v in pairs)+'};']
out+=['static const int tmItems[]={'+array('Gen2SpeciesPolishedCrystal.kt','gen2TmItemsPolishedCrystal')+'};']
s=(base/'Gen2PolishedCrystalGen.kt').read_text()
rows=re.findall(r'"([0-9a-f]{40})" to intArrayOf\(([^)]+)\),[^\n]*?// ([^\n]+)',s)
out+=['static const struct {const char *sha; unsigned at[26]; int faithful;} builds[] = {'+','.join('{"'+h+'",{'+a+'},'+('1' if 'faithful' in label else '0')+'}' for h,a,label in rows)+'};']
# Shared definitions are extracted, never maintained as a second address list.
reader=(base/'Gen2Reader.kt').read_text().split('val GEN2_POLISHED_CRYSTAL = GEN2_CRYSTAL.copy(',1)[1].split('\n)',1)[0]
ram={}
for field,bank,addr in re.findall(r'(\w+) = wramx\((\d+), (0x[0-9A-Fa-f]+)\)',reader):
 ram[field]=(int(bank)<<16)|int(addr,16)
for field,addr in re.findall(r'(\w+) = (0x[0-9A-Fa-f]+)L',reader):
 ram[field]=int(addr,16)
required='partyCount partyMons numItems numMedicine numBalls numBerries numKeyItems tmsHms money battleMode battleMon enemyMon dexCaught dexSeen otPartyCount otPartyMons curOTMon'.split()
assert all(k in ram for k in required), 'Upstream RAM config changed; review Gen2Reader.kt'
out+=['enum { '+','.join('PC_RAM_'+k+'='+hex(v) for k,v in sorted(ram.items()))+' };']
out+=['static const int dexOrder[]={'+array('Gen2PolishedCrystalGen.kt','gen2DexOrderPolishedCrystal')+'};']
chars=re.findall(r'"(?:[^"\\]|\\.)*"',s.split('internal val polishedTextChars:',1)[1])
assert len(chars)==256, 'Upstream character map changed'
out+=['static const char *textChars[256]={'+','.join(chars)+'};']
art=(base/'Gen2Art.kt').read_text().split('private val POLISHED_CHARS:',1)[1].split('/**',1)[0]
assert "put(0x80 + i, 'A' + i); put(0xA0 + i, 'a' + i)" in art
font={0x80+i:ord('A')+i for i in range(26)}
font.update({0xA0+i:ord('a')+i for i in range(26)})
assert "put(0xE0 + i, '0' + i)" in art
font.update({0xE0+i:ord('0')+i for i in range(10)})
for literal,start in re.findall(r'("(?:[^"\\]|\\.)*")\.forEachIndexed \{ i, c -> put\((0x[0-9A-Fa-f]+) \+ i, c\)',art):
 for i,ch in enumerate(json.loads(literal)):font[int(start,16)+i]=ord(ch)
for key,ch in re.findall(r"(0x[0-9A-Fa-f]+) to '((?:[^'\\]|\\.)*)'",art):font[int(key,16)]=ord(ast.literal_eval("'"+ch+"'"))
assert font[0xBE]==ord('♂') and font[0xE9]==ord('9')
out+=['static const unsigned fontCodepoints[128]={'+','.join('['+str(k-0x80)+']='+str(v) for k,v in sorted(font.items()))+'};']
mapSource=(base/'MapSecDataPolishedCrystal.kt').read_text()
places=re.findall(r'(\d+) to MapSecInfo\("([^"]+)", (-?\d+), (\d+), (\d+), (\d+), (\d+)\)',mapSource)
assert len(places)>100
out+=['static const struct {const char *name; int region,x,y,w,h;} places[]={'+','.join('['+k+']={'+json.dumps(name)+','+','.join((r,x,y,w,h))+'}' for k,name,r,x,y,w,h in places)+'};']
links=re.findall(r'(\d+) to (\d+)',mapSource.split('val mapLandmarksPolishedCrystal:',1)[1])
out+=['static const struct {unsigned map; int landmark;} mapLinks[]={'+','.join('{'+k+','+v+'}' for k,v in links)+'};']
chart=(base/'TypeChartPolishedCrystal.kt').read_text().split('val typeEffectivenessPolishedCrystal:',1)[1]
out+=['static const struct {int pair,percent;} typeMatchups[]={'+','.join('{'+k+','+v+'}' for k,v in re.findall(r'(\d+) to (\d+)',chart))+'};']
flowSource=(base/'Gen2Dex.kt').read_text().split('private val COMPOUND_ENDS = setOf(',1)[1].split(')',1)[0]
compoundWords=re.findall(r'"([A-Za-z-]+)"',flowSource)
assert len(compoundWords)>10
out+=['static const char *compoundEnds[]={'+','.join(json.dumps(w) for w in compoundWords)+'};']
assert len(rows)>0 and all(len(a.split(','))==26 for _,a,_ in rows), 'Upstream ROM table schema changed'
inputs=sorted({m for m in re.findall(r"'([A-Za-z0-9_]+\.kt)'",Path(__file__).read_text())})
inputs=sorted(set(inputs + GUIDE_INPUTS))
guide=generate(base)
manifest={'schema':1,'inputs':{name:hashlib.sha256((base/name).read_bytes()).hexdigest() for name in inputs},'ram_fields':len(ram),'builds':len(rows),'dex_entries':len(re.findall(r'\b\d+\b',array('Gen2PolishedCrystalGen.kt','gen2DexOrderPolishedCrystal')))}
parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args()
for name,content in [('polished_guide_tables.h',guide),('polished_tables.h','\n'.join(out)+'\n'),('upstream-tables.json',json.dumps(manifest,indent=2)+'\n')]:
 path=Path(__file__).with_name(name)
 if args.check:
  assert path.exists() and path.read_text()==content, name+' is stale; run python3 pocket/gen_polished.py'
 else: path.write_text(content)
print('Upstream tables verified' if args.check else 'Upstream tables generated')
