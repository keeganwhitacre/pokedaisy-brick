"""Run the actual launcher/backend with an original cart; inspect its PNG output."""
from pathlib import Path
import os
import struct
import subprocess
import sys
import tempfile
import zlib

rom = str(Path(sys.argv[1]).resolve())
command = sys.argv[2:]  # optional QEMU prefix followed by the actual executable
with tempfile.TemporaryDirectory(prefix='pd-ds-frontend-') as directory:
    env = dict(os.environ, SDL_VIDEODRIVER='offscreen' if len(command) > 1 else 'dummy',
               SDL_AUDIODRIVER='dummy', SDL_RENDER_DRIVER='software')
    image = Path(directory) / 'actual.png'
    result = subprocess.run(command + ['--data', directory, '--frames', '5',
                            '--shot', str(image), rom], env=env, capture_output=True, text=True, timeout=70)
    assert result.returncode == 0, result.stderr
    assert 'DSperate DSPT' in result.stderr and 'core frames 5' in result.stderr, result.stderr
    png = image.read_bytes()
    assert png[:8] == b'\x89PNG\r\n\x1a\n'
    width, height = struct.unpack_from('>II', png, 16)
    compressed, offset = bytearray(), 8
    while offset < len(png):
        size = struct.unpack_from('>I', png, offset)[0]
        if png[offset+4:offset+8] == b'IDAT':
            compressed.extend(png[offset+8:offset+8+size])
        offset += size + 12
    raw = zlib.decompress(compressed)
    stride = width * 3 + 1
    assert all(raw[y*stride] == 0 for y in range(height))  # pd_png's filter
    pixel = raw[(height//2)*stride + 1 + (width//2)*3:][:3]
    assert pixel == b'\0\xfb\0', pixel
    # The actual companion opens but correctly refuses this unsupported cart.
    env['POCKET_DS_COMPANION_AT'] = '2'
    result = subprocess.run(command + ['--data', directory, '--frames', '5', rom],
                            env=env, capture_output=True, text=True, timeout=70)
    assert result.returncode == 0 and 'core frames 2' in result.stderr, result.stderr
print('PASS real DS launcher, diagnostic display screenshot and companion pause')
