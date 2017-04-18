import sys, os
import time 
from datetime import datetime, timedelta

from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol
from thrift.transport import TTransport
from hbase import Hbase
from hbase.ttypes import *

def Open(host, port):
    #connect to hbase thrift server
    transport = TTransport.TBufferedTransport(TSocket.TSocket(host, port))
    protocol = TBinaryProtocol.TBinaryProtocolAccelerated(transport)

    #create and open the client connection
    client = Hbase.Client(protocol)
    transport.open()
    return transport, client


def Close(transport, client):
    transport.close()


def test(host, port):
    bicycle_id = 1000100
    transport, client = Open(host, port)

    while True:
        avg_delay = 100000000000000000
        end_time = datetime.now()
        start_time = end_time - timedelta(seconds=1000) 
        end_time = int(time.mktime(end_time.timetuple()))
        start_time = int(time.mktime(start_time.timetuple()))

        id = client.scannerOpenWithStop(tableName='bicycle_track',
                    startRow=str(bicycle_id)+'@'+str(start_time), 
                    stopRow=str(bicycle_id)+'@'+str(end_time), 
                    columns=['position'], 
                    attributes=None)

        result = client.scannerGetList(id, 100000)
        if len(result):
            timestamp = result[-1].row
            #print timestamp, end_time, end_time - int(timestamp.split('@')[-1])
            avg_delay = min(avg_delay, end_time - int(timestamp.split('@')[-1]))
        print 'average delay = ', avg_delay

    Close(transport, client)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print 'usage: query host port'
        exit(1)

    test(sys.argv[1], int(sys.argv[2]))
