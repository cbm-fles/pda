#!/bin/bash

echo `pda-config --include`

INCLUDE_DIR=`pda-config --include`
TMP_PWD=`pwd`
cd $INCLUDE_DIR
INCLUDE_FILES=`find . -iname '*.h'`
cd $TMP_PWD

for INC in $INCLUDE_FILES
do
	BIN_NAME=`basename $INC .h`
	INCLUDE_NAME=`echo $INC | awk '{ print substr($0,3) }'`	
	cat > $BIN_NAME.c <<EOL
#include <$INCLUDE_NAME>
#include <stdio.h>
int main( int argc, const char* argv[] )
{
	printf( "Test $BIN_NAME.\n" );
}
EOL
done

