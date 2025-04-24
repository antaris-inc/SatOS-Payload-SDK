#!/bin/bash -e
#
# Copyright 2023 Antaris, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

DEB_NAME="satos-payload-sdk-agent"
BUILD_ROOT=`pwd`
VERSION=`cat $BUILD_ROOT/VERSION`
OUTPUT_DIR="$BUILD_ROOT/dist"
DEB_OUTPUT_DIR="$OUTPUT_DIR/$DEB_NAME"

mkdir -p $DEB_OUTPUT_DIR $DEB_OUTPUT_DIR/DEBIAN

# Copy files from into appropriate locations

mkdir -p $DEB_OUTPUT_DIR/opt/antaris/sdk-agent
cp $BUILD_ROOT/sdk-agent/run-agent.sh $DEB_OUTPUT_DIR/opt/antaris/sdk-agent/
cp $BUILD_ROOT/lib/proxy/proxy-agent/agent.py $DEB_OUTPUT_DIR/opt/antaris/sdk-agent/
cp $BUILD_ROOT/lib/proxy/proxy-agent/hexdump.py $DEB_OUTPUT_DIR/opt/antaris/sdk-agent/
cp $BUILD_ROOT/lib/proxy/proxy-agent/socket_proxy.py $DEB_OUTPUT_DIR/opt/antaris/sdk-agent/

ARCH=`$(dpkg --print-architecture)`

# Set package metadata and build

cat << EOM > $DEB_OUTPUT_DIR/DEBIAN/control
Package: $DEB_NAME
Version: $VERSION
Maintainer: antaris
Architecture: ${ARCH}
Description: SatOS Payload SDK agent
Depends: python3, unzip, jq
EOM

cd "$OUTPUT_DIR"
dpkg-deb --build "$DEB_NAME"

if [ $? -eq 0 ]
then
	echo "Packaged $DEB_NAME.deb successfully"
	rm -fr $DEB_NAME
fi

