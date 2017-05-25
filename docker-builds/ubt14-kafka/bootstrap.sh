#!/bin/bash
sudo /usr/sbin/sshd -D &
/usr/local/kafka/bin/kafka-server-start.sh -daemon /usr/local/kafka/config/server.properties
echo cmd is $1
$1
