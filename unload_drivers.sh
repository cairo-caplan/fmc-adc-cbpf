#!/bin/bash

# Script to unload fmc-adc kernel modules for the Ex 8 of ISOTDAQ
# @author Cairo Caplan <cairo@cbpf.br>
# @file unload.sh

sudo su -c "rmmod fmc-adc-100m14b
	rmmod spec
	rmmod zio
	rmmod fmc
	dmesg | tail
echo ' '
echo 'Drivers Unloaded'
echo ' '
"


