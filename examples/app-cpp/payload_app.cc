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

#include "antaris_api.h"

#define MAX_STR_LEN 256
#define SEQ_PARAMS_LEN 64
#define SEQ_NAME_LEN   32

#define HelloWorld_ID                   "HelloWorld"
#define HelloWorld_IDX                  0
#define HelloFriend_ID                  "HelloFriend"
#define HelloFriend_IDX                 1
#define LogLocation_ID                  "LogLocation"
#define LogLocation_IDX                 2
#define SEQUENCE_ID_MAX                 3

#define APP_STATE_ACTIVE                1  // 1 => Indicates application is running

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

static void sequence_cleanup(mythreadState_t *mythread)
{
    printf("%s: state %s, cleaning up sequence processor\n", mythread->seq_id, mythread->state);
    pthread_mutex_unlock(&mythread->cond_lock);
    printf("%s: cleaned up...\n", mythread->seq_id);
    return;
}

static void sequence_lock(mythreadState_t *mythread)
{
    printf("%s: state %s, scheduled_deadline %ul\n", mythread->seq_id, mythread->state, mythread->scheduled_deadline);

    /* lock mutex - all cond-waits require this */
    pthread_mutex_lock(&mythread->cond_lock);

    printf("%s: starting sequence ...\n", mythread->seq_id);
    return;
}

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

    sequence_lock(mythread);

    // Initialize correlation_id
    mythread->correlation_id = 0;

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

    mythread->correlation_id += 1;
    strcpy(&mythread->state[0], "WAITING_FOR_GET_LOCATION");
    printf("%s: state %s\n", mythread->seq_id, mythread->state);
    /* wait for condition */
    pthread_cond_wait(&mythread->condition, &mythread->cond_lock);

    if (shutdown_requested) {
        printf("%s: exiting as shutdown was requested\n", mythread->seq_id);
        sequence_cleanup(mythread);
        return;
    }

    // Request PC to stage a file download
    ReqStageFileDownloadParams stage_file_download_params = {0};
    stage_file_download_params.correlation_id = mythread->correlation_id;
    strncpy(&stage_file_download_params.file_path[0], "telemetry/payload_teledata.txt", ANTARIS_MAX_FILE_NAME_LENGTH);
    ret = api_pa_pc_stage_file_download(channel, &stage_file_download_params);

    printf("%s: sent stage file-download request with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_stage_file_download failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_stage_file_download returned success, ret %d\n", __FUNCTION__, ret);
    }

    mythread->correlation_id += 1;
    strcpy(&mythread->state[0], "WAITING_FOR_STAGE_FILE_DOWNLOAD_COMPLETION");
    printf("%s: state %s\n", mythread->seq_id, mythread->state);
    /* wait for condition */
    pthread_cond_wait(&mythread->condition, &mythread->cond_lock);

    if (shutdown_requested) {
        printf("%s: exiting as shutdown was requested\n", mythread->seq_id);
        sequence_cleanup(mythread);
        return;
    }

    // Request PC to power-off my payload device
    ReqPayloadPowerControlParams payload_power_control_params = {0};
    payload_power_control_params.correlation_id = mythread->correlation_id;
    payload_power_control_params.power_operation = 0;	//0=>Power-Off, 1=>Power-On, 2=>Power-Cycle
    ret = api_pa_pc_payload_power_control(mythread->channel, &payload_power_control_params);

    printf("%s: sent payload-power-ctrl request with correlation_id %u\n", mythread->seq_id, mythread->correlation_id);
    if (An_SUCCESS != ret) {
        fprintf(stderr, "%s: api_pa_pc_payload_power_control failed, ret %d\n", __FUNCTION__, ret);
        _exit(-1);
    } else {
        printf("%s: api_pa_pc_payload_power_control returned success, ret %d\n", __FUNCTION__, ret);
    }

    mythread->correlation_id += 1;
    strcpy(&mythread->state[0], "WAITING_FOR_PAYLOAD_POWER_OFF");
    printf("%s: state %s\n", mythread->seq_id, mythread->state);
    /* wait for condition */
    pthread_cond_wait(&mythread->condition, &mythread->cond_lock);

    if (shutdown_requested) {
        printf("%s: exiting as shutdown was requested\n", mythread->seq_id);
        sequence_cleanup(mythread);
        return;
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

    current_time_in_ms = time(nullptr) * 1000;

    if (mythread->scheduled_deadline > current_time_in_ms) {
        printf("CONGRATULATIONS! :  %s completed at %ul earlier than the deadline %ul\n", mythread->seq_id, current_time_in_ms, mythread->scheduled_deadline);
    } else {
        printf("WARNING! :  %s completed at %ul, after the deadline %ul\n", mythread->seq_id, current_time_in_ms, mythread->scheduled_deadline);
    }

    /* wait for condition */
    pthread_cond_wait(&mythread->condition, &mythread->cond_lock);
    // TODO : Put a check here if thread woke up due to shutdown-request from PC or something else
    strcpy(&mythread->state[0], "COMPLETED-READY-TO-RESTART");
    printf("%s : state %s\n", mythread->seq_id, mythread->state);

    sequence_cleanup(mythread);
    return;
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
    mythreadState_t *thread_state = payload_sequences_fsms[current_sequence_idx];
    strcpy(thread_state->received_name, &start_seq_param->sequence_id[0]);
    strncpy(thread_state->seq_params, start_seq_param->sequence_params, SEQ_PARAMS_LEN);
    thread_state->scheduled_deadline = start_seq_param->scheduled_deadline;
    fsmThreadStart(thread_state);

    return An_SUCCESS;
}

void wakeup_seq_fsm(mythreadState_t *threadState)
{
    sleep(1); // avoid lost wakeups by giving fsm thread time to block on its pthread_cond_wait
    pthread_cond_signal(&threadState->condition);
}

AntarisReturnCode shutdown_app(ShutdownParams *shutdown_param)
{
    RespShutdownParams   resp_shutdown_params;

    printf("shutdown_app : Got Shutdown request from PC\n");

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

    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_idx]);

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
    
    // Callback functions (PC => PA)
    AntarisApiCallbackFuncList callback_func_list = {
            start_sequence: start_sequence,
            shutdown_app : shutdown_app,
            process_health_check : process_health_check,
            process_response_register : process_response_register,
            process_response_get_current_location : process_response_get_current_location,
            process_response_stage_file_download : process_response_stage_file_download,
            process_response_payload_power_control : process_response_payload_power_control,
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

    printf("Cleaning up sequence resources\n");

    fsmThreadCleanup(payload_sequences_fsms[HelloWorld_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[HelloFriend_IDX]);
    fsmThreadCleanup(payload_sequences_fsms[LogLocation_IDX]);

    // Delete Channel
    api_pa_pc_delete_channel(channel);

    printf("==== All Done: Exiting Main Thread ====\n\n");

    _exit(0);
}
