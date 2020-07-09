#!/bin/sh
/usr/sbin/squid
/usr/local/sbin/sockd -D
sleep 5
/usr/local/sbin/mix -c /etc/mix3.conf
sleep 2
/usr/local/sbin/mix -c /etc/mix2.conf
sleep 2
/usr/local/sbin/mix -c /etc/mix1.conf
