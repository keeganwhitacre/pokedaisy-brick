"""Exercise installed shell launchers without invoking an emulator or power."""
from pathlib import Path
import json, os, shutil, subprocess, tempfile

source = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix="pd launch ") as temp:
    card = Path(temp) / "SD card"
    pak = card / "Tools/tg5040/PokeDaisy.pak"
    pak.mkdir(parents=True)
    shutil.copy2(source / "launch.sh", pak / "launch.sh")
    exe = pak / "pokedaisy-pocket"
    exe.write_text("#!/usr/bin/env python3\nimport json,os,sys\njson.dump(sys.argv[1:],open(os.environ['PD_CAPTURE'],'w'))\n")
    exe.chmod(0o755)
    capture = Path(temp) / "arguments.json"
    env = dict(os.environ, SDCARD_PATH=str(card), PD_CAPTURE=str(capture))
    rom = str(card / "Roms/Polished Crystal (POKEDAISY)/Crystal $HOME.gbc")
    (pak / "launch.log").write_bytes(b"x" * 1048577)
    subprocess.run(["sh", str(source / "launch_emu.sh"), rom, "--fresh"], env=env, check=True)
    assert json.loads(capture.read_text()) == ["--data", str(pak), rom, "--fresh"]
    assert (pak / "launch.previous.log").stat().st_size == 1048577
    assert all((pak / name).is_dir() for name in ("roms", "saves", "states"))
    subprocess.run(["sh", str(pak / "launch.sh"), "--library"], env=env, check=True)
    assert json.loads(capture.read_text()) == ["--data", str(pak), "--library"]
print("PASS direct NextUI wrapper: quoted SD/ROM paths, literal arguments, shared data root, explicit library and bounded logs")
