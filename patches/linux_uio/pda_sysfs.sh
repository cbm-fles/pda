#!/bin/bash

UIO_PATH="/sys/bus/pci/drivers/uio_pci_dma/"
UIO_GROUP="pda"

find -L $UIO_PATH -maxdepth 1 -iname "*:*:*.*" \
-exec chown -R root:$UIO_GROUP {}/dma \;       \
-exec chmod -R gu+r+w {}/dma \;

find -L $UIO_PATH -maxdepth 1 -iname "*:*:*.*" \
-exec chmod -R o-r-w-x {}/dma \;

find -L $UIO_PATH -maxdepth 1 -iname "*:*:*.*" -exec bash -c          \
'chown -R root:"$0" "$1"/bar* "$1"/resource* "$1"/config ;            \
chmod -R gu+r+w-x "$1"/bar* "$1"/resource* "$1"/config' $UIO_GROUP {} \;

find -L $UIO_PATH -maxdepth 1 -iname "*:*:*.*" -exec bash -c          \
'chmod -R o-r-w-x "$1"/bar* "$1"/resource* "$1"/config' $UIO_GROUP {} \;

find -L $UIO_PATH -maxdepth 1 -iname "*:*:*.*" -exec bash -c          \
'chmod -R 444 "$1"/resource' $UIO_GROUP {} \;

chown root:$UIO_GROUP /dev/uio*
chmod gu+r+w          /dev/uio*

chown root:$UIO_GROUP $UIO_PATH/new_id
chmod gu+r+w          $UIO_PATH/new_id

exit  0
