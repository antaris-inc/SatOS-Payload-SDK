/*
 * Copyright 2022 Antaris, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <cstdlib>
#include <ctime>

#include "antaris_api.h"
#include "antaris_api_gpio.h"
#include "antaris_api_pyfunctions.h"
#include "antaris_can_api.h"
#include "antaris_api_parser.h"
#include "antaris_api_i2c.h"

#define MAX_STR_LEN 256
#define SEQ_PARAMS_LEN 64
#define SEQ_NAME_LEN   32

#define HelloWorld_ID                   "HelloWorld"
#define HelloWorld_IDX                  0
#define HelloFriend_ID                  "HelloFriend"
#define HelloFriend_IDX                 1
#define LogLocation_ID                  "LogLocation"
#define LogLocation_IDX                 2
#define TestGPIO_Sequence_ID            "TestGPIO"
#define TestGPIO_Sequence_IDX           3
#define StageFile_Sequence_ID           "StageFile"
#define StageFile_Sequence_IDX          4
#define TestCANBus_Sequence_ID          "TestCANBus"
#define TestCANBus_Sequence_IDX         5
#define EpsVoltageTelemetry_ID          "EpsVoltageTm"
#define EpsVoltageTelemetry_IDX         6
#define GnssDataTelemetry_ID            "GnssDataTm"
#define GnssDataTelemetry_IDX           7
#define TestI2CBUS_ID                   "TestI2CBus"
#define TestI2CBUS_IDX                  8
#define PowerControl_ID                 "PowerControl"
#define PowerControl_IDX                9
#define SesThermMgmt_ID                 "SesThermMgmnt"
#define SesThermMgmt_IDX                10
#define SesTempReq_ID                   "SesTempReq"
#define SesTempReq_IDX                  11
#define PaSatosMsg_ID                   "PaSatOsMsg"
#define PaSatosMsg_IDX                  12
#define ReadAcIp_ID                     "ReadAcIp"
#define ReadAcIp_IDX                    13
#define SEQUENCE_ID_MAX                 14

#define APP_STATE_ACTIVE                0  // Application State : Good (0), Error (non-Zero)

#define STAGE_FILE_DOWNLOAD_DIR         "/opt/antaris/outbound/"    // path for staged file download
#define STAGE_FILE_NAME                 "SampleFile.txt"            // name of staged file
#define MAX_DATA_BYTES 8
#define SEND_MSG_LIMIT 10
#define MAX_PAYLOAD_DATA_SIZE 1020


/*
 * Following counters should be incremented whenever
 * a reqeust/response (to PC) API hits error
 */
unsigned short reqs_to_pc_in_err_cnt = 0;
unsigned short resps_to_pc_in_err_cnt = 0;
// Global variable to record application state
unsigned short application_state = APP_STATE_ACTIVE;	
static unsigned int debug = 0;
static int shutdown_requested = 0;
// Global variable channel
AntarisChannel channel;

struct mythreadState_s;

typedef void (*fsm_entry_fn_t)(struct mythreadState_s *);

typedef struct mythreadState_s {
    unsigned short int      correlation_id;
    char                    state[MAX_STR_LEN];
    char                    seq_id[32];
    char                    received_name[MAX_STR_LEN];
    char                    seq_params[SEQ_PARAMS_LEN];
    unsigned long           scheduled_deadline;	//timestamp by when this sequence should be over
    unsigned int            counter;
    pthread_cond_t          condition;
    pthread_mutex_t         cond_lock;
    AntarisChannel          channel;
    pthread_t               thread_id;
    fsm_entry_fn_t          entry_fn;
} mythreadState_t;

//This sequence should complete within its scheduled_deadline
void handle_HelloWorld(mythreadState_t *mythread)
{
    AntarisReturnCode ret;

    printf("\n Handling sequence: hello, world! \n");

    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], HelloWorld_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    }
}

void handle_HelloFriend(mythreadState_t *mythread)
{
    AntarisReturnCode ret;
    unsigned long current_time_in_ms;

    printf("Handling sequence: hello, %s \n", mythread->seq_params);

    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], HelloFriend_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    }
}

static void handle_LogLocation(mythreadState_t *mythread)
{
    AntarisReturnCode ret;
    unsigned long current_time_in_ms;

    // Request PC to get current location
    ReqGetCurrentLocationParams get_current_location_params = {0};
    get_current_location_params.correlation_id = mythread->correlation_id;
    ret = api_pa_pc_get_current_location(channel, &get_current_location_params);

    printf("%s: sent get-current-location request with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_get_current_location failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_get_current_location returned success, ret %d\n", __FUNCTION__, ret);
    }

    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], LogLocation_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    }

    return;
}

void handle_Eps_Voltage_Telemetry_Request(mythreadState_t *mythread){
    AntarisReturnCode ret;
    char *seq_params_lower = (char *)malloc(strlen(mythread->seq_params) + 1);
    UINT16 periodicity_in_ms = 2000;   // Periodicity = 0 indicates one time EPS voltage data frequency. Max is 1 minute.
    for(int i = 0;mythread->seq_params[i] != '\0';i++){
        seq_params_lower[i] = tolower(mythread->seq_params[i]);
    }
    seq_params_lower[strlen(mythread->seq_params)] = '\0';
    if(strcmp(seq_params_lower, "stop") == 0){
        printf("\n Sending Get Eps Voltage telemetry stop request \n");

        ReqGetEpsVoltageStopReq req_get_eps_voltage_stop_req = {0};
        req_get_eps_voltage_stop_req.correlation_id = mythread->correlation_id;
        ret = api_pa_pc_get_eps_voltage_stop_req(channel,&req_get_eps_voltage_stop_req);
        if(ret == An_SUCCESS){
            printf("Get Eps Voltage telemetry stop request success, ret %d\n",ret);
        }
        else{
            fprintf(stderr, " Get Eps Voltage telemetry stop request failed, ret %d\n", ret);
        }
    }
    else if(strcmp(seq_params_lower, "start") == 0){
        printf("\n Sending Get Eps Voltage telemetry start request \n");
        ReqGetEpsVoltageStartReq req_get_eps_voltage_start_req = {0};
        req_get_eps_voltage_start_req.correlation_id = mythread->correlation_id;
        req_get_eps_voltage_start_req.periodicity_in_ms = periodicity_in_ms;
        ret = api_pa_pc_get_eps_voltage_start_req(channel,&req_get_eps_voltage_start_req);
        if(ret == An_SUCCESS){
            printf("Get Eps Voltage telemetry start request success, ret %d\n",ret);
        }
        else{
            fprintf(stderr, " Get Eps Voltage telemetry start request failed, ret %d\n", ret);
        }
    }
    else{
        printf("Incorrect parameters. Parameter can be 'stop' or 'start'");
    }

    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], EpsVoltageTelemetry_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    }
}

void handle_gnss_data_Telemetry_Request(mythreadState_t *mythread){
    AntarisReturnCode ret;
    char *seq_params_lower = (char *)malloc(strlen(mythread->seq_params) + 1);
    UINT16 periodicity_in_ms = 2000;    // Periodicity = 0 indicates one time GNSS EPH data. Max is 1 minute
    INT8 eph2_enable = 1;

    for(int i = 0;mythread->seq_params[i] != '\0';i++){
        seq_params_lower[i] = tolower(mythread->seq_params[i]);
    }
    seq_params_lower[strlen(mythread->seq_params)] = '\0';
    if(strcmp(seq_params_lower, "stop") == 0){
        printf("\n Sending GNSS EPH data Telemetry stop request \n");

        ReqGnssEphStopDataReq req_gnss_eph_stop_data_request = {0};
        req_gnss_eph_stop_data_request.correlation_id = mythread->correlation_id;
        ret = api_pa_pc_gnss_eph_stop_req(channel,&req_gnss_eph_stop_data_request);
        if(ret == An_SUCCESS){
            printf("GNSS EPH data Telemetry stop request success, ret %d\n",ret);
        }
        else{
            fprintf(stderr, " GNSS EPH data Telemety stop request failed, ret %d\n", ret);
        }
    }
    else if(strcmp(seq_params_lower, "start") == 0){
        printf("\n Sending GNSS EPH data Telemetry start request \n");
        ReqGnssEphStartDataReq req_gnss_eph_start_data_request = {0};
        req_gnss_eph_start_data_request.correlation_id = mythread->correlation_id;
        req_gnss_eph_start_data_request.periodicity_in_ms = periodicity_in_ms;
        req_gnss_eph_start_data_request.eph2_enable = eph2_enable;
        ret = api_pa_pc_gnss_eph_start_req(channel,&req_gnss_eph_start_data_request);
        if(ret == An_SUCCESS){
            printf("GNSS EPH data Telemetry start request success, ret %d\n",ret);
        }
        else{
            fprintf(stderr, " GNSS EPH data Telemetry start request failed, ret %d\n", ret);
        }
    }
    else{
        printf("Incorrect parameters. Parameter can be 'stop' or 'start'");
    }
    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], GnssDataTelemetry_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    }


}

void handle_TestGPIO(mythreadState_t *mythread)
{
    AntarisReturnCode ret;
    AntarisApiGPIO api_gpio;
    AntarisApiParser api_parser;
    gpio_s gpio_info;
    int i = 0;
    int8_t readPin, writePin, val;
    int8_t GPIO_ERROR = -1;

    printf("\n Handling sequence: TestGPIO! \n");

    ret = api_parser.api_pa_pc_get_gpio_info(&gpio_info);

    if (ret != An_SUCCESS) {
        printf("Error: json file is not configured properly. Kindly check configurations done in ACP \n");
        return;
    }
    printf("Total gpio pins = %d \n", gpio_info.pin_count);

    if (ret != An_SUCCESS) {
        printf("Error: Unable to initialize GPIO Lib \n");
        return;
    }

    ret = api_gpio.api_pa_pc_init_gpio_lib();
    if (ret != An_SUCCESS) {
        printf("Error: Init GPIO lib failed \n");
        return;
    }

    while (i < gpio_info.pin_count) {
        // Read initial value of GPIO pins.
        // Assume GPIO pins are in loopback mode, their value must be same.

        readPin = gpio_info.pins[i];
        writePin = gpio_info.pins[i+1];
        val = api_gpio.api_pa_pc_read_gpio(gpio_info.gpio_port, readPin);

        if (val == GPIO_ERROR) {
            printf("Error in pin no %d \n", int(readPin));
            return;
        }
        printf("Initial Gpio value of pin no %d is %d \n", int(readPin), val);
                   
        // Toggle the value
        val = val ^ 1; 

        // Writing value to WritePin.
        ret = api_gpio.api_pa_pc_write_gpio(gpio_info.gpio_port, writePin, val);
        if (ret == GPIO_ERROR) {
            printf("Error in pin no %d \n", int(writePin));
            return;
        }
        printf("Written %d successfully to pin no %d \n", val, int(writePin));
        
        /* As Read and Write pins are back-to-back connected, 
           Reading value of Read pin to confirm GPIO success/failure
         */
        val = api_gpio.api_pa_pc_read_gpio(gpio_info.gpio_port, readPin);
        if (val == GPIO_ERROR) {
            printf("Error in pin no %d \n", int(readPin));
            return;
        }
        printf("Final Gpio value of pin no %d is %d \n", int(readPin), val);
         
        i += 2;
    }

    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], TestGPIO_Sequence_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } 
    
    printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    api_gpio.api_pa_pc_deinit_gpio_lib();
}

void handle_StageFile(mythreadState_t *mythread)
{
    AntarisReturnCode ret;
    FILE *fp = NULL;
    size_t filename_size = 0;
    ReqStageFileDownloadParams download_file_params = {0};
    char full_file_path[256];

    printf("\n Handling sequence: StageFile! \n");

    filename_size = strnlen(STAGE_FILE_NAME, MAX_FILE_OR_PROP_LEN_NAME);

    if (filename_size > MAX_FILE_OR_PROP_LEN_NAME) {
        printf("Error: Stagefile path can not be greater than %d \n", MAX_FILE_OR_PROP_LEN_NAME);
        goto exit_sequence;
    }

    // Pass just the filename
    sprintf(download_file_params.file_path, "%s", STAGE_FILE_NAME);

    // Create the full path for file creation
    sprintf(full_file_path, "%s%s", STAGE_FILE_DOWNLOAD_DIR, STAGE_FILE_NAME);

    // Adding dummy data in file
    fp = fopen(full_file_path, "w");
    if (fp == NULL) {
        printf("Error: Can not open file %s. Sequence failed \n", full_file_path);
        goto exit_sequence;
    }
    fprintf(fp, "Testing file download with payload \n");
    fclose(fp);

    printf("Info: Created file at %s \n", full_file_path);
    printf("Info: Staging file = %s \n", download_file_params.file_path);  // Should print "SampleFile.txt"

    download_file_params.file_priority = FILE_DL_PRIORITY_NORMAL;
    download_file_params.file_dl_band = FILE_DL_SBAND;
    // Staging file
    ret = api_pa_pc_stage_file_download(channel, &download_file_params);

    if (ret == An_GENERIC_FAILURE) {
        printf("Error: Failed to stage file %s \n", download_file_params.file_path);
    }
exit_sequence:    
    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], StageFile_Sequence_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } 
    
    printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    
}

void handle_TestCANBus(mythreadState_t *mythread)
{
    AntarisReturnCode ret = An_SUCCESS;
    AntarisApiCAN canInfo;
    int dlc = 8;
    printf("Test CAN bus\n");

    // Default data
    char data[] = "0x123 0x11,0x12,0x13,0x14,0x15,0x16,0x17";
    char *parts[2];

    // Split input string into Arbitration ID and Data
    char *token = strtok(data, " ");
    for (int i = 0; i < 2 && token != NULL; i++) {
        parts[i] = token;
        token = strtok(NULL, " ");
    }

    if (parts[0] == NULL || parts[1] == NULL) {
        printf("Input format is incorrect. Using default arbitration ID and data bytes.\n");
    }

    // Convert Arbitration ID
    int arb_id = (int)strtol(parts[0], NULL, 16);

    // Convert Data Bytes
    unsigned char data_bytes[MAX_DATA_BYTES];
    int data_count = 0;
    token = strtok(parts[1], ",");
    while (token != NULL && data_count < MAX_DATA_BYTES) {
        data_bytes[data_count++] = (unsigned char)strtol(token, NULL, 16);
        token = strtok(NULL, ",");
    }

    // Get CAN bus info
    canInfo.api_pa_pc_get_can_dev(&canInfo);
    printf("Total CAN bus ports = %d\n", canInfo.can_port_count);

    // Select first available CAN device
    if (canInfo.can_port_count == 0) {
        printf("No CAN devices available!\n");
        return;
    }
    
    // Start receiver threads for each CAN device
    for (int i = 0; i < canInfo.can_port_count; i++) {
        printf("Starting CAN receiver on = %s \n", canInfo.can_dev[i]);
        canInfo.api_pa_pc_start_can_receiver_thread(i);
    }

    // Send test messages on each CAN bus
    int send_msg_limit = 10;
    for (int loopCounter = 0; loopCounter < send_msg_limit; loopCounter++) {
        for (int i = 0; i < canInfo.can_port_count; i++) {
            canInfo.api_pa_pc_send_can_message(i, arb_id + i, data_bytes, dlc);
        }
        sleep(1);
    }

    printf("Checking received data \n");

    for (int i = 0; i < canInfo.can_port_count; i++) {
        while (canInfo.api_pa_pc_get_can_message_received_count(i) > 0) {
            struct can_frame frame = canInfo.api_pa_pc_read_can_data(i);
            printf("Received message from %s with id %d \n ",  canInfo.can_dev[i], frame.can_id);
            for (int j = 0; j < CAN_MAX_DLEN; j++) {
                printf("%x \t", frame.data[j]);
            }
            printf("\n");
        }
        printf("Message queue from %s dev is empty now \n",canInfo.can_dev[i]);
        sleep(1);
    }

    printf("Completed reading\n");

exit_sequence:    
    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], StageFile_Sequence_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } 
    
    printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    
}

void handle_TestI2CBus(mythreadState_t *mythread)
{
    AntarisReturnCode ret;
    AntarisApiI2C api_i2c;
    AntarisApiParser api_parser;
    i2c_s i2c_info;
    int i = 0;
    uint8_t read, write_len = 5;
    uint8_t writebuf[5] = {0x4, 0x5, 0x6, 0x7, 0x8}; 
    uint16_t address = 0xA0;
    uint16_t index = 0;

    printf("\n Handling sequence: TestI2CBus! \n");

    ret = api_parser.api_pa_pc_get_i2c_dev(&i2c_info);

    if (ret != An_SUCCESS) {
        printf("Error: json file is not configured properly. Kindly check configurations done in ACP \n");
        return;
    }
    printf("Total i2c ports = %d \n", i2c_info.i2c_port_count);

    ret = api_i2c.api_pa_pc_init_i2c_lib();
    if (ret != An_SUCCESS) {
        printf("Error: Init I2C lib failed \n");
        return;
    }

    if (i2c_info.i2c_port_count> 0) {
        ret = api_i2c.api_pa_pc_read_i2c_bus(i2c_info.i2c_dev[0], address, index, &read);
        if (ret != An_SUCCESS) {
            printf("Error: Read I2C failed \n");
            return;
        }

        printf("I2c read = %d \n", read);

        ret = api_i2c.api_pa_pc_write_i2c_bus(i2c_info.i2c_dev[0], address, index, writebuf, write_len);
        if (ret != An_SUCCESS) {
            printf("Error: Write I2C failed \n");
            return;
        }
    }

    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], TestI2CBUS_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } 
    api_i2c.api_pa_pc_deinit_i2c_lib();
    
    printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
}
void handle_PowerControl(mythreadState_t *mythread){
    printf("Handling payload power");
    AntarisReturnCode ret;
    // char* power_state = mythread->seq_params;
    printf("parmas is %s\n",mythread->seq_params);
    char *endptr;
    errno = 0;
    long power_state = strtol(mythread->seq_params,&endptr,10);
    if (errno != 0 || endptr == mythread->seq_params) {
        printf("incorrect parameters. not able to convert string to integer.\n");
        return;
    }
     else {
        if(power_state != 0 && power_state != 1){
            printf("incorrect parameters. parameters can be only 0 or 1.\n");
        }
        else {
            UINT16 hw_id = 0x4001;
            ReqPayloadPowerControlParams PayloadPowerControl = {0};
            PayloadPowerControl.hw_id = hw_id;
            PayloadPowerControl.correlation_id = mythread->correlation_id;
            PayloadPowerControl.power_operation = power_state;
            ret = api_pa_pc_payload_power_control(channel,&PayloadPowerControl);
            if(ret == An_SUCCESS){
                    printf("Payload power control request success, ret %d\n",ret);
            } else{
                fprintf(stderr, " payload power control request failed, ret %d\n", ret);
            }
        }
    }
    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], PowerControl_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    }

}

void handle_ses_therm_mgmnt(mythreadState_t *mythread){
    printf("Handling Ses thermal management request");
    AntarisReturnCode ret;
    UINT8 hardware_id = 0;  // 0:SESA , 1:SESB
    UINT32 duration = 2000 ;  // millisecond
    UINT8 upper_threshold = 25; // in celsius
    UINT8 lower_threshold = 20; // in celsius

    char *seq_params_lower = (char *)malloc(strlen(mythread->seq_params) + 1);
    for(int i = 0;mythread->seq_params[i] != '\0';i++){
        seq_params_lower[i] = tolower(mythread->seq_params[i]);
    }
    seq_params_lower[strlen(mythread->seq_params)] = '\0';
    if(strcmp(seq_params_lower, "stop") == 0){
        printf("\n Sending stop SES thermal management request \n");

        StopSesThermMgmntReq stop_ses_therm_mgmt_req = {0};
        stop_ses_therm_mgmt_req.correlation_id = mythread->correlation_id;
        stop_ses_therm_mgmt_req.hardware_id = hardware_id;
        ret = api_pa_pc_stop_ses_therm_mgmnt_req(channel, &stop_ses_therm_mgmt_req);
        if(ret == An_SUCCESS){
            printf("stop SES thermal management request success, ret %d\n",ret);
        }
        else{
            fprintf(stderr, " stop SES thermal management request failed, ret %d\n", ret);
        }
    }
    else if(strcmp(seq_params_lower, "start") == 0){
        printf("\n Sending start SES thermal management request \n");
        StartSesThermMgmntReq start_ses_therm_mgmt_req = {0};
        start_ses_therm_mgmt_req.correlation_id = mythread->correlation_id;
        start_ses_therm_mgmt_req.duration = duration;
        start_ses_therm_mgmt_req.hardware_id = hardware_id;
        start_ses_therm_mgmt_req.lower_threshold = lower_threshold;
        start_ses_therm_mgmt_req.upper_threshold = upper_threshold;
        ret = api_pa_pc_start_ses_therm_mgmnt_req(channel, &start_ses_therm_mgmt_req);
        if(ret == An_SUCCESS){
            printf("start SES thermal management request success, ret %d\n",ret);
        }
        else{
            fprintf(stderr, " start SES thermal management request failed, ret %d\n", ret);
        }
    }
    else{
        printf("Incorrect parameters. Parameter can be 'stop' or 'start'");
    }

    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], SesThermMgmt_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    }

}

void handle_ses_temp_req(mythreadState_t *mythread){

    printf("Handling SES Temperature request");
    AntarisReturnCode ret;
    UINT8 hardware_id = 0;  // 0:SESA , 1:SESB
    SesTempReq ses_temp_req = {0};
    ses_temp_req.correlation_id = mythread->correlation_id;
    ses_temp_req.hardware_id = hardware_id;
    ret = api_pa_pc_ses_temp_req(channel,&ses_temp_req);
    if(ret == An_SUCCESS){
        printf("SES temp request success, ret %d\n",ret);
    }
    else{
        fprintf(stderr, " SES temp request failed, ret %d\n", ret);
    }

     // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], SesTempReq_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    }

}

void handle_pa_satos_message(mythreadState_t *mythread){

    printf("Handling Pa-SatOS message");
    AntarisReturnCode ret;
    UINT16 command = 1;  // command ID
    INT8 Payload_data[MAX_PAYLOAD_DATA_SIZE] = { 0x12, 0x34, 0x56 };  // Can be up to 1020 bytes
    PaSatOsMsg pa_satos_msg = {0};
    pa_satos_msg.correlation_id = mythread->correlation_id;
    pa_satos_msg.command_id = command;
    memcpy(&pa_satos_msg.payload_data,Payload_data,sizeof(Payload_data));
    ret = api_pa_pc_pa_satos_message(channel, &pa_satos_msg);
    if(ret == An_SUCCESS){
        printf("Pa SatOS message success, ret %d\n",ret);
    }
    else{
        fprintf(stderr, " Pa SatOS message failed, ret %d\n", ret);
    }

     // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], PaSatosMsg_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
    }

}

void handle_ac_ip_read(mythreadState_t *mythread){

    AntarisReturnCode ret;
    AntarisApiParser api_parser;
    char ac_ip[32] = {0};
    
    printf("\n Handling sequence: Read Application controller IP! \n");

    ret = api_parser.api_pa_pc_get_ac_ip(ac_ip);

    if (ret != An_SUCCESS) {
        printf("Error: json file is not configured properly. Kindly check configurations done in ACP \n");
        return;
    }
    else{
        printf("Application controller Ip is : %s",ac_ip);
    }
    
    // Tell PC that current sequence is done
    CmdSequenceDoneParams sequence_done_params = {0};
    strcpy(&sequence_done_params.sequence_id[0], ReadAcIp_ID);
    ret = api_pa_pc_sequence_done(channel, &sequence_done_params);

    printf("%s: sent sequence-done notification with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_sequence_done failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } 
    
    printf("%s: api_pa_pc_sequence_done returned success, ret %d\n", __FUNCTION__, ret);
}

// Table of Sequence_id : FsmThread
mythreadState_t *payload_sequences_fsms[SEQUENCE_ID_MAX];
unsigned int current_sequence_idx = HelloWorld_IDX;

void *threadStartWrapper(void *s)
{
    mythreadState_t *_state = (mythreadState_t *)s;

    _state->entry_fn(_state);

    return NULL;
}

mythreadState_t *fsmThreadCreate(AntarisChannel channel, unsigned int counter, const char *seq_id, fsm_entry_fn_t func)
{
    mythreadState_t *state = (mythreadState_t *)malloc(sizeof(mythreadState_t));
    
    if (NULL == state) {
        return state;
    }

    state->channel = channel;
    state->counter = counter;
    strcpy(state->seq_id, seq_id);
    state->entry_fn = func;
    strcpy(&state->state[0], "NOT_STARTED");

    pthread_mutex_init(&state->cond_lock, NULL);
    pthread_cond_init(&state->condition, NULL);

    return state;
}

static void fsmThreadCleanup(mythreadState_t *state)
{
    pthread_cond_destroy(&state->condition);
    pthread_mutex_destroy(&state->cond_lock);
    free(state);
}

int fsmThreadStart(mythreadState_t *threadState)
{
    pthread_attr_t attr;
    void *thread_exit_status;

    if (strcmp(threadState->state, "NOT_STARTED") != 0) {
        // wait for the previous thread's cleanup
        printf("%s: Requesting earlier sequence processor's cleanup\n", threadState->seq_id);
        sleep(1);
        pthread_cond_signal(&threadState->condition);
        printf("%s: waiting for previous sequence processor's cleanup\n", threadState->seq_id);
        pthread_join(threadState->thread_id, &thread_exit_status);
    }

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    return pthread_create(&threadState->thread_id, &attr, threadStartWrapper, threadState);
}

static int get_sequence_idx_from_seq_string(INT8 *sequence_string)
{
    printf("%s: Decoding Sequence %s\n", __FUNCTION__, sequence_string);
    if (strcmp(sequence_string, HelloWorld_ID) == 0) {
        printf("\t => %d\n", HelloWorld_IDX);
        return HelloWorld_IDX;
    } else if (strcmp(sequence_string, HelloFriend_ID) == 0) {
        printf("\t => %d\n", HelloFriend_IDX);
        return HelloFriend_IDX;
    } else if (strcmp(sequence_string, LogLocation_ID) == 0) {
        printf("\t => %d\n", LogLocation_IDX);
        return LogLocation_IDX;
    } else if (strcmp(sequence_string, TestGPIO_Sequence_ID) == 0) {
        printf("\t => %d\n", TestGPIO_Sequence_IDX);
        return TestGPIO_Sequence_IDX;
    } else if (strcmp(sequence_string, StageFile_Sequence_ID) == 0) {
        printf("\t => %d\n", StageFile_Sequence_IDX);
        return StageFile_Sequence_IDX;
    } else if (strcmp(sequence_string, TestCANBus_Sequence_ID) == 0) {
        printf("\t => %d\n", TestCANBus_Sequence_IDX);
        return TestCANBus_Sequence_IDX;
    } else if (strcmp(sequence_string, EpsVoltageTelemetry_ID) == 0) {
        printf("\t => %d\n", EpsVoltageTelemetry_IDX);
        return EpsVoltageTelemetry_IDX;
    } else if (strcmp(sequence_string, GnssDataTelemetry_ID) == 0) {
        printf("\t => %d\n", GnssDataTelemetry_IDX);
        return GnssDataTelemetry_IDX;
    } else if (strcmp(sequence_string, TestI2CBUS_ID) == 0) {
        printf("\t => %d\n", TestI2CBUS_IDX);
        return TestI2CBUS_IDX;
    }  else if (strcmp(sequence_string, PowerControl_ID) == 0) {
        printf("\t => %d\n", PowerControl_IDX);
        return PowerControl_IDX;
    }   else if (strcmp(sequence_string, SesThermMgmt_ID) == 0) {
        printf("\t => %d\n", SesThermMgmt_IDX);
        return SesThermMgmt_IDX;
    }   else if (strcmp(sequence_string, SesTempReq_ID) == 0) {
        printf("\t => %d\n", SesTempReq_IDX);
        return SesTempReq_IDX;
    }   else if (strcmp(sequence_string, PaSatosMsg_ID) == 0) {
        printf("\t => %d\n", PaSatosMsg_IDX);
        return PaSatosMsg_IDX;
    }   else if (strcmp(sequence_string, ReadAcIp_ID) == 0) {
        printf("\t => %d\n", ReadAcIp_IDX);
        return ReadAcIp_IDX;
    }   

    
    printf("Unknown sequence, returning -1\n");
    return -1;
}

// Callback functions (PC => Application)
AntarisReturnCode start_sequence(StartSequenceParams *start_seq_param)
{
    printf("start_sequence with received id %s, params %s\n", &start_seq_param->sequence_id[0], &start_seq_param->sequence_params[0]);
    if (debug) {
        displayStartSequenceParams(start_seq_param);
    }

    // <Payload Application Business Logic>
    // Start Sequence FSM Thread
    current_sequence_idx = get_sequence_idx_from_seq_string(&start_seq_param->sequence_id[0]);
    if (current_sequence_idx ==  -1) {
        printf("Invalid Sequence \n");
        return An_GENERIC_FAILURE;
    }
    mythreadState_t *thread_state = payload_sequences_fsms[current_sequence_idx];
    strcpy(thread_state->received_name, &start_seq_param->sequence_id[0]);
    strncpy(thread_state->seq_params, start_seq_param->sequence_params, SEQ_PARAMS_LEN);
    thread_state->scheduled_deadline = start_seq_param->scheduled_deadline;
    thread_state->correlation_id = start_seq_param->correlation_id;
    fsmThreadStart(thread_state);

    return An_SUCCESS;
}

void wakeup_seq_fsm(mythreadState_t *threadState)
{   
    if (threadState == NULL) {
        printf("thread state is NULL!!\n");
        return;
    }
    sleep(1); // avoid lost wakeups by giving fsm thread time to block on its pthread_cond_wait
    printf("called wakeup_seq_fsm for %s\n", threadState->seq_id);
    pthread_cond_signal(&threadState->condition);
}

AntarisReturnCode process_req_payload_metrics(ReqPayloadMetricsParams *payload_metrics_param)
{
    PayloadMetricsResponse   resp_payload_metrics_params;

    printf("Before resp_payload_metrics_params\n");

    time_t now = time(0);
    UINT64 epoch = static_cast<UINT64>(now);
    resp_payload_metrics_params.correlation_id = payload_metrics_param->correlation_id;
    resp_payload_metrics_params.used_counter = 8;   // Example value
    resp_payload_metrics_params.timestamp = epoch;

    size_t max_metrics_counter = sizeof(((struct PayloadMetricsResponse*)0)->metrics) / sizeof(struct PayloadMetricsInfo);
    size_t metrics_name = sizeof(((struct PayloadMetricsInfo*)0)->names);

    // Total number of counters supported is 'max_metrics_counter', hence setting all names to 0
    for (int i = 0; i < max_metrics_counter ; i++) {
        resp_payload_metrics_params.metrics[i].counter = 0; 
        memset(resp_payload_metrics_params.metrics[i].names, 0, metrics_name);
    }

    // Set counter, names values
    for (int i=0; i< resp_payload_metrics_params.used_counter ; i++) {
        resp_payload_metrics_params.metrics[i].counter = i + 1;    // Example value
        sprintf(resp_payload_metrics_params.metrics[i].names, "%s_%d", "Counter", i);   // Example value
        printf("%d = %s \n", resp_payload_metrics_params.metrics[i].counter, resp_payload_metrics_params.metrics[i].names);
    }

    printf("payload_metrics_param : Got payload_metrics_param request from PC\n");

    if (debug) {
        displayPayloadMetricsResponse((const void *)&resp_payload_metrics_params);
    }

    api_pa_pc_response_payload_metrics(channel, &resp_payload_metrics_params);

    return An_SUCCESS;
}

AntarisReturnCode shutdown_app(ShutdownParams *shutdown_param)
{
    if (shutdown_param == NULL){
        printf("ERROR: shutdown params are NULL!");
        _exit(-1);
    }
    RespShutdownParams   resp_shutdown_params;

    printf("shutdown_app : Got Shutdown request from PC\n");
    printf("return code for shutdown is %u\n",shutdown_param->shutdown_reason);

    if (debug) {
        displayShutdownParams(shutdown_param);
    }

    shutdown_requested = 1;

    // <Payload Application Business Logic>
    // 1. Shutdown Payload Hardware
    // 2. Do whatever is required before shutting down the application
   
    // Now respond back to Payload Controller about shutdown completion 
    resp_shutdown_params.correlation_id = shutdown_param->correlation_id;
    resp_shutdown_params.req_status = An_SUCCESS;

    api_pa_pc_response_shutdown(channel, &resp_shutdown_params);

    printf("Current sequence id: %d\n", current_sequence_idx);

    for ( int thread_seq_id = 0; thread_seq_id < SEQUENCE_ID_MAX; thread_seq_id++ ) {
        printf("Stopping %s\n", payload_sequences_fsms[thread_seq_id]->seq_id);
        wakeup_seq_fsm(payload_sequences_fsms[thread_seq_id]);
    }

    return An_SUCCESS;
}

AntarisReturnCode process_health_check(HealthCheckParams *health_check_param)
{
    RespHealthCheckParams   resp_health_check_params;

    printf("process_health_check\n");
    if (debug) {
        displayHealthCheckParams(health_check_param);
    }

    resp_health_check_params.correlation_id = health_check_param->correlation_id;
    resp_health_check_params.application_state = application_state;
    resp_health_check_params.reqs_to_pc_in_err_cnt = reqs_to_pc_in_err_cnt;
    resp_health_check_params.resps_to_pc_in_err_cnt = resps_to_pc_in_err_cnt;

    api_pa_pc_response_health_check(channel, &resp_health_check_params);

    return An_SUCCESS;
}

AntarisReturnCode process_response_get_eps_voltage(GetEpsVoltage *get_eps_voltage)
{
    printf("process_response_get_eps_voltage\n");
    printf("eps voltage is: %f\n",get_eps_voltage->eps_voltage);
    if (debug) {
        displayGetEpsVoltage(get_eps_voltage);
    }

    // #<Payload Application Business Logic>
    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_idx]);
    return An_SUCCESS;
}

AntarisReturnCode process_response_thrml_ntf(SesThermalStatusNtf *ses_thermal_status_ntf)
{
    printf("processing SES thermal status notification\n");

    if (ses_thermal_status_ntf->heater_pwr_status == 0) {
        if (ses_thermal_status_ntf->hw_id_of_pwr_status == 0) { // 0: SESA
            printf("SESA power ON/OFF success\n");
        } else if (ses_thermal_status_ntf->hw_id_of_pwr_status == 1)  { // 1: SESB
            printf("SESB power ON/OFF success\n");
        } else {
            printf("Invalid HW ID\n");
        }
    } else {
        if (ses_thermal_status_ntf->hw_id_of_pwr_status == 0) { // 0: SESA
            printf("SESA power ON/OFF failure\n");
        } else if (ses_thermal_status_ntf->hw_id_of_pwr_status == 1)  { // 1: SESB
            printf("SESB power ON/OFF failure\n");
        } else {
            printf("Invalid HW ID\n");
        }
    }
    if (ses_thermal_status_ntf->heater_temp_status == 0) {
        if (ses_thermal_status_ntf->hw_id_of_temp_status == 0) { // 0: SESA    
            printf("SESA Temparature : %hhu celsius\n",ses_thermal_status_ntf->temperature);
        } else if (ses_thermal_status_ntf->hw_id_of_temp_status == 1)  { // 1: SESB
            printf("SESB Temparature : %hhu celsius\n",ses_thermal_status_ntf->temperature);
        } else {
            printf("Invalid HW ID\n");
        }
    } 

    if (debug) {
        displaySesThermalStatusNtf(ses_thermal_status_ntf);
    }
    // #<Payload Application Business Logic>
    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_idx]);
    return An_SUCCESS;
}

AntarisReturnCode process_response_ses_temp(RespSesTempReqParams *ses_temp_req_params)
{
    printf("Current temperature = %u\n",(unsigned int)ses_temp_req_params->temperature);
    printf("Heater power status = %u\n",(unsigned int)ses_temp_req_params->heater_pwr_status);  // 0:OFF, 1:ON
    if (debug) {
        displayRespSesTempReqParams(ses_temp_req_params);
        
    }
    // #<Payload Application Business Logic>
    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_idx]);
    return An_SUCCESS;
}

AntarisReturnCode process_response_pa_satos_msg(RespPaSatOsMsg *resp_pa_satos_message)
{
    printf("Command id = %hu , status = %d\n", resp_pa_satos_message->command_id, resp_pa_satos_message->req_status);
    if (debug) {
        displayPaSatOsMsg(resp_pa_satos_message);
        
    }
    // #<Payload Application Business Logic>
    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_idx]);
    return An_SUCCESS;
}

AntarisReturnCode process_response_gnss_eph_data(GnssEphData *gnss_eph_data)
{
    printf("process_response_gnss_eph_data\n");
    if (debug) {
        displayGnssEphData(gnss_eph_data);
    }

    const char *fields[] = {
        "Time Validity",
        "ECI Position Validity",
        "ECI Velocity Validity",
        "ECEF Position Validity",
        "ECEF Velocity Validity",
        "Angular Rate Validity",
        "Attitude Quaternion Validity",
        "Lat-Lon-Altitude Validity",
        "Nadir Vector Validity",
        "Geodetric Nadir Vector Validity",
        "Beta Angle Validity"
    };

    if(gnss_eph_data->gps_timeout_flag == 1) {
        printf("gps_fix_time: %d",gnss_eph_data->gps_eph_data.gps_fix_time);
        printf("gps_sys_time: %lld",gnss_eph_data->gps_eph_data.gps_sys_time);
        OBC_time obc = gnss_eph_data->gps_eph_data.obc_time;
        printf("obc_time : %02d:%02d:%02d.%03d Date: %02d/%02d/%d\n",
               obc.hour, obc.minute, obc.millisecond / 1000,
               obc.millisecond % 1000, obc.date, obc.month, obc.year);
        for(int i = 0; i<3; i++){
            printf("gps_position_ecef: %d\n",gnss_eph_data->gps_eph_data.gps_position_ecef[i]);
        }

        for(int i = 0; i<3; i++){
            printf("gps_velocity_ecef: %d\n",gnss_eph_data->gps_eph_data.gps_velocity_ecef[i]);
           
        }
        printf("gps_validity_flag_pos_vel: %d\n",gnss_eph_data->gps_eph_data.gps_validity_flag_pos_vel);
    } else if(gnss_eph_data->adcs_timeout_flag == 1) {
        printf("ADCS Orbit Propagator/System Time = %f\n",gnss_eph_data->adcs_eph_data.orbit_time);
        printf("ECI Position X (km) = %f\n",gnss_eph_data->adcs_eph_data.eci_position_x);
        printf("ECI Position Y (km) = %f\n",gnss_eph_data->adcs_eph_data.eci_position_y);
        printf("ECI Position Z (km) = %f\n",gnss_eph_data->adcs_eph_data.eci_position_z);
        printf("ECI Velocity X (km/s) = %f\n",gnss_eph_data->adcs_eph_data.eci_velocity_x);
        printf("ECI Velocity Y (km/s) = %f\n",gnss_eph_data->adcs_eph_data.eci_velocity_y);
        printf("ECI Velocity Z (km/s) = %f\n",gnss_eph_data->adcs_eph_data.eci_velocity_z);
        printf("ECEF Position X (km) = %f\n",gnss_eph_data->adcs_eph_data.ecef_position_x);
        printf("ECEF Position Y (km) = %f\n",gnss_eph_data->adcs_eph_data.ecef_position_y);
        printf("ECEF Position Z (km) = %f\n",gnss_eph_data->adcs_eph_data.ecef_position_z);
        printf("ECEF Velocity X (km/s) = %f\n",gnss_eph_data->adcs_eph_data.ecef_velocity_x);
        printf("ECEF Velocity Y (km/s) = %f\n",gnss_eph_data->adcs_eph_data.ecef_velocity_y);
        printf("ECEF Velocity Z (km/s) = %f\n",gnss_eph_data->adcs_eph_data.ecef_velocity_z);
        printf("X axis Angular rate (deg/s) = %f\n",gnss_eph_data->adcs_eph_data.ang_rate_x);
        printf("Y axis Angular rate (deg/s) = %f\n",gnss_eph_data->adcs_eph_data.ang_rate_y);
        printf("Z axis Angular rate (deg/s) = %f\n",gnss_eph_data->adcs_eph_data.ang_rate_z);
        printf("Attitude Quaternion 1 = %f\n",gnss_eph_data->adcs_eph_data.att_quat_1);
        printf("Attitude Quaternion 2 = %f\n",gnss_eph_data->adcs_eph_data.att_quat_2);
        printf("Attitude Quaternion 3 = %f\n",gnss_eph_data->adcs_eph_data.att_quat_3);
        printf("Attitude Quaternion 4 = %f\n",gnss_eph_data->adcs_eph_data.att_quat_4);
        printf("Latitude (deg) = %f\n",gnss_eph_data->adcs_eph_data.latitude);
        printf("Longitude (deg) = %f\n",gnss_eph_data->adcs_eph_data.longitude);
        printf("Altitude (km) %f\n",gnss_eph_data->adcs_eph_data.altitude);
        printf("X Nadir Vector %f\n",gnss_eph_data->adcs_eph_data.nadir_vector_x);
        printf("Y Nadir Vector %f\n",gnss_eph_data->adcs_eph_data.nadir_vector_y);
        printf("Z Nadir Vector %f\n",gnss_eph_data->adcs_eph_data.nadir_vector_z);
        printf("X Geodetic Nadir Vector %f\n",gnss_eph_data->adcs_eph_data.gd_nadir_vector_x);
        printf("Y Geodetic Nadir Vector %f\n",gnss_eph_data->adcs_eph_data.gd_nadir_vector_y);
        printf("Z Geodetic Nadir Vector %f\n",gnss_eph_data->adcs_eph_data.gd_nadir_vector_z);
        printf("Beta Angle (deg) %f\n",gnss_eph_data->adcs_eph_data.beta_angle);
        for (int i = 0; i < sizeof(fields)/sizeof(fields[0]); i++) {
            int bit_value = (gnss_eph_data->adcs_eph_data.validity_flags >> i) & 1;
            printf("%s: %d\n", fields[i], bit_value);

        }
    }
    // #<Payload Application Business Logic>
    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_idx]);
    return An_SUCCESS;
}

AntarisReturnCode process_response_register(RespRegisterParams *resp_register_param)
{
    printf("process_response_register\n");
    if (debug) {
        displayRespRegisterParams(resp_register_param);
    }

    // <Payload Application Business Logic>
    return An_SUCCESS;
}

AntarisReturnCode process_response_get_current_location(RespGetCurrentLocationParams *resp_get_curr_location_param)
{
    printf("process_response_get_current_location\n");
    if (debug) {
        displayRespGetCurrentLocationParams(resp_get_curr_location_param);
    }

    // #<Payload Application Business Logic>
    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_idx]);
    return An_SUCCESS;
}

AntarisReturnCode process_response_stage_file_download(RespStageFileDownloadParams *resp_stage_file_download)
{
    printf("process_response_stage_file_download\n");
    if (debug) {
        displayRespStageFileDownloadParams(resp_stage_file_download);
    }

    // #<Payload Application Business Logic>
    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_idx]);
    return An_SUCCESS;
}

AntarisReturnCode process_response_payload_power_control(RespPayloadPowerControlParams *resp_payload_power_control)
{
    printf("process_response_payload_power_control\n");
    if (debug) {
        displayRespPayloadPowerControlParams(resp_payload_power_control);
    }

    // #<Payload Application Business Logic>
    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_idx]);
    return An_SUCCESS;
}

int main(int argc, char *argv[])
{
    unsigned short int correlation_id = 0;
    AntarisReturnCode ret;
    void *exit_status;

    init_satos_lib();
    
    // Callback functions (PC => PA)
    AntarisApiCallbackFuncList callback_func_list = {
            start_sequence: start_sequence,
            shutdown_app : shutdown_app,
            process_health_check : process_health_check,
            process_response_register : process_response_register,
            process_response_get_current_location : process_response_get_current_location,
            process_response_stage_file_download : process_response_stage_file_download,
            process_response_payload_power_control : process_response_payload_power_control,
            req_payload_metrics: process_req_payload_metrics,
            process_cb_gnss_eph_data : process_response_gnss_eph_data,
            process_cb_get_eps_voltage: process_response_get_eps_voltage,
            process_response_ses_temp_req: process_response_ses_temp,
            process_cb_ses_thrml_ntf: process_response_thrml_ntf,
            process_pa_satos_msg_response: process_response_pa_satos_msg,
    };

    // Create Channel to talk to Payload Controller (PC)
    channel = api_pa_pc_create_channel(&callback_func_list);

    if (channel == (AntarisChannel)NULL) {
        fprintf(stderr, "api_pa_pc_create_channel failed \n");
        _exit(-1);
    }

    // Create FSM threads (arg : channel, count, seq_id, fsm-function)
    payload_sequences_fsms[HelloWorld_IDX] = fsmThreadCreate(channel, 1, HelloWorld_ID, handle_HelloWorld);
    payload_sequences_fsms[HelloFriend_IDX] = fsmThreadCreate(channel, 1, HelloFriend_ID, handle_HelloFriend);
    payload_sequences_fsms[LogLocation_IDX] = fsmThreadCreate(channel, 1, LogLocation_ID, handle_LogLocation);
    payload_sequences_fsms[TestGPIO_Sequence_IDX] = fsmThreadCreate(channel, 1, TestGPIO_Sequence_ID, handle_TestGPIO);
    payload_sequences_fsms[StageFile_Sequence_IDX] = fsmThreadCreate(channel, 1, StageFile_Sequence_ID, handle_StageFile);
    payload_sequences_fsms[TestCANBus_Sequence_IDX] = fsmThreadCreate(channel, 1, TestCANBus_Sequence_ID, handle_TestCANBus);
    payload_sequences_fsms[EpsVoltageTelemetry_IDX] = fsmThreadCreate(channel, 1, EpsVoltageTelemetry_ID, handle_Eps_Voltage_Telemetry_Request);
    payload_sequences_fsms[GnssDataTelemetry_IDX] = fsmThreadCreate(channel, 1, GnssDataTelemetry_ID, handle_gnss_data_Telemetry_Request);
    payload_sequences_fsms[TestI2CBUS_IDX] = fsmThreadCreate(channel, 1, TestI2CBUS_ID, handle_TestI2CBus);
    payload_sequences_fsms[PowerControl_IDX] = fsmThreadCreate(channel, 1, PowerControl_ID, handle_PowerControl);
    payload_sequences_fsms[SesThermMgmt_IDX] = fsmThreadCreate(channel, 1, SesThermMgmt_ID, handle_ses_therm_mgmnt);
    payload_sequences_fsms[SesTempReq_IDX] = fsmThreadCreate(channel, 1, SesTempReq_ID, handle_ses_temp_req);
    payload_sequences_fsms[PaSatosMsg_IDX] = fsmThreadCreate(channel, 1, PaSatosMsg_ID, handle_pa_satos_message);
    payload_sequences_fsms[ReadAcIp_IDX] = fsmThreadCreate(channel, 1, ReadAcIp_ID, handle_ac_ip_read);

    // Register application with PC
    // 2nd parameter decides PC's action on PA's health check failure
    // 0 => No Action, 1 => Reboot PA
    ReqRegisterParams register_params = {0};
    register_params.correlation_id = correlation_id;
    register_params.health_check_fail_action = 0;
    ret = api_pa_pc_register(channel, &register_params);
    printf("api_pa_pc_register returned %d (%s)\n", ret, ret == An_SUCCESS ? "SUCCESS" : "FAILURE");
    correlation_id += 1;

    // After registration, simulated PC will ask application to start sequence HelloWorld

    printf("All sequences ready to start ...\n");

    while (shutdown_requested == 0) {
        sleep(1);
    }

    printf("Detected shutdown request, waiting for sequence cleanups\n");

    // Wait for all FSM threads to complete

    if (strcmp(payload_sequences_fsms[HelloWorld_IDX]->state, "NOT_STARTED") != 0) {
        pthread_join(payload_sequences_fsms[HelloWorld_IDX]->thread_id, &exit_status);
    }

    if (strcmp(payload_sequences_fsms[HelloFriend_IDX]->state, "NOT_STARTED") != 0) {    
        pthread_join(payload_sequences_fsms[HelloFriend_IDX]->thread_id, &exit_status);
    }

    if (strcmp(payload_sequences_fsms[LogLocation_IDX]->state, "NOT_STARTED") != 0) {
        pthread_join(payload_sequences_fsms[LogLocation_IDX]->thread_id, &exit_status);
    }

    if (strcmp(payload_sequences_fsms[TestGPIO_Sequence_IDX]->state, "NOT_STARTED") != 0) {
        pthread_join(payload_sequences_fsms[TestGPIO_Sequence_IDX]->thread_id, &exit_status);
    }

    if (strcmp(payload_sequences_fsms[StageFile_Sequence_IDX]->state, "NOT_STARTED") != 0) {
        pthread_join(payload_sequences_fsms[StageFile_Sequence_IDX]->thread_id, &exit_status);
    }
    if (strcmp(payload_sequences_fsms[EpsVoltageTelemetry_IDX]->state, "NOT_STARTED") != 0) {
        pthread_join(payload_sequences_fsms[EpsVoltageTelemetry_IDX]->thread_id, &exit_status);
    }

    if (strcmp(payload_sequences_fsms[GnssDataTelemetry_IDX]->state, "NOT_STARTED") != 0) {
        pthread_join(payload_sequences_fsms[GnssDataTelemetry_IDX]->thread_id, &exit_status);
    }
    
    if (strcmp(payload_sequences_fsms[TestI2CBUS_IDX]->state, "NOT_STARTED") != 0) {
        pthread_join(payload_sequences_fsms[TestI2CBUS_IDX]->thread_id, &exit_status);
    }

    if (strcmp(payload_sequences_fsms[SesThermMgmt_IDX]->state, "NOT_STARTED") != 0) {
        pthread_join(payload_sequences_fsms[SesThermMgmt_IDX]->thread_id, &exit_status);
    }

    if (strcmp(payload_sequences_fsms[SesTempReq_IDX]->state, "NOT_STARTED") != 0) {
        pthread_join(payload_sequences_fsms[SesTempReq_IDX]->thread_id, &exit_status);
    }

    if (strcmp(payload_sequences_fsms[PaSatosMsg_IDX]->state, "NOT_STARTED") != 0) {
        pthread_join(payload_sequences_fsms[PaSatosMsg_IDX]->thread_id, &exit_status);
    }

    if (strcmp(payload_sequences_fsms[ReadAcIp_IDX]->state, "NOT_STARTED") != 0) {
        pthread_join(payload_sequences_fsms[ReadAcIp_IDX]->thread_id, &exit_status);
    }
    
    printf("Cleaning up sequence resources\n");

    fsmThreadCleanup(payload_sequences_fsms[HelloWorld_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[HelloFriend_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[LogLocation_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[TestGPIO_Sequence_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[StageFile_Sequence_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[EpsVoltageTelemetry_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[GnssDataTelemetry_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[TestI2CBUS_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[SesThermMgmt_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[SesTempReq_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[PaSatosMsg_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[ReadAcIp_IDX]);

    // Delete Channel
    api_pa_pc_delete_channel(channel);

    printf("==== All Done: Exiting Main Thread ====\n\n");

    // Deafault timeout is set to exit the function running inside this function
    with_timeout_deinit_satos_lib();
    _exit(0);
}
