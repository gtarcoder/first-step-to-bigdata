import Dependencies._
      val hadoopVersion = "2.7.3"
      val hbaseVersion = "1.2.4"
      val sparkVersion = "2.1.0"
      val zkVersion = "3.4.6"

lazy val spark_hbase = (project in file(".")).
  settings(
    inThisBuild(List(
      organization := "cn.pku",
      scalaVersion := "2.11.8",
      version      := "",
      name         := "Spark Kafka HBase",
      libraryDependencies += "org.apache.spark" %% "spark-core" % sparkVersion,
      libraryDependencies += "org.apache.spark" %% "spark-streaming" % sparkVersion,
      libraryDependencies += "org.apache.spark" %% "spark-streaming-kafka" % "1.6.3",

      libraryDependencies += "org.apache.hadoop" % "hadoop-common" % hadoopVersion,
      libraryDependencies += "org.apache.hadoop" % "hadoop-client" % hadoopVersion,

      libraryDependencies += "org.apache.hbase" % "hbase-common" % hbaseVersion,
      libraryDependencies += "org.apache.hbase" % "hbase-client" % hbaseVersion,
      libraryDependencies += "org.apache.hbase" % "hbase-server" % hbaseVersion,
      libraryDependencies += "org.apache.hbase" % "hbase" % hbaseVersion
      /*
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
