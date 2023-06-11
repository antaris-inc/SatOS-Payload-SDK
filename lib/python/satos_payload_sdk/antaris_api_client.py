#
#   Copyright 2022 Antaris, Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from concurrent import futures
import logging
import time
import pdb
import json

import grpc

from satos_payload_sdk import antaris_api_common as api_common
from satos_payload_sdk.gen import antaris_api_pb2
from satos_payload_sdk.gen import antaris_api_pb2_grpc
from satos_payload_sdk.gen import antaris_api_types as api_types
from satos_payload_sdk.gen import antaris_sdk_version as sdk_version
from satos_payload_sdk import antaris_file_download as file_download

api_debug = 0
g_shutdown_grace_seconds=5
g_shutdown_grace_for_grace=2
g_ANTARIS_CALLBACK_GRACE_DELAY=10

g_SERVER_CERT_FILE="/opt/antaris/sdk/server.crt"
g_CLIENT_CERT_FILE="/opt/antaris/app/client.crt"
g_CLIENT_KEY_FILE="/opt/antaris/app/client.key"
g_CONFIG_JSON_FILE="/opt/antaris/app/config.json"
g_COOKIE_STR="cookie"

g_KEEPALIVE_TIME_MS = 10000
g_KEEPALIVE_TIMEOUT_MS = 5000
g_KEEPALIVE_PERMIT_WITHOUT_CALLS = True
g_MAX_PING_STRIKES = 0

class AntarisChannel:
    def __init__(self, grpc_client_handle, grpc_server_handle, pc_to_app_server, is_secure, callback_func_list):
        self.grpc_client_handle = grpc_client_handle
        self.grpc_server_handle = grpc_server_handle
        self.pc_to_app_server = pc_to_app_server
        self.is_secure = is_secure
        self.start_sequence = callback_func_list['StartSequence']
        self.shutdown_app = callback_func_list['Shutdown']
        self.process_health_check = callback_func_list['HealthCheck']
        self.process_resp_register = callback_func_list['RespRegister']
        self.process_resp_get_curr_location = callback_func_list['RespGetCurrentLocation']
        self.process_resp_stage_file_download = callback_func_list['RespStageFileDownload']
        self.process_resp_payload_power_control = callback_func_list['RespPayloadPowerControl']
        try :
            # Read config info
            jsonfile = open(g_CONFIG_JSON_FILE, 'r')
            self.jsfile_data = json.load(jsonfile)
        except Exception as e :
            print("Got exception while reading json. Exception : {}".format(e) )

class PCToAppService(antaris_api_pb2_grpc.AntarisapiApplicationCallbackServicer):
    def set_channel(self, channel):
        self.channel = channel

    def PA_StartSequence(self, request, context):
        if self.channel.start_sequence:
            app_request = api_types.peer_to_app_StartSequenceParams(request)
            app_ret = self.channel.start_sequence(app_request)
            return antaris_api_pb2.AntarisReturnType(return_code = app_ret)
        else:
            return antaris_api_pb2.AntarisReturnType(return_code = api_types.AntarisReturnCode.An_NOT_IMPLEMENTED)

    def PA_ShutdownApp(self, request, context):
        if self.channel.shutdown_app:
            app_request = api_types.peer_to_app_ShutdownParams(request)
            app_ret = self.channel.shutdown_app(app_request)
            return antaris_api_pb2.AntarisReturnType(return_code = app_ret)
        else:
            return antaris_api_pb2.AntarisReturnType(return_code = api_types.AntarisReturnCode.An_NOT_IMPLEMENTED)

    def PA_ProcessHealthCheck(self, request, context):
        if self.channel.process_health_check:
            app_request = api_types.peer_to_app_HealthCheckParams(request)
            app_ret = self.channel.process_health_check(app_request)
            return antaris_api_pb2.AntarisReturnType(return_code = app_ret)
        else:
            return antaris_api_pb2.AntarisReturnType(return_code = api_types.AntarisReturnCode.An_NOT_IMPLEMENTED)

    def PA_ProcessResponseRegister(self, request, context):
        if self.channel.process_resp_register:
            app_request = api_types.peer_to_app_RespRegisterParams(request)
            app_ret = self.channel.process_resp_register(app_request)
            return antaris_api_pb2.AntarisReturnType(return_code = app_ret)
        else:
            return antaris_api_pb2.AntarisReturnType(return_code = api_types.AntarisReturnCode.An_NOT_IMPLEMENTED)

    def PA_ProcessResponseGetCurrentLocation(self, request, context):
        if self.channel.process_resp_get_curr_location:
            app_request = api_types.peer_to_app_RespGetCurrentLocationParams(request)
            app_ret =  self.channel.process_resp_get_curr_location(app_request)
            return antaris_api_pb2.AntarisReturnType(return_code = app_ret)
        else:
            return antaris_api_pb2.AntarisReturnType(return_code = api_types.AntarisReturnCode.An_NOT_IMPLEMENTED)

    def PA_ProcessResponseStageFileDownload(self, request, context):
        if self.channel.process_resp_stage_file_download:
            app_request = api_types.peer_to_app_RespStageFileDownloadParams(request)
            app_ret = self.channel.process_resp_stage_file_download(app_request)
            return antaris_api_pb2.AntarisReturnType(return_code = app_ret)
        else:
            return antaris_api_pb2.AntarisReturnType(return_code = api_types.AntarisReturnCode.An_NOT_IMPLEMENTED)

    def PA_ProcessResponsePayloadPowerControl(self, request, context):
        if self.channel.process_resp_payload_power_control:
            app_request = api_types.peer_to_app_RespPayloadPowerControlParams(request)
            app_ret = self.channel.process_resp_payload_power_control(app_request)
            return antaris_api_pb2.AntarisReturnType(return_code = app_ret)
        else:
            return antaris_api_pb2.AntarisReturnType(return_code = api_types.AntarisReturnCode.An_NOT_IMPLEMENTED)

def api_pa_pc_create_channel_common(secure, callback_func_list):
    global g_SERVER_CERT_FILE
    global g_CLIENT_CERT_FILE
    global g_CLIENT_KEY_FILE
    global g_ANTARIS_CALLBACK_GRACE_DELAY

    pc_endpoint = "{}:{}".format(api_common.g_PAYLOAD_CONTROLLER_IP, api_common.g_PC_GRPC_SERVER_PORT)
    app_endpoint = "{}:{}".format(api_common.g_LISTEN_IP, api_common.g_PA_GRPC_SERVER_PORT)

    print("api_pa_pc_create_channel_common")

    is_endpoint_free = api_common.is_server_endpoint_available(api_common.g_LISTEN_IP, int(api_common.g_PA_GRPC_SERVER_PORT))

    if not is_endpoint_free:
        print("Callback endpoint {} is not free".format(app_endpoint))
        return None

    print("Starting server")
    
    server_options = []
    if api_common.g_TRUETWIN_ENABLE == '1':
        """
        grpc.keepalive_time_ms: The period (in milliseconds) after which a keepalive ping is
            sent on the transport.
        grpc.keepalive_timeout_ms: The amount of time (in milliseconds) the sender of the keepalive
            ping waits for an acknowledgement. If it does not receive an acknowledgment within
            this time, it will close the connection.
        grpc.http2.min_ping_interval_without_data_ms: Minimum allowed time (in milliseconds)
            between a server receiving successive ping frames without sending any data/header frame.
        grpc.max_connection_idle_ms: Maximum time (in milliseconds) that a channel may have no
            outstanding rpcs, after which the server will close the connection.
        grpc.max_connection_age_ms: Maximum time (in milliseconds) that a channel may exist.
        grpc.max_connection_age_grace_ms: Grace period (in milliseconds) after the channel
            reaches its max age.
        grpc.http2.max_pings_without_data: How many pings can the client send before needing to
            send a data/header frame.
        grpc.keepalive_permit_without_calls: If set to 1 (0 : false; 1 : true), allows keepalive
            pings to be sent even if there are no calls in flight.
        For more details, check: https://github.com/grpc/grpc/blob/master/doc/keepalive.md
        """
        server_options.extend ([("grpc.keepalive_time_ms", g_KEEPALIVE_TIME_MS), 
                                 ("grpc.keepalive_timeout_ms", g_KEEPALIVE_TIMEOUT_MS), 
                                 ("grpc.keepalive_permit_without_calls", g_KEEPALIVE_PERMIT_WITHOUT_CALLS),
                                 ("grpc.http2.max_ping_strikes", g_MAX_PING_STRIKES)])

    if api_common.g_SSL_ENABLE == '0':
        print("Creating insecure channel")
        client_handle = antaris_api_pb2_grpc.AntarisapiPayloadControllerStub(grpc.insecure_channel(pc_endpoint))
        if api_common.g_TRUETWIN_ENABLE == '0':
            server_handle =  grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        else:
            server_handle =  grpc.server(futures.ThreadPoolExecutor(max_workers=10), options=server_options)

        server_handle.add_insecure_port(app_endpoint)
    else:
        print("SSL is enabled in sdk_env.conf file, creating secure channel")
        try :
            root_certs = open(g_SERVER_CERT_FILE, 'rb').read()
        except Exception as e :
            print("Can not read file :", g_SERVER_CERT_FILE)
            quit()

        credentials = grpc.ssl_channel_credentials(root_certs)
 
        channel = grpc.secure_channel( pc_endpoint , credentials)
        
        client_handle = antaris_api_pb2_grpc.AntarisapiPayloadControllerStub(channel)

        if api_common.g_TRUETWIN_ENABLE == '0':
            server_handle =  grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        else:
            server_handle =  grpc.server(futures.ThreadPoolExecutor(max_workers=10), options=server_options)

        print("Creating secure channel")
        try :
            private_key = open( g_CLIENT_KEY_FILE, 'rb').read()
        except Exception as e :
            print("Can not open file :", g_CLIENT_KEY_FILE)
            quit()

        try :
            certificate_chain = open( g_CLIENT_CERT_FILE, 'rb').read()
        except Exception as e :
            print("Can not open file :", g_CLIENT_CERT_FILE)
            quit()

        credentials = grpc.ssl_server_credentials(
            [(private_key, certificate_chain)]
        )
        server_handle.add_secure_port(app_endpoint, credentials)

    pc_to_app_server = PCToAppService()
    antaris_api_pb2_grpc.add_AntarisapiApplicationCallbackServicer_to_server(pc_to_app_server, server_handle)
    channel = AntarisChannel(client_handle, server_handle, pc_to_app_server, secure, callback_func_list)
    pc_to_app_server.set_channel(channel)
    server_handle.start()
    time.sleep(g_ANTARIS_CALLBACK_GRACE_DELAY)

    print("started callback server and created channel")

    return channel

# API functions called by Payload Application and served by Payload Controller
def api_pa_pc_create_channel(callback_func_list):
   channel = api_pa_pc_create_channel_common(True, callback_func_list)
   return channel

def api_pa_pc_create_insecure_channel(callback_func_list):
   channel = api_pa_pc_create_channel_common(False, callback_func_list)
   return channel

def api_pa_pc_delete_channel(channel):
    global g_shutdown_grace_seconds
    global g_shutdown_grace_for_grace
    quiesce_time = g_shutdown_grace_seconds + g_shutdown_grace_for_grace

    print("api_pa_pc_delete_channel, stopping callback server")
    channel.grpc_server_handle.stop(g_shutdown_grace_seconds).wait()
    print("callback server successfully stopped, sleeping for quiesce time (s) = {}".format(quiesce_time))
    time.sleep(quiesce_time)
    return 0

def api_pa_pc_register(channel, register_params):
    print("api_pa_pc_registering with SDK version {}.{}.{}".
                format(
                    sdk_version.ANTARIS_PA_PC_SDK_MAJOR_VERSION,
                    sdk_version.ANTARIS_PA_PC_SDK_MINOR_VERSION,
                    sdk_version.ANTARIS_PA_PC_SDK_PATCH_VERSION))

    if (api_debug):
        register_params.display()
    peer_params = api_types.app_to_peer_ReqRegisterParams(register_params)

    peer_params.sdk_version.major = sdk_version.ANTARIS_PA_PC_SDK_MAJOR_VERSION
    peer_params.sdk_version.minor = sdk_version.ANTARIS_PA_PC_SDK_MINOR_VERSION
    peer_params.sdk_version.patch = sdk_version.ANTARIS_PA_PC_SDK_PATCH_VERSION
    metadata = ((g_COOKIE_STR , "{}".format(channel.jsfile_data[g_COOKIE_STR]) ) , )
    peer_ret = channel.grpc_client_handle.PC_register(peer_params , metadata=metadata)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_get_current_location(channel, get_location_params):
    print("api_pa_pc_get_curent_location")
    if (api_debug):
        get_location_params.display()

    peer_params = api_types.app_to_peer_ReqGetCurrentLocationParams(get_location_params)
    metadata = ( (g_COOKIE_STR , "{}".format(channel.jsfile_data[g_COOKIE_STR]) ) , )
    peer_ret = channel.grpc_client_handle.PC_get_current_location(peer_params , metadata=metadata)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_stage_file_download(channel, download_file_params):
    print("api_pa_pc_stage_file_download")
    if (api_debug):
        download_file_params.display()
    file_stage = file_download.File_Stage(download_file_params, channel.jsfile_data)
    File_download_status=file_stage.file_download()
    peer_params = api_types.app_to_peer_ReqStageFileDownloadParams(download_file_params)
    metadata = ( (g_COOKIE_STR , "{}".format(channel.jsfile_data[g_COOKIE_STR]) ) , )
    peer_ret = channel.grpc_client_handle.PC_stage_file_download(peer_params , metadata = metadata)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_sequence_done(channel, sequence_done_params):
    print("api_pa_pc_sequence_done")
    if (api_debug):
        sequence_done_params.display()
    peer_params = api_types.app_to_peer_CmdSequenceDoneParams(sequence_done_params)
    metadata = ( (g_COOKIE_STR , "{}".format(channel.jsfile_data[g_COOKIE_STR]) ) , )
    peer_ret = channel.grpc_client_handle.PC_sequence_done(peer_params , metadata=metadata)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_payload_power_control(channel, payload_power_control_params):
    print("api_pa_pc_payload_power_control")
    if (api_debug):
        payload_power_control_params.display()
    peer_params = api_types.app_to_peer_ReqPayloadPowerControlParams(payload_power_control_params)
    metadata = ( (g_COOKIE_STR , "{}".format(channel.jsfile_data[g_COOKIE_STR]) ) , )
    peer_ret = channel.grpc_client_handle.PC_payload_power_control(peer_params , metadata=metadata)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_response_health_check(channel, response_health_check_params):
    print("api_pa_pc_response_health_check")
    if (api_debug):
        response_health_check_params.display()
    peer_params = api_types.app_to_peer_RespHealthCheckParams(response_health_check_params)
    metadata = ( (g_COOKIE_STR , "{}".format(channel.jsfile_data[g_COOKIE_STR]) ) , )
    peer_ret = channel.grpc_client_handle.PC_response_health_check(peer_params , metadata=metadata)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_response_shutdown(channel, response_shutdown_params):
    print("api_pa_pc_response_shutdown")
    if (api_debug):
        response_shutdown_params.display()
    peer_params = api_types.app_to_peer_RespShutdownParams(response_shutdown_params)
    metadata = ( (g_COOKIE_STR , "{}".format(channel.jsfile_data[g_COOKIE_STR]) ) , )
    peer_ret = channel.grpc_client_handle.PC_response_shutdown(peer_params , metadata=metadata)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code
