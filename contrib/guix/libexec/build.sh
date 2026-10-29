#!/usr/bin/env bash
# Runs inside the Guix container, with no network access: builds the wheel for
# $HOST (or the sdist) from the source archive and the Python headers fetch.sh
# downloaded, and writes it and its SHA256SUMS.part to /outdir.
#
# Every wheel is cp311-abi3: the modules only use CPython's stable ABI, so one
# wheel per platform works on CPython 3.11 and later. The compiler flags are
# setup.py's (-O3 -fno-strict-aliasing, -std=c99 for verthash).
export LC_ALL=C
export TZ=UTC
set -o errexit -o pipefail -o nounset
umask 0022

: "${HOST:?}" "${SOURCE_DATE_EPOCH:?}" "${SRC_TAR:?}" "${JOBS:?}"
export SOURCE_DATE_EPOCH
export HOME=/tmp/home
mkdir -p "$HOME"

DL=/cache/dl
PBS="cpython-3.12.15+20261021"

# Given a package name and an output name, return the path of that output in
# the container's profile (from Bitcoin Core's contrib/guix/libexec/prelude.bash)
store_path() {
    grep --extended-regexp "/[^-]{32}-${1}-[^-]+${2:+-${2}}" "${GUIX_ENVIRONMENT}/manifest" \
        | head --lines=1 \
        | sed --expression='s|\x29*$||' \
              --expression='s|^[[:space:]]*"||' \
              --expression='s|"[[:space:]]*$||'
}

# Point the cross compiler at its own libc and nothing else
cross_paths() {
    local libc="$1" kernel="${2:-}" gcc_lib_store gcc_libs gcc_lib
    gcc_lib_store="$(store_path "gcc-cross-${HOST}" lib)"
    gcc_libs=( "${gcc_lib_store}/lib/gcc/${HOST}"/* )
    gcc_lib="${gcc_libs[0]}"
    export CROSS_C_INCLUDE_PATH="${gcc_lib}/include:${gcc_lib}/include-fixed:${libc}/include${kernel:+:${kernel}/include}"
    export CROSS_LIBRARY_PATH="${gcc_lib_store}/lib:${gcc_lib}:${libc}/lib"
    unset LIBRARY_PATH CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH CROSS_CPLUS_INCLUDE_PATH
    # Guix's ld wrapper otherwise adds an RPATH entry for every -L, pointing
    # the binary at /gnu/store
    export GUIX_LD_WRAPPER_DISABLE_RPATH=yes
}

tar -xf "$SRC_TAR" -C /distsrc
SRC=/distsrc/src
cd "$SRC"
mkdir -p /outdir

# extract_python TARBALL: python-build-standalone's install_only layout is python/...
extract_python() {
    rm -rf /tmp/pbs
    mkdir -p /tmp/pbs
    tar -xzf "$DL/$1" -C /tmp/pbs
}

COMMON_SOURCES="scrypt.c Lyra2RE.c Sponge.c Lyra2.c sha3/blake.c sha3/groestl.c sha3/keccak.c
                sha3/cubehash.c sha3/bmw.c sha3/skein.c h2.c tiny_sha3/sha3.c"
# module, its source, extra flags (as setup.py)
MODULES="verthash:verthashmodule.c:-std=c99
lyra2re3_hash:lyra2re3module.c:
lyra2re2_hash:lyra2re2module.c:
lyra2re_hash:lyra2remodule.c:
vtc_scrypt_new:scryptmodule.c:"

# build_modules CC CFLAGS SUFFIX LINK...: each module with every common source,
# as setup.py builds them, into /tmp/modules/<module><SUFFIX>
build_modules() {
    local cc="$1" cflags="$2" suffix="$3"
    shift 3
    rm -rf /tmp/obj /tmp/modules
    mkdir -p /tmp/modules
    local line module source extra src objs
    while IFS=: read -r module source extra; do
        mkdir -p "/tmp/obj/$module"
        objs=()
        for src in $COMMON_SOURCES $source; do
            obj="/tmp/obj/$module/$(echo "$src" | tr / _).o"
            # shellcheck disable=SC2086
            $cc $cflags $extra -c "$src" -o "$obj"
            objs+=("$obj")
        done
        # shellcheck disable=SC2086
        $cc "$@" -o "/tmp/modules/${module}${suffix}" "${objs[@]}"
    done <<< "$MODULES"
}

BASE_CFLAGS="-O3 -fno-strict-aliasing -DNDEBUG -DPy_LIMITED_API=0x030B0000 -I. -I./sha3 -I./tiny_sha3 -ffile-prefix-map=${SRC}=."
ABI=cp311-abi3

case "$HOST" in
    sdist)
        echo "== sdist"
        python3 setup.py --quiet sdist --formats=gztar --dist-dir=/tmp/sdist1
        # setuptools' tar.gz is not reproducible; repack it
        mkdir -p /tmp/sdist2
        tar -xzf /tmp/sdist1/*.tar.gz -C /tmp/sdist2
        dir="$(basename /tmp/sdist2/*)"
        find /tmp/sdist2 -exec touch --no-dereference --date="@${SOURCE_DATE_EPOCH}" {} +
        out="${dir}.tar.gz"
        tar --sort=name --owner=0 --group=0 --numeric-owner --mode='u+rw,go+r-w,a+X' \
            -C /tmp/sdist2 -cf - "$dir" | gzip -9n > "/outdir/$out"
        ;;

    *-linux-gnu)
        arch="${HOST%%-*}"
        extract_python "${PBS}-${arch}-unknown-linux-gnu-install_only.tar.gz"
        cross_paths "$(store_path "glibc-cross-${HOST}")" "$(store_path "linux-libre-headers-cross-${HOST}")"
        echo "== modules ($HOST)"
        build_modules "${HOST}-gcc" "$BASE_CFLAGS -fPIC -I/tmp/pbs/python/include/python3.12" .abi3.so \
            -shared -Wl,-O1 -Wl,--as-needed -Wl,-z,relro -Wl,-z,now
        for m in /tmp/modules/*; do
            "${HOST}-strip" --strip-unneeded -R .comment "$m"
            # manylinux: only libc (and on aarch64 the dynamic loader, which exports
            # the stack protector's __stack_chk_guard there), nothing newer than
            # glibc 2.31, no search path
            needed="$("${HOST}-readelf" -d "$m" | sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p' | tr '\n' ' ')"
            case "$needed" in
                "libc.so.6 "|"libc.so.6 ld-linux-aarch64.so.1 ") ;;
                *) echo "ERR: $(basename "$m") needs: $needed" >&2; exit 1 ;;
            esac
            if [ "$needed" != "libc.so.6 " ]; then
                echo "$(basename "$m"): from the loader: $("${HOST}-readelf" --dyn-syms -W "$m" | grep -oE '(__stack_chk_guard|__tls_get_addr|_dl_[a-z_]+)@[A-Z_0-9.]+' | sort -u | tr '\n' ' ')"
            fi
            if "${HOST}-readelf" -d "$m" | grep -E 'RPATH|RUNPATH'; then echo "ERR: search path in $m" >&2; exit 1; fi
            newest="$("${HOST}-readelf" -V --wide "$m" | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -1)"
            printf '%s\n' "$newest" GLIBC_2.31 | sort -CV || { echo "ERR: $(basename "$m") needs $newest" >&2; exit 1; }
            echo "$(basename "$m"): needs $needed, newest symbol $newest"
        done
        python3 /libexec/wheel.py /outdir "${ABI}-manylinux_2_31_${arch}" /tmp/modules "$SRC"
        ;;

    x86_64-w64-mingw32)
        extract_python "${PBS}-x86_64-pc-windows-msvc-install_only.tar.gz"
        cross_paths "$(store_path mingw-w64-x86_64-winpthreads)"
        echo "== modules ($HOST)"
        # MS_WIN64: Python's Windows pyconfig.h only sets 64-bit sizes for MSVC otherwise.
        # Linking python3.dll, the stable ABI's DLL, rather than python3XX.dll.
        build_modules "${HOST}-gcc" "$BASE_CFLAGS -DMS_WIN64 -I/tmp/pbs/python/include" .pyd \
            -shared -static-libgcc -Wl,--no-insert-timestamp -L/tmp/pbs/python -lpython3
        for m in /tmp/modules/*; do
            "${HOST}-strip" --strip-unneeded "$m"
            echo "$(basename "$m"): imports $("${HOST}-objdump" -p "$m" | sed -n 's/^\s*DLL Name: //p' | tr '\n' ' ')"
        done
        python3 /libexec/wheel.py /outdir "${ABI}-win_amd64" /tmp/modules "$SRC"
        ;;

    *-apple-darwin)
        case "$HOST" in
            x86_64-apple-darwin) arch=x86_64; pbs_arch=x86_64 ;;
            arm64-apple-darwin)  arch=arm64;  pbs_arch=aarch64 ;;
        esac
        extract_python "${PBS}-${pbs_arch}-apple-darwin-install_only.tar.gz"
        # only the SDK's headers and libraries, none of the profile's
        unset LIBRARY_PATH CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH OBJC_INCLUDE_PATH OBJCPLUS_INCLUDE_PATH
        cc="clang --target=${arch}-apple-macos11 -mmacos-version-min=11.0 -isysroot /sdk -nostdlibinc -iwithsysroot/usr/include -fuse-ld=lld"
        echo "== modules ($HOST)"
        # lld signs arm64 binaries ad hoc, which macOS requires; strip would break that
        build_modules "$cc" "$BASE_CFLAGS -I/tmp/pbs/python/include/python3.12" .abi3.so \
            -bundle -undefined dynamic_lookup
        for m in /tmp/modules/*; do
            echo "$(basename "$m"): $(llvm-objdump --macho --private-headers "$m" | grep -cE 'LC_CODE_SIGNATURE') code signature(s)"
        done
        python3 /libexec/wheel.py /outdir "${ABI}-macosx_11_0_${arch}" /tmp/modules "$SRC"
        ;;

    *)
        echo "ERR: unsupported host $HOST" >&2
        exit 1
        ;;
esac

cd /outdir
sha256sum -- *.whl *.tar.gz 2>/dev/null > SHA256SUMS.part || true
[ -s SHA256SUMS.part ] || { echo "ERR: no output" >&2; exit 1; }
cat SHA256SUMS.part
