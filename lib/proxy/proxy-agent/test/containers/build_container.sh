#!/bin/bash

PROXY_DOCKERFILE=tools/proxy-agent/test/containers/Dockerfile
PROXY_CONTAINER_NAME=antaris_proxy_test

docker build -f ${PROXY_DOCKERFILE} -t ${PROXY_CONTAINER_NAME} .