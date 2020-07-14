#!/bin/sh

mkdir -p /var/sd
telnetd

/usr/bin/fdrdeamon &

RET=0
while true
do
    mount -t ext4 /dev/mmcblk0 /var/sd
    RET=$?
    if [ 0 = $RET ]; then
     	break
    fi
    sleep 2
done


STARTUP_SCRIPT=/var/sd/app/bin/startup.sh
if [ -e $STARTUP_SCRIPT ]; then
    . $STARTUP_SCRIPT
fi

