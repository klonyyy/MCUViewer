#!/usr/bin/env bash
git config --global --add safe.directory /__w/STMViewer/STMViewer

rm -rf build
mkdir -p build
cd build
mkdir packages

mkdir -p linux
cd linux
cmake -DPRODUCTION=TRUE ../..
make package -j32
cp *.deb ../packages
cd - 

mkdir -p windows
cd windows
cmake -DPLATFORM=WIN -DPRODUCTION=TRUE ../..
make package -j32
cp *win64.exe ../packages
cd - 


