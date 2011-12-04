#!/bin/bash

PROJECT=librazorqt
NO_OBSOLETE="-noobsolete"

if [ "$1" = "" ]; then
    TS_FILE=${PROJECT}_`echo $LANG | awk -F"." '{print($1)}'`.ts
else
    TS_FILE=$1
fi


lupdate $NO_OBSOLETE -recursive ..  -ts $TS_FILE  && linguist $TS_FILE

