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

if [ "${RUN_SDK_AGENT}" != "" ]; then

	if [ "${CONFIG}" == "" ]; then
		echo "CONFIG must be set"
		exit 2
	fi

	CONFIG_ABS=`pwd`/${CONFIG}

	rm -fr /tmp/sdk-agent-init/
	mkdir -p /tmp/sdk-agent-init/
	cd /tmp/sdk-agent-init/
	unzip ${CONFIG_ABS}
	sudo dpkg -i *.deb

	sudo chown -R ${USER}:root /opt/antaris/
	sudo chown -R ${USER}:root /workspace

	sudo /opt/antaris/sdk-agent/run-agent.sh &> /opt/antaris/logs/sdk-agent.log &
fi

CONFIG_FILE=/opt/antaris/app/config.json
if [ ! -f "${CONFIG_FILE}" ]; then
	echo "config file not found at ${CONFIG_FILE}"
	exit 1
fi

# execute entrypoint based on config
ENTRYPOINT_FILE=`jq --raw-output  '.Storage.Entrypoint_File' ${CONFIG_FILE}`
$ENTRYPOINT_FILE
