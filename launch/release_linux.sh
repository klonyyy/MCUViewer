#!/usr/bin/env bash
git config --global --add safe.directory /__w/MCUViewer/MCUViewer

rm -rf build
mkdir -p build
cd build
mkdir packages

mkdir -p linux
cd linux
cmake -DPRODUCTION=TRUE ../..
make package -j32
cp *.deb ../packages
cp *.rpm ../packages
cd - 


