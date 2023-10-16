#
#   Copyright 2022 Antaris, Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

# Use the official Ubuntu 22.04 base image
FROM ubuntu:22.04

# Update the package list and upgrade existing packages
RUN apt-get update && apt-get upgrade -y

# Install required packages
# heavy tools as a bottom layer for docker image-caching
RUN apt-get install -y wget gcc g++ gdb cmake golang protobuf-compiler autoconf automake libtool pkg-config git \
    libssl-dev libbz2-dev libffi-dev sudo tree zip texlive tcpdump net-tools dpkg-dev python3 python3-pip

# Install Google Test
RUN wget https://github.com/google/googletest/archive/release-1.12.1.tar.gz \
    && tar -xf release-1.12.1.tar.gz \
    && cd googletest-release-1.12.1 \
    && cmake -DBUILD_SHARED_LIBS=ON . \
    && make \
    && make install

# Clean up the package cache
RUN apt-get clean

# Upgrade pip for Python 3
RUN python3 -m pip install --upgrade pip

# Install grpcio and grpcio-tools
RUN python3 -m pip install grpcio grpcio-tools build

ARG ANTARIS_INSTALLATIONS_PATH=/usr/local/antaris

# install protoc
ADD https://github.com/protocolbuffers/protobuf/releases/download/v3.15.8/protoc-3.15.8-linux-x86_64.zip /tmp/.
RUN mkdir -p ${ANTARIS_INSTALLATIONS_PATH}/protoc && unzip /tmp/protoc-3.15.8-linux-x86_64.zip -d ${ANTARIS_INSTALLATIONS_PATH}/protoc/.

# Clean up
RUN rm -rf /tmp/*

SHELL ["/bin/bash", "-c"]
# c++
ARG GRPC_INSTALL_DIR=${ANTARIS_INSTALLATIONS_PATH}/grpc
RUN cd /root/ && git clone --recurse-submodules -b v1.46.3 --depth 1 --shallow-submodules https://github.com/grpc/grpc
RUN cd /root/grpc && mkdir -p cmake/build && pushd cmake/build && mkdir -p ${GRPC_INSTALL_DIR} && cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=${GRPC_INSTALL_DIR} \
      ../.. && make -j 4 && make install && popd

SHELL ["/bin/bash", "-c"]
# api-generator dependencies
RUN python3 -m pip install xmlschema

# documentation dependencies
RUN python3 -m pip install sphinx furo

RUN echo "PS1='🐳  \[\033[1;34m\]\u @ \[\033[1;36m\]\h \[\033[1;34m\]\W\[\033[0;35m\] \[\033[1;36m\] $ \[\033[0m\]'" >> .bashrc
RUN echo "export PATH=$PATH:${ANTARIS_GOPATH}/bin:${ANTARIS_INSTALLATIONS_PATH}/protoc" >> .bashrc
RUN echo "export GOPATH=$GOPATH:$ANTARIS_GOPATH" >> .bashrc
RUN echo "export PYTHONPATH=/workspace/lib/python/satos_payload_sdk" >> .bashrc
RUN echo "cat /etc/motd" >> .bashrc


CMD ["/bin/bash"]
WORKDIR /workspace
