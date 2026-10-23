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
    system = Path(temp) / "NextUI system"
    (system / "bin").mkdir(parents=True)
    governor = system / "bin/governor.sh"
    governor.write_text('#!/bin/sh\nprintf "%s\\n" "$1" >> "$PD_POLICY_CAPTURE"\nexit "${PD_POLICY_FAIL:-0}"\n')
    policy = Path(temp) / "policies.txt"
    env.update(SYSTEM_PATH=str(system), PD_POLICY_CAPTURE=str(policy))
    env.pop("POCKET_CPU_MODE", None)
    subprocess.run(["sh", str(pak / "launch.sh"), "--fresh"], env=env, check=True)
    assert policy.read_text() == "auto\n"
    env["POCKET_CPU_MODE"] = "powersave"
    subprocess.run(["sh", str(pak / "launch.sh")], env=env, check=True)
    assert policy.read_text() == "auto\npowersave\n"
    env["POCKET_CPU_MODE"] = "inherit"
    subprocess.run(["sh", str(pak / "launch.sh")], env=env, check=True)
    assert policy.read_text() == "auto\npowersave\n"
    env.update(POCKET_CPU_MODE="auto", PD_POLICY_FAIL="1")
    subprocess.run(["sh", str(pak / "launch.sh"), "--library"], env=env, check=True)
    assert json.loads(capture.read_text()) == ["--data", str(pak), "--library"]
    assert "CPU policy request failed" in (pak / "launch.log").read_text()
print("PASS direct NextUI wrapper: quoted SD/ROM paths, literal arguments, shared data root, explicit library and bounded logs")
print("PASS NextUI-owned Auto/Powersave requests, inherit opt-out and helper failure fallback; no CPU/power controls invoked")
