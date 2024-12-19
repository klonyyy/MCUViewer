#!/usr/bin/env bash
git config --global --add safe.directory /__w/MCUViewer/MCUViewer

rm -rf build
mkdir -p build
cd build
mkdir packages

mkdir -p macos
cd macos
cmake -DPRODUCTION=TRUE ../..
make
cp MCUViewer* ../packages
cd - 
