#!/bin/bash -e

cd build

# Configure
cmake ..

# Compile & link
cmake --build .

# For testing at Berkeley
# TESTFILE="/mnt_blpd18/datax/incoming/guppi_59711_53086_001288_J0408-15-0_0001.0000.raw"

# For testing on a local machine
TESTFILE=`readlink -f ~/seticore/data/golden_synthesized_input.0000.raw`

if [ ! -f "$TESTFILE" ]; then
    echo could not find testfile: $TESTFILE
    exit 1
fi

time ./tests $TESTFILE


