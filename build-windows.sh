#!/bin/bash

make clean

x86_64-w64-mingw32-windres  version-info.rc -O coff -o version-info.res
x86_64-w64-mingw32-windres  icon.rc -O coff -o icon.res

x86_64-w64-mingw32-gcc -O3 -Wall -Wextra h1.c tiny_sha3/sha3.c -o create-verthash-datafile.exe icon.res version-info.res
