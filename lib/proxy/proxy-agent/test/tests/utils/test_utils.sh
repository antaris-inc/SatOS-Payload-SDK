#!/bin/bash

NETWORK_NAME=atmos_proxy_network
NETWORK_SUBNET=172.155.155.0/24
TEST_CONTAINER_IMAGE=antaris_proxy_test
SERVER_CONTAINER_NAME=antaris_proxy_test_atmos
CLIENT_CONTAINER_NAME=antaris_proxy_test_user
ENTRY_SCRIPT=./entrypoint.sh
SPAWN_SCRIPT=./spawn_with_io_files.sh
PROXY_ROOT_DIR="tools/proxy-agent"
WRITER=./write_to_file.sh
READER=./read_from_file.sh
PC_SIM="/assets/pc-sim/pc-sim"
PC_SIM_LABEL="PC_SIM"
PAYLOAD_APP="python3 /assets/samples/python/payload_app.py"
PAYLOAD_APP_LABEL="APP"

IO_FILE_DIR=/data/
INPUT_SUFFIX=_input
OUTPUT_SUFFIX=_output

ATMOS_AGENT_CMD="python3 ${PROXY_ROOT_DIR}/agent.py -m atmos -i 172.155.155.2 -p 19051 -s 127.0.0.1 -t 50053 -l 127.0.0.1 -o 50051"
USER_AGENT_CMD="python3 ${PROXY_ROOT_DIR}/agent.py -m user -i 172.155.155.2 -p 19051 -s 127.0.0.1 -t 50051 -l 127.0.0.1 -o 50053"

destroy_network() {
    echo "Removing existing network ${NETWORK_NAME} if any"
    docker network rm ${NETWORK_NAME}
}

create_network() {
    echo "Creating test-network ${NETWORK_NAME} with subnet ${NETWORK_SUBNET}"
    docker network create -d bridge --subnet ${NETWORK_SUBNET} ${NETWORK_NAME}

    # docker inspect ${NETWORK_NAME}
}

kill_server_container() {
    docker rm -f ${SERVER_CONTAINER_NAME}
}

start_server_container() {
    echo "Starting server container ${SERVER_CONTAINER_NAME}"
    docker run -d --init --name ${SERVER_CONTAINER_NAME} --network ${NETWORK_NAME} ${TEST_CONTAINER_IMAGE} -m atmos -t 50053 -o 50051
}

kill_client_container() {
    docker rm -f ${CLIENT_CONTAINER_NAME}
}

start_client_container() {
    echo "Starting client container ${CLIENT_CONTAINER_NAME}"
    docker run -d --init --name ${CLIENT_CONTAINER_NAME} --network ${NETWORK_NAME} ${TEST_CONTAINER_IMAGE} -m user -t 50051 -o 50053
}

check_client_container() {
    client_container_ps=$(docker ps | grep ${CLIENT_CONTAINER_NAME})
    echo ${client_container_ps}
}

check_server_container() {
    server_container_ps=$(docker ps | grep ${SERVER_CONTAINER_NAME})
    echo ${server_container_ps}
}

check_both_containers() {
    client=$(check_client_container)
    server=$(check_server_container)

    if [ X"" != X"${client}" ] && [ X"" != X"${server}" ]
    then
        echo "ALIVE"
    else
        echo ""
    fi
}

kill_client_container() {
    docker rm -f ${CLIENT_CONTAINER_NAME}
}

kill_server_container() {
    docker rm -f ${SERVER_CONTAINER_NAME}
}

start_containers() {
    kill_client_container
    kill_server_container
    destroy_network

    create_network

    start_server_container

    start_client_container
}

# args: LABEL exec [arg1 [...]]
start_atmos_process_with_io_files() {
    label=$1
    remaining_args=$*
    shift
    remaining_args=$*
    echo "Args were $*, label ${label}, now remaining ${remaining_args}"
    outfile_name=${IO_FILE_DIR}/${label}${OUTPUT_SUFFIX}
    infile_name=${IO_FILE_DIR}/${label}${INPUT_SUFFIX}
    docker exec ${SERVER_CONTAINER_NAME} ${SPAWN_SCRIPT} ${infile_name} ${outfile_name} $remaining_args
    docker exec ${SERVER_CONTAINER_NAME} ps -aef
}

# args: LABEL exec [arg1 [...]]
start_user_process_with_io_files() {
    label=$1
    remaining_args=$*
    shift
    remaining_args=$*
    echo "Args were $*, label ${label}, now remaining ${remaining_args}"
    outfile_name=${IO_FILE_DIR}/${label}${OUTPUT_SUFFIX}
    infile_name=${IO_FILE_DIR}/${label}${INPUT_SUFFIX}
    docker exec ${CLIENT_CONTAINER_NAME} ${SPAWN_SCRIPT} ${infile_name} ${outfile_name} $remaining_args
    docker exec ${CLIENT_CONTAINER_NAME} ps -aef
}

# args: LABEL content
write_to_atmos_input() {
    label=$1
    shift
    remaining_args=$*
    infile_name=${IO_FILE_DIR}/${label}${INPUT_SUFFIX}
    echo "Writing \"${remaining_args}\" to ${infile_name}"
    docker exec ${SERVER_CONTAINER_NAME} ${WRITER} ${infile_name} ${remaining_args}
}

# args: LABEL content
write_to_user_input() {
   label=$1
    shift
    remaining_args=$*
    infile_name=${IO_FILE_DIR}/${label}${INPUT_SUFFIX}
    echo "Writing \"${remaining_args}\" to ${infile_name}"
    docker exec ${CLIENT_CONTAINER_NAME} ${WRITER} ${infile_name} ${remaining_args}
}

# args: LABEL
read_from_atmos_output() {
    label=$1
    shift
    remaining_args=$*
    outfile_name=${IO_FILE_DIR}/${label}${OUTPUT_SUFFIX}
    content=$(docker exec ${SERVER_CONTAINER_NAME} ${READER} ${outfile_name})
    echo ${content}
}

# args: LABEL
read_from_user_output() {
    label=$1
    shift
    remaining_args=$*
    outfile_name=${IO_FILE_DIR}/${label}${OUTPUT_SUFFIX}
    content=$(docker exec ${CLIENT_CONTAINER_NAME} ${READER} ${outfile_name})
    echo "${content}"
}

exec_atmos_cmd() {
    docker exec -t ${SERVER_CONTAINER_NAME} $*
}

exec_user_cmd() {
    docker exec -t ${CLIENT_CONTAINER_NAME} $*
}

find_container_process_by_cmdline() {
    container_name=$1
    shift
    pid_list=$(docker exec ${container_name} pgrep -f "$*")
    echo ${pid_list}
}

find_atmos_process_by_cmdline() {
    process_pattern=$*
    container_name=${SERVER_CONTAINER_NAME}
    pid_list=$(find_container_process_by_cmdline ${container_name} ${process_pattern})
    echo ${pid_list}
}

find_user_process_by_cmdline() {
    process_pattern=$*
    container_name=${CLIENT_CONTAINER_NAME}
    pid_list=$(find_container_process_by_cmdline ${container_name} ${process_pattern})
    echo ${pid_list}
}

kill_atmos_process() {
    echo "kill_atmos_process: got args $*"
    process_pattern=$*
    echo "Searching for atmos-container process pattern ${process_pattern}"
    pid_list=$(find_atmos_process_by_cmdline ${process_pattern})
    echo "PIDs found: ${pid_list}"
    for p in ${pid_list}
    do
        echo "Killing pid ${p}"
        exec_atmos_cmd kill -9 ${p}
    done
}

kill_user_process() {
    echo "kill_user_process: got args $*"
    process_pattern=$*
    echo "Searching for user-container process pattern ${process_pattern}"
    pid_list=$(find_user_process_by_cmdline ${process_pattern})
    echo "PIDs found: ${pid_list}"
    for p in ${pid_list}
    do
        echo "Killing pid ${p}"
        exec_user_cmd kill -9 ${p}
    done
}

log_everything() {
    echo "========= Atmos side ===================="
    echo 'Atmos Agent container logs =========>'
    docker logs ${SERVER_CONTAINER_NAME}

    echo "Atmos data dir and content ==========>"
    exec_atmos_cmd ls -ltr ${IO_FILE_DIR}
    docker exec -t ${SERVER_CONTAINER_NAME} ${READER} ${IO_FILE_DIR}/*

    echo "============= User side ============"
    docker logs ${CLIENT_CONTAINER_NAME}

    echo "User data dir and content ==========>"
    exec_user_cmd ls -ltr ${IO_FILE_DIR}
    docker exec -t ${CLIENT_CONTAINER_NAME} ${READER} ${IO_FILE_DIR}/*
}

init_pc_sim()
{
    echo "Init PC-SIM app"
    start_atmos_process_with_io_files ${PC_SIM_LABEL} ${PC_SIM}
}

init_payload_app()
{
    if [ -z "$1" ]
    then
            echo "Taken default lable : ${PAYLOAD_APP_LABEL}"
    else
            echo "new lable : $1"
            PAYLOAD_APP_LABEL=$1
    fi

    echo "Init payload sample app"
    start_user_process_with_io_files  ${PAYLOAD_APP_LABEL} ${PAYLOAD_APP}
}

kill_payload_app()
{
    echo "going to kill Payload_app"
    kill_user_process payload_app.py

    echo "Check port status"
    port_status
}

port_status()
{
    status=$(exec_user_cmd netstat -ant | grep 50053 | awk '{print $6}')
    echo "Current Port status : $status"
    echo "Wait for port to get free"

    while true
    do 
        status=$(exec_user_cmd netstat -ant | grep 50053 | awk '{print $6}')
        if [ -z "$status" ]
        then
                echo "Port is free now"
                break
        fi
    done
    sleep 5
}

kill_user_agent()
{
    echo "Kill user agent"
    exec_user_cmd killall5 -9 agent.py
    docker ps
}

declare_test_pass()
{
    echo "Test PASSED"
}