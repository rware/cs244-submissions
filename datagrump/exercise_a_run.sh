#!/bin/bash

set -e

# Somewhat hacky script to modify window size and dump results to a file. If
# it's stupid and it works, it ain't stupid.
for windowsize in `seq 8 15`;
do
    echo $windowsize
    sed -r -i.bak "s/the_window_size = [0-9]*;/the_window_size = $windowsize;/g" controller.cc
    make > /dev/null
    ./run-no-upload
    echo
done
