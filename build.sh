#!/bin/sh

#export CC=clang
#export CXX=clang++

cmake -Wno-dev -G Ninja \
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
-B build/release src
ninja -C build/release

