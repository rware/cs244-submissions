#!/usr/bin/env gnuplot

set term png
set output 'output.png'
set datafile separator ','
set logscale x
set xrange [5000:50]
set key off
set xlabel 'Delay (ms)'
set ylabel 'Throughput (Mbits/s)'
set title 'Delay and Throughput for Fixed-Size Windows'
plot 'data.csv' using 3:2:1 with labels point offset character -1, character .5
