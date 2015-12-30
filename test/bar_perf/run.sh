#!/bin/bash

PATHNAME=`dirname $0`
LD_LIBRARY_PATH=`pda-config --ldlibrarypath` $PATHNAME/bar_perf $@
LD_LIBRARY_PATH=`pda-config --ldlibrarypath` valgrind --leak-check=full --show-reachable=yes $PATHNAME/bar_perf $@
