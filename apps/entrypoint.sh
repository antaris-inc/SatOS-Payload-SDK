#!/bin/bash
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
# NOTE FOR PAYLOAD APPLICATION DEVELOPERS
#
# This is a sample/example entrypoint.sh script for Payload developers.
# It can be customized for your payload application based on how
# you plan to run it in different modes (i.e. normal, upgrade, reset etc).
#
# Usage :
#
# Its a /etc/init.d script to launch payload application everytime hosting
# Virtual Machine (VM) boots up. Moreover, it can be used to run other
# operations instead of launching application, i.e. upgrade.
# The script takes input of operation (& its parameters if any) from
# the content of a file /opt/antaris/mode staged in the VM storage.
# Content of file /opt/antaris/mode is updated by payload specific
# telecommand sent/uploaded from Ground Station command center.
#
# Example :-
#
# 1. Using Ground Station command-center, a new package file
#    is uploaded in storage of payload application hosting VM.
#
# After file upload is completed.
#
# 2. A telecommand is issued to Spacecraft to writes desired
#    mode of operation (and parameters if any) in /opt/antaris/mode
#    file in the VM
#
# 3. A telecommand is issued to bring up payload application VM
#
# 4. /etc/init.d/entrypoint.sh will parse /opt/antaris/mode file
#    and determine desired operation-mode (& its parameters if any)
#    and calll appropriate (developer provided binary/script) to
#    do that mode of operation (i.e. run, upgrade, run in specific mode etc)

MODE_FILE="/opt/antaris/app/mode"
#Default operation
OP="normal"

start() {
	# Check the mode file to determine what operation to run
	if [ -f $MODE_FILE ]
	then
		OP=$(cat $MODE_FILE)
	fi

	case "${OP}" in
		normal)
		# Run Payload Application
		echo "Launching Payload Application"
		# <Launch your payload application from here>
		;;
		upgrade)
		# Upgrade Payload Application
		echo "Upgrading Payload Application"
		# <Inject code here to upgrade payload application>
		;;
		reset)
		# Reset Payload Application
		echo "Resetting Payload Application"
		# <Inject code here to reset payload application>
		;;
		*)
		# Error
		echo "ERROR : Invalid mode : ${OP}"
		;;
	esac
}

stop() {
	echo "Invoke application shutdown request from here"

}

case "$1" in
    start)
       start
       ;;
    stop)
       stop
       ;;
esac

exit 0
