package grad
import java.util

//import grad.RedisClient
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

object KafkaSparkRedis
{

  def main(args: Array[String]): Unit =
  {
    if (args.length < 4) {
        System.err.println("Usage: KafkaSparkRedis <broker> <zookeeper> <topic> <redisDbIndex>")
        System.exit(1)
   }

    val Array(broker, zk, topic, dbIndex) = args
        
    val sparkConf = new SparkConf().setAppName("BicycleTrackMonitor")
    val sc = new SparkContext(sparkConf)
    val ssc = new StreamingContext(sc, Seconds(3))

    val kafkaConf = Map("metadata.broker.list" -> broker,
                        "zookeeper.connect" -> zk,
                        "group.id" -> "kafka-spark-redis",
                        "zookeeper.connection.timeout.ms" -> "1000")

    val lines = KafkaUtils.createStream[Array[Byte], String, DefaultDecoder, StringDecoder](
      ssc, kafkaConf, Map(topic -> 1),
      StorageLevel.MEMORY_ONLY_SER).map(_._2)

    lines.map(convert2).foreachRDD(rdd => {
                //sortByKey().
                if(!rdd.isEmpty()){
                        

                    rdd.foreachPartition(partition => {

                        val jedis = RedisClient.pool.getResource
                        //使用1 号数据库
                        jedis.select(dbIndex.toInt)
                        val pl = jedis.pipelined()

                        partition.foreach(record => {
                            val id = record._1
                            val time = record._2._1
                            val info = record._2._2 + '@' + getCurrentTimestamp()
                            val zset_name = "info_" + id
                            //add to zset
                            pl.zadd(zset_name, time, info) 
                            //println("write to redis :" + zset_name + ", " + info)
                        })

                        pl.sync()
                        RedisClient.pool.returnResource(jedis)

                    })

                }
            })

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
}
