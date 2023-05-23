#!/bin/bash

PACKAGE_NAME="antaris-application-pkg"
BUILD_ROOT=$1
VERSION=`cat $BUILD_ROOT/VERSION`
echo "BUILD_ROOT for [$PACKAGE_NAME] is [$BUILD_ROOT] ."
OUTPUT_DIR="$BUILD_ROOT/output"
PACKAGE_ROOT_DIR="$OUTPUT_DIR/$PACKAGE_NAME/"
PKG_TGT_DIR="/opt/antaris/"
PKG_DEBIAN_DIR="$PACKAGE_ROOT_DIR/DEBIAN/"
PKG_LIST_FILE="$BUILD_ROOT/build-tools/scripts/app_pkg_files.txt"
FILE_LIST=`cat $PKG_LIST_FILE`

read -r -d '' PKG_CONTROL_CONTENTS <<  MESSAGE_END
Package: $PACKAGE_NAME  
Version: $VERSION  
Maintainer: antaris  
Architecture: amd64  
Description: This is package providing sample payload application.   
MESSAGE_END

read -r -d '' PRE_INSTALL_SCRIPT <<  MESSAGE_END
#!/bin/bash
# Nothing to do
MESSAGE_END

read -r -d '' POST_INSTALL_SCRIPT <<  MESSAGE_END
#!/bin/bash
mv $PKG_TGT_DIR/entrypoint.sh /etc/init.d
MESSAGE_END

# clean older package
rm -rf $PACKAGE_ROOT_DIR/*
rm -rf "$OUTPUT_DIR/$PACKAGE_NAME.deb"

# Keep package files at specified locations
mkdir -p "$PACKAGE_ROOT_DIR/$PKG_TGT_DIR"
mkdir -p "$PKG_DEBIAN_DIR"
build-tools/scripts/place_files.sh "$PKG_LIST_FILE" "$PACKAGE_ROOT_DIR/$PKG_TGT_DIR"

# Control file and scripts
touch $PKG_DEBIAN_DIR/control
echo  "$PKG_CONTROL_CONTENTS" > $PKG_DEBIAN_DIR/control
echo "$PRE_INSTALL_SCRIPT" > $PKG_DEBIAN_DIR/preinst
chmod 775 $PKG_DEBIAN_DIR/preinst
echo "$POST_INSTALL_SCRIPT" > $PKG_DEBIAN_DIR/postinst
chmod 775 $PKG_DEBIAN_DIR/postinst
cd $PACKAGE_ROOT_DIR/../ 

# build package
cd "$PACKAGE_ROOT_DIR/.."
dpkg-deb --build  "$PACKAGE_NAME"

if [ $? -eq 0 ]
then
	echo "Copied $PACKAGE_NAME.deb in to target directory. "
fi
