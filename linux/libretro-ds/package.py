"""Package a separate NXRedux emulator; no firmware or existing pak is replaced."""
from pathlib import Path
import argparse
import shutil
import subprocess
import tempfile
import zipfile
p=argparse.ArgumentParser()
p.add_argument('core',type=Path)
p.add_argument('output',type=Path)
p.add_argument('--strip',default='aarch64-nextui-linux-gnu-strip')
p.add_argument('--source-ref',default='brick/ds-libretro')
a=p.parse_args()
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as temp:
    stage=Path(temp)
    pak=stage/'Emus/tg5040/PDDS.pak'
    pak.mkdir(parents=True)
    subprocess.run([a.strip,'--strip-unneeded','-o',str(pak/'pokedaisy_ds_libretro.so'),str(a.core.resolve())],check=True)
    for name in ['launch.sh','default.cfg']:
        shutil.copyfile(Path(__file__).parent/name,pak/name)
    (pak/'launch.sh').chmod(0o755)
    shutil.copyfile(Path(__file__).parent/'README.md',stage/'README.md')
    shutil.copyfile(Path(__file__).parent/'COMPANION-UPDATE.md',stage/'COMPANION-UPDATE.md')
    licenses=pak/'licenses';licenses.mkdir()
    shutil.copyfile(root/'LICENSE',licenses/'PokeDaisy-GPL-3.0.txt')
    ds=root/'third_party/dsperate-lr/DSperate'
    for source in sorted(ds.rglob('*')):
        # DSperate's own notices accompany its embedded fonts / FreeBIOS / miniz.
        if source.is_file() and source.name.lower().startswith(('license','copying','notice')):
            rel=source.relative_to(ds)
            dest=licenses/'DSperate'/rel
            dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(source,dest)
    shutil.copyfile(root/'NOTICE',licenses/'PokeDaisy-NOTICE.txt')
    (pak/'SOURCE.txt').write_text('PokeDaisy source: https://github.com/keeganwhitacre/pokedaisy-brick branch brick/ds-libretro\n'
        f'Exact source ref: {a.source_ref}\n'
        'Build: linux/libretro-ds/README.md. Initialize third_party/dsperate-lr recursively.\n'
        'No ROMs, BIOS or saves are included.\n')
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(a.output,'w',zipfile.ZIP_DEFLATED) as z:
        for f in sorted(stage.rglob('*')):
            if f.is_file():z.write(f,f.relative_to(stage))
print(a.output)
