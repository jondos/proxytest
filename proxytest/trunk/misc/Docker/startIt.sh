#!/bin/sh
/usr/local/sbin/sockd -D
/usr/local/sbin/mix -c /etc/mix3.conf
/usr/local/sbin/mix -c /etc/mix2.conf
/usr/local/sbin/mix -c /etc/mix1.conf
exec /bin/sh