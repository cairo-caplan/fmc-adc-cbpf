#!/bin/bash

x=1
while [ $x -eq 1 ]
do
      usleep 2000000;
      ./fald-simple-acq -a 100 -b 0 -n 1 -t 100 -c 0 0x100 > /tmp/fmcadc.0x0100.ch1.dat.INT
      #./acq_program -a 1000 -b 0 -n 1 -g 1 -c 1 -t 100 -r 10 0100;
done

  
  
  