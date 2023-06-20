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

#include <cstdio>
#include <thread>

#include <fstream>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "antaris_api.h"
#include "antaris_api_internal.h"
#include "antaris_pc_to_app_api.h"
#include "antaris_sdk_environment.h"

#include "antaris_api.grpc.pb.h"
#include "antaris_api.pb.h"

#define MAX_IP_ADDR_STRLEN          24
#define MAX_GRPC_ENDPOINT_STRLEN    64
#define AUTH_KEY_START_OFFSET       (0)
#define AUTH_KEY_END_OFFSET         (AUTH_KEY_START_OFFSET + AUTH_KEY_LEN - 1 )        
#define APP_ID_OFFSET               (AUTH_KEY_END_OFFSET + 1)
#define APP_ID_LEN                  (3)

#define KEEPALIVE_TIME_MS                   20000
#define KEEPALIVE_TIMEOUT_MS                10000
#define KEEPALIVE_PERMIT_WITHOUT_CALLS      1

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::Status;

extern char g_TRUETWIN_ENABLE;

static AntarisReturnCode
prepare_endpoint_string(std::string &out_string, INT8 *peer_ip_str, UINT16 port);

class AppToPCClient {
 public:
  AppToPCClient(std::shared_ptr<Channel> channel)
      : app_grpc_handle_(antaris_api_peer_to_peer::AntarisapiApplicationCallback::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.

  AntarisReturnCode InvokeStartSequence(StartSequenceParams *req_params) {
        antaris_api_peer_to_peer::StartSequenceParams cb_req;
        antaris_api_peer_to_peer::AntarisReturnType cb_response;
        Status cb_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_StartSequenceParams(req_params, &cb_req);

        cb_status = app_grpc_handle_->PA_StartSequence(&context, cb_req, &cb_response);

        // Act upon its status.
        if (cb_status.ok()) {
            return (AntarisReturnCode)(cb_response.return_code());
        } else {
            return An_GENERIC_FAILURE;
        }
  }

  AntarisReturnCode InvokeShutdown(ShutdownParams *req_params) {
        antaris_api_peer_to_peer::ShutdownParams cb_req;
        antaris_api_peer_to_peer::AntarisReturnType cb_response;
        Status cb_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_ShutdownParams(req_params, &cb_req);

        cb_status = app_grpc_handle_->PA_ShutdownApp(&context, cb_req, &cb_response);

        // Act upon its status.
        if (cb_status.ok()) {
            return (AntarisReturnCode)(cb_response.return_code());
        } else {
            return An_GENERIC_FAILURE;
        }
  }

  AntarisReturnCode InvokeProcessResponseRegister(RespRegisterParams *req_params) {
        antaris_api_peer_to_peer::RespRegisterParams cb_req;
        antaris_api_peer_to_peer::AntarisReturnType cb_response;
        Status cb_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_RespRegisterParams(req_params, &cb_req);

        cb_status = app_grpc_handle_->PA_ProcessResponseRegister(&context, cb_req, &cb_response);

        // Act upon its status.
        if (cb_status.ok()) {
            return (AntarisReturnCode)(cb_response.return_code());
        } else {
            return An_GENERIC_FAILURE;
        }
  }

  AntarisReturnCode InvokeProcessResponseGetCurrentLocation(RespGetCurrentLocationParams *req_params) {
        antaris_api_peer_to_peer::RespGetCurrentLocationParams cb_req;
        antaris_api_peer_to_peer::AntarisReturnType cb_response;
        Status cb_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_RespGetCurrentLocationParams(req_params, &cb_req);

        cb_status = app_grpc_handle_->PA_ProcessResponseGetCurrentLocation(&context, cb_req, &cb_response);

        // Act upon its status.
        if (cb_status.ok()) {
            return (AntarisReturnCode)(cb_response.return_code());
        } else {
            return An_GENERIC_FAILURE;
        }
  }

  AntarisReturnCode InvokeProcessResponseStageFileDownload(RespStageFileDownloadParams *req_params) {
        antaris_api_peer_to_peer::RespStageFileDownloadParams cb_req;
        antaris_api_peer_to_peer::AntarisReturnType cb_response;
        Status cb_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_RespStageFileDownloadParams(req_params, &cb_req);

        cb_status = app_grpc_handle_->PA_ProcessResponseStageFileDownload(&context, cb_req, &cb_response);

        // Act upon its status.
        if (cb_status.ok()) {
            return (AntarisReturnCode)(cb_response.return_code());
        } else {
            return An_GENERIC_FAILURE;
        }
  }

  AntarisReturnCode InvokeProcessResponsePayloadPowerControl(RespPayloadPowerControlParams *req_params) {
        antaris_api_peer_to_peer::RespPayloadPowerControlParams cb_req;
        antaris_api_peer_to_peer::AntarisReturnType cb_response;
        Status cb_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_RespPayloadPowerControlParams(req_params, &cb_req);

        cb_status = app_grpc_handle_->PA_ProcessResponsePayloadPowerControl(&context, cb_req, &cb_response);

        // Act upon its status.
        if (cb_status.ok()) {
            return (AntarisReturnCode)(cb_response.return_code());
        } else {
            return An_GENERIC_FAILURE;
        }
  }

  AntarisReturnCode InvokeProcessProcessHealthCheck(HealthCheckParams *req_params) {
        antaris_api_peer_to_peer::HealthCheckParams cb_req;
        antaris_api_peer_to_peer::AntarisReturnType cb_response;
        Status cb_status;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        app_to_peer_HealthCheckParams(req_params, &cb_req);

        cb_status = app_grpc_handle_->PA_ProcessHealthCheck(&context, cb_req, &cb_response);

        // Act upon its status.
        if (cb_status.ok()) {
            return (AntarisReturnCode)(cb_response.return_code());
        } else {
            return An_GENERIC_FAILURE;
        }
  }

 private:
  std::unique_ptr<antaris_api_peer_to_peer::AntarisapiApplicationCallback::Stub> app_grpc_handle_;
  std::uint32_t appId;
};

class PC2AppInternalContext {
public:
    AppToPCClient       *client_api_handle;
    std::string         client_cb_endpoint;
};

class AppToPCApiService final : public antaris_api_peer_to_peer::AntarisapiPayloadController::Service {
public:
    AppToPCApiService(std::string ep, PCAppCallbackFn_t cb) {
        server_endpoint_ = ep;
        user_callbacks_ = cb;
        user_cb_ctx_ = (PCApiServerContext)this;
        server_thread_ = NULL;
        b_server_thread_started_ = false;
    }

    ~AppToPCApiService(void) {
        if (server_thread_) {
            delete server_thread_;
        }
    }

    void startServer(UINT32 ssl_flag) {
        server_thread_ = new std::thread(LaunchGrpcServer, this, ssl_flag);

        while (!b_server_thread_started_) {
            sleep(1);
        }

        return;
    }

    void stopServer(void) {
        if (!b_server_thread_started_) {
            return;
        }

        assert(server_thread_ != NULL);

        grpc_server_ptr_->Shutdown();
        server_thread_->join();
        assert(b_server_thread_started_ == false);

        delete server_thread_;
        server_thread_ = NULL;

        grpc_server_ptr_.reset(); // drop the only reference, this should free up the shared_ptr // TODO invesitgate if this is leaking memory
    }

private:
    static void getPeerEndpoint(::grpc::ServerContext* context, char *tgt_string) {
        std::string peer_string = context->peer();
        size_t first_colon_at = peer_string.find_first_of(':');
        const INT8 *full_string = peer_string.c_str();

        if (first_colon_at >= 0 && first_colon_at < peer_string.length()) {
            strcpy(tgt_string, &full_string[first_colon_at + 1]);
        } else {
            strcpy(tgt_string, full_string);
        }

        return;
    }

    static cookie_t decodeCookie(::grpc::ServerContext* context){
        cookie_t cookie; 
        std::memset(&cookie , 0 , sizeof(cookie) );
        try {
            cookie.appId = -1;
            std::multimap< grpc::string_ref, grpc::string_ref > md = context->client_metadata();

            // Please note key needs to be lower case. 
            std::string s((md.find(COOKIE_STR)->second).data(),  (md.find(COOKIE_STR)->second).length());
            std::string sub_s = s.substr(APP_ID_OFFSET , APP_ID_LEN);
            cookie.appId = std::stoi( sub_s ) ;
            std::size_t len = s.substr(AUTH_KEY_START_OFFSET , AUTH_KEY_LEN).copy(cookie.auth_key , (AUTH_KEY_LEN) , 0);
            if (len != AUTH_KEY_LEN)
            {
                std::cout<< "Incorrect length of Authentication key. : " << len << std::endl;
            }
            cookie.auth_key[AUTH_KEY_LEN] = '\0';
        } catch(...) {
            std::cout << "Exception decoding cokie. \n";
        }
        return cookie;
    }

    Status PC_register(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::ReqRegisterParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
        AppToPCCallbackParams_t api_request_register = {0};
        AppToPCCallbackParams_t api_request_sdk_version = {0};
        AntarisReturnType api_response = {return_code: An_SUCCESS};
        cookie_t cookie;
        cookie = decodeCookie(context);

        api_request_sdk_version.sdk_version.major = request->sdk_version().major();
        api_request_sdk_version.sdk_version.minor = request->sdk_version().minor();
        api_request_sdk_version.sdk_version.patch = request->sdk_version().patch();

        user_callbacks_(user_cb_ctx_, cookie , e_app2PC_sdkVersionInfo, &api_request_sdk_version, &api_response.return_code);

        if (An_SUCCESS != api_response.return_code) {
            goto done; /* user has indicated version incompatibility */
        }

        peer_to_app_ReqRegisterParams(request, &api_request_register);

        user_callbacks_(user_cb_ctx_, cookie , e_app2PC_register, &api_request_register, &api_response.return_code);

done:
        app_to_peer_AntarisReturnType(&api_response, response);

        return Status::OK;
    }

    Status PC_get_current_location(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::ReqGetCurrentLocationParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
        AppToPCCallbackParams_t api_request = {0};
        AntarisReturnType api_response = {return_code: An_SUCCESS};
        cookie_t cookie;
        cookie = decodeCookie(context);
        peer_to_app_ReqGetCurrentLocationParams(request, &api_request);

        user_callbacks_(user_cb_ctx_ , cookie ,  e_app2PC_getCurrentLocation, &api_request, &api_response.return_code);

        app_to_peer_AntarisReturnType(&api_response, response);

        return Status::OK;
    }

    Status PC_stage_file_download(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::ReqStageFileDownloadParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
        AppToPCCallbackParams_t api_request = {0};
        AntarisReturnType api_response = {return_code: An_SUCCESS};
        cookie_t cookie;
        cookie = decodeCookie(context);

        peer_to_app_ReqStageFileDownloadParams(request, &api_request);

        user_callbacks_(user_cb_ctx_ , cookie , e_app2PC_stageFileDownload, &api_request, &api_response.return_code);

        app_to_peer_AntarisReturnType(&api_response, response);

        return Status::OK;
    }

    Status PC_sequence_done(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::CmdSequenceDoneParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
        AppToPCCallbackParams_t api_request = {0};
        AntarisReturnType api_response = {return_code: An_SUCCESS};
        cookie_t cookie;
        cookie = decodeCookie(context);

        peer_to_app_CmdSequenceDoneParams(request, &api_request);

        user_callbacks_(user_cb_ctx_ , cookie , e_app2PC_sequenceDone, &api_request, &api_response.return_code);

        app_to_peer_AntarisReturnType(&api_response, response);

        return Status::OK;
    }

    Status PC_payload_power_control(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::ReqPayloadPowerControlParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
        AppToPCCallbackParams_t api_request = {0};
        AntarisReturnType api_response = {return_code: An_SUCCESS};
        cookie_t cookie;
        cookie = decodeCookie(context);

        peer_to_app_ReqPayloadPowerControlParams(request, &api_request);

        user_callbacks_(user_cb_ctx_ , cookie , e_app2PC_payloadPowerControl, &api_request, &api_response.return_code);

        app_to_peer_AntarisReturnType(&api_response, response);

        return Status::OK;
    }

    Status PC_response_health_check(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::RespHealthCheckParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
        AppToPCCallbackParams_t api_request = {0};
        AntarisReturnType api_response = {return_code: An_SUCCESS};
        cookie_t cookie;
        cookie = decodeCookie(context);

        peer_to_app_RespHealthCheckParams(request, &api_request);

        user_callbacks_(user_cb_ctx_ , cookie , e_app2PC_healthCheckResponse, &api_request, &api_response.return_code);

        app_to_peer_AntarisReturnType(&api_response, response);

        return Status::OK;
    }

    Status PC_response_shutdown(::grpc::ServerContext* context, const ::antaris_api_peer_to_peer::RespShutdownParams* request, ::antaris_api_peer_to_peer::AntarisReturnType* response) {
        AppToPCCallbackParams_t api_request = {0};
        AntarisReturnType api_response = {return_code: An_SUCCESS};
        cookie_t cookie;
        cookie = decodeCookie(context);

        peer_to_app_RespShutdownParams(request, &api_request);

        user_callbacks_(user_cb_ctx_ , cookie , e_app2PC_shutdownResponse, &api_request, &api_response.return_code);

        app_to_peer_AntarisReturnType(&api_response, response);

        return Status::OK;
    }

private:
    static void LaunchGrpcServer(AppToPCApiService *ctx, UINT32 ssl_flag) {
        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
        ServerBuilder builder;
        
        if(ssl_flag)  {
            std::ifstream ssl_cert_file(PC_SSL_CERTFICATE_FILE);
            std::string str;
            std::ifstream ssl_key(PC_SSL_KEY_FILE);
            std::string key;

            if ( ssl_cert_file.fail() ) {
                printf("Server certificate file not found \n");
                return ;
            }

            if ( ssl_key.fail() ) {
                printf("Error in ssl key file \n");
                return;
            }

            ssl_cert_file.seekg(0, std::ios::end);   
            str.reserve(ssl_cert_file.tellg());
            ssl_cert_file.seekg(0, std::ios::beg);

            str.assign((std::istreambuf_iterator<char>(ssl_cert_file)),
            std::istreambuf_iterator<char>());

            ssl_key.seekg(0, std::ios::end);   
            key.reserve(ssl_key.tellg());
            ssl_key.seekg(0, std::ios::beg);

            key.assign((std::istreambuf_iterator<char>(ssl_key)),
            std::istreambuf_iterator<char>());

            grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp {key, str};
 
            grpc::SslServerCredentialsOptions ssl_opts;
            ssl_opts.pem_root_certs="";
            ssl_opts.pem_key_cert_pairs.push_back(pkcp);

            builder.AddListeningPort(ctx->server_endpoint_, grpc::SslServerCredentials(ssl_opts));
        } else {
            // Listen on the given address without any authentication mechanism.
            builder.AddListeningPort(ctx->server_endpoint_, grpc::InsecureServerCredentials());
        }
        // Register "service" as the instance through which we'll communicate with
        // clients. In this case it corresponds to an *synchronous* service.
        builder.RegisterService(ctx);
  
        // Finally assemble the server.
        std::shared_ptr<Server> server(builder.BuildAndStart());
        ctx->grpc_server_ptr_ = server;

        // this is not thread-safe, but we are very close to having a running server, so we can
        // let it be known that the server is ready.
        ctx->b_server_thread_started_ = true;

        // Wait for the server to shutdown. Note that some other thread must be
        // responsible for shutting down the server for this call to ever return.
        server->Wait();

        ctx->b_server_thread_started_ = false;
    }

    std::string                 server_endpoint_;
    PCAppCallbackFn_t           user_callbacks_;
    PCApiServerContext          user_cb_ctx_;
    std::thread                 *server_thread_;
    bool                        b_server_thread_started_;
    std::shared_ptr<Server>     grpc_server_ptr_;
};

PCApiServerContext an_pc_pa_create_server(UINT16 port, PCAppCallbackFn_t callback_fn, UINT32 ssl_flag)
{
    std::string server_listen_endpoint;

    if (!callback_fn) {
        return NULL;
    }

    if (An_SUCCESS != prepare_endpoint_string(server_listen_endpoint, (INT8 *)g_LISTEN_IP, port)) {
        return NULL;
    }

    printf("%s: Creating server with ip %s, port %u\n", __FUNCTION__, (INT8 *)g_LISTEN_IP, port);

    AppToPCApiService *ctx = new AppToPCApiService(server_listen_endpoint, callback_fn);

    if (!ctx) {
        return NULL;
    }

    // start the server before letting this routine return
    ctx->startServer(ssl_flag);

    return (PCApiServerContext)ctx;
}

void an_pc_pa_delete_server(PCApiServerContext ctx) {
    AppToPCApiService *server_ctx = (AppToPCApiService *)ctx;

    if (!server_ctx) {
        return;
    }

    server_ctx->stopServer();

    delete server_ctx;

    return;
}

static AntarisReturnCode
prepare_endpoint_string(std::string &out_string, INT8 *peer_ip_str, UINT16 port)
{
    char tmp_buf[MAX_GRPC_ENDPOINT_STRLEN] = {0};

    if (!peer_ip_str || std::strlen(peer_ip_str) > MAX_IP_ADDR_STRLEN) {
        return An_INVALID_PARAMS;
    }

    std::sprintf(tmp_buf, "%s:%u", peer_ip_str, port);

    out_string = tmp_buf;

    return An_SUCCESS;
}

//////////////////////////////////////// API implementation //////////////////////

extern "C" {
PCToAppClientContext an_pc_pa_create_client(INT8 *peer_ip_str, UINT16 port, INT8 *client_ssl_addr, UINT32 ssl_flag)
{
    PC2AppInternalContext   *internal_ctx = new PC2AppInternalContext();
    AntarisReturnCode       tmp_ret;

    grpc::ChannelArguments args;

    if (!internal_ctx) {
        goto done;
    }

    printf("%s: creating client-channel with ip %s, port %u\n", __FUNCTION__, peer_ip_str, port);

    if (An_SUCCESS != prepare_endpoint_string(internal_ctx->client_cb_endpoint, peer_ip_str, port)) {
        goto fail_cleanup_ctx;
    }
    if (g_TRUETWIN_ENABLE == ENABLED) {
        // Sample way of setting keepalive arguments on the client channel. Here we
        // are configuring a keepalive time period of 20 seconds, with a timeout of 10
        // seconds. Additionally, pings will be sent even if there are no calls in
        // flight on an active connection.
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, KEEPALIVE_TIME_MS);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, KEEPALIVE_TIMEOUT_MS);
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, KEEPALIVE_PERMIT_WITHOUT_CALLS);
    }   
    if (ssl_flag) {
        std::ifstream t(client_ssl_addr);
        std::string cacert;
        grpc::SslCredentialsOptions ssl_opts;

        t.seekg(0, std::ios::end);   
        cacert.reserve(t.tellg());
        t.seekg(0, std::ios::beg);

        cacert.assign((std::istreambuf_iterator<char>(t)),
                std::istreambuf_iterator<char>());

        ssl_opts.pem_root_certs=cacert;

        auto ssl_creds = grpc::SslCredentials(ssl_opts);
        if (g_TRUETWIN_ENABLE == ENABLED) { 
            internal_ctx->client_api_handle = new AppToPCClient(grpc::CreateCustomChannel(internal_ctx->client_cb_endpoint, ssl_creds, args));
        } else {
            internal_ctx->client_api_handle = new AppToPCClient(grpc::CreateChannel(internal_ctx->client_cb_endpoint, ssl_creds));
        }
    } else {
        if (g_TRUETWIN_ENABLE == ENABLED) {
            internal_ctx->client_api_handle = new AppToPCClient(grpc::CreateCustomChannel(internal_ctx->client_cb_endpoint, grpc::InsecureChannelCredentials(), args));
        } else {
            internal_ctx->client_api_handle = new AppToPCClient(grpc::CreateChannel(internal_ctx->client_cb_endpoint, grpc::InsecureChannelCredentials()));
        }
    }
    if (!internal_ctx->client_api_handle) {
        goto fail_cleanup_ctx;
    }

done:
    return (PCToAppClientContext)internal_ctx;

fail_cleanup_ctx:
    delete internal_ctx;
    internal_ctx = NULL;
    goto done;

fail_cleanup_api_handle:
    delete internal_ctx->client_api_handle;
    goto fail_cleanup_ctx;
}

void an_pc_pa_delete_client(PCToAppClientContext ctx)
{
    PC2AppInternalContext   *internal_ctx = (PC2AppInternalContext *)ctx;

    if (!internal_ctx) {
        return;
    }

    if (internal_ctx->client_api_handle) {
        delete internal_ctx->client_api_handle;
    }

    delete internal_ctx;

    return;
}

AntarisReturnCode an_pc_pa_invoke_api(PCToAppClientContext ctx, PCToAppApiId_e api_id, PCToAppApiParams_t *api_params)
{
    AntarisReturnCode           ret = An_INVALID_PARAMS;
    PC2AppInternalContext       *internal_ctx = (PC2AppInternalContext *)ctx;

    if (!internal_ctx || !api_params || !internal_ctx->client_api_handle) {
        return ret;
    }

    switch (api_id) {
    case e_PC2App_startSequence:
        ret = internal_ctx->client_api_handle->InvokeStartSequence(&api_params->start_sequence);
    break;

    case e_PC2App_shutdownApp:
        ret = internal_ctx->client_api_handle->InvokeShutdown(&api_params->shutdown);
    break;

    case e_PC2App_responseRegister:
        ret = internal_ctx->client_api_handle->InvokeProcessResponseRegister(&api_params->resp_register);
    break;

    case e_PC2App_responseGetCurrentLocation:
        ret = internal_ctx->client_api_handle->InvokeProcessResponseGetCurrentLocation(&api_params->resp_location);
    break;

    case e_PC2App_responseStageFileDownload:
        ret = internal_ctx->client_api_handle->InvokeProcessResponseStageFileDownload(&api_params->resp_stage_file_download);
    break;

    case e_PC2App_responsePayloadPowerControl:
        ret = internal_ctx->client_api_handle->InvokeProcessResponsePayloadPowerControl(&api_params->resp_payload_power_ctrl);
    break;

    case e_PC2App_processHealthCheck:
        ret = internal_ctx->client_api_handle->InvokeProcessProcessHealthCheck(&api_params->health_check);
    break;

    } // switch api_id

    return ret;
}

} // extern "C"
