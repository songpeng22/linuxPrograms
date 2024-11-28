#!/bin/bash
# CMake build
# build write side
mkdir -p build_write
pushd build_write
cmake ../src_write
make
popd

# build read side
mkdir -p build_read
pushd build_read
cmake ../src_read
make
#make VERBOSE=1
popd
mv build_write/pipe_write . -v
mv build_read/pipe_read . -v
rm -rf build_*

# Makefile build
#pushd src
#make
#popd
#mv src/main . -v
#pwd
