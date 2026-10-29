# vertcoinhash-python

[vertcoinhash](https://pypi.org/project/vertcoinhash) is a complete merge of [vtc-scrypt](https://github.com/vertcoin-project/vtc-scrypt), [lyra2re-hash-python](https://github.com/metalicjames/lyra2re-hash-python) and [verthash-pospace](https://github.com/vertcoin-project/verthash-pospace)

## Installing vertcoinhash package from PyPI

```
pip install vertcoinhash
```

The [PyPI packages](https://pypi.org/project/vertcoinhash/) are built reproducibly with [Guix](contrib/guix/README.md).
[GitHub Actions](https://github.com/vertcoin-project/vertcoinhash-python/blob/master/.github/workflows/guix.yml)
runs that build for each tag, tests every wheel on its own platform, and uploads the same files to PyPI.

There is one wheel per platform for CPython 3.11 and later: Linux (x86_64, aarch64), macOS (x86_64, arm64) and Windows (AMD64).
Older CPythons build the sdist from source.

## Usage

Each module has `getPoWHash(header)`, which takes an 80-byte block header (any bytes-like object) and returns the 32-byte hash:

| module | proof of work | Vertcoin blocks |
|---|---|---|
| `vtc_scrypt_new` | scrypt-N | below 208,301 |
| `lyra2re_hash` | Lyra2RE | 208,301 to 346,999 |
| `lyra2re2_hash` | Lyra2REv2 | 347,000 to 1,080,000 |
| `lyra2re3_hash` | Lyra2REv3 | 1,080,001 to 1,499,999 |
| `verthash` | Verthash | 1,500,000 and later |

`verthash.getPoWHash(header, datfile)` also takes the contents of verthash.dat. That can be a read-only `mmap`, so the 1.2 GiB file need not be read into memory:

```python
import mmap, verthash

with open('verthash.dat', 'rb') as f:
    datfile = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
pow_hash = verthash.getPoWHash(header, datfile)
```

A header that is not 80 bytes raises `ValueError`. The GIL is released while hashing.

## Building vertcoinhash package from source
```
git clone https://github.com/vertcoin-project/vertcoinhash-python && cd vertcoinhash-python
python -m pip install .
```

gcc and clang builds use `-fno-strict-aliasing`. Without it, gcc at -O2 and above miscompiles
scrypt-N, Lyra2REv2 and Lyra2REv3, which then return wrong (even varying) hashes.

## Testing vertcoinhash package

test.py checks a trial hashing of a dummy header on all five proofs of work used by Vertcoin.
Verthash is checked when verthash.dat is in the current directory or named by `VERTHASH_DATAFILE`.
```
python test.py
    scrypt-n f48c557d72190f1734e20d1a2db41bdde7fecf506b7c89897ae1f5bd67355728
    lyra2re f95157b026fa4f8f417a6729dd07e409ab5426f0a74a734dafdf1fd4c40b7c6f
    lyra2re2 74a88f79cf96bea82ba3ef04f327f62bdf6d24cd9540f4dfb365ba7a7aa83800
    lyra2re3 6d38a43c52662c5a8f3e1e10dfa9f1236a27babc9a4efa9f0a49000000000000
    verthash e0f6c10b4a38f35a6cdcc26d32a7ed8c3bfc5d827a9bc72647afa324b70d0463
```

## Building verthash.dat

`verthash.create_datafile(path, progress_cb=None)` writes verthash.dat to `path` (replacing any file there),
in about 15 to 30 seconds on a desktop. It is a port of Vertcoin Core's generator: the file is written through a
memory map of `path + '.tmp'`, which is renamed to `path` when done, so it needs no 1.2 GiB of memory.
The GIL is released while it runs, except to call `progress_cb(fraction)` about 40 times:

```python
import verthash

verthash.create_datafile('verthash.dat', progress_cb=lambda f: print(f"{f:.0%}"))
```

An exception raised by `progress_cb` stops the generation and is raised from `create_datafile`. Other errors raise `OSError`.

The h1 program generates it too:
```
git clone https://github.com/vertcoin-project/vertcoinhash-python && cd vertcoinhash-python
make all
./h1
    Generating PoS file...
    Done generating PoS file: 93.210013 sec
```

## Testing verthash.dat

h2 runs a trial hashing of the same dummy header as in test.py

```
./h2
    allocating buffer...
    reading data file...
    SHA3 hashes...
    memory seeks...
    E0F6C10B4A38F35A6CDCC26D32A7ED8C3BFC5D827A9BC72647AFA324B70D0463

sha256sum verthash.dat
    a55531e843cd56b010114aaf6325b0d529ecf88f8ad47639b6ededafd721aa48  verthash.dat
```
