"""Keep host policy out of the shared renderer and portable companion core."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
shared = list((root / '3ds/core').glob('*.[ch]'))
shared += list((root / 'app/src/main/cpp').glob('pk_wide_gb*'))
for path in shared:
    text = path.read_text()
    for forbidden in ('POCKET_NATIVE', '../../../../pocket/', 'pocket/polished'):
        assert forbidden not in text, f'{path.relative_to(root)} imports host policy: {forbidden}'
print('PASS shared C and GB renderer contain no Linux adapter imports or POCKET_NATIVE branches')
