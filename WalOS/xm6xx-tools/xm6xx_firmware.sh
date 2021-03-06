#!/bin/bash

TOPDIR=`pwd`
FIRMWARE_PATH=$TOPDIR/firmware
PKG_TOOL_PATH=$TOPDIR/xm6xx-tools
PKG_DIRNAME=firmware


#fix me start
EMMC_PATH="$TOPDIR/../emmc"
VERSION="V3.0.1"
FIRMWARE_FILE="$EMMC_PATH/* $UPGRADE_FILE"

APP_TARGET="$TOPDIR/install/fdrserver"
APP_TARGET_INSTALL_PATH="$PKG_DIRNAME/usr"

echo "$FIRMWARE_FILE"
#fixme end

PKG_TAR_FILENAME=firmware.tar.gz
PKG_FINAL_FILENAME=firmware-"$VERSION".bin
PKG_HASHI_FILENAME=hash.txt


mkdir -p $FIRMWARE_PATH
cd $FIRMWARE_PATH
rm -rf $PKG_TAR_FILENAME $PKG_FINAL_FILENAME $PKG_HASHI_FILENAME $PKG_DIRNAME
mkdir -p $PKG_DIRNAME
cp -rf $FIRMWARE_FILE $PKG_DIRNAME/ 
cp $APP_TARGET $APP_TARGET_INSTALL_PATH/

RETVAL=$?
if [ $RETVAL -ne 0 ]; then
    exit 1;
fi

tar zcvf $PKG_TAR_FILENAME $PKG_DIRNAME

sha256sum $PKG_TAR_FILENAME | awk '{printf $1}' > $PKG_HASHI_FILENAME
cat $PKG_HASHI_FILENAME $PKG_TAR_FILENAME > $PKG_FINAL_FILENAME
#rm -f $PKG_TAR_FILENAME $PKG_HASHI_FILENAME


