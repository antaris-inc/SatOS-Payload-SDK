#!/bin/bash

RESPONSE_TEMPLATE="template_responses/py_sample_app_template_responses"
SETTLIING_TIME=60
APP_SETTLIING_TIME=15

# Expected payload app process repponse, when the app is Running/Alive
EXPECT_ALIVE_APP_PS_RESP="python3 /assets/samples/python/payload_app.py"

source ./utils/test_utils.sh

#step 1 : start both conatainers
start_containers

ps_app_status()
{
        echo "Check payload app port status"
        port_status

        ps_status=`docker exec ${CLIENT_CONTAINER_NAME} ps -aef`

        echo "${ps_status}"

        echo "${ps_status}" | grep "python3 /assets/samples/python/payload_app.py" | head -2 |tail -1 | awk '{print $8,$9}'

        #Expected behaviour payload app not runniung 
        if [ "${ps_status}" == "${EXPECT_ALIVE_APP_PS_RESP}" ]
        then
                echo "Payload App is ALIVE ...FAIL"
                exit -1
        else
                echo "Payload App is not running"
        fi
}

response_reg_verify()
{
        echo "                                                                          "
        echo "                                                                          "
        echo "-------------------------- Rsult Verification ----------------------------"
        echo "                                                                          "
        echo "                                                                          "

        if [ -z "${APP_RECEIVED_STRING}" ]
        then
                echo "Received response data empty"
                exit -1
        fi

        expected_output=`head -5 ${RESPONSE_TEMPLATE}`
        result=`echo "${APP_RECEIVED_STRING}" | head -5`


        if [ "$expected_output" == "$result" ]
        then
                echo "APP Registration successfull"
        else
                echo "APP Registration FAIL...!"
                exit -1
        fi

        echo "--------------------------------------------------------------------------"
        echo "                                                                          "

        echo "Start Sequence checking"
        expected_output="start_sequence"
        result=`echo "${APP_RECEIVED_STRING}" | grep "start_sequence" | awk '{print $1}'`

        if [ "$expected_output" == "$result" ]
        then
                echo "$(grep "start_sequence" ${RESPONSE_TEMPLATE} | awk '{print $4}') started"
        else
                echo "$(grep "start_sequence" ${RESPONSE_TEMPLATE} | awk '{print $4}') not started"
                exit -1
        fi

        echo "--------------------------------------------------------------------------"
        echo "                                                                          "

        echo "Check get location api invoked or not"
        expected_output="Invoked api_pa_pc_get_current_location, got return-code 0 => An_SUCCESS"
        result=`echo "${APP_RECEIVED_STRING}" | grep "Invoked api_pa_pc_get_current_location"`

        if [ "$expected_output" == "$result"  ]
        then
                echo "$expected_output"
        else
                echo "Not Invoked api_pa_pc_get_current_location func"
                exit -1
        fi

        echo "--------------------------------------------------------------------------"
        echo "                                                                          "

        echo "Check file download api invoked or not"
        expected_output="Invoked api_pa_pc_stage_file_download, got return-code 0 => An_SUCCESS"
        result=`echo "${APP_RECEIVED_STRING}" | grep "Invoked api_pa_pc_stage_file_download"`

        if [ "$expected_output" == "$result"  ]
        then
                echo "$expected_output"
        else
                echo "Not Invoked api_pa_pc_stage_file_download func"
                exit -1
        fi

        echo "--------------------------------------------------------------------------"
        echo "                                                                          "


        echo "Check power control api invoked or not"
        expected_output="Invoked api_pa_pc_payload_power_control, got return-code 0 => An_SUCCESS"
        result=`echo "${APP_RECEIVED_STRING}" | grep "Invoked api_pa_pc_payload_power_control"`

        if [ "$expected_output" == "$result"  ]
        then
                echo "$expected_output"
        else
                echo "Not Invoked api_pa_pc_payload_power_control func"
                exit -1
        fi

        echo "--------------------------------------------------------------------------"
        echo "                                                                          "


        echo "Check sequence completion"
        expected_output="Exiting Sequence_A"
        result=`echo "${APP_RECEIVED_STRING}" | grep "Exiting Sequence_A"`

        if [ "$expected_output" == "$result"  ]
        then
                echo "Successfully completed : $(grep "Exiting Sequence_A" ${RESPONSE_TEMPLATE} | awk '{print $2}')"
        else
                echo "Error in $(grep "Exiting Sequence_A" ${RESPONSE_TEMPLATE} | awk '{print $2}') execution"
                exit -1
        fi

        echo "--------------------------------------------------------------------------"
        echo "                                                                          "


        echo "Check main thread completion"
        expected_output="Exiting Main Thread"
        result=`echo "${APP_RECEIVED_STRING}" | grep "Exiting Main Thread"`
        if [ "$expected_output" == "$result"  ]
        then
                echo "$expected_output"
        else
                echo "Main thread execution error"
                exit -1
        fi
        

        echo "--------------------------------------------------------------------------"
        echo "                                                                          "
        echo "                                                                          "
        echo "Test PASSED"
}

iter=0
loop_count=3

while [ $iter -lt $loop_count ]
do
        #step 2 : init payload app in client container
        echo "INIT Payload APP"
        init_payload_app


        #step 3 : verify payload app running or not

        # Check payload app process repponse, wheather it is Dead or Alive
        echo "**********************   PS app status   **********************"
        ps_app_status

        echo "                                                                          "
        echo "Verify client_container running or not after payload app died"
        echo "                                                                          "
        check_alive=$(check_client_container)
        echo "Check-alive = ${check_alive}"
        if [ -z "${check_alive}" ]
        then
                echo "Could not assert if client container is alive or not"
                exit -1
        else
                echo "                                                                          "
                echo "Client container is ALIVE"
                echo "                                                                          "
        fi

        #step 4 : init PC-SIM
        init_pc_sim


        #step 5 : Restart payload aap again
        init_payload_app


        # step 6 : verify app reg successful against PC-SIM
        echo "Allowing settling time"
        sleep ${SETTLIING_TIME}

        echo "Reading received content on APP server side"
        APP_RECEIVED_STRING=$(read_from_user_output ${PAYLOAD_APP_LABEL})

        echo "From ${PAYLOAD_APP_LABEL} received content \"${APP_RECEIVED_STRING}\""


        echo "Result verfication func "
        response_reg_verify

        #step 7 : Kill both pc-sim and payload app for next iteration
        kill_atmos_process pc-sim
        kill_payload_app

        iter=$(($iter + 1))
        echo "Iteration $iter completed successfully"
done

echo "                                                                          "
echo "--------------------------------------------------------------------------"

check_alive=$(check_both_containers)

echo "Check-alive = ${check_alive}"

if [ X"${check_alive}" = X"ALIVE" ]
then
    echo "Both containers still ALIVE"
else
    log_error_and_exit "Could not assert if both containers still alive"
    exit -1
fi

echo "                                                                          "
echo "--------------------------------------------------------------------------"

declare_test_pass