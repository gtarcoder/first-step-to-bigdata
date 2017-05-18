#!/bin/bash
if [ $# -ne 1 ]
then
    echo 'usage run.sh process_num'
    exit 1
fi

for i in `seq 1 $1`
do
    echo 'starting process ' $i
    ./receive 1000000 > ./$i.log &
done
