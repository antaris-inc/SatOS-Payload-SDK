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

CONFIG_DIR="/opt/antaris/app"
JQ="jq"

function wait_till_proxy_connect() {

	echo "Waiting for proxy connection "
	sleep 2
	while true; do
		line=`tail -1 /opt/antaris/logs/proxy.log`
		if [[ "$line" == "INFO:__main__:Connecting to atmos-agent, will retry till this works" ]]; then
			echo "Waiting for proxy connect .... "
			sleep 2
		else
			break;
		fi
	done

}	

if ! which $JQ  > /dev/null; then
        echo "$JQ Command not found! Installing "
        sudo apt install -y $JQ
fi

RUN_PROXY=`jq --raw-output  '.Proxy.Run_Proxy' ${CONFIG_DIR}/config.json`

if [[ ${RUN_PROXY} == "1" ]]; then
	PROXY_IP=`jq --raw-output  '.Proxy.Proxy_IP' ${CONFIG_DIR}/config.json`
	PROXY_PORT=`jq --raw-output  '.Proxy.Proxy_Port' ${CONFIG_DIR}/config.json`
	mkdir -p /opt/antaris/logs
	/usr/bin/python3 /opt/antaris/proxy/proxy-agent/agent.py -m user -i ${PROXY_IP} -p ${PROXY_PORT} -s 127.0.0.1 -t 50051 -l 127.0.0.1 -o 50053 &> /opt/antaris/logs/proxy.log &
	wait_till_proxy_connect
	echo "Proxy connected"
	echo "You may run payload application now"
else
	echo "Run_Proxy is disabled in config.json file"
fi
