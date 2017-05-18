#!/bin/bash
if [ $# -ne 1 ]
then
    echo 'usage run.sh process_num'
    exit 1
fi

for i in `seq 1 $1`
do
    echo 'starting process ' $i
    ./receive kafka1:9092,kafka2:9092,kafka3:9092 bicycle_track > /dev/null &
done
