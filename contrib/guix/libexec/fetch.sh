#!/usr/bin/env bash
# Runs inside the Guix container, with network access: downloads the files in
# contrib/guix/downloads.txt that the builds of $HOSTS need into /cache/dl,
# each checked against its sha256, so the cache can only hold what the commit names.
export LC_ALL=C
set -o errexit -o pipefail -o nounset
umask 0022

export HOME=/tmp/home
mkdir -p "$HOME" /tmp/work /cache/dl
tar -xf "${SRC_TAR:?}" -C /tmp/work
cd /tmp/work/src

export SSL_CERT_DIR="${GUIX_ENVIRONMENT}/etc/ssl/certs"
export SSL_CERT_FILE="${SSL_CERT_DIR}/ca-certificates.crt"

python3 - contrib/guix/downloads.txt <<'EOF'
import hashlib, os, sys, urllib.request
hosts = set(os.environ['HOSTS'].split())
for line in open(sys.argv[1]):
    if line.startswith('#') or not line.strip():
        continue
    sha, for_hosts, name, url = line.split()
    if not hosts & set(for_hosts.split(',')):
        continue
    path = os.path.join('/cache/dl', name)
    def ok():
        h = hashlib.sha256()
        with open(path, 'rb') as f:
            for chunk in iter(lambda: f.read(1 << 20), b''):
                h.update(chunk)
        return h.hexdigest() == sha
    if os.path.exists(path) and ok():
        continue
    print(f'downloading {name}', flush=True)
    with urllib.request.urlopen(url, timeout=300) as r, open(path + '.part', 'wb') as f:
        while chunk := r.read(1 << 20):
            f.write(chunk)
    os.replace(path + '.part', path)
    if not ok():
        os.unlink(path)
        sys.exit(f'sha256 mismatch for {name}')
EOF
echo "fetch done"
