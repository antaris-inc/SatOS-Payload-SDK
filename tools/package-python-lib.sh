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

BUILD_ROOT=`pwd`
mkdir -p $BUILD_ROOT/dist

apt-get update -y --fix-missing
apt-get upgrade -y

python3 -m pip install --upgrade pip
apt install -y python3.10-venv

cd lib/python
python3 -m pip install build

python3 -m build
mv dist/*.whl $BUILD_ROOT/dist
