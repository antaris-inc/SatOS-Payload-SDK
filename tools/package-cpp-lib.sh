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

DEB_NAME="satos-payload-sdk-cpp"
BUILD_ROOT=`pwd`
VERSION=`cat $BUILD_ROOT/VERSION`
OUTPUT_DIR="$BUILD_ROOT/dist"
DEB_OUTPUT_DIR="$OUTPUT_DIR/$DEB_NAME"

mkdir -p $DEB_OUTPUT_DIR $DEB_OUTPUT_DIR/DEBIAN

# Copy files from lib/cpp into appropriate locations

mkdir -p $DEB_OUTPUT_DIR/lib/antaris
cp $BUILD_ROOT/lib/cpp/libantaris_api.a $DEB_OUTPUT_DIR/lib/antaris

mkdir -p $DEB_OUTPUT_DIR/lib/antaris/gen
cp $BUILD_ROOT/lib/cpp/gen/antaris_api.h $DEB_OUTPUT_DIR/lib/antaris/gen

mkdir -p $DEB_OUTPUT_DIR/lib/antaris/tools
cp $BUILD_ROOT/lib/python/satos_payload_sdk/antaris_api_gpio.py $DEB_OUTPUT_DIR/lib/antaris/tools
cp $BUILD_ROOT/lib/python/satos_payload_sdk/antaris_file_download.py $DEB_OUTPUT_DIR/lib/antaris/tools

mkdir -p $DEB_OUTPUT_DIR/lib/antaris/include
cp $BUILD_ROOT/lib/cpp/include/antaris_api_common.h $DEB_OUTPUT_DIR/lib/antaris/include
cp $BUILD_ROOT/lib/cpp/include/antaris_api_internal.h $DEB_OUTPUT_DIR/lib/antaris/include
cp $BUILD_ROOT/lib/cpp/include/antaris_cpp_standard_includes.h $DEB_OUTPUT_DIR/lib/antaris/include
cp $BUILD_ROOT/lib/cpp/include/antaris_pc_to_app_api.h $DEB_OUTPUT_DIR/lib/antaris/include
cp $BUILD_ROOT/lib/cpp/include/antaris_sdk_environment.h $DEB_OUTPUT_DIR/lib/antaris/include
cp $BUILD_ROOT/lib/cpp/include/antaris_api_gpio.h $DEB_OUTPUT_DIR/lib/antaris/include
cp $BUILD_ROOT/lib/cpp/include/antaris_can_api.h $DEB_OUTPUT_DIR/lib/antaris/include
cp $BUILD_ROOT/lib/cpp/include/antaris_api_pyfunctions.h $DEB_OUTPUT_DIR/lib/antaris/include
cp $BUILD_ROOT/vendor/cJSON/interface/cJSON.h $DEB_OUTPUT_DIR/lib/antaris/include

cp $BUILD_ROOT/Makefile.inc $DEB_OUTPUT_DIR/lib/antaris/include

# Set package metadata and build

cat << EOM > $DEB_OUTPUT_DIR/DEBIAN/control
Package: $DEB_NAME
Version: $VERSION
Maintainer: antaris
Architecture: amd64
Description: C++ SatOS payload application library
EOM

cd "$OUTPUT_DIR"
dpkg-deb --build "$DEB_NAME"

if [ $? -eq 0 ]
then
	echo "Packaged $DEB_NAME.deb successfully"
	rm -fr $DEB_NAME
fi
