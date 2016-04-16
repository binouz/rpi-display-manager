#!/bin/sh

NAME="rpi-display-manager"
DAEMON="/usr/bin/rpi-display-manager"

case "$1" in
	start)
		params=`grep -Eo 'disp=[^[:blank:]]*' /proc/cmdline`
		if [ $? -eq 0 ]; then
			params=`echo $params | cut -d '=' -f2 | sed -e 's/,/ /g'`

			for p in ${params}; do
				echo "params: ${p}"

				if [ ${p} = "1080p" ]; then
					echo "Enabling 1080p"
					export HI379X_DISP_WIDTH=1920
					export HI379X_DISP_HEIGHT=1080
				elif [ ${p} = "native" ]; then
					echo "Enabling sink native mode"
					export HI379X_DISP_USE_NATIVE=1
				fi
			done
		fi

		killall $NAME
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
