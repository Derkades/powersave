#!/bin/bash
set -e

if [ -z "$1" ] || [ -z "$2" ]
then
    echo "Usage: $0 <target fps> <target latency>"
    exit 1
fi

CONTENT="$1\n$2"

adb exec-out sh -c "echo \"$CONTENT\" > /data/local/workingdir/target.txt"
