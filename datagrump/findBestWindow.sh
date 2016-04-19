#!/bin/bash

for i in {1..30..2}
  do
    sed "s/INSERTWINDOWSIZE/$i/g" controllerTemplate.cc > controller.cc;
    make;
    echo "Now running window size $i" | tee -a output.txt;
    ./run-contest test22 2>&1 | tee -a output.txt;
  done
