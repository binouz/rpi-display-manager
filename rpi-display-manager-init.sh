#!/bin/sh

NAME="rpi-display-manager"
DAEMON="/usr/bin/rpi-display-manager"

case "$1" in
	start)
	        $DAEMON &
		;;

	stop)
		killall $NAME
		;;

	*)
		"Usage: $0 {start|stop}"
		exit 1
esac

exit 0
