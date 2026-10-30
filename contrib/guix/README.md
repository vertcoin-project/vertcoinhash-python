# Reproducible builds with Guix

`contrib/guix/guix-build` builds the vertcoinhash wheels and sdist inside Guix
containers. The inputs are a fixed Guix commit (the one Bitcoin Core v32.0 pins),
the commit's source, and the Python headers pinned in `downloads.txt`, so the
same commit gives the same files byte for byte.

| `HOSTS` entry | Output |
|---|---|
| `sdist` | `vertcoinhash-<version>.tar.gz` |
| `x86_64-linux-gnu` | `vertcoinhash-<version>-cp311-abi3-manylinux_2_17_x86_64.manylinux2014_x86_64.whl` |
| `aarch64-linux-gnu` | `vertcoinhash-<version>-cp311-abi3-manylinux_2_17_aarch64.manylinux2014_aarch64.whl` |
| `arm-linux-gnueabihf` | `vertcoinhash-<version>-cp311-abi3-manylinux_2_17_armv7l.manylinux2014_armv7l.whl` |
| `powerpc64le-linux-gnu` | `vertcoinhash-<version>-cp311-abi3-manylinux_2_17_ppc64le.manylinux2014_ppc64le.whl` |
| `riscv64-linux-gnu` | `vertcoinhash-<version>-cp311-abi3-manylinux_2_27_riscv64.whl` |
| `x86_64-w64-mingw32` | `vertcoinhash-<version>-cp311-abi3-win_amd64.whl` |
| `x86_64-apple-darwin` | `vertcoinhash-<version>-cp311-abi3-macosx_11_0_x86_64.whl` |
| `arm64-apple-darwin` | `vertcoinhash-<version>-cp311-abi3-macosx_11_0_arm64.whl` |

```
./contrib/guix/guix-build                       # everything
HOSTS="sdist x86_64-linux-gnu" ./contrib/guix/guix-build
```

Outputs, and a `SHA256SUMS.part` for each host, are written to
`guix-build-<commit>/output/<host>/`. The build is of the committed tree, so commit first.
The macOS hosts need `SDK_PATH`, a directory holding Bitcoin Core's
`Xcode-26.1.1-17B100-extracted-SDK-with-libcxx-headers` (from
`https://bitcoincore.org/depends-sources/sdks/`, sha256
`9600fa93644df674ee916b5e2c8a6ba8dacf631996a65dc922d003b98b5ea3b1`).

## How it works

- **fetch** (`libexec/fetch.sh`) is the only step with network access. It downloads the
  python-build-standalone CPython 3.12 builds in `downloads.txt`, checked against their sha256.
  Only their headers are used, and on Windows `python3.dll` to link against.
- **build** (`libexec/build.sh`) runs one container per host, without network, and compiles
  the five modules with `setup.py`'s flags (`-O3 -fno-strict-aliasing`, `-std=c99` for verthash):
  - Linux (x86_64, aarch64, armv7l, ppc64le, riscv64): Bitcoin Core's GCC 14 targeting glibc 2.31.
    The build checks that each module needs only `libc.so.6` (and the target's dynamic loader)
    and no glibc symbol newer than 2.17, or 2.27 on riscv64, where glibc starts: so the wheels are
    `manylinux_2_17` (also tagged `manylinux2014`, for pip before 20.3), and `manylinux_2_27` on riscv64.
  - Windows: Bitcoin Core's MinGW-w64 toolchain, linked against `python3.dll`.
  - macOS: clang/lld 19 against the SDK. lld signs the arm64 modules ad hoc.
- **wheels**: `libexec/wheel.py` writes each wheel with sorted entries and fixed times. They are
  built against the stable ABI (`Py_LIMITED_API` 3.11), so one `cp311-abi3` wheel per platform works on CPython 3.11 and later.

## Releases

`.github/workflows/guix.yml` runs this build on tags and by hand, and prints the hashes to
compare with a local build. It then tests each wheel on its own platform with CPython 3.11
and 3.14, builds the sdist from source, and checks Verthash against a `verthash.dat`
generated with `h1`. When every test passes on a `v*` tag, it uploads the same nine files to PyPI, as
the project's trusted publisher (the `pypi` environment of this workflow), with attestations
of their provenance.

To release: bump the version in `setup.py`, commit, then push a `v<version>` tag. Before
pushing the tag, a local `./contrib/guix/guix-build` of that commit should give the same hashes.
