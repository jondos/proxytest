#!/bin/sh
#
# Start/Stop the jondonym mix server

### BEGIN INIT INFO
# Provides: mix
# Required-Start: $network
# Required-Stop: $network
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: jondonym mix server

### END INIT INFO


set -e

DESC="JonDonym mix server"
NAME="mix"
DAEMON="/usr/sbin/mix"
PIDFILE="/var/run/mix.pid"
MIXCONF="/etc/mix/config.xml"
MAX_FILEDESCRIPTORS=32768


case $1 in
  start)
	if [ -f $CONF ] ; then
	  echo -n "Raising maximum number of filedescriptors to $MAX_FILEDESCRIPTORS"
	  if ulimit -n "$MAX_FILEDESCRIPTORS" ; then
		echo "."
	  else
		echo ": FAILED."
	  fi
	  echo -n "Starting $DESC..." 
	  start-stop-daemon --start --quiet --pidfile $PIDFILE --exec $DAEMON -- -c $MIXCONF --pidfile=$PIDFILE
	  echo "done."
	else
	  echo "Mix config not found! Do not start mix server."
	;;

  stop)
	echo "Stopping $DESC: $NAME"
	start-stop-daemon --stop --pidfile $PIDFILE --exec $DAEMON
	;;

  status)
	ret=0
	status_of_proc -p $PIDFILE $DAEMON 2>/dev/null || ret=$?
	;;

  reload|force-reload|restart)
	$0 stop
	sleep 1
	$0 start
	;;

  *)
	echo "Usage: $0 (start|stop|reload|force-reload|restart|status)" >&2
	exit 1
	;;
esac

exit 0
