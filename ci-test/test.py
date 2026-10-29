"""Checks every hash against known values for a test header.

Verthash needs verthash.dat: it is used if VERTHASH_DATAFILE names it or it is
in the current directory, and otherwise skipped (unless REQUIRE_VERTHASH=1).
"""
import mmap
import os
import sys
import threading

import vtc_scrypt_new
import lyra2re_hash
import lyra2re2_hash
import lyra2re3_hash
import verthash

header = bytes.fromhex(
    '000000203a297b4b7685170d7644b43e5a6056234cc2414edde454a87580e1967d14c107'
    '8c13ea916117b0608732f3f65c2e03b81322efc0a62bcee77d8a9371261970a58a5a715d'
    'a80e031b02560ad8')

expected = {
    'scrypt-n': (vtc_scrypt_new, 'f48c557d72190f1734e20d1a2db41bdde7fecf506b7c89897ae1f5bd67355728'),
    'lyra2re': (lyra2re_hash, 'f95157b026fa4f8f417a6729dd07e409ab5426f0a74a734dafdf1fd4c40b7c6f'),
    'lyra2re2': (lyra2re2_hash, '74a88f79cf96bea82ba3ef04f327f62bdf6d24cd9540f4dfb365ba7a7aa83800'),
    'lyra2re3': (lyra2re3_hash, '6d38a43c52662c5a8f3e1e10dfa9f1236a27babc9a4efa9f0a49000000000000'),
}
VERTHASH = 'e0f6c10b4a38f35a6cdcc26d32a7ed8c3bfc5d827a9bc72647afa324b70d0463'

failed = False


def check(name, got, want):
    global failed
    ok = got.hex() == want
    failed |= not ok
    print(f"{name:9} {got.hex()} {'ok' if ok else 'MISMATCH, expected ' + want}")


def check_raises(name, fn, *args):
    global failed
    try:
        fn(*args)
    except ValueError:
        print(f"{name:9} rejects bad input: ok")
        return
    failed = True
    print(f"{name:9} accepted bad input: MISMATCH")


for name, (module, want) in expected.items():
    check(name, module.getPoWHash(header), want)
    check(name, module.getPoWHash(bytearray(header)), want)  # any bytes-like object
    check_raises(name, module.getPoWHash, header[:79])

datafile = os.environ.get('VERTHASH_DATAFILE') or ('verthash.dat' if os.path.exists('verthash.dat') else None)
if datafile:
    with open(datafile, 'rb') as f:
        data = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)  # no need to read 1.2 GiB into memory
        check('verthash', verthash.getPoWHash(header, data), VERTHASH)
        check_raises('verthash', verthash.getPoWHash, header[:79], data)
        check_raises('verthash', verthash.getPoWHash, header, b'')
        # the GIL is released while hashing, so threads can hash concurrently
        results = []
        threads = [threading.Thread(target=lambda: results.append(verthash.getPoWHash(header, data))) for _ in range(4)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        check('verthash', results[0], VERTHASH)
        failed |= len(set(results)) != 1
        del data
elif os.environ.get('REQUIRE_VERTHASH') == '1':
    print('verthash  no verthash.dat: MISMATCH')
    failed = True
else:
    print('verthash  skipped (no verthash.dat; set VERTHASH_DATAFILE)')

sys.exit(1 if failed else 0)
