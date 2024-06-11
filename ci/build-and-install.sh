#!/bin/bash

# create build directory
mkdir -p build

cd build
cmake ..

# build and install
make install
