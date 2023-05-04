#!/usr/bin/env bash
rm -rf build
mkdir -p build
cd build
mkdir packages

mkdir -p windows
cd windows
cmake -DPLATFORM=WIN ../..
make package -j16
cp *win64.exe ../packages
cd - 

mkdir -p linux
cd linux
cmake ../..
make package -j16
cp *.deb ../packages
cd - 
