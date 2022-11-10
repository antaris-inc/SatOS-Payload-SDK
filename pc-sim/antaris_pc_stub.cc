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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <string>

#include "antaris_api.h"
#include "antaris_api_internal.h"
#include "antaris_pc_to_app_api.h"
#include "config_db.h"

#define PC_SEQUENCE_ID_A   "Sequence_A"
#define PC_SEQUENCE_ID_B   "Sequence_B"
#define PC_SEQUENCE_ID_C   "Sequence_C"

typedef struct AntarisInternalServerContext_s {
    PCToAppClientContext                app_callback_client;;
    int                                 encryption; // 0 => no-encryption, 1 => encryption enabled
    PCToAppApiId_e                      cb_id;
    unsigned short int                  cb_correlation_id;
    pthread_cond_t                      condition;
    pthread_mutex_t                     cond_lock;
    pthread_t                           simulated_pc_thread_id;
    bool                                server_ready;
} AntarisInternalServerContext_t;

void *simulated_pc(void *thread_param);
void app_to_pc_api_handler(PCApiServerContext ctx, const INT8 *peer_string, AppToPCCallbackId_e cb_id, AppToPCCallbackParams_t *cb_params, AntarisReturnCode *out_return_code);
void wakeup_pc(AntarisInternalServerContext_t *channel, PCToAppApiId_e cb_id, unsigned short int correlation_id);

// Logic and data behind the server's behavior.
class PayloadControllerServiceImpl {
public:
    void PC_register(const ReqRegisterParams* api_request, AntarisReturnCode* response) {
        // Instantiate the client. It requires a channel, out of which the actual RPCs
        // are created. This channel models a connection to an endpoint specified by
        // the argument "--target=" which is the only expected argument.
        // We indicate that the channel isn't authenticated (use of
        // InsecureChannelCredentials()).
        server_context = (AntarisInternalServerContext_t *)malloc(sizeof(AntarisInternalServerContext_t));
        pthread_attr_t attr;

        if (!server_context) {
            *response = An_OUT_OF_RESOURCES;
            return;
        }

        memset(server_context, 0, sizeof(AntarisInternalServerContext_t));

        pthread_mutex_init(&server_context->cond_lock, NULL);
        pthread_cond_init(&server_context->condition, NULL);

        pthread_attr_init(&attr);
        pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
        pthread_create(&server_context->simulated_pc_thread_id, &attr, simulated_pc, server_context);

        std::string target_str(APP_CALLBACK_GRPC_CONNECT_ENDPOINT);

        *response = An_SUCCESS;

        server_context->app_callback_client = an_pc_pa_create_client((INT8 *)PEER_IP, APP_GRPC_CALLBACK_PORT);

        printf("PC_register: Created callback channel using %s\n", target_str.c_str());

        while (!server_context->server_ready) {
            sleep(1);
        }

        printf("Waking up simumated_pc for responding with cb_id %d\n", e_PC2App_responseRegister);
        wakeup_pc(server_context, e_PC2App_responseRegister, api_request->correlation_id);

        return;
    }

    void PC_get_current_location(const ReqGetCurrentLocationParams* api_request, AntarisReturnCode* response) {
        // earlier -> wakeup_pc(channel_ctx, (void *)channel_ctx->callbacks.process_response_get_current_location, correlation_id);
        *response = An_SUCCESS;

        printf("Received get-current-location request with correlation id %d\n", api_request->correlation_id);

        wakeup_pc(server_context, e_PC2App_responseGetCurrentLocation, api_request->correlation_id);

        return;
    }

    void PC_stage_file_download(const ReqStageFileDownloadParams* api_request, AntarisReturnCode* response) {
        // earlier ->  wakeup_pc(channel_ctx, (void *)channel_ctx->callbacks.process_response_stage_file_download, stage_file_download_params->correlation_id);
        *response = An_SUCCESS;

        // displayReqStageFileDownloadParams(&api_request);

        printf("Received download-file-to-gs request with correlation id %d\n", api_request->correlation_id);

        wakeup_pc(server_context, e_PC2App_responseStageFileDownload, api_request->correlation_id);

        return;
    }

    void PC_sequence_done(const CmdSequenceDoneParams* api_request, AntarisReturnCode* response) {
        *response = An_SUCCESS;

        // displayCmdSequenceDoneParams(api_request);

        printf("Received cmd-sequence-done request with seqeunce id %d\n", api_request->sequence_id);

	// Using -1 as correlation id for the time being. There is no response to sequence_done
        wakeup_pc(server_context, e_PC2App_shutdownApp, -1);

        return;
    }

    void PC_payload_power_control(const ReqPayloadPowerControlParams* api_request, AntarisReturnCode* response) {
        // earlier ->  wakeup_pc(channel_ctx, (void *)channel_ctx->callbacks.process_response_payload_power_control, payload_power_control_params->correlation_id);
        *response = An_SUCCESS;

        displayReqPayloadPowerControlParams(api_request);

        printf("Received payload-power-control request with correlation id %d\n", api_request->correlation_id);

        wakeup_pc(server_context, e_PC2App_responsePayloadPowerControl, api_request->correlation_id);

        return;
    }

    void PC_response_health_check(const RespHealthCheckParams* api_request, AntarisReturnCode* response) {
        *response = An_SUCCESS;

        displayRespHealthCheckParams(api_request);

        printf("Received health-check response with correlation id %d\n", api_request->correlation_id);

	//PC does not need to respond back to PA in response to response to health-check
        //wakeup_pc(server_context, e_PC2App_responseHealthCheck, api_request->correlation_id);

        return;
    }
 
    void PC_response_shutdown(const RespShutdownParams* api_request, AntarisReturnCode* response) {
        *response = An_SUCCESS;

        displayRespShutdownParams(api_request);

        printf("Received shutdown response with correlation id %d\n", api_request->correlation_id);

       //PC does not need to respond back to PA in response to response to shutdown 
        //wakeup_pc(server_context, e_PC2App_responseShutdown, api_request->correlation_id);

        return;
    }

    private:
        AntarisInternalServerContext_t *server_context;

};

PayloadControllerServiceImpl g_pc_service;

void app_to_pc_api_handler(PCApiServerContext ctx, const INT8 *peer_string, AppToPCCallbackId_e cb_id, AppToPCCallbackParams_t *cb_params, AntarisReturnCode *out_return_code)
{
    printf("app_to_pc_api_handler: Received server-api request on ctx %p, from %s, cb-id %u\n", ctx, peer_string, cb_id);
    *out_return_code = An_NOT_IMPLEMENTED;

    switch (cb_id) {

    case e_app2PC_register:
        g_pc_service.PC_register(&cb_params->register_request, out_return_code);
        break;

    case e_app2PC_getCurrentLocation:
        g_pc_service.PC_get_current_location(&cb_params->get_location, out_return_code);
        break;

    case e_app2PC_stageFileDownload:
        g_pc_service.PC_stage_file_download(&cb_params->stage_file_download, out_return_code);
        break;

    case e_app2PC_sequenceDone:
        g_pc_service.PC_sequence_done(&cb_params->sequence_done, out_return_code);
        break;

    case e_app2PC_payloadPowerControl:
        g_pc_service.PC_payload_power_control(&cb_params->payload_power_ctrl, out_return_code);
        break;

    case e_app2PC_healthCheckResponse:
        g_pc_service.PC_response_health_check(&cb_params->health_check_response, out_return_code);
        break;

    case e_app2PC_shutdownResponse:
        g_pc_service.PC_response_shutdown(&cb_params->shutdown_response, out_return_code);
        break;

    } // switch cb_id

    printf("app_to_pc_api_handler: Returning %d\n", *out_return_code);

    return;
}

static void get_start_seq_from_user_conf(StartSequenceParams *out_params, char *conf_data)
{
    char *id_end = strchr(conf_data, ':');
    unsigned int field_len;
    char *param_end, *duration_end;

    /* extract seq-id */
    field_len = (unsigned long long)id_end - (unsigned long long)conf_data;

    strncpy(out_params->sequence_id, conf_data, field_len);

    /* extract params */
    param_end = strchr(id_end + 1, ':');

    field_len = (unsigned long long)param_end - ((unsigned long long)id_end + 1);

    strncpy(out_params->sequence_params, id_end + 1, field_len);

    /* extract duration */
    sscanf(param_end + 1, "%llu", &out_params->scheduled_deadline);

    /* make it relative to 'now' */
    out_params->scheduled_deadline += time(NULL) * 1000;

    return;
}

static void send_start_sequence_with_user_conf(AntarisInternalServerContext_t *channel, char *conf_data)
{
    StartSequenceParams start_sequence_params = {correlation_id: 0};
    PCToAppApiParams_t api_params = {0};

    get_start_seq_from_user_conf(&start_sequence_params, &conf_data[0]);

    memcpy(&api_params, &start_sequence_params, sizeof(start_sequence_params));

    printf("%s: ============> Invoke start-sequence for sequence-id %s, params %s, deadline %llu\n",
        __FUNCTION__, start_sequence_params.sequence_id, start_sequence_params.sequence_params, start_sequence_params.scheduled_deadline);

    // displayStartSequenceParams(&start_sequence_params);
    an_pc_pa_invoke_api(channel->app_callback_client, e_PC2App_startSequence, &api_params);

    return;
}

// API functions called by Payload Application and served by Payload Controller
void *simulated_pc(void *thread_param)
{
    AntarisInternalServerContext_t *channel = (AntarisInternalServerContext_t *)thread_param;
    PCToAppApiParams_t api_params;
    int api_id;
    char step_data[SEQ_CONF_MAX_DATA_LEN];
    int conf_ret;

    printf("Simulated PC Started\n");
    channel->server_ready = true;
    pthread_mutex_lock(&channel->cond_lock);

    while (1) {
        pthread_cond_wait(&channel->condition, &channel->cond_lock); // sleep until next api is called
        if (channel->cb_id == e_PC2App_invalid) {
            fprintf(stderr, "Simulated PC Exiting\n");
            break;
        }

        printf("Payload Controller woken up by peer request/response with cb_id %d\n", channel->cb_id);

        /*
         * slow down the PC-stub so that rpc immediate response to the client's API call does not fall behind the asynchronous response
         * or subsequent callback from this thread.
         */
        sleep(2);

        memset(&api_params, 0, sizeof(api_params));

        if (channel->cb_id == e_PC2App_responseRegister) {
            RespRegisterParams resp_register_params = {correlation_id: channel->cb_correlation_id, req_status: An_SUCCESS};

            printf("%s: Sending register response for corr-id %u\n", __FUNCTION__, channel->cb_correlation_id);

            strcpy(&resp_register_params.auth_token[0], "auth_token");
            
            memcpy(&api_params, &resp_register_params, sizeof(resp_register_params));

            an_pc_pa_invoke_api(channel->app_callback_client, e_PC2App_responseRegister, &api_params);

	        uint64_t timeSinceEpochMilliseconds = (time(nullptr) + 30) * 1000;	//deadline after 30 seconds from now

            // Start the first user configured sequence, or a default
            StartSequenceParams start_sequence_params = {correlation_id: 0, scheduled_deadline:timeSinceEpochMilliseconds};

            conf_ret = scenario_get_conf(SCENARIO_API_ID_REG_RESPONSE, 1, &api_id, &step_data[0]);

            if (SEQ_CONF_FOUND == conf_ret) {
                printf("%s: [registration-response] User step-data found for start-sequence\n", __FUNCTION__);
                send_start_sequence_with_user_conf(channel, &step_data[0]);
            } else {
                strcpy(&start_sequence_params.sequence_id[0], PC_SEQUENCE_ID_A);
                printf("%s: no user-step found for start sequence, using default\n", __FUNCTION__);

                memset(&api_params, 0, sizeof(api_params));
                memcpy(&api_params, &start_sequence_params, sizeof(start_sequence_params));

                printf("%s: ============> [Registration-Request] Invoke start-sequence for sequence-id %s, params %s, deadline %llu\n",
                    __FUNCTION__, start_sequence_params.sequence_id, start_sequence_params.sequence_params, start_sequence_params.scheduled_deadline);

                // displayStartSequenceParams(&start_sequence_params);
                an_pc_pa_invoke_api(channel->app_callback_client, e_PC2App_startSequence, &api_params);
            }

        } else if (channel->cb_id == e_PC2App_responseGetCurrentLocation) {
            RespGetCurrentLocationParams resp_get_curr_location_params = {correlation_id: channel->cb_correlation_id, req_status: An_SUCCESS,
                                                                                                        longitude: 100, latitude: 100, altitude: 100};
            memcpy(&api_params, &resp_get_curr_location_params, sizeof(resp_get_curr_location_params));
            an_pc_pa_invoke_api(channel->app_callback_client, e_PC2App_responseGetCurrentLocation, &api_params);
        } else if (channel->cb_id == e_PC2App_responseStageFileDownload) {
            RespStageFileDownloadParams resp_stage_file_download_params = {correlation_id: channel->cb_correlation_id, req_status: An_SUCCESS};
            memcpy(&api_params, &resp_stage_file_download_params, sizeof(resp_stage_file_download_params));
            an_pc_pa_invoke_api(channel->app_callback_client, e_PC2App_responseStageFileDownload, &api_params);
        } else if (channel->cb_id == e_PC2App_responsePayloadPowerControl) {
            RespPayloadPowerControlParams resp_payload_power_control_params = {correlation_id: channel->cb_correlation_id, req_status: An_SUCCESS};
            memcpy(&api_params, &resp_payload_power_control_params, sizeof(resp_payload_power_control_params));
            an_pc_pa_invoke_api(channel->app_callback_client, e_PC2App_responsePayloadPowerControl, &api_params);
        } else if (channel->cb_id == e_PC2App_shutdownApp) {
            conf_ret = scenario_get_conf(SCENARIO_API_ID_SEQUENCE_DONE, 1, &api_id, &step_data[0]);

            if (SEQ_CONF_FOUND == conf_ret) {
	            printf("%s: [sequence-done] User step-data found for start-sequence\n", __FUNCTION__);
                send_start_sequence_with_user_conf(channel, &step_data[0]);
            } else {
                ShutdownParams req_shutdown_params = {correlation_id: channel->cb_correlation_id, grace_time: 10};
                memcpy(&api_params, &req_shutdown_params, sizeof(req_shutdown_params));
                printf("%s: No succeeding user-configured start-seqeunce found, shutting down app\n", __FUNCTION__);
                an_pc_pa_invoke_api(channel->app_callback_client, e_PC2App_shutdownApp, &api_params);
            }
	    }
    }
    pthread_mutex_unlock(&channel->cond_lock);

    return channel;
}

void wakeup_pc(AntarisInternalServerContext_t *channel, PCToAppApiId_e cb_id, unsigned short int correlation_id)
{
    printf("%s: Waking up PC-sim thread with cb_id %d, corr-id %u\n", __FUNCTION__, cb_id, correlation_id);
    sleep(1); // allow simulate_pc to wait for cond at top of loop, should be solved with synchronization, or a semaphore!
    channel->cb_id = cb_id;
    channel->cb_correlation_id = correlation_id;
    printf("%s: Signalled\n", __FUNCTION__);
    pthread_cond_signal(&channel->condition); // signal that a new api has been called 
}

int main(int argc, char *argv[])
{
    PCApiServerContext server_ctx = an_pc_pa_create_server(SERVER_GRPC_PORT, app_to_pc_api_handler);

    scenario_register_conf(argc, argv);

    printf("Server has started at %s\n", SERVER_GRPC_LISTEN_ENDPOINT);

    while (1) {
        sleep (10);
    }

    return 0;
}
