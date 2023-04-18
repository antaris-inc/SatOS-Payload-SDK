#!/bin/bash

RESPONSE_TEMPLATE="template_responses/py_sample_app_template_responses"
SETTLIING_TIME=60

source ./utils/test_utils.sh

start_containers


# App to PC api path

echo "INIT both PC_SIM and APP"

init_pc_sim 

init_payload_app

echo "Allowing settling time"
sleep ${SETTLIING_TIME}

echo "Reading received content on APP server side"
APP_RECEIVED_STRING=$(read_from_user_output ${PAYLOAD_APP_LABEL})

echo "From ${PAYLOAD_APP_LABEL} received content \"${APP_RECEIVED_STRING}\""

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

echo "Result verfication func "
response_reg_verify

check_alive=$(check_both_containers)

echo "Check-alive = ${check_alive}"

if [ X"${check_alive}" = X"ALIVE" ]
then
        echo "Both containers still ALIVE"
else
        log_error_and_exit "Could not assert if both containers still alive"
        exit -1
fi

declare_test_pass