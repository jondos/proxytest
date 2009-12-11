#! /bin/sh
#
# skeleton	example file to build /etc/init.d/ scripts.
#		This file should be used to construct scripts for /etc/init.d.
#
#		Written by Miquel van Smoorenburg <miquels@cistron.nl>.
#		Modified for Debian 
#		by Ian Murdock <imurdock@gnu.ai.mit.edu>.
#
# Version:	@(#)skeleton  1.9  26-Feb-2001  miquels@cistron.nl
#

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/mix
NAME=mix
DESC=mix
MIXCONFDIR=/etc/mix

test -x $DAEMON || exit 0

# Include mix defaults if available
if [ -f /etc/default/mix ] ; then
	. /etc/default/mix
fi

set -e

case "$1" in
  start)
	echo -n "Starting $DESC with: "
	FOUND=""
	for CONF in $MIXCONFDIR/*
	do
	    if [ -r $CONF ]; then
		FOUND="true"
		DAEMON_OPTS="-c $CONF"
		CONFFILE="`basename $CONF`"
		DAEMON_OPTS="$DAEMON_OPTS --pidfile /var/run/$NAME-$CONFFILE.pid"
		start-stop-daemon --start --quiet --exec $DAEMON -- $DAEMON_OPTS
		echo -n "$CONF;"
	    fi
	done
	echo ""
	if [ -z $FOUND ]; then
	    echo "Warning: No Mix started, because no readable configuration file found in $MIXCONFDIR !" 
	fi    
	;;
  stop)
	echo -n "Stopping $DESC with: "
	for CONF in /var/run/$NAME-*	
	do
	    if [ -r $CONF ]; then
		start-stop-daemon --stop --quiet --oknodo --pidfile $CONF \
		--exec $DAEMON
		echo -n "$CONF;"
	    fi	
	done
	echo ""
	;;
  #reload)
	#
	#	If the daemon can reload its config files on the fly
	#	for example by sending it SIGHUP, do it here.
	#
	#	If the daemon responds to changes in its config file
	#	directly anyway, make this a do-nothing entry.
	#
	# echo "Reloading $DESC configuration files."
	# start-stop-daemon --stop --signal 1 --quiet --pidfile \
	#	/var/run/$NAME.pid --exec $DAEMON
  #;;
  restart)
	#
	#	If the "reload" option is implemented, move the "force-reload"
	#	option to the "reload" entry above. If not, "force-reload" is
	#	just the same as "restart".
	#
	echo "Restarting $DESC: "
	/etc/init.d/$NAME stop
	sleep 1
	/etc/init.d/$NAME start
	;;
  *)
	N=/etc/init.d/$NAME
	# echo "Usage: $N {start|stop|restart|reload|force-reload}" >&2
	echo "Usage: $N {start|stop|restart}" >&2
	exit 1
	;;
esac

exit 0
