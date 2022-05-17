#!/bin/bash -e

cd build

# Configure
cmake ..

# Compile & link
cmake --build .

./build/run_tests
