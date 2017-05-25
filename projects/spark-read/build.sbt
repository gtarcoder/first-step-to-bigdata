import Dependencies._
      val hadoopVersion = "2.7.3"
      val hbaseVersion = "1.2.4"
      val sparkVersion = "2.1.0"
      val zkVersion = "3.4.6"

lazy val spark_redis = (project in file(".")).
  settings(
    inThisBuild(List(
      organization := "cn.pku",
      scalaVersion := "2.11.8",
      version      := "",
      name         := "Spark Kafka Read",
      libraryDependencies += "org.apache.spark" %% "spark-core" % sparkVersion,
      libraryDependencies += "org.apache.spark" %% "spark-streaming" % sparkVersion,
      libraryDependencies += "org.apache.spark" %% "spark-streaming-kafka" % "1.6.3"
      
      /*
      libraryDependencies += "RedisLabs" % "spark-redis" % "0.3.2" from "https://dl.bintray.com/spark-packages/maven/RedisLabs/spark-redis/0.3.2/spark-redis-0.3.2.jar"
      libraryDependencies += "RedisLabs" % "spark-redis" % "0.3.2"
      libraryDependencies += "org.apache.hbase" % "hbase-it" % hbaseVersion,
      libraryDependencies += "org.apache.hbase" % "hbase-prefix-tree" % hbaseVersion,
      libraryDependencies += "org.apache.hbase" % "hbase-hadoop-compat" % hbaseVersion,
      libraryDependencies += "org.apache.hbase" % "hbase-hadoop2-compat" % hbaseVersion,
      libraryDependencies += "org.apache.hbase" % "hbase-shell" % hbaseVersion,
      libraryDependencies += "org.apache.hbase" % "hbase-testing-util" % hbaseVersion,
      libraryDependencies += "org.apache.hbase" % "hbase-thrift" % hbaseVersion,
      libraryDependencies += "org.apache.hbase" % "hbase-protocol" % hbaseVersion
      */
    ))
  )
