#!/bin/bash
# CMake build
# build client side
mkdir -p build_client
pushd build_client
cmake ../src_client
make
popd

# build server side
mkdir -p build_server
pushd build_server
cmake ../src_server
make
#make VERBOSE=1
popd
mv build_client/socket_client . -v
mv build_server/socket_server . -v
rm -rf build_*

# Makefile build
#pushd src
#make
#popd
#mv src/main . -v
#pwd
