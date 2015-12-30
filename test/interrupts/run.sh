#!/bin/bash

PATHNAME=`dirname $0`

for (( c=0; c<10; c++ ))
do
    $PATHNAME/interrupt_test > log.txt &
    sleep 60
    killall -s SIGINT interrupt_test
    sleep 2
    cat $PATHNAME/log.txt
done
