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
#include <fstream>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "antaris_api.h"
#include "antaris_api_internal.h"
#include "antaris_sdk_environment.h"

#include "antaris_api.grpc.pb.h"
#include "antaris_api.pb.h"
#include "antaris_sdk_version.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::Status;

unsigned int api_debug = 0;

extern char g_SSL_ENABLE;

extern char g_TRUETWIN_ENABLE;

#define ANTARIS_CALLBACK_GRACE_DELAY    10

class PCServiceClient {
 public:
  PCServiceClient(std::shared_ptr<Channel> channel)
      : stub_(antaris_api_peer_to_peer::AntarisapiPayloadController::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.

  AntarisReturnCode SetCookie(char *str){
    if (NULL == str ){
        return An_INVALID_PARAMS;
    }
    if (strlen(str) > COOKIE_LEN){
        return An_INVALID_PARAMS;
    }
    strcpy(this->cookie_str , str);
    return An_SUCCESS;
  }

  AntarisReturnCode Invoke_PC_register(ReqRegisterParams *req_params) {
        antaris_api_peer_to_peer::ReqRegisterParams pc_req;
        antaris_api_peer_to_peer::AntarisReturnType pc_response;
        antaris_api_peer_to_peer::AntarisSdkVersion *p_version;
        Status pc_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_ReqRegisterParams(req_params, &pc_req);


        context.AddMetadata( COOKIE_STR, this->cookie_str);

        p_version = pc_req.mutable_sdk_version();
    
        p_version->set_major(ANTARIS_PA_PC_SDK_MAJOR_VERSION);
        p_version->set_minor(ANTARIS_PA_PC_SDK_MINOR_VERSION);
        p_version->set_patch(ANTARIS_PA_PC_SDK_PATCH_VERSION);

        printf("%s: Invoking PC_register api towards PC, sdk version %d.%d.%d\n",
                __FUNCTION__, ANTARIS_PA_PC_SDK_MAJOR_VERSION, ANTARIS_PA_PC_SDK_MINOR_VERSION,
                ANTARIS_PA_PC_SDK_PATCH_VERSION);

        pc_status = stub_->PC_register(&context, pc_req, &pc_response);

        printf("%s: Got return code %s\n", __FUNCTION__, pc_status.ok() ? "OK" : "NOT-OK");

        AntarisReturnCode tmp_return;

        // Act upon its status.
        if (pc_status.ok()) {
             tmp_return = (AntarisReturnCode)(pc_response.return_code());
        } else {
            tmp_return = An_GENERIC_FAILURE;
        }

        return tmp_return;
  }

  AntarisReturnCode Invoke_PC_get_current_location(ReqGetCurrentLocationParams *req_params) {
        antaris_api_peer_to_peer::ReqGetCurrentLocationParams pc_req;
        antaris_api_peer_to_peer::AntarisReturnType pc_response;
        Status pc_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_ReqGetCurrentLocationParams(req_params, &pc_req);
        context.AddMetadata(COOKIE_STR, this->cookie_str);

        printf("%s: Invoking PC_get_current_location api towards PC\n", __FUNCTION__);
        pc_status = stub_->PC_get_current_location(&context, pc_req, &pc_response);

        AntarisReturnCode tmp_return;

        // Act upon its status.
        if (pc_status.ok()) {
             tmp_return = (AntarisReturnCode)(pc_response.return_code());
        } else {
            tmp_return = An_GENERIC_FAILURE;
        }

        return tmp_return;
  }

  AntarisReturnCode Invoke_PC_stage_file_download(ReqStageFileDownloadParams *req_params) {
        antaris_api_peer_to_peer::ReqStageFileDownloadParams pc_req;
        antaris_api_peer_to_peer::AntarisReturnType pc_response;
        Status pc_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_ReqStageFileDownloadParams(req_params, &pc_req);
        context.AddMetadata(COOKIE_STR, this->cookie_str);

        pc_status = stub_->PC_stage_file_download(&context, pc_req, &pc_response);

        AntarisReturnCode tmp_return;

        // Act upon its status.
        if (pc_status.ok()) {
             tmp_return = (AntarisReturnCode)(pc_response.return_code());
        } else {
            tmp_return = An_GENERIC_FAILURE;
        }

        return tmp_return;
  }

  AntarisReturnCode Invoke_PC_sequence_done(CmdSequenceDoneParams *req_params) {
        antaris_api_peer_to_peer::CmdSequenceDoneParams pc_req;
        antaris_api_peer_to_peer::AntarisReturnType pc_response;
        Status pc_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_CmdSequenceDoneParams(req_params, &pc_req);
        context.AddMetadata(COOKIE_STR, this->cookie_str);

        pc_status = stub_->PC_sequence_done(&context, pc_req, &pc_response);

        AntarisReturnCode tmp_return;

        // Act upon its status.
        if (pc_status.ok()) {
             tmp_return = (AntarisReturnCode)(pc_response.return_code());
        } else {
            tmp_return = An_GENERIC_FAILURE;
        }

        return tmp_return;
  }

  AntarisReturnCode Invoke_PC_payload_power_control(ReqPayloadPowerControlParams *req_params) {
        antaris_api_peer_to_peer::ReqPayloadPowerControlParams pc_req;
        antaris_api_peer_to_peer::AntarisReturnType pc_response;
        Status pc_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_ReqPayloadPowerControlParams(req_params, &pc_req);
        context.AddMetadata(COOKIE_STR, this->cookie_str);

        pc_status = stub_->PC_payload_power_control(&context, pc_req, &pc_response);

        AntarisReturnCode tmp_return;

        // Act upon its status.
        if (pc_status.ok()) {
             tmp_return = (AntarisReturnCode)(pc_response.return_code());
        } else {
            tmp_return = An_GENERIC_FAILURE;
        }

        return tmp_return;
  }

  AntarisReturnCode Invoke_PC_response_health_check(RespHealthCheckParams *req_params) {
        antaris_api_peer_to_peer::RespHealthCheckParams pc_req;
        antaris_api_peer_to_peer::AntarisReturnType pc_response;
        Status pc_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_RespHealthCheckParams(req_params, &pc_req);
        context.AddMetadata(COOKIE_STR, this->cookie_str);

        pc_status = stub_->PC_response_health_check(&context, pc_req, &pc_response);

        AntarisReturnCode tmp_return;

        // Act upon its status.
        if (pc_status.ok()) {
             tmp_return = (AntarisReturnCode)(pc_response.return_code());
        } else {
            tmp_return = An_GENERIC_FAILURE;
        }

        return tmp_return;
  }

  AntarisReturnCode Invoke_PC_response_shutdown(RespShutdownParams *req_params) {
        antaris_api_peer_to_peer::RespShutdownParams pc_req;
        antaris_api_peer_to_peer::AntarisReturnType pc_response;
        Status pc_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_RespShutdownParams(req_params, &pc_req);
        // Please note key needs to be lower case. This was not mentioned in the documentation, 
        // but there seems to be such constraint.
        context.AddMetadata(COOKIE_STR, this->cookie_str);

        pc_status = stub_->PC_response_shutdown(&context, pc_req, &pc_response);

        AntarisReturnCode tmp_return;

        // Act upon its status.
        if (pc_status.ok()) {
             tmp_return = (AntarisReturnCode)(pc_response.return_code());
        } else {
            tmp_return = An_GENERIC_FAILURE;
        }

        return tmp_return;
  }

 private:
  std::unique_ptr<antaris_api_peer_to_peer::AntarisapiPayloadController::Stub> stub_;
  char cookie_str[COOKIE_LEN+1];
};

typedef class AntarisInternalClientChannelContext_s AntarisInternalClientChannelContext_t;

// Logic and data behind the server's behavior.
class AppCallbackServiceImpl final : public antaris_api_peer_to_peer::AntarisapiApplicationCallback::Service {
public:
    Status PA_StartSequence(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::StartSequenceParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response);

    Status PA_ShutdownApp(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::ShutdownParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response);

    Status PA_ProcessResponseRegister(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::RespRegisterParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response);

    Status PA_ProcessResponseGetCurrentLocation(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::RespGetCurrentLocationParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response);

    Status PA_ProcessResponseStageFileDownload(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::RespStageFileDownloadParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response);

    Status PA_ProcessResponsePayloadPowerControl(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::RespPayloadPowerControlParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response);

    Status PA_ProcessHealthCheck(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::HealthCheckParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response);

public:

    void set_client_channel_ctx(AntarisInternalClientChannelContext_t *ctx) {
        client_channel_ctx_ = ctx;
    }

private:
    AntarisInternalClientChannelContext_t *client_channel_ctx_;
};

class AntarisInternalClientChannelContext_s {
public:
    PCServiceClient                 *pc_service_handle;
    AppCallbackServiceImpl          callback_service_handle;
    std::shared_ptr<Server>         server_ptr;
    int                             encryption; // 0 => no-encryption, 1 => encryption enabled
    AntarisApiCallbackFuncList      callbacks;
    unsigned short int              cb_correlation_id;
    pthread_t                       callback_thread_id;
    unsigned short int              callback_server_ready;
};

Status AppCallbackServiceImpl::PA_ProcessHealthCheck(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::HealthCheckParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
    HealthCheckParams app_request;
    AntarisReturnCode app_ret = An_NOT_IMPLEMENTED;
    
    if (client_channel_ctx_->callbacks.process_health_check) {
        peer_to_app_HealthCheckParams((void *)request, &app_request);
        app_ret = client_channel_ctx_->callbacks.process_health_check(&app_request);
    }

    response->set_return_code((::antaris_api_peer_to_peer::AntarisReturnCode)(app_ret));

    return Status::OK;
}

Status AppCallbackServiceImpl::PA_ProcessResponsePayloadPowerControl(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::RespPayloadPowerControlParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
    RespPayloadPowerControlParams app_request;
    AntarisReturnCode app_ret = An_NOT_IMPLEMENTED;
    
    if (client_channel_ctx_->callbacks.process_response_payload_power_control) {
        peer_to_app_RespPayloadPowerControlParams((void *)request, &app_request);
        app_ret = client_channel_ctx_->callbacks.process_response_payload_power_control(&app_request);
    }

    response->set_return_code((::antaris_api_peer_to_peer::AntarisReturnCode)(app_ret));

    return Status::OK;
}

Status AppCallbackServiceImpl::PA_StartSequence(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::StartSequenceParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
    StartSequenceParams app_request;
    AntarisReturnCode app_ret = An_NOT_IMPLEMENTED;
    
    printf("%s: Received Start-sequence from PC, invoking app callback fn %p\n", __FUNCTION__, client_channel_ctx_->callbacks.start_sequence);

    if (client_channel_ctx_->callbacks.start_sequence) {
        peer_to_app_StartSequenceParams((void *)request, &app_request);
        app_ret = client_channel_ctx_->callbacks.start_sequence(&app_request);
    }

    response->set_return_code((::antaris_api_peer_to_peer::AntarisReturnCode)(app_ret));

    return Status::OK;
}

Status AppCallbackServiceImpl::PA_ShutdownApp(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::ShutdownParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
    ShutdownParams app_request;
    AntarisReturnCode app_ret = An_NOT_IMPLEMENTED;
    
    if (client_channel_ctx_->callbacks.shutdown_app) {
        peer_to_app_ShutdownParams((void *)request, &app_request);
        app_ret = client_channel_ctx_->callbacks.shutdown_app(&app_request);
    }

    response->set_return_code((::antaris_api_peer_to_peer::AntarisReturnCode)(app_ret));

    return Status::OK;
}

Status AppCallbackServiceImpl::PA_ProcessResponseRegister(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::RespRegisterParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
    RespRegisterParams app_request;
    AntarisReturnCode app_ret = An_NOT_IMPLEMENTED;
    
    printf("%s: Received Response-Register from PC, invoking app callback fn %p\n", __FUNCTION__, client_channel_ctx_->callbacks.process_response_register);

    if (client_channel_ctx_->callbacks.process_response_register) {
        peer_to_app_RespRegisterParams((void *)request, &app_request);
        app_ret = client_channel_ctx_->callbacks.process_response_register(&app_request);
    }

    response->set_return_code((::antaris_api_peer_to_peer::AntarisReturnCode)(app_ret));

    return Status::OK;
}

Status AppCallbackServiceImpl::PA_ProcessResponseGetCurrentLocation(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::RespGetCurrentLocationParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
    RespGetCurrentLocationParams app_request;
    AntarisReturnCode app_ret = An_NOT_IMPLEMENTED;
    
    if (client_channel_ctx_->callbacks.process_response_get_current_location) {
        peer_to_app_RespGetCurrentLocationParams((void *)request, &app_request);
        app_ret = client_channel_ctx_->callbacks.process_response_get_current_location(&app_request);
    }

    response->set_return_code((::antaris_api_peer_to_peer::AntarisReturnCode)(app_ret));

    return Status::OK;
}

Status AppCallbackServiceImpl::PA_ProcessResponseStageFileDownload(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::RespStageFileDownloadParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
    RespStageFileDownloadParams app_request;
    AntarisReturnCode app_ret = An_NOT_IMPLEMENTED;
    
    if (client_channel_ctx_->callbacks.process_response_stage_file_download) {
        peer_to_app_RespStageFileDownloadParams((void *)request, &app_request);
        app_ret = client_channel_ctx_->callbacks.process_response_stage_file_download(&app_request);
    }

    response->set_return_code((::antaris_api_peer_to_peer::AntarisReturnCode)(app_ret));

    return Status::OK;
}

void *start_callback_server(void *thread_param)
{
    AntarisInternalClientChannelContext_t *ctx = (AntarisInternalClientChannelContext_t *)thread_param;
    std::string callback_address(g_APP_GRPC_LISTEN_ENDPOINT);

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;

    if (g_SSL_ENABLE == ENABLED) {
        printf("SSL is enabled \n");
        std::ifstream ssl_cert_file(CLIENT_SSL_CERTIFICATE_FILE);
        std::string cert;
        std::ifstream ssl_key_file(CLIENT_SSL_KEY_FILE);
        std::string key;

        if (ssl_cert_file.fail()) {
            printf("Error in opening %s file \n", CLIENT_SSL_CERTIFICATE_FILE);
            return NULL;
        }

        if (ssl_key_file.fail()) {
            printf("Error in opening %s file \n", CLIENT_SSL_KEY_FILE);
            return NULL;
        }

        ssl_cert_file.seekg(0, std::ios::end);   
        cert.reserve(ssl_cert_file.tellg());
        ssl_cert_file.seekg(0, std::ios::beg);

        cert.assign((std::istreambuf_iterator<char>(ssl_cert_file)),
        std::istreambuf_iterator<char>());

        ssl_key_file.seekg(0, std::ios::end);   
        key.reserve(ssl_key_file.tellg());
        ssl_key_file.seekg(0, std::ios::beg);

        key.assign((std::istreambuf_iterator<char>(ssl_key_file)),
        std::istreambuf_iterator<char>());

        grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp {key, cert};
 
        grpc::SslServerCredentialsOptions ssl_opts;
        ssl_opts.pem_root_certs="";
        ssl_opts.pem_key_cert_pairs.push_back(pkcp);

        builder.AddListeningPort(callback_address, grpc::SslServerCredentials(ssl_opts));
    } else {
        // Listen on the given address without any authentication mechanism.
        builder.AddListeningPort(callback_address, grpc::InsecureServerCredentials());
    }

    // Listen on the given address without any authentication mechanism.
    // builder.AddListeningPort(callback_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&ctx->callback_service_handle);

    ctx->callback_service_handle.set_client_channel_ctx(ctx);

    // Finally assemble the server.
    std::shared_ptr<Server> server(builder.BuildAndStart());
    ctx->server_ptr = server;

    std::cout << "PC -> App callback server listening on " << callback_address << std::endl;

    ctx->callback_server_ready = 1;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();

    std::cout << "PC -> App callback server exited ... ";

    return NULL;
}

extern "C" {

void displayAntarisChannel(AntarisChannel obj)
{
    AntarisInternalClientChannelContext_t *channel_ctx = (AntarisInternalClientChannelContext_t *)obj;
    printf("AntarisChannel Client Context %p\n", obj);
    printf("grpc_handle %p\n", channel_ctx->pc_service_handle);
    printf("encryption %u\n", channel_ctx->encryption);
}

AntarisChannel api_pa_pc_create_channel(AntarisApiCallbackFuncList *callback_func_list)
{
    sdk_environment_read_config();
    cJSON* p_cJson = NULL;
    char cookie[COOKIE_LEN + 1];
    read_config_json(&p_cJson);
    if (p_cJson == NULL){
        std::cout << "Failed to read the config.json\n" ;
    } else {
        cJSON *pJsonStr = cJSON_GetObjectItem(p_cJson , COOKIE_STR);
        if (cJSON_IsString(pJsonStr) ){
            std::cout << "JSON Cookie is string. \n";
            char* str = cJSON_GetStringValue(pJsonStr);
            if ( (str != NULL) && (strlen(str) <= COOKIE_LEN) ){
                memcpy(cookie, str , COOKIE_LEN);
                cookie[COOKIE_LEN] = '\0';
            } else {
                std::cout << "Failed to read cookie from the json";
            }
            std::cout << "JSON Cookie value : " << cookie << " \n";
        }
        cJSON_Delete(p_cJson);
    }
    if (!is_server_endpoint_available(&g_LISTEN_IP[0], g_PA_GRPC_SERVER_PORT)) {
        printf("App endpoint %s not available\n", g_APP_GRPC_LISTEN_ENDPOINT);
        return NULL;
    }

    AntarisInternalClientChannelContext_t *channel = new AntarisInternalClientChannelContext_t;
    std::string target_str(g_PC_GRPC_CONNECT_ENDPOINT);

    printf("api_pa_pc_create_channel\n");

    // TODO :
    // Create GRPC channel and connect with PC. Currently passing
    // grpc handle as None while creating AntarisChannel

    if (!channel) {
        fprintf(stderr, "%s: channel alloc fail\n", __FUNCTION__);
        goto done;
    }
    
    if (g_SSL_ENABLE == ENABLED) {
        printf("SSL is enabled \n");
        std::ifstream ssl_server_cert(SERVER_SSL_CERT_FILE);
        std::string cacert;
        grpc::SslCredentialsOptions ssl_opts;
  
        if (ssl_server_cert.fail()) {
            printf("Error in opening %s file \n", SERVER_SSL_CERT_FILE);
            return NULL;
        }

        ssl_server_cert.seekg(0, std::ios::end);   
        cacert.reserve(ssl_server_cert.tellg());
        ssl_server_cert.seekg(0, std::ios::beg);
        
        cacert.assign((std::istreambuf_iterator<char>(ssl_server_cert)),
                std::istreambuf_iterator<char>());

        ssl_opts.pem_root_certs=cacert;

        auto ssl_creds = grpc::SslCredentials(ssl_opts);
        channel->pc_service_handle  = new PCServiceClient(grpc::CreateChannel(target_str, ssl_creds));
    } else {
        channel->pc_service_handle  = new PCServiceClient(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    }
    channel->pc_service_handle->SetCookie(cookie);


    if (!channel->pc_service_handle) {
        fprintf(stderr, "%s: pc_service_handle alloc fail\n", __FUNCTION__);
        goto client_alloc_fail;
    }

    memcpy(&channel->callbacks, callback_func_list, sizeof(AntarisApiCallbackFuncList));

    // let the server start before we allow the client to start using the handle
    channel->server_ptr.reset();
    channel->callback_server_ready = 0;

    // start a callback service
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_create(&channel->callback_thread_id, &attr, start_callback_server, channel);

    do {
        sleep(ANTARIS_CALLBACK_GRACE_DELAY);
    } while (0 == channel->callback_server_ready);

    sleep(ANTARIS_CALLBACK_GRACE_DELAY);

done:
    return (AntarisChannel)channel;

client_alloc_fail:
    free(channel);
    channel = NULL;
    goto done;
}

AntarisReturnCode api_pa_pc_delete_channel(AntarisChannel channel)
{
    void *exit_status = NULL;
    AntarisInternalClientChannelContext_t *channel_ctx = (AntarisInternalClientChannelContext_t *)channel;
    AntarisReturnCode ret = An_SUCCESS;

    printf("api_pa_pc_delete_channel\n");

    if (!channel_ctx) {
        ret = An_GENERIC_FAILURE;
        return ret;
    }

    if (channel_ctx->server_ptr) {
        channel_ctx->server_ptr->Shutdown();
        pthread_join(channel_ctx->callback_thread_id, &exit_status);
        channel_ctx->server_ptr.reset(); // drop the only reference, this should free up the shared_ptr // TODO invesitgate if this is leaking memory
    }

    delete channel_ctx;

    return ret;
}

AntarisReturnCode api_pa_pc_register(AntarisChannel channel, ReqRegisterParams *register_params)
{
    AntarisInternalClientChannelContext_t *channel_ctx = (AntarisInternalClientChannelContext_t *)channel;
    AntarisReturnCode ret = An_SUCCESS;

    printf("api_pa_pc_register\n");

    if (!channel_ctx || !channel_ctx->pc_service_handle || !register_params) {
        ret = An_GENERIC_FAILURE;
        return ret;
    }

    if (api_debug) {
        displayReqRegisterParams(register_params);
    }

    return channel_ctx->pc_service_handle->Invoke_PC_register(register_params);
}

AntarisReturnCode api_pa_pc_get_current_location(AntarisChannel channel, ReqGetCurrentLocationParams *get_location_params)
{
    AntarisInternalClientChannelContext_t *channel_ctx = (AntarisInternalClientChannelContext_t *)channel;
    AntarisReturnCode ret = An_SUCCESS;

    printf("api_pa_pc_get_curent_location\n");

    if (!channel_ctx || !channel_ctx->pc_service_handle) {
        ret = An_GENERIC_FAILURE;
        return ret;
    }
    
    return channel_ctx->pc_service_handle->Invoke_PC_get_current_location(get_location_params);
}

AntarisReturnCode api_pa_pc_stage_file_download(AntarisChannel channel, ReqStageFileDownloadParams *download_file_params)
{
    AntarisInternalClientChannelContext_t *channel_ctx = (AntarisInternalClientChannelContext_t *)channel;
    AntarisReturnCode ret = An_SUCCESS;

    printf("api_pa_pc_stage_file_download\n");

    if (!channel_ctx || !channel_ctx->pc_service_handle || !download_file_params) {
        ret = An_GENERIC_FAILURE;
        return ret;
    }

    if (api_debug) {
        displayReqStageFileDownloadParams(download_file_params);
    }

    return channel_ctx->pc_service_handle->Invoke_PC_stage_file_download(download_file_params);
}

AntarisReturnCode api_pa_pc_sequence_done(AntarisChannel channel, CmdSequenceDoneParams *sequence_done_params)
{
    AntarisInternalClientChannelContext_t *channel_ctx = (AntarisInternalClientChannelContext_t *)channel;
    AntarisReturnCode ret = An_SUCCESS;

    printf("api_pa_pc_sequence_done\n");

    if (!channel_ctx || !channel_ctx->pc_service_handle || !sequence_done_params) {
        ret = An_GENERIC_FAILURE;
        return ret;
    }

    if (api_debug) {
        displayCmdSequenceDoneParams(sequence_done_params);
    }

    return channel_ctx->pc_service_handle->Invoke_PC_sequence_done(sequence_done_params);
}

AntarisReturnCode api_pa_pc_payload_power_control(AntarisChannel channel, ReqPayloadPowerControlParams *payload_power_control_params)
{
    AntarisInternalClientChannelContext_t *channel_ctx = (AntarisInternalClientChannelContext_t *)channel;
    AntarisReturnCode ret = An_SUCCESS;

    printf("api_pa_pc_payload_power_control\n");

    if (!channel_ctx || !channel_ctx->pc_service_handle || !payload_power_control_params) {
        ret = An_GENERIC_FAILURE;
        return ret;
    }

    if (api_debug) {
        displayReqPayloadPowerControlParams(payload_power_control_params);
    }

    return channel_ctx->pc_service_handle->Invoke_PC_payload_power_control(payload_power_control_params);
}

AntarisReturnCode api_pa_pc_response_health_check(AntarisChannel channel, RespHealthCheckParams *response_health_check_params)
{
    AntarisInternalClientChannelContext_t *channel_ctx = (AntarisInternalClientChannelContext_t *)channel;
    AntarisReturnCode ret = An_SUCCESS;

    printf("api_pa_pc_response_health_check\n");

    if (!channel_ctx || !channel_ctx->pc_service_handle || !response_health_check_params) {
        ret = An_GENERIC_FAILURE;
        return ret;
    }

    if (api_debug) {
        displayRespHealthCheckParams(response_health_check_params);
    }

    return channel_ctx->pc_service_handle->Invoke_PC_response_health_check(response_health_check_params);
}

AntarisReturnCode api_pa_pc_response_shutdown(AntarisChannel channel, RespShutdownParams *response_shutdown_params)
{
    AntarisInternalClientChannelContext_t *channel_ctx = (AntarisInternalClientChannelContext_t *)channel;
    AntarisReturnCode ret = An_SUCCESS;

    printf("api_pa_pc_response_shutdown\n");

    if (!channel_ctx || !channel_ctx->pc_service_handle || !response_shutdown_params) {
        ret = An_GENERIC_FAILURE;
        return ret;
    }

    if (api_debug) {
        displayRespShutdownParams(response_shutdown_params);
    }

    return channel_ctx->pc_service_handle->Invoke_PC_response_shutdown(response_shutdown_params);
}

} // extern C
