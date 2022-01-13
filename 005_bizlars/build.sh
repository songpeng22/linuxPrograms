#!/bin/bash

cd ./static_library
make clean
make
cd ../shared_library
make clean
make
cd ../main
make clean
make

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../shared_library
./main/main
