#!/bin/bash

PC_SERVER="nc -l 127.0.0.1 50051"
PC_SERVER_LABEL="PC"
APP_CALLBACK_SERVER="nc -l 127.0.0.1 50053"
APP_CALLBACK_LABEL="CALLBACK_SERVER"
API_CLIENT="nc 127.0.0.1 50051"
API_CLIENT_LABEL="APP"
CALLBACK_CLIENT="nc 127.0.0.1 50053"
CALLBACK_CLIENT_LABEL="CALLBACK_CLIENT"

SETTLIING_TIME=4

source ./utils/test_utils.sh

start_containers

start_atmos_process_with_io_files ${PC_SERVER_LABEL} ${PC_SERVER}
start_user_process_with_io_files ${APP_CALLBACK_LABEL} ${APP_CALLBACK_SERVER}
start_user_process_with_io_files ${API_CLIENT_LABEL} ${API_CLIENT}
start_atmos_process_with_io_files ${CALLBACK_CLIENT_LABEL} ${CALLBACK_CLIENT}

APP_TO_PC_API="hello"
PC_TO_APP_API_RESPONSE="world"
PC_TO_APP_CALLBACK="howdy"
APP_TO_PC_CALLBACK_RESPONSE="planet"

log_error_and_exit() {
    msg=$1

    echo "!!!!!!!!!!! $1 !!!!!!!!!!!!" >&2
    log_everything >&2
    echo "!!!!!!!!!!! $1 !!!!!!!!!!!!" >&2

    exit -1
}

# App to PC api path

echo "APP to PC api path check"

echo "Writing ${APP_TO_PC_API} to ${API_CLIENT_LABEL} input"
write_to_user_input ${API_CLIENT_LABEL} ${APP_TO_PC_API}

echo "Allowing settling time"
sleep ${SETTLIING_TIME}

echo "Reading received content on PC api server side"
PC_RECEIVED_STRING=$(read_from_atmos_output ${PC_SERVER_LABEL})

echo "From ${PC_SERVER_LABEL} received content \"${PC_RECEIVED_STRING}\""

if [ "${PC_RECEIVED_STRING}" = "${APP_TO_PC_API}" ]
then
    echo "Content matched - APP to PC api path OK"
else
    log_error_and_exit "FAIL: Content mismatch: sent ${APP_TO_PC_API} from ${APP_CLIENT_LABEL}, but got ${PC_RECEIVED_STRING} at ${PC_SERVER_LABEL}"
    exit -1
fi

# PC to APP api immediate response path

echo "PC to APP api immediate response path check"

echo "Writing ${PC_TO_APP_API_RESPONSE} to ${PC_SERVER_LABEL} input"
write_to_atmos_input ${PC_SERVER_LABEL} ${PC_TO_APP_API_RESPONSE}

echo "Allowing settling time"
sleep ${SETTLIING_TIME}

echo "Reading received content on APP api client side"
APP_API_RECEIVED_STRING=$(read_from_user_output ${API_CLIENT_LABEL})

echo "From ${API_CLIENT_LABEL} received content \"${APP_API_RECEIVED_STRING}\""

if [ "${APP_API_RECEIVED_STRING}" = "${PC_TO_APP_API_RESPONSE}" ]
then
    echo "Content matched - PC to APP api immediate response path OK"
else
    log_error_and_exit "FAIL: Content mismatch: sent ${PC_TO_APP_API_RESPONSE} from ${PC_SERVER_LABEL}, but got ${APP_API_RECEIVED_STRING} at ${API_CLIENT_LABEL}"
    exit -1
fi

# PC to APP callback path

echo "PC to APP callback path check"

echo "Writing ${PC_TO_APP_CALLBACK} to ${CALLBACK_CLIENT_LABEL} input"
write_to_atmos_input ${CALLBACK_CLIENT_LABEL} ${PC_TO_APP_CALLBACK}

echo "Allowing settling time"
sleep ${SETTLIING_TIME}

echo "Reading received content on APP callback server side"
APP_CALLBACK_RECEIVED_STRING=$(read_from_user_output ${APP_CALLBACK_LABEL})

echo "From ${APP_CALLBACK_LABEL} received content \"${APP_CALLBACK_RECEIVED_STRING}\""

if [ "${APP_CALLBACK_RECEIVED_STRING}" = "${PC_TO_APP_CALLBACK}" ]
then
    echo "Content matched - PC to APP callback path OK"
else
    log_error_and_exit "FAIL: Content mismatch: sent ${PC_TO_APP_CALLBACK} from ${CALLBACK_CLIENT_LABEL}, but got ${APP_CALLBACK_RECEIVED_STRING} at ${APP_CALLBACK_LABEL}"
    exit -1
fi

# APP to PC callback response

echo "App to PC callback response path check"

echo "Writing ${APP_TO_PC_CALLBACK_RESPONSE} to ${APP_CALLBACK_LABEL} input"
write_to_user_input ${APP_CALLBACK_LABEL} ${APP_TO_PC_CALLBACK_RESPONSE}

echo "Allowing settling time"
sleep ${SETTLIING_TIME}

echo "Reading received content on APP callback server side"
PC_CALLBACK_RESPONSE_RECEIVED_STRING=$(read_from_atmos_output ${CALLBACK_CLIENT_LABEL})

echo "From ${CALLBACK_CLIENT_LABEL} received content \"${PC_CALLBACK_RESPONSE_RECEIVED_STRING}\""

if [ "${PC_CALLBACK_RESPONSE_RECEIVED_STRING}" = "${APP_TO_PC_CALLBACK_RESPONSE}" ]
then
    echo "Content matched - App to PC callback response path OK"
else
    log_error_and_exit "FAIL: Content mismatch: sent ${APP_TO_PC_CALLBACK_RESPONSE} from ${APP_CALLBACK_LABEL}, but got ${PC_CALLBACK_RESPONSE_RECEIVED_STRING} at ${CALLBACK_CLIENT_LABEL}"
    exit -1
fi

echo "READ/WRITE tests PASSED"

echo "Killing user-process components in user-container, listing current user-container processes"
exec_user_cmd ps -aef

echo "Killing nc client from user-container: ${API_CLIENT}"
kill_user_process "${API_CLIENT}"

both_containers_alive=$(check_both_containers)

echo "both_containers_alive = ${both_containers_alive}"

if [ X"" = X"${both_containers_alive}" ]
then
    log_error_and_exit "Could not find both containers after killing ${API_CLIENT}"
    exit -1
else
    echo "Containers OK after killing ${API_CLIENT}"
    exec_user_cmd ps -aef
fi

echo "Killing nc server from user-container: ${APP_CALLBACK_SERVER}"
kill_user_process "${APP_CALLBACK_SERVER}"

both_containers_alive=$(check_both_containers)

echo "both_containers_alive = ${both_containers_alive}"

if [ X"" = X"${both_containers_alive}" ]
then
    log_error_and_exit "Could not find both containers after killing ${APP_CALLBACK_SERVER}"
    exit -1
else
    echo "Containers OK after killing ${APP_CALLBACK_SERVER}, listing processes"
    exec_user_cmd ps -aef
fi

echo "Listing containers"
docker ps

check_alive=$(check_both_containers)

echo "Check-alive = ${check_alive}"

if [ X"${check_alive}" = X"ALIVE" ]
then
    echo "Both containers still ALIVE"
else
    log_error_and_exit "Could not assert if both containers still alive"
    exit -1
fi

echo "Kill tests PASSED"

declare_test_pass
