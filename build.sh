#!/bin/sh
cmake -G Ninja -B build/release src
ninja -C build/release

