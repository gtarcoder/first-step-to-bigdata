#!/usr/bin/env python
# coding=utf-8

import os, sys, redis
import time, datetime
import threading

#redis 的IP地址，这里使用容器 redis2 的IP地址  
gRedisHost = '172.17.0.6'
#redis 数据库序号
gDbIndex = 1

def parseTrackInfo(info):
    info_arr = info.split(',')
    if len(info_arr) != 5:
        print 'invalid track info %s' % info
        return None
    id_time = info_arr[0].split('@')
    if (len(id_time) != 2):
        print 'invalide track info %s' % info
        return None

    time_writeredis = int(info_arr[4].split('@')[1])
    time_current = int(time.time())
    time_send = int(id_time[1])
    '''
    print 'current time is %s, trackinfo pushed into redis time is %s\n\
bicycle %s\'s track status at time %s is : \n\
                 longtitude : %s %s\n \
                latitude   : %s %s\n \
                angle      : %s\n \
                velocity   : %s\n' % (
                datetime.datetime.now(),
                datetime.datetime.fromtimestamp(time_write_2_redis).strftime('%Y-%m-%d %H:%M:%S'), 
                id_time[0], 
                datetime.datetime.fromtimestamp(time_send).strftime('%Y-%m-%d %H:%M:%S'), 
                info_arr[1],
                'E' if info_arr[1].find('-') < 0 else 'W',
                info_arr[2],
                'N' if info_arr[2].find('-') < 0 else 'S',
                info_arr[3],
                info_arr[4].split('@')[0])
    '''
    delay_send_2_writeredis = time_writeredis - time_send
    delay_redis_write_2_read = time_current -  time_writeredis
    delay_send_2_readredis = time_current - time_send
    return [delay_send_2_writeredis, delay_redis_write_2_read, delay_send_2_readredis]

def query(redis_conn, bicycle_id):
    zset_name = 'info_' + bicycle_id
    avg_delays = [0, 0, 0]
    count = 0
    while True:
        ret = redis_conn.zrange(zset_name, 0, -1)  
        if len(ret):
            delays = parseTrackInfo(ret[-1])
            for x in range(0, 3):
                avg_delays[x] += delays[x]
            count += 1
        if count == 100:
            print 'count = %d' % count
            print 'average delay from send to write redis = %d\naverage delay from write to read redis = %d\n\
average delay from send to read from redis is %d'%(avg_delays[0]/count, avg_delays[1]/count, avg_delays[2]/count)
            avg_delays = [0, 0, 0]
            count = 0

if __name__ == '__main__':
    pool = redis.ConnectionPool(host=gRedisHost, port=6379, db=gDbIndex)
    query(redis.Redis(connection_pool=pool), '1000016') 
