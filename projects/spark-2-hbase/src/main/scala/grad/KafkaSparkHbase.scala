package grad
import java.util

import kafka.serializer.{DefaultDecoder, StringDecoder}
import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.hbase.HBaseConfiguration
import org.apache.hadoop.hbase.client.{HBaseAdmin,HTable,Put,Get}
import org.apache.hadoop.hbase.io.ImmutableBytesWritable
import org.apache.hadoop.hbase.mapreduce.TableOutputFormat
import org.apache.hadoop.hbase.util.Bytes
import org.apache.hadoop.io.{LongWritable, Writable, IntWritable, Text}
import org.apache.hadoop.mapred.{TextOutputFormat, JobConf}
import org.apache.hadoop.mapreduce.lib.output.TextOutputFormat
import org.apache.spark.SparkConf
import org.apache.spark.rdd.PairRDDFunctions
import org.apache.spark.storage.StorageLevel
import org.apache.spark.streaming.kafka.{HasOffsetRanges, KafkaUtils}
import org.apache.spark.streaming.{Minutes, Seconds, StreamingContext}
import scala.collection.mutable.ListBuffer
import java.text.SimpleDateFormat
import java.util.Date

object KafkaSparkHbase
{

  def main(args: Array[String]): Unit =
  {
    if (args.length < 5) {
        System.err.println("Usage: KafkaSparkHbase <broker> <zookeeper> <topic> <hbase.root.dir> <htableName>")
        System.exit(1)
    }

    val Array(broker, zk, topic, hbaseRootDir, htableName) = args

    val sparkConf = new SparkConf().setAppName("BicycleTrackMonitor")
    val ssc = new StreamingContext(sparkConf, Seconds(2))

    val kafkaConf = Map("metadata.broker.list" -> broker,
                        "zookeeper.connect" -> zk,
                        "group.id" -> "kafka-spark-hbase",
                        "zookeeper.connection.timeout.ms" -> "1000")

    val lines = KafkaUtils.createStream[Array[Byte], String, DefaultDecoder, StringDecoder](
      ssc, kafkaConf, Map(topic -> 1),
      StorageLevel.MEMORY_ONLY_SER).map(_._2)
   

    lines.foreachRDD ( rdd => {
      if (!rdd.isEmpty){
        val conf = HBaseConfiguration.create()
        conf.set(TableOutputFormat.OUTPUT_TABLE, htableName)
        conf.set("hbase.zookeeper.quorum", zk)
        conf.set("hbase.zookeeper.property.clientPort", "2181")
        conf.set("hbase.rootdir", hbaseRootDir)
        conf.set("hbase.cluster.distributed", "true")

        val jobConf = new Configuration(conf)
        jobConf.set("mapreduce.job.output.key.class", classOf[Text].getName)
        jobConf.set("mapreduce.job.output.value.class", classOf[Text].getName)
        jobConf.set("mapreduce.outputformat.class", classOf[TableOutputFormat[Text]].getName)
     
        rdd.map(convert).saveAsNewAPIHadoopDataset(jobConf)
      }
    })

    ssc.start()
    ssc.awaitTermination()
  }

  def getCurrentTimestamp(): String={
        val now = new Date()
        val str = now.getTime + ""
        str //milliseconds 
        //str.substring(0,10)
  }


  def convert(t: (String)) = {
    val Array(id_time, longtitude, latitude, angle, velocity) = t.split(",")
    val p = new Put(Bytes.toBytes(id_time))
    p.add(Bytes.toBytes("position"), Bytes.toBytes("longtitude"), Bytes.toBytes(longtitude))
    p.add(Bytes.toBytes("position"), Bytes.toBytes("latitude"), Bytes.toBytes(latitude))
    p.add(Bytes.toBytes("move"), Bytes.toBytes("angle"), Bytes.toBytes(angle))
    p.add(Bytes.toBytes("move"), Bytes.toBytes("velocity"), Bytes.toBytes(velocity + '@' + getCurrentTimestamp()))
    (id_time, p)
  }
}
