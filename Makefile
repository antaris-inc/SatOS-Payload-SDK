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

.PHONY: no_default pc_submodule_tools build_cpp gen api_lib api_lib_clean gen_clean clean python_package cpp_package agent_package docs

include Makefile.inc
ARCH=x86_64
SHELL := /bin/bash

BUILD_TOOLS_DIR=tools
BUILD_CONTAINER_DIR=${BUILD_TOOLS_DIR}/containers
DOCKER_FILE_BASE="images/sdk-tools"
DOCKERFILE := ${DOCKER_FILE_BASE}/Dockerfile

OPTIMIZATION_LEVEL := -g

#Default language - C++
LANGUAGE=cpp
LANGUAGE_HELP='Please specify a language for code generation [python/cpp].'

LIB_DIR=./lib
OUTPUT_GEN_DIR := ${LIB_DIR}/${LANGUAGE}/gen
OUTPUT_GEN_PYTHON_DIR := ${LIB_DIR}/python
OUTPUT_GEN_CPP_DIR := ${LIB_DIR}/cpp/gen
OUTPUT_GEN_PROTO_DIR := defs/gen/proto
OUTPUT_GRPC_CPP_DIR = ${LIB_DIR}/cpp/gen/defs/gen/proto/
OUTPUT_LIB_DIR := ${LIB_DIR}/${LANGUAGE}
ANTARIS_CPP_LIB := libantaris_api.a
#OUTPUT_CPP_LIB := ${OUTPUT_BASE_DIR}/lib/${LANGUAGE}/${ANTARIS_CPP_LIB}
CPP_APPS_DIR=apps/samples/cpp

#PROTO_FILES := $(wildcard ${DEFS_DIR}/samples/*.proto ${OUTPUT_BASE_DIR}/gen/proto/*.proto)
PROTO_FILES := ${OUTPUT_GEN_PROTO_DIR}/antaris_api.proto
DEFS_DIR=${OUTPUT_GEN_PROTO_DIR}
CPP_LIB_DIR=lib/cpp
API_SPEC_GEN_TOOL=python3 ${BUILD_TOOLS_DIR}/types-generator/main.py
VERSION_GEN=${BUILD_TOOLS_DIR}/generate_build_info.sh
API_SPEC=./defs/api/antaris_api.xml
API_SPEC_SCHEMA=./defs/api/schema/antaris_api_schema.xsd
API_SPEC_GEN_BASE_OPTIONS=-i ${API_SPEC} -o ${LIB_DIR}/gen -s ${API_SPEC_SCHEMA}
VENDOR_cJSON_INCLUDES= -I vendor/cJSON/interface/
VENDOR_LIB_DIR=vendor
CONTAINER_IMAGE_NAME := payload_sdk_build_env_${ARCH}

DOCKER_BUILD=docker build --platform=linux/amd64
PYTHON_GEN=python3 -m grpc_tools.protoc
CPP_GEN=/usr/local/antaris/grpc/bin/protoc
GRPC_CPP_PLUGIN=/usr/local/antaris/grpc/bin/grpc_cpp_plugin
DOCKER_RUN_CMD=docker run --platform=linux/amd64
DOCKER_EXEC_CMD=docker exec
DOCKER_RM_CMD=docker rm -f
WORKSPACE_MAPPING_DIR=/workspace
BUILD_CONTAINER_NAME=payload_sdk_build_env

# To help in building sample cpp application which is located inside examples/app-cpp.
SRC_DIR = examples/app-cpp
SRCS = $(wildcard $(SRC_DIR)/*.cc)
# Define the target executable
TARGET = $(SRC_DIR)/payload_app

no_default:
	@echo No default make target configured. Please proceed as per acommpanying documentation.

pc_submodule_tools:
	@${DOCKER_BUILD} --build-arg CONTAINER_USER=$(USER) --build-arg CONTAINER_UID=`id -u` --build-arg CONTAINER_GID=`id -g` -f ${DOCKERFILE} -t ${CONTAINER_IMAGE_NAME} .

build_cpp:
	@${DOCKER_BUILD} --build-arg CONTAINER_USER=$(USER) --build-arg CONTAINER_UID=`id -u` --build-arg CONTAINER_GID=`id -g` -f ${DOCKERFILE} -t ${CONTAINER_IMAGE_NAME} .
	@${DOCKER_RM_CMD} ${BUILD_CONTAINER_NAME} 2>/dev/null
	@${DOCKER_RUN_CMD} -v `pwd`:${WORKSPACE_MAPPING_DIR} --rm --name ${BUILD_CONTAINER_NAME} -it ${CONTAINER_IMAGE_NAME} make cpp_example

gen:
	@echo ">>>>>>> Translating API-spec to user-facing python interfaces >>>>>>>"
	${API_SPEC_GEN_TOOL} ${API_SPEC_GEN_BASE_OPTIONS} -o ${LIB_DIR} -l python
	@echo ">>>>>>>>>>> Translating API-spec to user-facing cpp interfaces >>>>>>>>>>"
	${API_SPEC_GEN_TOOL} ${API_SPEC_GEN_BASE_OPTIONS} -o ${LIB_DIR} -l cpp
	@echo ">>>>>>>>>>> Translating API-spec to network facing proto >>>>>>>>>>"
	${API_SPEC_GEN_TOOL} ${API_SPEC_GEN_BASE_OPTIONS} -o ${OUTPUT_GEN_PROTO_DIR} -l proto
	@mkdir -p ${OUTPUT_GEN_CPP_DIR}
	@echo ">>>>>>>>>>>> Generating python GRPC sources from generated proto files <<<<<<<<<"
	${PYTHON_GEN} -I satos_payload_sdk/gen=${DEFS_DIR} --python_out=${OUTPUT_GEN_PYTHON_DIR} --grpc_python_out=${OUTPUT_GEN_PYTHON_DIR} ${PROTO_FILES}
	@echo ">>>>>>>>>>>> Generating cpp GRPC sources from generated proto files <<<<<<<<<"
	${CPP_GEN} --cpp_out=${OUTPUT_GEN_CPP_DIR} --grpc_out=${OUTPUT_GEN_CPP_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${PROTO_FILES}

#################### C++ API dependencies ########################
CPP_OBJS = \
    ${VENDOR_LIB_DIR}/cJSON/src/cJSON.o \
    ${CPP_LIB_DIR}/antaris_api_gpio.o \
    ${OUTPUT_GEN_DIR}/antaris_api_autogen.o \
    ${OUTPUT_GRPC_CPP_DIR}/antaris_api.grpc.pb.o \
    ${OUTPUT_GRPC_CPP_DIR}/antaris_api.pb.o \
    ${CPP_LIB_DIR}/antaris_api_common.o \
    ${CPP_LIB_DIR}/antaris_sdk_environment.o \
    ${CPP_LIB_DIR}/antaris_api_client.o \
    ${CPP_LIB_DIR}/antaris_api_server.o \
    ${CPP_LIB_DIR}/antaris_api_pyfunctions.o

${OUTPUT_LIB_DIR}/${ANTARIS_CPP_LIB}: $(CPP_OBJS)
	ar cr $@ $^

%.o: %.cc
	g++ ${OPTIMIZATION_LEVEL} -c $< ${GRPC_CPP_ADDITIONAL_INCLUDES} -I ${CPP_LIB_DIR}/include -I ${OUTPUT_GRPC_CPP_DIR} -I ${OUTPUT_GEN_CPP_DIR} -I /usr/include/python3.10 -o $@

%.o: %.c
	gcc ${OPTIMIZATION_LEVEL} -c $< ${VENDOR_cJSON_INCLUDES} -o $@
###################################################################

api_lib:
	@echo Generating version info
	@echo "GRPC CPP I : ${GRPC_CPP_ADDITIONAL_INCLUDES} , VENDOR LIB : ${VENDOR_LIB_DIR}"
	@${VERSION_GEN}
	@if [ "${LANGUAGE}" == "python" ]; then \
		echo "Nothing to build."; \
		tree ${OUTPUT_LIB_DIR}; \
	elif [ "${LANGUAGE}" == "cpp" ]; then \
		echo "Building C++ API library..."; \
		$(MAKE) -j $(OUTPUT_LIB_DIR)/$(ANTARIS_CPP_LIB); \
		tree ${OUTPUT_LIB_DIR}; \
		echo "Contents of ${OUTPUT_LIB_DIR}/${ANTARIS_CPP_LIB}:"; \
		ar -t ${OUTPUT_LIB_DIR}/${ANTARIS_CPP_LIB}; \
	else \
		echo "Unknown LANGUAGE=${LANGUAGE}. ${LANGUAGE_HELP}"; \
		exit -1; \
	fi

api_lib_clean:
	rm -rf ${OUTPUT_GEN_DIR}/antaris_api_autogen.o ${OUTPUT_LIB_DIR}/${ANTARIS_CPP_LIB} ${OUTPUT_GRPC_CPP_DIR}/*.o

agent_package:
	./tools/package-agent.sh

python_package:
	./tools/package-python-lib.sh

cpp_package:
	./tools/package-grpc-lib.sh
	./tools/package-cpp-lib.sh

docs:
	sphinx-build docs/src dist/docs

cpp_example: all
	$(info    SRC_DIR is $(SRC_DIR))
	g++ ${OPTIMIZATION_LEVEL} -o $(TARGET)  $(SRCS) ${VENDOR_cJSON_INCLUDES} -I /usr/local/include/ -I ${CPP_LIB_DIR}/include -I ${OUTPUT_GEN_DIR} -L ${OUTPUT_LIB_DIR} -lantaris_api -lpthread ${GRPC_CPP_ADDITIONAL_LIBS} -lpython3.10;

all: api_lib pc_sim sample_app

gen_clean:
	rm -rf lib/python/satos_payload_sdk/gen
	rm -rf lib/cpp/gen
	rm -rf defs/gen

clean:
	rm -rf ${CPP_LIB_DIR}/*.o
	rm -rf ${CPP_LIB_DIR}/*.a
	rm -rf output
