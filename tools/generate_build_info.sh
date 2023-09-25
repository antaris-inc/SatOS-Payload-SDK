#!/bin/bash -e

OUTPUT_C_HEADER=./lib/cpp/gen/antaris_sdk_version.h
OUTPUT_PYTHON_FILE=./lib/python/satos_payload_sdk/gen/antaris_sdk_version.py
VERSION_FILE=./VERSION

echo "
/*
 * Copyright Antaris Inc 2022
*/

/*
 * CAUTION: DO NOT CHANGE
 * This is an auto-generated file. Any manual changes to this file will neither be checked in nor persist till the next build.
 */

#ifndef __ANTARIS_SDK_VERSION_H__
#define __ANTARIS_SDK_VERSION_H__
" > ${OUTPUT_C_HEADER}

echo "
'''
Copyright Antaris Inc 2022

CAUTION: DO NOT CHANGE
This is an auto-generated file. Any manual changes to this file will neither be checked in nor persist till the next build.

'''
" > ${OUTPUT_PYTHON_FILE}

# build user
USER_INFO=$(id)
echo "
#define ANTARIS_SDK_BUILD_USER       \"${USER_INFO}\"
" >> ${OUTPUT_C_HEADER}

echo "
ANTARIS_SDK_BUILD_USER=\"${USER_INFO}\"
" >> ${OUTPUT_PYTHON_FILE}

# build timestamp
TIMESTAMP=$(date)
echo "
#define ANTARIS_SDK_BUILD_TIME       \"${TIMESTAMP}\"
" >> ${OUTPUT_C_HEADER}

echo "
ANTARIS_SDK_BUILD_TIME=\"${TIMESTAMP}\"
" >> ${OUTPUT_PYTHON_FILE}

# SDK version
PA_PC_SDK_VERSION=$(cat ${VERSION_FILE})
MAJOR_VERSION=$(echo ${PA_PC_SDK_VERSION} | cut -f 1 -d '.')
MINOR_VERSION=$(echo ${PA_PC_SDK_VERSION} | cut -f 2 -d '.')
PATCH_VERSION=$(echo ${PA_PC_SDK_VERSION} | cut -f 3 -d '.')
echo "
#define ANTARIS_PA_PC_SDK_MAJOR_VERSION             ${MAJOR_VERSION}
#define ANTARIS_PA_PC_SDK_MINOR_VERSION             ${MINOR_VERSION}
#define ANTARIS_PA_PC_SDK_PATCH_VERSION             ${PATCH_VERSION}
" >> ${OUTPUT_C_HEADER}

echo "
ANTARIS_PA_PC_SDK_VERSION='${PA_PC_SDK_VERSION}'
ANTARIS_PA_PC_SDK_MAJOR_VERSION=${MAJOR_VERSION}
ANTARIS_PA_PC_SDK_MINOR_VERSION=${MINOR_VERSION}
ANTARIS_PA_PC_SDK_PATCH_VERSION=${PATCH_VERSION}
" >> ${OUTPUT_PYTHON_FILE}

# file closing
echo "
#endif
" >> ${OUTPUT_C_HEADER}
