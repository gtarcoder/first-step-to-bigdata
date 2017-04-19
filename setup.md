####**开发环境搭建**
&nbsp;&nbsp;&nbsp;&nbsp;first step to big data, 使用了3台高配物理主机，在物理主机上创建多个docker容器  
来搭建kafka集群、hadoop集群、hbase集群、spark集群和redis集群。  


#####**0. docker 安装和物理机设置**
&nbsp;&nbsp;&nbsp;&nbsp;在多台物理主机(我实际使用的是3台)上，分别安装docker。以centos 为例：
```
yum -y remove docker docker-common container-selinux
yum -y remove docker-selinux
yum install -y yum-utils
yum-config-manager \
            --add-repo \
                https://docs.docker.com/engine/installation/linux/repo_files/centos/docker.repo

yum makecache fast
yum -y install docker-engine
systemctl start docker  
systemctl enable docker  #设置成开机启动
```

&nbsp;&nbsp;&nbsp;&nbsp;实际使用3台物理主机搭建大数据处理集群，这三台机器的主机名和IP分别为  
| 主机名 | dev-11 | dev-12 | dev-13 |
|---- | ---- | ----- | -----|
|IP | 162.105.75.113 | 162.105.75.220| 162.105.75.66|  

&nbsp;&nbsp;&nbsp;&nbsp;由于三台机器上需要公用好多资源，所以在dev-11上设置了NFS共享目录，在dev-12和dev-13上挂在dev-11上的  
共享目录到本机的同名目录.(所以数据存放在 dev-11:/home/skc/workspace/graduate_design/， dev-12和dev-13可以将该共享目录挂载  
到本机的同名目录 )

#####**1. Docker 虚拟化层搭建**
&nbsp;&nbsp;&nbsp;&nbsp;使用docker的overlay网络，搭建跨主机的docker集群，使得一个主机上可以运行多个docker容器，而多个主机上的  
各个docker容器之间可以相互通信。
&nbsp;&nbsp;&nbsp;&nbsp;详细过程见博客 [docker应用-5 (使用overlay网络进行容器间跨物理主机通信)][1]

#####**2. hadoop 集群搭建**
&nbsp;&nbsp;&nbsp;&nbsp;使用docker镜像 debugman007/ubt14-hadoop-hbase-zk:v1， 含有hadoop，hbase，zookeeper.

######**2.1 创建容器**
dev-11 上创建一个hadoop-master节点
```
docker create -it --name master1 -h master1.multihost --net multihost\
           -v /home/skc/workspace/graduate_design/docker_data/hadoop/data:/home/hadoop/data \
           -v /home/skc/workspace/graduate_design/docker_data/hadoop/conf:/usr/local/hadoop/etc/hadoop\
           -v /home/skc/workspace/graduate_design/docker_data/hbase/conf:/usr/local/hbase/conf\
           -v /home/skc/workspace/graduate_design/docker_data/zookeeper/conf:/usr/local/zookeeper/conf\
           -v /etc/localtime:/etc/localtime \
           -P -p 50070:50070 -p 8088:8088 -p 50030:50030 -p 16010:16010 \
           debugman007/ubt14-hadoop-hbase-zk:v1 0
// 上面语句中末尾的0为启动项参数，表示zookeeper节点序号。如果没有，则表示不作为一个zookeeper节点
```

dev-12 上创建slave1节点
```
docker create -it --name slave1 -h slave1.multihost --net multihost \
           -v /home/skc/workspace/graduate_design/docker_data/hadoop/conf:/usr/local/hadoop/etc/hadoop\
           -v /home/skc/workspace/graduate_design/docker_data/hbase/conf:/usr/local/hbase/conf\
           -v /home/skc/workspace/graduate_design/docker_data/zookeeper/conf:/usr/local/zookeeper/conf\
           -v /etc/localtime:/etc/localtime \
           -P -p 50075:50075 -p 16030:16030 \
           debugman007/ubt14-hadoop-hbase-zk:v1 1
```

继续在 dev-13 上创建slave2, slave3 节点

######**2.2 启动容器**
```
docker start master1
docker attach master1
```

######**2.3 启动hadoop和hbase进程**
首先创建从master1到slave1,salve2,slave3三台机器的无密码登录，在master1上执行 ssh-copy-id grad@master1, 
    ssh-copy-id grad@slave1, ssh-copy-id grad@slave2,ssh-copy-id grad@slave3

(1) 启动zookeeper
在步骤2.1中创建的 master1, slave1, slave2 容器在启动的时候，会自动启动zookeeper进程。可以通过  
命令 zkServer.sh status 查看当前的zookeeper状态

(2) 启动 hadoop
(如果需要重新格式化namenode，则执行 hdfs namenode -format)  
到master1机器的 /usr/local/hadoop/sbin 目录下，执行 start-dfs.sh 和 start-yarn.sh。

(3) 启动 hbase
到master1机器中执行 start-hbase.sh

之后，在 master1,slave1,slave2,salve3上分别执行jps，查看是否已经启动了所需要的进程。在master1上应该存在进程  
SecondaryNameNode, NameNode, ResourceManager, HMaster
在slave1,slave2,slave3上应该存在进程 DataNode, NodeManager, HRegionServer. 在master1,slave1,slave2上应该存在进程QuorumPeerMain.  

#####**3. kafka集群搭建**
&nbsp;&nbsp;&nbsp;&nbsp;在 dev-11上搭建三台kafka 容器，使用镜像 debugman007/ubt14-kafka:v1  
```
docker create -it --name kafka1 -h kafka1 --net multihost -v /etc/localtime:/etc/localtime debugman007/ubt14-kafka:v1
docker create -it --name kafka2 -h kafka2 --net multihost -v /etc/localtime:/etc/localtime debugman007/ubt14-kafka:v1
docker create -it --name kafka3 -h kafka3 --net multihost -v /etc/localtime:/etc/localtime debugman007/ubt14-kafka:v1
```

启动并进入各个容器后，进行kafka的设置:  
```
broker=1 #kafka1，kafka2，kafka3分别不同
lsteners=PLAINTEXT://:9092
port=9092
host.name=kafka2
advertised.host.name=kafka2
zookeeper.connect=master1:2181,slave1:2181,slave2:2181 #使用之前搭建好的zookeeper
```
然后，启动kafka `kafka-server-start.sh -daemon /usr/local/kafka/config/server.properties`  

#####**4. spark集群搭建**
&nbsp;&nbsp;&nbsp;&nbsp;在dev-12上搭建三台 spark 机器, 使用镜像 debugman007/ubt14-yarn-spark:v1  
######**4.1 创建容器**
创建 spark-master1  
```
docker create -it --name spark-master1 -h spark-master1.multihost --net multihost \
           -p 8088:8088 -p 8042:8042 -p 4040:4040 \
           -v /home/skc/workspace/graduate_design/docker_data/spark/conf:/usr/local/spark/conf \
           -v /home/skc/workspace/graduate_design/docker_data/spark/yarn-conf:/usr/local/hadoop/etc/hadoop \
           -v /etc/localtime:/etc/localtime \
           debugman007/ubt14-yarn-spark:v1
```

创建 spark-slave1, spark-slave2  
```
docker create -it --name spark-slave1 -h spark-slave1.multihost --net multihost \
           -p 8089:8088 -p 8043:8042 -p 4041:4040 \
           -v /home/skc/workspace/graduate_design/docker_data/spark/conf:/usr/local/spark/conf \
           -v /home/skc/workspace/graduate_design/docker_data/spark/yarn-conf:/usr/local/hadoop/etc/hadoop \
           -v /etc/localtime:/etc/localtime \
           debugman007/ubt14-yarn-spark:v1
```

######**4.2 启动spark进程**
由于spark使用的是yarn-cluster的集群模式，需要运行hadoop的进程.   
(1) 格式化namenode  
`hdfs namenode -format`
(2) 启动hadoop进程  
`/usr/local/hadoop/sbin/start-all.sh`  
(3) 启动spark进程  
`/usr/local/spark/sbin/start-all.sh`  
之后，jps查看spark-master1上的进程应该有: NameNode, SecondaryNameNode, ResourceManager, Master.  
jps查看spark-slave1,spark-slave2上的进程应该有: DataNode, NodeManager, Worker  


#####**5. redis集群搭建**
&nbsp;&nbsp;&nbsp;&nbsp;在dev-13上搭建三台redis机器，使用镜像 debugman007/ubt14-redis:v1  
```
docker create -it --name redis1 -h redis1 --net multihost \
           -v /etc/localtime:/etc/localtime \
           debugman007/ubt14-redis:v1
```
debugman007/ubt14-redis:v1 镜像在容器启动的时候，就自动启动了redis server


[1]: http://www.cnblogs.com/gtarcoder/p/6425669.html 
