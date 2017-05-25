#!/bin/bash
if [ $# -ne 4 ]
then
    echo 'not enough arguments, usage: test.sh jarfile rdd_ms kafka_partition spark_parallelism'
    exit 1
else
    echo 'execute ' $1
fi
spark-submit --class grad.KafkaSparkRedis \
	--executor-cores 8 \
	--executor-memory 2G \
	--driver-memory 3G \
	--conf spark.default.parallelism=16 \
	--master spark://spark-master2:7077  $1 \
	kafka1:9092,kafka2:9092,kafka3:9092 \
	master1,slave1,slave2 \
	bicycle_track \
	1  \
	$2 \
	$3 \
	$4
	
