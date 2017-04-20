first step to big data, 需要完成车辆轨迹数据的接收、处理、存储和检索。  

按照 setup.md 文档里的步骤搭建开发部署环境，在该环境之上，实现了如下工程：  
1. 模拟轨迹数据
projects/send-2-server 目录下为 c++ 实现的模拟轨迹数据程序. 使用make命令进行build  

2. 接收数据并存储入kafka   
projects/receive-2-kafka 目录下为 c++ 实现的接收存储程序，其中使用了开源库 [librdkafka][1]. 使用make命令进行build   

3. spark-streaming 读取kafka的数据，进行计算，并存储入hbase  
projects/spark-2-hbase 目录下为 scala 实现的处理程序，处理程序暂时并未对轨迹数据进行任何处理，而是直接存储入hbase.使用sbt clean compile package assembly 命令进行
构建、并打包二进制。

4. spark-streaming 读取kafka的数据，进行计算，并存储入redis  
projects/spark-2-redis 目录下为 scala 实现的处理程序，处理程序暂时并未对轨迹数据进行任何处理，而是直接存储入redis.使用sbt clean compile package assembly 命令进行
构建、并打包二进制。  


5. 从redis服务器中检索数据  
projects/query-redis 目录下为python 实现的检索程序，检索某个或某几个车辆的最近1000个定位点的轨迹  

6. 从hbase服务器中检索数据  
projects/query-hbase 目录下为python 实现的检索程序，检索某个或某几个车辆的历史轨迹  


[1]: https://github.com/edenhill/librdkafka
