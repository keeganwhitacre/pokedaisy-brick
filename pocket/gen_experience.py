"""Reuse upstream's six experience tables; no curves inferred or ROM data read."""
from pathlib import Path
import re,hashlib,sys
source=Path(__file__).resolve().parents[1]/'app/src/main/kotlin/com/pokedaisy/app/companion/data/ExpTables.kt'
policy=source.with_name('SnapshotView.kt')
assert 'intArrayOf(0, 3, 4, 5).getOrNull(it)' in policy.read_text(),'Review upstream Polished growth mapping'
raw=source.read_text();body=raw.split('val gen3ExperienceTables:',1)[1]
tables=[re.findall(r'\b\d+\b',s) for s in re.findall(r'intArrayOf\((.*?)\)',body,re.S)]
assert len(tables)==6 and all(len(t)==101 for t in tables),'Review upstream experience table shape'
out='/* Generated from upstream ExpTables.kt; GPL-3.0. SHA256 '+hashlib.sha256(source.read_bytes()).hexdigest()+' */\n'
out+='/* Progress rules: SnapshotView.kt SHA256 '+hashlib.sha256(policy.read_bytes()).hexdigest()+' */\n'
out+='static const uint32_t experienceTables[6][101]={'+','.join('{'+','.join(t)+'}' for t in tables)+'};\n'
p=Path(__file__).with_name('experience_tables.h')
if '--check' in sys.argv:assert p.read_text()==out,'Run gen_experience.py'
else:p.write_text(out)
print('Experience tables verified' if '--check' in sys.argv else 'Experience tables generated')
