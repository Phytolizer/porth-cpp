#!/usr/bin/env bash
set -xeu

cmake -B build -G Ninja -Wno-dev -DCMAKE_BUILD_TYPE=Release
cmake --build build

for f in "$(dirname "$0")"/tests/*.porth; do
    ./build/porth_cpp sim "$f"
    ./build/porth_cpp com -r "$f"
done
