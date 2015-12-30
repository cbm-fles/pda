#!/bin/bash

make install
modprobe interrupt
sleep 60
rmmod interrupt

dmesg | tail
