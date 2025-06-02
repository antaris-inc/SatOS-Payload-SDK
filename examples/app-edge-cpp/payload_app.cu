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
#include <cuda_runtime.h>

#include "antaris_api.h"
#include "antaris_api_gpio.h"
#include "antaris_api_pyfunctions.h"
#include "antaris_can_api.h"

#define MAX_STR_LEN 256
#define SEQ_PARAMS_LEN 64
#define SEQ_NAME_LEN   32

#define HelloWorld_ID                   "HelloWorld"
#define HelloWorld_IDX                  0

#define SEQUENCE_ID_MAX                 2

#define APP_STATE_ACTIVE                0  // Application State : Good (0), Error (non-Zero)

#define MAX_DATA_BYTES 8
#define SEND_MSG_LIMIT 10

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

__global__ void helloFromGPU() {
    printf("Handling sequence: Hello World from GPU thread %d of %d\n", threadIdx.x, blockDim.x);
}

//This sequence should complete within its scheduled_deadline
void handle_HelloWorld(mythreadState_t *mythread)
{
    AntarisReturnCode ret;

    helloFromGPU<<<1, 5>>>();

    // Wait for GPU to finish before accessing results
    cudaDeviceSynchronize();

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
    };

    // Create Channel to talk to Payload Controller (PC)
    channel = api_pa_pc_create_channel(&callback_func_list);

    if (channel == (AntarisChannel)NULL) {
        fprintf(stderr, "api_pa_pc_create_channel failed \n");
        _exit(-1);
    }

    // Create FSM threads (arg : channel, count, seq_id, fsm-function)
    payload_sequences_fsms[HelloWorld_IDX] = fsmThreadCreate(channel, 1, HelloWorld_ID, handle_HelloWorld);

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

    

    printf("Cleaning up sequence resources\n");

    fsmThreadCleanup(payload_sequences_fsms[HelloWorld_IDX]);

    // Delete Channel
    api_pa_pc_delete_channel(channel);

    printf("==== All Done: Exiting Main Thread ====\n\n");

    // Deafault timeout is set to exit the function running inside this function
    with_timeout_deinit_satos_lib();
    _exit(0);
}
