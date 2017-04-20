####  测试步骤  

#####  0. 启动集群软件  
1. 启动hadoop/hbase集群的分布式进程，这可以在dev-11上的master1容器中完成；然后在master1上启动thrift server，用于外部访问hbase  
2. 启动spark集群的分布式进程，可以在dev-12上的spark-master1容器中完成  
3. 启动kafka集群的分布式进程，在dev-11上启动kafka1,kafka2,kafka3容器后，会自动启动kafka进程  
4. 启动redis集群的分布式进程，在dev-13上启动redis1,redis2,redis3容器后，会自动启动redis进程  


#####  1. 启动 receive-2-kafka程序  
receive-2-kafka程序用于接收UDP传过来的位置信息，然后将信息放入kafka集群中。**在dev-11上搭建kafka1,kafka2,kafka3三台机器的集群**，到receive-2-kafka 目录中执行
./receive kafka1:9092,kafka2:9092,kafka3:9092  bicycle_track
其中第一个参数为 kafka 的broker，需要在dev-11机器的/etc/hosts 文件中添加 kafka1，kafka2，kafka3三个容器的名称和IP的对应；第二个参数为使用的topic名称。  

#####  2. 启动 spark-2-redis /spark-2-hbase 程序  
spark-2-redis 和 spark-2-hbase 利用spark从kafka中获取数据，进行处理，然后存入hbase和redis。**在dev-12上搭建spark-master1,spark-slave1,spark-slave2三台机器的集群**，
将 hbase.sh 和 redis.sh 以及 spark-2-redis/target/scala-2.11/spark_redis-assembly-.jar 和 spark-2-hbase/target/scala-2.11/spark_hbase-assembly-.jar **拷贝到 spark-master1的 /home/grad 目录下，然后执行** ./hbase.sh 或者 ./redis.sh即可。  

#####  3. 启动 query-redis / query-hbase 程序  
quey-redis在 dev-13机器上执行（因为redis容器都在dev-13上）， query-hbase需要到dev-11上执行，因为thrift server在dev-11上的master1容器中。
在query-redis和query-hbase目录下，分别执行 python test.py 可以从redis和hbase中查询轨迹数据。注意test.py 文件中有写死的IP地址，他们需要根据实际部署情况进行相应的修改。  

#####  4. 启动 send-2-server 程序  
在另外一台机器上，到 send-2-server 目录下，执行 ./send 162.105.75.113 1000000 即可。其中162.105.75.113 为dev-11的IP地址，需要根据实际情况修改；1000000表示需要发送多少条记录。  


