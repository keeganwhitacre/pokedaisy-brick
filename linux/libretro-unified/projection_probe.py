"""Private DSperate v3.3 libretro projection experiment; never edits shipped cores.

Scales ALL submitted 3D before clipping. Not a safe persistent game option.
PD_DS_PROBE_PERCENT=100/90/80 selects the fixed process-wide scale.
"""
from pathlib import Path
import argparse,json,shlex,shutil,subprocess,hashlib
p=argparse.ArgumentParser();p.add_argument('build',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
build=a.build.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
root=Path(__file__).resolve().parents[2]
source=root/'third_party/dsperate-lr/DSperate/src/core/gpu/gpu3d.cpp'
original=source.read_bytes();text=original.decode()
marker='  for (int c = 0; c < 3; ++c) vt.col[c] = (vertex_color_[c] << 12) + 0xFFF;'
assert text.count(marker)==1,'Review the pinned DSperate vertex path'
text=text.replace(marker,'''  // PRIVATE PRESENTATION RESEARCH: one native geometry/raster pass.
  static const int probe = [](){
    const char* v=std::getenv("PD_DS_PROBE_PERCENT");
    int n=v?std::atoi(v):100;return n>=50 && n<=100?n:100;
  }();
  vt.pos[0]=static_cast<s32>(static_cast<s64>(vt.pos[0])*probe/100);
  vt.pos[1]=static_cast<s32>(static_cast<s64>(vt.pos[1])*probe/100);
  vt.oc=outcode(vt.pos);
'''+marker)
probe=out/'gpu3d.cpp';probe.write_text(text)
entry=next(e for e in json.loads((build/'compile_commands.json').read_text()) if Path(e['file']).resolve()==source.resolve())
cmd=shlex.split(entry['command']);obj=out/'gpu3d.cpp.o'
cmd[cmd.index('-o')+1]=str(obj);cmd[cmd.index('-c')+1]=str(probe)
clean=[];i=0
while i<len(cmd):
 if cmd[i] in ('-MF','-MT','-MQ'):i+=2;continue
 if cmd[i] in ('-MD','-MMD'):i+=1;continue
 clean.append(cmd[i]);i+=1
subprocess.run(clean,cwd=entry['directory'],check=True)
original_archive=build/'dsperate/src/core/libdsperate_core.a'
before=hashlib.sha256(original_archive.read_bytes()).hexdigest()
archive=out/'libdsperate_probe.a';shutil.copyfile(original_archive,archive)
compiler=Path(cmd[0]);ar=str(compiler).replace('g++','ar') if 'g++' in compiler.name and 'aarch64' in compiler.name else 'ar'
subprocess.run([ar,'r',str(archive),str(obj)],check=True)
link=shlex.split(subprocess.check_output(['ninja','-C',str(build),'-t','commands','pokedaisy_ds_libretro.so'],text=True).splitlines()[-1])
if link[:2]==[':','&&']:link=link[2:]
if link[-2:]==['&&',':']:link=link[:-2]
assert not any(x in link for x in ('&&',';','|'))
link[link.index('-o')+1]=str(out/'pokedaisy_ds_probe.so')
link=[str(archive) if x=='dsperate/src/core/libdsperate_core.a' else x for x in link if not x.startswith('-Wl,--dependency-file=')]
subprocess.run(link,cwd=build,check=True)
assert source.read_bytes()==original
assert hashlib.sha256(original_archive.read_bytes()).hexdigest()==before
print(out/'pokedaisy_ds_probe.so')
