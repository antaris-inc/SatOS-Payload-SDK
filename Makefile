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

.PHONY: no_default tools build_shell gen old_gen api_lib api_lib_clean gen_clean pc_sim pc_sim_clean sample_app sample_app_clean clean release_pkg sdk_pkg payload_app_pkg

ARCH=x86_64
SHELL := /bin/bash

BUILD_TOOLS_DIR=build-tools
BUILD_CONTAINER_DIR=${BUILD_TOOLS_DIR}/containers
DOCKER_FILE_BASE=${BUILD_CONTAINER_DIR}/Dockerfile.build.
DOCKERFILE := ${DOCKER_FILE_BASE}${ARCH}

#Default language - C++
LANGUAGE=cpp
LANGUAGE_HELP='Please specify a language for code generation [python/cpp].'

LIB_DIR=./lib
OUTPUT_GEN_DIR := ${LIB_DIR}/${LANGUAGE}/gen
OUTPUT_GEN_PYTHON_DIR := ${LIB_DIR}/python/src/gen
OUTPUT_GEN_CPP_DIR := ${LIB_DIR}/cpp/gen
OUTPUT_GEN_PROTO_DIR := defs/gen/proto
OUTPUT_GRPC_CPP_DIR = ${LIB_DIR}/cpp/gen/defs/gen/proto/
OUTPUT_LIB_DIR := ${LIB_DIR}/${LANGUAGE}
OUTPUT_RELEASE_DIR := release
ANTARIS_CPP_LIB := libantaris_api.a
#OUTPUT_CPP_LIB := ${OUTPUT_BASE_DIR}/lib/${LANGUAGE}/${ANTARIS_CPP_LIB}
CPP_APPS_DIR=apps/samples/cpp
SAMPLE_SRC_DIR := apps/samples/${LANGUAGE}/payload
PC_SIM_DIR := pc-sim

#PROTO_FILES := $(wildcard ${DEFS_DIR}/samples/*.proto ${OUTPUT_BASE_DIR}/gen/proto/*.proto)
PROTO_FILES := ${OUTPUT_GEN_PROTO_DIR}/antaris_api.proto
DEFS_DIR=${OUTPUT_GEN_PROTO_DIR}
CPP_LIB_DIR=lib/cpp
API_SPEC_GEN_TOOL=python3 ${BUILD_TOOLS_DIR}/scripts/types-generator/main.py
VERSION_GEN=${BUILD_TOOLS_DIR}/scripts/generate_build_info.sh
API_SPEC=./defs/api/antaris_api.xml
API_SPEC_SCHEMA=./defs/api/schema/antaris_api_schema.xsd
API_SPEC_GEN_BASE_OPTIONS=-i ${API_SPEC} -o ${LIB_DIR}/gen -s ${API_SPEC_SCHEMA}
GRPC_CPP_ADDITIONAL_INCLUDES=-I /usr/local/antaris/grpc/include/
GRPC_CPP_ADDITIONAL_LIBS=-L /usr/local/antaris/grpc/lib64/ -L /usr/local/antaris/grpc/lib/ -lprotobuf -lgrpc++ -lgrpc -lgrpc++_reflection -lgpr -lupb -labsl_bad_optional_access -labsl_cord -labsl_raw_logging_internal -labsl_cordz_info -labsl_cordz_handle -labsl_base -labsl_spinlock_wait -labsl_synchronization -labsl_base -labsl_malloc_internal -labsl_synchronization -labsl_symbolize -labsl_debugging_internal -labsl_demangle_internal -labsl_time -labsl_time_zone -labsl_int128 -labsl_graphcycles_internal -labsl_stacktrace -labsl_debugging_internal -labsl_cordz_functions -labsl_exponential_biased -labsl_cord_internal -labsl_throw_delegate -labsl_strings -labsl_strings_internal -labsl_status -labsl_str_format_internal -labsl_statusor -labsl_bad_variant_access -lre2 -lcares -laddress_sorting -labsl_hash -labsl_city -labsl_low_level_hash -labsl_random_internal_randen_slow -labsl_random_internal_platform -labsl_random_internal_randen_hwaes_impl -labsl_random_internal_pool_urbg -labsl_random_internal_seed_material -labsl_random_seed_gen_exception -labsl_random_internal_randen -labsl_random_internal_randen_hwaes -lpthread -lssl -lcrypto -lz

CONTAINER_IMAGE_NAME := payload_sdk_build_env_${ARCH}

DOCKER_BUILD=docker build --platform=linux/amd64
PYTHON_GEN=python3 -m grpc_tools.protoc
CPP_GEN=/usr/local/antaris/grpc/bin/protoc
GRPC_CPP_PLUGIN=/usr/local/antaris/grpc/bin/grpc_cpp_plugin
GOLANG_GEN=${BUILD_TOOLS_DIR}/scripts/gen_go.sh
RELEASE_PKG_CMD=${BUILD_TOOLS_DIR}/scripts/release.sh
RELEASE_FILES=${BUILD_TOOLS_DIR}/scripts/release_files.txt
SDK_PKG_CMD=${BUILD_TOOLS_DIR}/scripts/sdk_pkg.sh
SAMPLE_APP_PKG_CMD=${BUILD_TOOLS_DIR}/scripts/build_app_pkg.sh
DOCKER_RUN_CMD=docker run --platform=linux/amd64
DOCKER_EXEC_CMD=docker exec
DOCKER_RM_CMD=docker rm -f
WORKSPACE_MAPPING_DIR=/workspace
BUILD_CONTAINER_NAME=payload_sdk_build_env

no_default:
	@echo No default make target configured. Please proceed as per acommpanying documentation.

build_env:
	@${DOCKER_BUILD} --build-arg CONTAINER_USER=$(USER) --build-arg CONTAINER_UID=`id -u` --build-arg CONTAINER_GID=`id -g` -f ${DOCKERFILE} -t ${CONTAINER_IMAGE_NAME} .
	@${DOCKER_RM_CMD} ${BUILD_CONTAINER_NAME} 2>/dev/null
	@${DOCKER_RUN_CMD} -v `pwd`:${WORKSPACE_MAPPING_DIR} --rm --name ${BUILD_CONTAINER_NAME} -it ${CONTAINER_IMAGE_NAME} /bin/bash

build_env_shell:
	@${DOCKER_EXEC_CMD} -it ${BUILD_CONTAINER_NAME} /bin/bash

gen:
	@echo ">>>>>>> Translating API-spec to user-facing python interfaces >>>>>>>"
	${API_SPEC_GEN_TOOL} ${API_SPEC_GEN_BASE_OPTIONS} -o ${LIB_DIR} -l python
	@echo ">>>>>>>>>>> Translating API-spec to user-facing cpp interfaces >>>>>>>>>>"
	${API_SPEC_GEN_TOOL} ${API_SPEC_GEN_BASE_OPTIONS} -o ${LIB_DIR} -l cpp
	@echo ">>>>>>>>>>> Translating API-spec to network facing proto >>>>>>>>>>"
	${API_SPEC_GEN_TOOL} ${API_SPEC_GEN_BASE_OPTIONS} -o ${OUTPUT_GEN_PROTO_DIR} -l proto
	@mkdir -p ${OUTPUT_GEN_PYTHON_DIR} ${OUTPUT_GEN_CPP_DIR}
	@echo ">>>>>>>>>>>> Generating python GRPC sources from generated proto files <<<<<<<<<"
	${PYTHON_GEN} -I ${DEFS_DIR} --python_out=${OUTPUT_GEN_PYTHON_DIR} --grpc_python_out=${OUTPUT_GEN_PYTHON_DIR} ${PROTO_FILES}
	@echo ">>>>>>>>>>>> Generating cpp GRPC sources from generated proto files <<<<<<<<<"
	${CPP_GEN} --cpp_out=${OUTPUT_GEN_CPP_DIR} --grpc_out=${OUTPUT_GEN_CPP_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${PROTO_FILES}

api_lib:
	@echo Generating version info
	@${VERSION_GEN}
	@if [ "${LANGUAGE}" == "python" ]; then																													\
		echo nothing to build;								\
		#tree ${OUTPUT_LIB_DIR};							\
												\
	elif [ "${LANGUAGE}" == "cpp" ]; then																													\
		#mkdir -p ${OUTPUT_LIB_DIR} ${OUTPUT_BIN_DIR} ;																										\
		echo building cpp api library;																														\
		g++ -g -c ${OUTPUT_GEN_DIR}/antaris_api_autogen.cc -I ${CPP_LIB_DIR}/include ${GRPC_CPP_ADDITIONAL_INCLUDES} -I ${OUTPUT_GRPC_CPP_DIR} -I ${OUTPUT_GEN_DIR} -o ${OUTPUT_GEN_DIR}/antaris_api_autogen.o ;					\
		g++ -g -c ${OUTPUT_GRPC_CPP_DIR}/antaris_api.grpc.pb.cc ${GRPC_CPP_ADDITIONAL_INCLUDES} -I ${OUTPUT_GRPC_CPP_DIR} -I ${OUTPUT_GEN_CPP_DIR} -o ${OUTPUT_GRPC_CPP_DIR}/antaris_api.grpc.pb.o ;								\
		g++ -g -c ${OUTPUT_GRPC_CPP_DIR}/antaris_api.pb.cc ${GRPC_CPP_ADDITIONAL_INCLUDES} -I ${OUTPUT_GRPC_CPP_DIR} -I ${OUTPUT_GEN_CPP_DIR} -o ${OUTPUT_GRPC_CPP_DIR}/antaris_api.pb.o ;											\
		g++ -g -c ${CPP_LIB_DIR}/antaris_api_common.cc ${GRPC_CPP_ADDITIONAL_INCLUDES} -I ${CPP_LIB_DIR}/include -I ${OUTPUT_GRPC_CPP_DIR} -I ${OUTPUT_GEN_CPP_DIR} -o ${CPP_LIB_DIR}/antaris_api_common.o ;						\
		g++ -g -c ${CPP_LIB_DIR}/antaris_sdk_environment.cc ${GRPC_CPP_ADDITIONAL_INCLUDES} -I ${CPP_LIB_DIR}/include -I ${OUTPUT_GRPC_CPP_DIR} -I ${OUTPUT_GEN_CPP_DIR} -o ${CPP_LIB_DIR}/antaris_sdk_environment.o ;				\
		g++ -g -c ${CPP_LIB_DIR}/antaris_api_client.cc ${GRPC_CPP_ADDITIONAL_INCLUDES} -I ${OUTPUT_GRPC_CPP_DIR} -I ${CPP_LIB_DIR}/include -I ${OUTPUT_GEN_DIR} -o ${CPP_LIB_DIR}/antaris_api_client.o ;							\
		g++ -g -c ${CPP_LIB_DIR}/antaris_api_server.cc ${GRPC_CPP_ADDITIONAL_INCLUDES} -I ${OUTPUT_GRPC_CPP_DIR} -I ${CPP_LIB_DIR}/include -I ${OUTPUT_GEN_DIR} -o ${CPP_LIB_DIR}/antaris_api_server.o ;							\
		ar cr ${OUTPUT_LIB_DIR}/${ANTARIS_CPP_LIB} ${OUTPUT_GEN_DIR}/antaris_api_autogen.o ${CPP_LIB_DIR}/antaris_api_client.o ${CPP_LIB_DIR}/antaris_api_server.o ${OUTPUT_GRPC_CPP_DIR}/antaris_api.grpc.pb.o ${CPP_LIB_DIR}/antaris_api_common.o ${OUTPUT_GRPC_CPP_DIR}/antaris_api.pb.o ${CPP_LIB_DIR}/antaris_sdk_environment.o ; \
		tree ${OUTPUT_LIB_DIR};							\
		echo "content of api-lib ${OUTPUT_LIB_DIR}/${ANTARIS_CPP_LIB} ===>";	\
		ar -t ${OUTPUT_LIB_DIR}/${ANTARIS_CPP_LIB};				\
	elif [ "${LANGUAGE}" == "golang" ]; then																												\
		echo not implemented;																																\
		exit -1;																																			\
	else																																					\
		echo "Unknown LANGUAGE=${LANGUAGE}. ${LANGUAGE_HELP}";																								\
		exit -1;																																			\
	fi

api_lib_clean:
	rm -rf ${OUTPUT_GEN_DIR}/antaris_api_autogen.o ${OUTPUT_LIB_DIR}/${ANTARIS_CPP_LIB} ${OUTPUT_GRPC_CPP_DIR}/*.o

pc_sim:
	echo Linking PC simulator;																	\
	g++ -g ${PC_SIM_DIR}/antaris_pc_stub.cc ${PC_SIM_DIR}/config_db.cc -I ${OUTPUT_GEN_DIR} -I ${CPP_LIB_DIR}/include -L ${OUTPUT_LIB_DIR} -lantaris_api -lpthread ${GRPC_CPP_ADDITIONAL_LIBS} -o ${PC_SIM_DIR}/pc-sim ; 	\

	@tree ${PC_SIM_DIR}

sample_app:
	@if [ "${LANGUAGE}" == "python" ]; then																		\
		echo nothing to build;																			\
	elif [ "${LANGUAGE}" == "cpp" ]; then																		\
		mkdir -p ${OUTPUT_LIB_DIR} ;																\
		echo Linking Sample Application	;																	\
		g++ -g ${SAMPLE_SRC_DIR}/payload_app.cc -I ${SAMPLE_SRC_DIR}  -I ${CPP_LIB_DIR}/include -I ${OUTPUT_GEN_DIR} -L ${OUTPUT_LIB_DIR} -lantaris_api -lpthread ${GRPC_CPP_ADDITIONAL_LIBS} -o ${SAMPLE_SRC_DIR}/payload_app; 	\
	else																						\
		echo "Unknown LANGUAGE=${LANGUAGE}. ${LANGUAGE_HELP}";															\
		exit -1;																				\
	fi
	@tree apps/samples/cpp/payload
	@tree apps/samples/python

all: api_lib pc_sim sample_app

pc_sim_clean:
	rm -rf pc-sim/pc-sim
	rm -rf ${CPP_LIB_DIR}/*.o
	rm -rf ${CPP_LIB_DIR}/*.a

sample_app_clean:
	rm -rf ${SAMPLE_SRC_DIR}/payload_app
	rm -rf ${CPP_LIB_DIR}/*.o
	rm -rf ${CPP_LIB_DIR}/*.a

gen_clean:
	rm -rf lib/python/src/gen
	rm -rf lib/cpp/gen
	rm -rf defs/gen

clean:
	rm -rf pc-sim/pc-sim
	rm -rf ${SAMPLE_SRC_DIR}/payload_app
	rm -rf ${CPP_LIB_DIR}/*.o
	rm -rf ${CPP_LIB_DIR}/*.a
	rm -rf ${OUTPUT_RELEASE_DIR}
	rm -rf output

release_pkg:
	@${RELEASE_PKG_CMD} ${RELEASE_FILES}
	@tree ${OUTPUT_RELEASE_DIR}

sdk_pkg:
	${SDK_PKG_CMD} `pwd`

payload_app_pkg:
	${SAMPLE_APP_PKG_CMD} `pwd`

release: clean gen all release_pkg
