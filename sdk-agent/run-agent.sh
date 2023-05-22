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

CONFIG_FILE="/opt/antaris/app/config.json"

RUN_PROXY=`jq --raw-output  '.Proxy.Run_Proxy' ${CONFIG_FILE}`
if [[ ${RUN_PROXY} != "1" ]]; then
	echo "Config indicates proxy should be disabled, exiting."
	exit 1
fi

PROXY_IP=`jq --raw-output  '.Proxy.Proxy_IP' ${CONFIG_FILE}`
PROXY_PORT=`jq --raw-output  '.Proxy.Proxy_Port' ${CONFIG_FILE}`

python3 /opt/antaris/sdk-agent/agent.py -m user -i ${PROXY_IP} -p ${PROXY_PORT} -s 127.0.0.1 -t 50051 -l 127.0.0.1 -o 50053
