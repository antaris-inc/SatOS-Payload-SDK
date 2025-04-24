#!/bin/bash -e

# Copyright 2024 Your Name/Organization

DEB_NAME="grpc-dev"
BUILD_ROOT=$(pwd)
VERSION="1.46.3-custom" # Adjust version as needed, should match sdk_tools' version. 
OUTPUT_DIR="$BUILD_ROOT/dist"
DEB_OUTPUT_DIR="$OUTPUT_DIR/$DEB_NAME"
ANTARIS_INSTALLATIONS_PATH="/usr/local/antaris"
GRPC_INSTALL_DIR="${ANTARIS_INSTALLATIONS_PATH}/grpc"

mkdir -p "$DEB_OUTPUT_DIR" "$DEB_OUTPUT_DIR/DEBIAN"

# Copy gRPC libraries and headers
mkdir -p "$DEB_OUTPUT_DIR/usr/local/antaris/grpc"
cp -r "$GRPC_INSTALL_DIR/"* "$DEB_OUTPUT_DIR/usr/local/antaris/grpc"

ARCH=`$(dpkg --print-architecture)`

# Set package metadata and build
cat << EOM > $DEB_OUTPUT_DIR/DEBIAN/control
Package: $DEB_NAME
Version: $VERSION
Maintainer: antaris
Architecture: ${ARCH}
Description: gRPC development libraries and headers
EOM

cd "$OUTPUT_DIR"
dpkg-deb --build "$DEB_NAME" 

if [ $? -eq 0 ]; then
    echo "Packaged $DEB_NAME.deb successfully"
    rm -fr "$DEB_NAME"
else
    echo "Packaging $DEB_NAME.deb failed"
    exit 1
fi