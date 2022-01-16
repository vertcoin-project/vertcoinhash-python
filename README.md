# vertcoinhash-python

[vertcoinhash](https://pypi.org/project/vertcoinhash) is a complete merge of [vtc-scrypt](https://github.com/vertcoin-project/vtc-scrypt), [lyra2re-hash-python](https://github.com/metalicjames/lyra2re-hash-python) and [verthash-pospace](https://github.com/vertcoin-project/verthash-pospace)

## Installing vertcoinhash package from PyPI

```
pip install vertcoinhash
```

[PyPI packages](https://pypi.org/project/vertcoinhash/) are built on [cibuildwheel](https://github.com/pypa/cibuildwheel) using [Github actions](https://github.com/vertcoin-project/vertcoinhash-python/blob/master/.github/workflows/build_wheels.yml)

## Building vertcoinhash package from source
```
git clone https://github.com/vertcoin-project/vertcoinhash-python && cd vertcoinhash-python
python setup.py install
```

## Testing vertcoinhash package

test.py runs a trial hashing of a dummy header on all five proofs of work used by Vertcoin
```
python test.py
    scrypt-n f48c557d72190f1734e20d1a2db41bdde7fecf506b7c89897ae1f5bd67355728
    lyra2re f95157b026fa4f8f417a6729dd07e409ab5426f0a74a734dafdf1fd4c40b7c6f
    lyra2re2 74a88f79cf96bea82ba3ef04f327f62bdf6d24cd9540f4dfb365ba7a7aa83800
    lyra2re3 6d38a43c52662c5a8f3e1e10dfa9f1236a27babc9a4efa9f0a49000000000000
    verthash e0f6c10b4a38f35a6cdcc26d32a7ed8c3bfc5d827a9bc72647afa324b70d0463
```

## Building verthash.dat

h1 generates verthash.dat
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
