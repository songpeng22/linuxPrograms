#!/bin/bash
# CMake build
# build side
mkdir -p build
pushd build
cmake ../src
make
popd
mv ./build/program . -v
rm -rf build

# Makefile build
#pushd src
#make
#popd
#mv src/main . -v
#pwd
