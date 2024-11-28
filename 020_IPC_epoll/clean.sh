#!/bin/bash

FILE_PATH=`realpath $0`
FILE_DIR=`dirname $FILE_PATH`
echo "FILE_PATH is $FILE_PATH"
echo "FILE_DIR is $FILE_DIR"

rm $FILE_DIR/socket* -v

