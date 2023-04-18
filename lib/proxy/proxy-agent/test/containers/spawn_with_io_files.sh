#!/bin/bash

echo "$0: Syntax"
echo "$0 IN_FILE OUTFILE cmd [args ...]"

echo "Got $# args: $*"

if [ $# -lt 3 ]
then
    echo "Too few args, need at least 3" >&2
    exit -1
fi

IN_FILE=$1

OUT_FILE=$2

shift
shift

echo "Remaining args: $*"

rm -rf ${IN_FILE} ${OUT_FILE}
touch ${IN_FILE} ${OUT_FILE}

(tail -f ${IN_FILE} | $*) 2>&1 >> ${OUT_FILE} &
