#!/usr/bin/env python
# coding=utf-8
import sys, os
import time, datetime

from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol
from thrift.transport import TTransport
from hbase import Hbase
from hbase.ttypes import *

#thrift server的ip地址。docker容器 master1中执行 hbase-daemon.sh start thrift， 可以启动thrift server
#此IP地址就是 master1的对外IP地址
gHbaseHost = '172.17.0.8'
gHbasePort = 9090

def openConnection(host, port):
    #connect to hbase thrift server
    transport = TTransport.TBufferedTransport(TSocket.TSocket(host, port))
    protocol = TBinaryProtocol.TBinaryProtocolAccelerated(transport)

    #create and open the client connection
    client = Hbase.Client(protocol)
    transport.open()
    return transport, client


def closeConnection(transport, client):
    transport.close()


def parseTrackInfo(tresult):
    id_time = tresult.row.split('@')
    longtitude = tresult.columns['position:longtitude'].value
    latitude = tresult.columns['position:latitude'].value
    angle = tresult.columns['move:angle'].value
    velocity = tresult.columns['move:velocity'].value

    time_write_2_redis = int(velocity.split('@')[1])
    time_current = int(1000*time.time())
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
                datetime.datetime.fromtimestamp(int(id_time[1])).strftime('%Y-%m-%d %H:%M:%S'), 
                longtitude,
                'E' if longtitude.find('-') < 0 else 'W',
                latitude,
                'N' if latitude.find('-') < 0 else 'S',
                angle,
                velocity.split('@')[0])
    '''

    delay_send_2_writeredis = time_write_2_redis - time_send
    delay_redis_write_2_read = time_current -  time_write_2_redis
    delay_send_2_readredis = time_current - time_send
    return [delay_send_2_writeredis, delay_redis_write_2_read, delay_send_2_readredis]

def query(client, bicycle_id):
    count = 0
    avg_delays = [0, 0, 0]
    while True:
        id = client.scannerOpenWithStop(tableName='bicycle_track',
                        startRow=bicycle_id+'@000000', 
                        stopRow=str(int(bicycle_id) + 1) +'@000000', 
                        columns=['position', 'move'],
                        attributes=None)
        try:
            result = client.scannerGetList(id, 100)
        except BaseException, m:
            print 'exception occured', m
            time.sleep(10)
            continue
            
        if len(result) == 0:
            time.sleep(1)
            continue
        
        delays = parseTrackInfo(result[-1])
        for x in range(0, 3):
            avg_delays[x] += delays[x]
        count += 1

        if count % 100 == 0:
            print 'count = %d' % count
            print 'average delay from send to write redis = %d\naverage delay from write to read redis = %d\n\
average delay from send to read from redis is %d'%(avg_delays[0]/count, avg_delays[1]/count, avg_delays[2]/count)
            count = 0
            avg_delays = [0, 0, 0]


if __name__ == '__main__':
    transport, client = openConnection(gHbaseHost, gHbasePort)
    query(client, '1000001')
    closeConnection(transport, client)
