import sys, os
import time, datetime

from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol
from thrift.transport import TTransport
from hbase import Hbase
from hbase.ttypes import *

#thrift server的ip地址。docker容器 master1中执行 hbase-daemon.sh start thrift， 可以启动thrift server
#此IP地址就是 master1的对外IP地址
gHbaseHost = '172.17.0.2'
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

    print 'bicycle %s\'s track status at time %s is : \n\
                 longtitude : %s %s\n \
                latitude   : %s %s\n \
                angle      : %s\n \
                velocity   : %s\n' % (id_time[0], 
                datetime.datetime.fromtimestamp(int(id_time[1])).strftime('%Y-%m-%d %H:%M:%S'), 
                longtitude,
                'E' if longtitude.find('-') < 0 else 'W',
                latitude,
                'N' if latitude.find('-') < 0 else 'S',
                angle,
                velocity)

    
def query(client, bicycle_id):
    id = client.scannerOpenWithStop(tableName='bicycle_track',
                    startRow=bicycle_id+'@000000', 
                    stopRow=str(int(bicycle_id) + 1) +'@000000', 
                    columns=['position', 'move'],
                    attributes=None)
    result = []
    while True:
        ret = client.scannerGetList(id, 100000)
        result += ret
        if len(ret) == 0:
            break
    for tresult in result:
        parseTrackInfo(tresult)
        


if __name__ == '__main__':
    if len(sys.argv) == 1:
        print 'usage: query bicycle_id1 bicycle_id2 ....'
        exit(1)
    else:
        transport, client = openConnection(gHbaseHost, gHbasePort)
        for bid in sys.argv[1:]:
            query(client, bid)
        closeConnection(transport, client)


