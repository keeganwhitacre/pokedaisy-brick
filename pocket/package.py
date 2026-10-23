"""Package the actual ARM executable as a NextUI Tools pak; no ROMs or fixtures."""
from pathlib import Path
import shutil,sys
repo=Path(__file__).resolve().parents[1]
binary=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve()
if binary.read_bytes()[:4]!=b'\x7fELF':raise SystemExit('Not an ELF executable')
# ELF64 little-endian e_machine = EM_AARCH64 (183).
if int.from_bytes(binary.read_bytes()[18:20],'little')!=183:raise SystemExit('Expected AArch64 binary')
pak=out/'Tools/tg5040/PokeDaisy.pak';pak.mkdir(parents=True,exist_ok=True)
shutil.copy2(binary,pak/'pokedaisy-pocket');(pak/'pokedaisy-pocket').chmod(0o755)
shutil.copy2(repo/'pocket/launch.sh',pak/'launch.sh');(pak/'launch.sh').chmod(0o755)
emu=out/'Emus/tg5040/POKEDAISY.pak';emu.mkdir(parents=True,exist_ok=True)
shutil.copy2(repo/'pocket/launch_emu.sh',emu/'launch.sh');(emu/'launch.sh').chmod(0o755)
for name in ['roms','saves','states','lib']: (pak/name).mkdir(exist_ok=True)
for name in ['README.md','SOURCE_AUDIT.md','VERIFICATION.md','UPSTREAM.md','PARITY.md','ACTIONS.md','FUNCTIONAL_AUDIT.md','LIFECYCLE.md','INTERACTIONS.md','EXPANSION.md','SAVE_STATES.md','FLY_INVESTIGATION.md']:
 shutil.copy2(repo/'pocket'/name,pak/name)
for name in ['LICENSE','NOTICE']:shutil.copy2(repo/name,pak/name)
licenses=pak/'licenses';licenses.mkdir(exist_ok=True)
for src,name in [('third_party/mgba/LICENSE','mGBA-MPL-2.0.txt'),('third_party/mgba/src/third-party/blip_buf/license.txt','blip_buf-LGPL-2.1.txt'),('third_party/mgba/src/third-party/inih/LICENSE.txt','inih-BSD.txt')]:
 shutil.copy2(repo/src,licenses/name)
(licenses/'PixelOperator.txt').write_text('Pixel Operator by Jayvee Enaguas. Public domain under CC0.\nhttps://creativecommons.org/publicdomain/zero/1.0/\n')
print(pak)
