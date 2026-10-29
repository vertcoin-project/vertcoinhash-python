#!/usr/bin/env python3
"""Write a wheel of the extension modules in MODDIR, byte for byte the same
for the same inputs: entries sorted, fixed times and permissions.

    wheel.py OUTDIR TAG MODDIR SRCDIR
"""
import base64
import hashlib
import io
import os
import re
import sys
import zipfile

outdir, tag, moddir, srcdir = sys.argv[1:]

setup_py = open(os.path.join(srcdir, 'setup.py'), encoding='utf-8').read()


def setup_field(name):
    m = re.search(rf"\b{name}\s*=\s*'([^']*)'", setup_py)
    if not m:
        raise SystemExit(f'no {name} in setup.py')
    return m.group(1)


name, version = setup_field('name'), setup_field('version')
classifiers = re.findall(r"'([A-Za-z ]+ :: [^']+)'", setup_py)
readme = open(os.path.join(srcdir, 'README.md'), encoding='utf-8').read()

metadata = '\n'.join([
    'Metadata-Version: 2.1',
    f'Name: {name}',
    f'Version: {version}',
    f"Summary: {setup_field('description')}",
    f"Home-page: {setup_field('url')}",
    f"Author: {setup_field('author')}",
    f"Author-email: {setup_field('author_email')}",
    f"License: {setup_field('license')}",
    *[f'Classifier: {c}' for c in classifiers],
    f"Requires-Python: {setup_field('python_requires')}",
    'Description-Content-Type: text/markdown',
    '',
    readme,
])
wheel_meta = '\n'.join([
    'Wheel-Version: 1.0',
    'Generator: vertcoinhash contrib/guix',
    'Root-Is-Purelib: false',
    f'Tag: {tag}',
    '',
])
modules = sorted(os.listdir(moddir))
top_level = ''.join(m.split('.')[0] + '\n' for m in modules)

distinfo = f'{name}-{version}.dist-info'
files = [(m, open(os.path.join(moddir, m), 'rb').read(), 0o755) for m in modules]
files += [
    (f'{distinfo}/METADATA', metadata.encode(), 0o644),
    (f'{distinfo}/WHEEL', wheel_meta.encode(), 0o644),
    (f'{distinfo}/top_level.txt', top_level.encode(), 0o644),
]
record = io.StringIO()
for path, data, _ in files:
    digest = base64.urlsafe_b64encode(hashlib.sha256(data).digest()).rstrip(b'=').decode()
    record.write(f'{path},sha256={digest},{len(data)}\n')
record.write(f'{distinfo}/RECORD,,\n')
files.append((f'{distinfo}/RECORD', record.getvalue().encode(), 0o644))

# zip has no times before 1980
epoch = max(int(os.environ['SOURCE_DATE_EPOCH']), 315532800)
import time
date_time = time.gmtime(epoch)[:6]
out = os.path.join(outdir, f'{name}-{version}-{tag}.whl')
with zipfile.ZipFile(out, 'w') as z:
    # modules first, then the .dist-info with RECORD last, as wheels are written
    for path, data, mode in files:
        info = zipfile.ZipInfo(path, date_time=date_time)
        info.external_attr = (0o100000 | mode) << 16
        info.compress_type = zipfile.ZIP_DEFLATED
        info.create_system = 3
        z.writestr(info, data, compresslevel=9)
print(os.path.basename(out))
