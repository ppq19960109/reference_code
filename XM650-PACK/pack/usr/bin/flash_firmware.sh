#!/bin/sh

EMMC_PATH=/var/sd
FIRMWARE_PATH=/var/sd/firmware
APP_PATH=/var/sd/app

SRC_FILENAME=firmware.bin
DEST_FILENAME=firmware.tar.gz
HASH_FILENAME=hash.txt
HEAD_SIZE=64
BUSYBOX="$EMMC_PATH/busybox"
SHA256SUM="$BUSYBOX sha256sum"
TAR="$BUSYBOX tar"
DD="$BUSYBOX dd"

clean()
{
    echo "clean..."
    cd $EMMC_PATH
    rm -rf $SRC_FILENAME $DEST_FILENAME  $HASH_FILENAME $FIRMWARE_PATH
    sync
}


echo "**************************************************"
echo "flash firmware start..."
cd $EMMC_PATH
if [ -e $SRC_FILENAME ]; then
    echo "checking..."
else
    echo "firmware file not exist."
    clean
    exit 1
fi
chmod u+x $BUSYBOX
$DD if=$SRC_FILENAME of=$HASH_FILENAME bs=$HEAD_SIZE  count=1 skip=0 seek=0
$DD if=$SRC_FILENAME of=$DEST_FILENAME bs=$HEAD_SIZE  skip=1 seek=0

HASH_CHECKSUM1=`$SHA256SUM $DEST_FILENAME | awk '{printf $1}'`
HASH_CHECKSUM2=`cat $HASH_FILENAME`
echo $HASH_CHECKSUM1
echo $HASH_CHECKSUM2
if [ "$HASH_CHECKSUM1" != "$HASH_CHECKSUM2" ]; then
    echo "checksum failed."
    clean
    exit 1
fi

$TAR zxvf $DEST_FILENAME
RET=$?
if [ $RET -ne 0 ]; then
    echo "tar failed."
    clean
    exit 1
fi

rm -rf $APP_PATH
#mkdir -p $APP_PATH
#cp -r $FIRMWARE_PATH/* $APP_PATH/
mv $FIRMWARE_PATH $APP_PATH
sync
clean

echo "flash done"
echo "**************************************************"
reboot
exit 0

