#!/bin/bash
set -e -o pipefail -u
cd "${0%/*}"
mkdir -p "../build"
./BuildCpp.sh release 64 ../build/MakaronCmd -I ./ ./MakaronCmd.cpp ./Makaron.cpp
