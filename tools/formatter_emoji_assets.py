#!/usr/bin/env python3
"""Offline formatter artwork for host tests and IPK packaging.

Default: verify/unpack the versioned bundle, with no network or Pillow dependency.
Rebuild: --rebuild [--source-tar PATH] regenerates it from Twemoji v16.0.1.
Runtime uses the normal bounded asynchronous texture cache, never this script.
"""
import argparse
from hashlib import sha256
from io import BytesIO
import json
from pathlib import Path
import re
import tarfile
from urllib.request import urlopen

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'deploy/app/art/icones'
BUNDLE = ROOT / 'assets/formatter-emoji'
BASE = 'https://raw.githubusercontent.com/jdecked/twemoji/v16.0.1'
SYMBOLS = [
    ('💾', 'hard-drive'), ('👤', 'users'), ('👥', 'users'), ('🎞', 'film'),
    ('🎬', 'clapperboard'), ('🎥', 'film'), ('🔊', 'volume-2'), ('🎧', 'volume-2'),
    ('🌐', 'globe'), ('⚙', 'settings-2'), ('⚡', 'zap'), ('📦', 'package'),
    ('🏷', 'tag'), ('⏱', 'clock'), ('📝', 'captions'), ('📺', 'monitor-play'),
    ('📁', 'folders'), ('🔗', 'link'), ('✅', 'check'), ('❌', 'x'),
    ('⭐', 'star'), ('🔥', 'sparkles'), ('🔒', 'key'), ('⏳', 'clock'),
    ('🚀', 'zap'), ('💬', 'message-square'), ('📊', 'chart-column'), ('📌', 'tag'),
]


def rebuild(source):
    from PIL import Image
    data = Path(source).read_bytes() if source else urlopen(
        'https://codeload.github.com/jdecked/twemoji/tar.gz/refs/tags/v16.0.1', timeout=90).read()
    DEST.mkdir(parents=True, exist_ok=True)
    fallback = {em: 'aj_' + fb for em, fb in SYMBOLS}
    rows, manifest = [], []
    with tarfile.open(fileobj=BytesIO(data), mode='r:gz') as archive:
        for member in archive.getmembers():
            if '/assets/72x72/' not in member.name or not member.name.endswith('.png'):
                continue
            code = Path(member.name).stem
            if not re.fullmatch('[0-9a-f]+(?:-[0-9a-f]+)*', code):
                raise ValueError('Invalid emoji filename')
            emoji = ''.join(chr(int(c, 16)) for c in code.split('-'))
            key = emoji.replace('\ufe0f', '')
            icon = 'emoji_' + code
            image = Image.open(BytesIO(archive.extractfile(member).read())).convert('RGBA')
            image.thumbnail((48, 48), Image.Resampling.LANCZOS)
            path = DEST / (icon + '.png')
            image.save(path, optimize=True)
            rows.append((key, icon, fallback.get(key, 'aj_globe')))
            manifest.append(dict(emoji=emoji, file=path.name,
                url=f'{BASE}/assets/72x72/{code}.png', sha256=sha256(path.read_bytes()).hexdigest()))
    rows.append(('⚑', 'emoji_1f310', 'aj_globe'))
    rows.sort()
    assert len({row[0] for row in rows}) == len(rows)
    (ROOT / 'src/stream_formatter_emoji.inc').write_text(
        '// Generated from Twemoji v16.0.1, CC-BY 4.0. Sorted UTF-8 keys without VS16.\n'
        'static const struct { const char *emoji, *icone, *reserva; } EMOJI_ICONE[] = {\n' +
        ''.join(f'  {{ "{em}", "{ic}", "{fb}" }},\n' for em, ic, fb in rows) + '};\n', encoding='utf-8')
    (DEST / 'emoji-manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    # Chunked binary bundle avoids thousands of generated files in source control.
    output = BytesIO()
    with tarfile.open(fileobj=output, mode='w:gz', compresslevel=9) as archive:
        for entry in sorted(manifest, key=lambda x: x['file']):
            raw = (DEST / entry['file']).read_bytes()
            info = tarfile.TarInfo(entry['file']); info.size = len(raw); info.mode = 0o644
            archive.addfile(info, BytesIO(raw))
    packed = output.getvalue(); BUNDLE.mkdir(parents=True, exist_ok=True)
    for old in BUNDLE.glob('part-*.bin'): old.unlink()
    parts = []
    for offset in range(0, len(packed), 512 * 1024):
        path = BUNDLE / f'part-{len(parts):02d}.bin'
        block = packed[offset:offset + 512 * 1024]; path.write_bytes(block)
        parts.append(dict(file=path.name, sha256=sha256(block).hexdigest(), bytes=len(block)))
    (BUNDLE / 'bundle.json').write_text(json.dumps(dict(version='Twemoji v16.0.1',
        source_sha256=sha256(data).hexdigest(), sha256=sha256(packed).hexdigest(),
        count=len(manifest), parts=parts), indent=2) + '\n')
    print(f'Rebuilt {len(manifest)} emoji, {len(packed)} bundle bytes, {len(parts)} parts')


def unpack():
    metadata = json.loads((BUNDLE / 'bundle.json').read_text())
    blocks = []
    for part in metadata['parts']:
        if not re.fullmatch(r'part-[0-9]+\.bin', part['file']): raise ValueError('Invalid part')
        data = (BUNDLE / part['file']).read_bytes()
        if len(data) != part['bytes'] or sha256(data).hexdigest() != part['sha256']:
            raise ValueError('Corrupt emoji bundle part')
        blocks.append(data)
    packed = b''.join(blocks)
    if sha256(packed).hexdigest() != metadata['sha256']: raise ValueError('Corrupt emoji bundle')
    expected = {item['file']: item['sha256'] for item in json.loads((DEST / 'emoji-manifest.json').read_text())}
    seen = set()
    with tarfile.open(fileobj=BytesIO(packed), mode='r:gz') as archive:
        for member in archive:
            if not member.isfile() or member.name not in expected or member.name in seen or member.size > 100000:
                raise ValueError('Invalid emoji bundle member')
            data = archive.extractfile(member).read()
            if sha256(data).hexdigest() != expected[member.name]: raise ValueError('Corrupt emoji PNG')
            path = DEST / member.name
            if not path.exists() or path.read_bytes() != data: path.write_bytes(data)
            seen.add(member.name)
    if seen != set(expected) or len(seen) != metadata['count']: raise ValueError('Incomplete emoji bundle')
    print(f'Verified/unpacked {len(seen)} offline formatter emoji')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rebuild', action='store_true')
    parser.add_argument('--source-tar')
    args = parser.parse_args()
    if args.rebuild: rebuild(args.source_tar)
    else: unpack()
