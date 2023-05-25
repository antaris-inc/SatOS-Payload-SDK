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

from quay.io/centos/centos:8

RUN cd /etc/yum.repos.d/
RUN sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-*
RUN sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*

RUN dnf -y upgrade

RUN yum -y install epel-release
RUN yum repolist

RUN dnf makecache --refresh

# heavy tools as a bottom layer for docker image-caching
RUN dnf -y install wget gcc gcc-c++ gdb cmake protobuf-compiler autoconf automake libtool pkg-config git openssl-devel bzip2-devel libffi-devel

# install gtest
RUN wget https://github.com/google/googletest/archive/release-1.12.1.tar.gz
RUN tar -xf release-1.12.1.tar.gz
RUN cd googletest-release-1.12.1 && cmake -DBUILD_SHARED_LIBS=ON . && make && make install

# more tools go here
RUN dnf -y install sudo tree zip texlive-latex tcpdump net-tools dpkg

RUN yum -y clean all

#install Python 3.10
RUN cd /tmp/;\
      wget https://www.python.org/ftp/python/3.10.6/Python-3.10.6.tgz ; \
      tar -xvf Python-3.10.6.tgz; \
      cd Python-3.10.6;\
      ./configure;      \
      make; \
      make test;  \
      make install;     
RUN python3 -m pip install --upgrade pip

# install grpc components, see https://grpc.io/docs/languages/
# python grpc
RUN python3 -m pip install build grpcio grpcio-tools

ARG ANTARIS_INSTALLATIONS_PATH=/usr/local/antaris

# install protoc
ADD https://github.com/protocolbuffers/protobuf/releases/download/v3.15.8/protoc-3.15.8-linux-x86_64.zip /tmp/.
RUN mkdir -p ${ANTARIS_INSTALLATIONS_PATH}/protoc && unzip /tmp/protoc-3.15.8-linux-x86_64.zip -d ${ANTARIS_INSTALLATIONS_PATH}/protoc/.

# c++
ARG GRPC_INSTALL_DIR=${ANTARIS_INSTALLATIONS_PATH}/grpc
RUN cd /root/ && git clone --recurse-submodules -b v1.46.3 --depth 1 --shallow-submodules https://github.com/grpc/grpc
RUN cd /root/grpc && mkdir -p cmake/build && pushd cmake/build && mkdir -p ${GRPC_INSTALL_DIR} && cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=${GRPC_INSTALL_DIR} \
      ../.. && make -j 4 && make install && popd

# api-generator dependencies
RUN python3 -m pip install xmlschema

# documentation dependencies
RUN python3 -m pip install sphinx furo

RUN echo "export PATH=$PATH:${ANTARIS_INSTALLATIONS_PATH}/protoc" >> .bashrc

CMD ["/bin/bash"]
WORKDIR /workspace
