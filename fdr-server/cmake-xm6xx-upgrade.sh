#!/bin/bash

TARGET="build/bin/fdrserver"
INSTALL_PATH="install"
PKGTOOL_PATH="xm6xx-tools"
PKGTOOL_NAME="xm6xx_pkg.sh"

cp $TARGET $INSTALL_PATH/
$PKGTOOL_PATH/$PKGTOOL_NAME
