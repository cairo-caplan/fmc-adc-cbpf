#/bin/bash

if [ $1 = 1 ]
then
    echo $2 > /sys/devices/hw-adc-100m14b-0100/adc-100m14b-0100/cset0/ch0-50ohm-term
    echo "CH1 term ="
    cat /sys/devices/hw-adc-100m14b-0100/adc-100m14b-0100/cset0/ch0-50ohm-term
fi

if [ $1 = 2 ]
then
    echo $2 > /sys/devices/hw-adc-100m14b-0100/adc-100m14b-0100/cset0/ch1-50ohm-term
    echo "CH2 term ="
    cat /sys/devices/hw-adc-100m14b-0100/adc-100m14b-0100/cset0/ch1-50ohm-term
fi
