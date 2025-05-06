#!/bin/bash

name=antarisproxy

CURRENT_DIR=`pwd`
SDK_ROOT_DIR="${name}" 
SDK_PACKAGE_NAME=${SDK_ROOT_DIR}
SDK_DEBIAN_DIR="${SDK_ROOT_DIR}/DEBIAN"
ARCH=$(dpkg --print-architecture)

VERSION="0.1"

ROOT_DIR="${SDK_ROOT_DIR}/"
PROXY_DIR="${ROOT_DIR}"

read -r -d '' PKG_CONTROL_CONTENTS <<  MESSAGE_END
Package: $SDK_PACKAGE_NAME  
Version: $VERSION  
Maintainer: antaris  
Architecture: $ARCH  
Description: Debian package to install the proxy tool.  
MESSAGE_END

echo  Control contents : "$PKG_CONTROL_CONTENTS"
mkdir -p ${SDK_ROOT_DIR} 
mkdir -p ${SDK_DEBIAN_DIR}
mkdir -p ${ROOT_DIR}

cp -r ${CURRENT_DIR}/proxy-agent ${ROOT_DIR}/
rm -rf ${ROOT_DIR}/proxy-agent/test

touch $SDK_DEBIAN_DIR/control
echo  "$PKG_CONTROL_CONTENTS" > $SDK_DEBIAN_DIR/control

dpkg-deb --build  "$SDK_PACKAGE_NAME"

if [ $? -eq 0 ]
then
        echo "$SDK_PACKAGE_NAME.deb build success."
fi

echo "Listing package contents"
dpkg -c $SDK_PACKAGE_NAME.deb


rm -rf ${SDK_ROOT_DIR}

