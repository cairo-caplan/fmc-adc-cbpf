#!/bin/bash

# Script to load fmc-adc kernel modules for the Ex 8 of ISOTDAQ
# @author Cairo Caplan <cairo@cbpf.br>
# @file load_drivers.sh

sudo su -c "
	insmod fmc-bus/kernel/fmc.ko
	insmod zio/zio.ko
	insmod spec-sw/kernel/spec.ko fw_name=fmc/spec-init.bin-2012-12-14
	insmod kernel/fmc-adc-100m14b.ko gateware=fmc/spec-fmc-adc-v4.0.bin
	dmesg | tail
	chmod -R 777 /sys/devices/hw-adc-*
	chmod -R 777 /dev/zio/*
	echo ' '
	echo 'Drivers Loaded'
	echo ' '
"


