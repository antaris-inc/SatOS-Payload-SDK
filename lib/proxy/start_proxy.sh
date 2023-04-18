#!/bin/bash

CONFIG_DIR="/opt/antaris/app"
JQ="jq"

if ! which $JQ  > /dev/null; then
        echo "$JQ Command not found! Installing "
        sudo apt install -y $JQ
fi

RUN_PROXY=`jq --raw-output  '.Proxy.Run_Proxy' ${CONFIG_DIR}/config.json`

if [[ ${RUN_PROXY} == "1" ]]; then
	PROXY_IP=`jq --raw-output  '.Proxy.Proxy_IP' ${CONFIG_DIR}/config.json`
	PROXY_PORT=`jq --raw-output  '.Proxy.Proxy_Port' ${CONFIG_DIR}/config.json`
	/usr/bin/python3 /opt/antaris/proxy/proxy-agent/agent.py -m user -i ${PROXY_IP} -p ${PROXY_PORT} -s 127.0.0.1 -t 50051 -l 127.0.0.1 -o 50053
else
	echo "Run_Proxy is disabled in config.json file"
fi
