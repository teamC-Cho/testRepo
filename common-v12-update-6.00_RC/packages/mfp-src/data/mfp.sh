#!/bin/sh
# /etc/init.d/mfp: start MFP
# Default-Start:    2 3 4 5
# Default-Stop:     0 1 6
#
# chkconfig: 2345 90 10
#Runlevel : 3 = S90
#Runlevel : 4 = S90
#Runlevel : 5 = S90
#Runlevel : 0 = K20
#Runlevel : 6 = K20
PATH=/bin:/usr/bin:/sbin:/usr/sbin

MFP_NAME="mfp"
MFPEXEC="/usr/local/bin/mfp"


test -f /usr/local/bin/mfp || exit 0


# Options for start/restart the daemons
#


#


case "$1" in
  start)
    echo -n "Starting MFP "
    start-stop-daemon --start --quiet --exec $MFPEXEC -n $MFP_NAME
    echo "."
    ;;
  stop)
    echo -n "Stopping MFP"
    start-stop-daemon --stop --quiet --exec $MFPEXEC -n $MFP_NAME --signal HUP
    echo "."
    ;;
  restart)
	echo -n "Restarting MFP"
	start-stop-daemon --stop --quiet --exec $MFPEXEC -n $MFP_NAME --signal HUP
   	sleep 1
   	start-stop-daemon --start --quiet --exec $MFPEXEC -n $MFP_NAME
   	echo "."
   	echo -n
   	;;
  *)
    echo "Usage: /etc/init.d/mfp.sh {start|stop|restart}"
    exit 1
esac

exit 0
