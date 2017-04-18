#!/usr/bin/env python
#coding = utf-8

import os, sys, redis
import time, datetime
import threading

gRedisHost = '172.17.0.4'
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
    print 'bicycle %s\'s track status at time %s is : \n\
                 longtitude : %s %s\n \
                latitude   : %s %s\n \
                angle      : %s\n \
                velocity   : %s\n' % (id_time[0], 
                datetime.datetime.fromtimestamp(int(id_time[1])).strftime('%Y-%m-%d %H:%M:%S'), 
                info_arr[1],
                'E' if info_arr[1].find('-') < 0 else 'W',
                info_arr[2],
                'N' if info_arr[2].find('-') < 0 else 'S',
                info_arr[3],
                info_arr[4])



def query(redis_conn, bicycle_id):
    zset_name = 'info_' + bicycle_id
    ret = redis_conn.zrange(zset_name, 0, -1)    
    for info in ret:
        parseTrackInfo(info)

if __name__ == '__main__':
    pool = redis.ConnectionPool(host=gRedisHost, port=6379, db=gDbIndex)
    argc = len(sys.argv)
    if argc == 1:
        print 'usage : query bicycle_id1 bicycle_id2 ....'
        exit(1)
    else:
        for bid in sys.argv[1:]:
            query(redis.Redis(connection_pool=pool), bid) 
