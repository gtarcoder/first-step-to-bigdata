#!/bin/bash
if [ $# -ne 2 ]
then
    echo 'usage run.sh process_num packet_num'
    exit 1
else 
    echo 'execute ' $1 ' processes, ' $2 ' packets each process' 
fi

for i in `seq 1 $1`
do
    echo 'starting process ' $i
    ./kafka_writer $2 > ./$i.log &
done
