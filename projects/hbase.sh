#!/bin/bash

spark-submit --class grad.KafkaSparkHbase \
        --master spark://spark-master1:7077  spark_hbase-assembly-.jar \
	kafka1:9092,kafka2:9092,kafka3:9092 \
	master1,slave1,slave2 \
	bicycle_track \
	hdfs://master1:8020/hbase \
	bicycle_track

