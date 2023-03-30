#!/bin/bash

SDK_PACKAGE_NAME="antaris-payload-sdk"
BUILD_ROOT=$1
VERSION=`cat $BUILD_ROOT/VERSION`
echo "BUILD_ROOT for [$SDK_PACKAGE_NAME] is [$BUILD_ROOT] ."
OUTPUT_DIR="$BUILD_ROOT/output"
SDK_PACKAGE_ROOT_DIR="$OUTPUT_DIR/$SDK_PACKAGE_NAME/"
SDK_TGT_DIR="/opt/antaris/sdk/"
SDK_DEBIAN_DIR="$SDK_PACKAGE_ROOT_DIR/DEBIAN/"
SDK_LIST_FILE="$BUILD_ROOT/build-tools/scripts/sdk_pkg_files.txt"
FILE_LIST=`cat $SDK_LIST_FILE`
read -r -d '' PKG_CONTROL_CONTENTS <<  MESSAGE_END
Package: $SDK_PACKAGE_NAME  
Version: $VERSION  
Depends: python3, python3-pip
Maintainer: antaris  
Architecture: amd64  
Description: This is SDK package for payload application development.   
MESSAGE_END

read -r -d '' PRE_INSTALL_SCRIPT <<  MESSAGE_END
#!/bin/bash
MESSAGE_END

read -r -d '' POST_INSTALL_SCRIPT <<  MESSAGE_END
#!/bin/bash
pip3 install grpcio grpcio-tools 
echo "export PYTHONPATH=\"$PYTHONPATH:$SDK_TGT_DIR/lib/python/src:$SDK_TGT_DIR/lib/python/src/gen\" " >> ~/.bashrc
export PYTHONPATH="$PYTHONPATH:$SDK_TGT_DIR/lib/python/src:$SDK_TGT_DIR/lib/python/src/gen"
MESSAGE_END

echo  Control contents : "$PKG_CONTROL_CONTENTS"
rm -rf $SDK_PACKAGE_ROOT_DIR/*
rm -rf "$OUTPUT_DIR/$SDK_PACKAGE_NAME.deb"
mkdir -p "$SDK_PACKAGE_ROOT_DIR/$SDK_TGT_DIR"
mkdir -p "$SDK_DEBIAN_DIR"
echo "filelist : $FILE_LIST"
build-tools/scripts/place_files.sh "$SDK_LIST_FILE" "$SDK_PACKAGE_ROOT_DIR/$SDK_TGT_DIR"
touch $SDK_DEBIAN_DIR/control
echo  "$PKG_CONTROL_CONTENTS" > $SDK_DEBIAN_DIR/control
echo "$PRE_INSTALL_SCRIPT" > $SDK_DEBIAN_DIR/preinst
chmod 775 $SDK_DEBIAN_DIR/preinst
echo "$POST_INSTALL_SCRIPT" > $SDK_DEBIAN_DIR/postinst
chmod 775 $SDK_DEBIAN_DIR/postinst
cd $SDK_PACKAGE_ROOT_DIR/../ 

cd "$SDK_PACKAGE_ROOT_DIR/.."
dpkg-deb --build  "$SDK_PACKAGE_NAME"

if [ $? -eq 0 ]
then
	echo "Copied $SDK_PACKAGE_NAME.deb in to target directory. "
fi



