#!/bin/bash

for i in 50 55 60
  do
    sed "s/BLOOP/$i/g" controllerTemplate.cc > controller.cc;
    make;
    echo "Now running timeout $i" | tee -a output.txt;
    ./run-contest EACC 2>&1 | tee -a output.txt;
  done
