package grad
import java.util

import com.redislabs.provider.redis._
import kafka.serializer.{DefaultDecoder, StringDecoder}
import org.apache.spark.SparkContext
import org.apache.spark.SparkConf
import org.apache.spark.rdd.PairRDDFunctions
import org.apache.spark.storage.StorageLevel
import org.apache.spark.streaming.kafka.{HasOffsetRanges, KafkaUtils}
import org.apache.spark.streaming.{Duration, Time, Minutes, Seconds, Milliseconds, StreamingContext}
import scala.collection.mutable.ListBuffer

import java.text.SimpleDateFormat
import java.util.Date

object KafkaSparkRead
{

  def main(args: Array[String]): Unit =
  {
   if (args.length < 6) {
        System.err.println("Usage: KafkaSparkRedis <broker> <zookeeper> <topic> <redisDbIndex> <rdd time/ms> <kafka partition> <spark parallelism>")
        System.exit(1)
   }

    val Array(broker, zk, topic, rdd_time, kafka_partition, spark_parallelism) = args
        
    val sparkConf = new SparkConf().setAppName("BicycleTrackMonitor")
    val sc = new SparkContext(sparkConf)
    val ssc = new StreamingContext(sc, Milliseconds(rdd_time.toInt))

    val kafkaConf = Map("metadata.broker.list" -> broker,
                        "zookeeper.connect" -> zk,
                        "group.id" -> "kafka-spark-redis",
                        "zookeeper.connection.timeout.ms" -> "1000")
    /*
    val lines = KafkaUtils.createStream[Array[Byte], String, DefaultDecoder, StringDecoder](
      ssc, kafkaConf, Map(topic -> 1),
      StorageLevel.MEMORY_ONLY_SER).map(_._2)
    */

    val input_partitions = kafka_partition.toInt
    val streams = (1 to input_partitions) map { _ => 
        KafkaUtils.createStream[Array[Byte], String, DefaultDecoder, StringDecoder](ssc, kafkaConf, Map(topic -> 1), StorageLevel.MEMORY_ONLY_SER).map(_._2)
    }

    val lines = ssc.union(streams)
    lines.repartition(spark_parallelism.toInt)

    lines.foreachRDD(rdd => {
                if(!rdd.isEmpty()){
                    rdd.map(convert3).reduceByKey((x,y)=> x + y).collect().foreach(x => {
                        println("key = " + x._1 + ", record count = " + rdd.count() + ",  total delay = " + x._2 + " ms" + ", avg delay = " + x._2 / rdd.count() + " ms");} 
                        )
                    }
                    })

    /*
    lines.map(convert2).foreachRDD(rdd => {
                if(!rdd.isEmpty()){
                    val delay = 0
                    rdd.foreachPartition(partition => {
                        partition.foreach(record => {
                            val id = record._1
                            val time = record._2._1
                            val cur_time = (new Date()).getTime
                            

                            //println("write to redis :" + zset_name + ", " + info)
                        })
                    })
                }
            })
    */
    ssc.start()
    ssc.awaitTermination()
  }


  def getCurrentTimestamp(): String={
        val now = new Date()
        val str = now.getTime + ""
        str.substring(0,10)
  }

  def convert(t: (String)) = {
    val Array(id_time, longtitude, latitude, angle, velocity) = t.split(",")
    val Array(id, time) = id_time.split("@")
    (id, time + '@' + t)
  }

  def convert2(t: (String)) = {
    val Array(id_time, longtitude, latitude, angle, velocity) = t.split(",")
    val Array(id, time) = id_time.split("@")
    (id, (time.toDouble, t))
  }
  def convert3(t: (String)) = {
    val Array(id_time, longtitude, latitude, angle, velocity) = t.split(",")
    val Array(id, time) = id_time.split("@")
    val now = new Date().getTime
    ("hello", now - time.toDouble)
  }
}
