#!/usr/bin/env python3
"""Build a PRIVATE single-pass projection probe from an existing Make build.

Never changes the submodule, normal archive, or packaged executable. Outputs a
GPL derivative in a caller-supplied scratch directory. Not a supported option:
it deliberately scales ALL submitted 3D, including any 3D interface geometry.
Use copies of personal data. PD_DS_PROBE_PERCENT=100/90/80 controls the experiment.
"""
from pathlib import Path
import argparse, shlex, shutil, subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('build',type=Path)
p.add_argument('output',type=Path)
a=p.parse_args();build=a.build.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
root=Path(__file__).resolve().parents[3]
source=root/'third_party/dsperate/src/core/gpu/gpu3d.cpp'
s=source.read_text()
marker='  for (int c = 0; c < 3; ++c) vt.col[c] = (vertex_color_[c] << 12) + 0xFFF;'
if s.count(marker)!=1:raise SystemExit('Review DSperate vertex path before probing')
s=s.replace(marker,'''  // PRIVATE RESEARCH: same native target and single geometry/raster pass.
  static const int probe = [](){
    const char* v = std::getenv("PD_DS_PROBE_PERCENT");
    int n = v ? std::atoi(v) : 100;
    return n >= 50 && n <= 100 ? n : 100;
  }();
  vt.pos[0] = static_cast<s32>(static_cast<s64>(vt.pos[0]) * probe / 100);
  vt.pos[1] = static_cast<s32>(static_cast<s64>(vt.pos[1]) * probe / 100);
  vt.oc = outcode(vt.pos); // recompute after narrowing, including the NEON path
'''+marker)
s='#include <cstdlib>\n'+s
(out/'gpu3d.cpp').write_text(s)
flags={}
for line in (build/'dsperate/src/core/CMakeFiles/dsperate_core.dir/flags.make').read_text().splitlines():
 if ' = ' in line:
  k,v=line.split(' = ',1);flags[k]=shlex.split(v)
cmd=shlex.split((build/'CMakeFiles/pokedaisy-linux.dir/link.txt').read_text())
compiler=cmd[0]
extra=[x for x in cmd if x.startswith('--sysroot=')]
obj=out/'gpu3d.cpp.o' 
subprocess.run([compiler,*extra,*flags['CXX_DEFINES'],*flags['CXX_INCLUDES'],*flags['CXX_FLAGS'],'-c',str(out/'gpu3d.cpp'),'-o',str(obj)],check=True)
archive=out/'libdsperate_probe.a';shutil.copy2(build/'dsperate/src/core/libdsperate_core.a',archive)
subprocess.run(['ar','r',str(archive),str(obj)],check=True)
cmd[cmd.index('-o')+1]=str(out/'pokedaisy-projection-probe')
cmd=[str(archive) if x=='dsperate/src/core/libdsperate_core.a' else x for x in cmd]
cmd=[x for x in cmd if not x.startswith('-Wl,--dependency-file=')]
subprocess.run(cmd,cwd=build,check=True)
print(out/'pokedaisy-projection-probe')
