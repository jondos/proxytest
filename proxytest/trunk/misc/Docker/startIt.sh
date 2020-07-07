#!/bin/sh
/usr/sbin/squid
/usr/local/sbin/sockd -D
sleep 2
/usr/local/sbin/mix -c /etc/mix3.conf
/usr/local/sbin/mix -c /etc/mix2.conf
/usr/local/sbin/mix -c /etc/mix1.conf
