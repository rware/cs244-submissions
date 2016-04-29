#!/bin/bash

for i in 0.72 0.73 0.74 0.75
  do
    sed "s/BLOOP/$i/g" controllerTemplate.cc > controller.cc;
    make;
    echo "Now running scale $i" | tee -a output.txt;
    ./run-contest EACC 2>&1 | tee -a output.txt;
  done
