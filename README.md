####**my first step to big data**
&nbsp;&nbsp;&nbsp;&nbsp;In this project, I will build a distributed system to receive + process + restore + query big data(specifically speaking, location information of   
vehicles).
&nbsp;&nbsp;&nbsp;&nbsp;The distributed system will be built on a virtualization layer implemented by [Docker][1].
There will be a [Kafka][2] cluster to be used as a queue to receive and send location data; and a [spark][3] cluster to process those location data in real time; 
and a [hbase][4] cluster to restore those location data in database; and a [redis][5] cluster to serve as a cache for query from users.


[1]: https://www.docker.com/
[2]: https://kafka.apache.org/ 
[3]: http://spark.apache.org/
[4]: https://hbase.apache.org/
[5]: https://redis.io/


