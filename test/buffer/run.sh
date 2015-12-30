#!/bin/bash

PATHNAME=`dirname $0`
LD_LIBRARY_PATH=`pda-config --ldlibrarypath` $PATHNAME/buffer_test $@
LD_LIBRARY_PATH=`pda-config --ldlibrarypath` valgrind --leak-check=full --show-reachable=yes $PATHNAME/buffer_test $@
