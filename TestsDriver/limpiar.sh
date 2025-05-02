#!/bin/bash

for gpio in 521 522 523 529 534 539 514 515 516 do 
echo "$gpio" > /sys/class/gpio/unexport
done
