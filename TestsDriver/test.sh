#!/bin/bash

#make MODULE=berryclip_driver compile

#sudo insmod berryclip_driver.ko

DELAY=0.2

echo "Probando todas las combinaciones de LEDS"

for i in $(seq 0 63); do
	
	HEX=$(printf "%02X" $i)
	echo -e "\x$HEX" > /dev/leds
	sleep $DELAY
done

echo -e "\x00" > /dev/leds

echo "Prueba completada"

./darth_vader

./tetris

./buttons

#sudo rmmod berryclip_driver

#make MODULE=berryclip_driver vclean
