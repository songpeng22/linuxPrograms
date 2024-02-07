#!/bin/bash
# CMake build
mkdir -p build
pushd build
cmake ../src
make
#make VERBOSE=1
popd
mv build/APITest . -v
#rm -rf build

# Makefile build
#pushd src
#make
#popd
#mv src/main . -v
#pwd
