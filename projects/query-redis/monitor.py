#!/usr/bin/env python
#coding = utf-8

import os, sys, redis
import time
import threading

gRedisHost = '172.17.0.5'
gDbIndex = 1


class MonitorThread(threading.Thread):
    def __init__(self, conn, zset_names, fixed_size):
        threading.Thread.__init__(self)
        self.redis_conn = conn
        self.zset_names = zset_names
        self.zset_fixed_size = fixed_size

    def run(self):
        while True:
            for zset in self.zset_names:
                self.keepFixedSize(self.redis_conn, zset, self.zset_fixed_size) 

        time.sleep(10)
        print 'thread %s is monitoring...' % self.name

    def keepFixedSize(self, redis_conn, zset_name, fixed_size):
        count = redis_conn.zcard(zset_name)
        if count > fixed_size:
            redis_conn.zremrangebyrank(zset_name, 0, int(count) - (fixed_size + 1))


def monitorDB(pool, fixed_size, thread_num):
    if not thread_num:
        print 'invalid thread num'
        return
    r = redis.Redis(connection_pool=pool)
    ret = r.keys()
    owned_zset_count = len(ret) / thread_num
    for th in range(0, thread_num):
        zset_names = ret[th*owned_zset_count : (th+1)*owned_zset_count if th < thread_num - 1 else len(ret)]   
        thread = MonitorThread(redis.Redis(connection_pool=pool), zset_names, fixed_size)
        thread.start()


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print 'usage : monitor.sh redis_host'
        exit(1)
    gRedisHost = sys.argv[1]
    print 'redis host is ', gRedisHost
    pool = redis.ConnectionPool(host=gRedisHost, port=6379, db=gDbIndex)
    monitorDB(pool, 5, 10)
     
