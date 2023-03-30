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

import grpc

import satos_payload.antaris_api_common as api_common
import satos_payload.gen.antaris_api_types as api_types
from  satos_payload.gen import antaris_api_pb2, antaris_api_pb2_grpc
import satos_payload.gen.antaris_sdk_version as sdk_version

api_debug = 0
g_shutdown_grace_seconds=5
g_shutdown_grace_for_grace=2
g_ANTARIS_CALLBACK_GRACE_DELAY=10

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
    global g_ANTARIS_CALLBACK_GRACE_DELAY
    pc_endpoint = "{}:{}".format(api_common.g_PAYLOAD_CONTROLLER_IP, api_common.g_PC_GRPC_SERVER_PORT)
    app_endpoint = "{}:{}".format(api_common.g_LISTEN_IP, api_common.g_PA_GRPC_SERVER_PORT)

    print("api_pa_pc_create_channel_common")

    is_endpoint_free = api_common.is_server_endpoint_available(api_common.g_LISTEN_IP, int(api_common.g_PA_GRPC_SERVER_PORT))

    if not is_endpoint_free:
        print("Callback endpoint {} is not free".format(app_endpoint))
        return None

    client_handle = antaris_api_pb2_grpc.AntarisapiPayloadControllerStub(grpc.insecure_channel(pc_endpoint))

    server_handle =  grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    server_handle.add_insecure_port(app_endpoint)
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

    peer_ret = channel.grpc_client_handle.PC_register(peer_params)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_get_current_location(channel, get_location_params):
    print("api_pa_pc_get_curent_location")
    if (api_debug):
        get_location_params.display()

    peer_params = api_types.app_to_peer_ReqGetCurrentLocationParams(get_location_params)

    peer_ret = channel.grpc_client_handle.PC_get_current_location(peer_params)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_stage_file_download(channel, download_file_params):
    print("api_pa_pc_stage_file_download")
    if (api_debug):
        download_file_params.display()
    peer_params = api_types.app_to_peer_ReqStageFileDownloadParams(download_file_params)

    peer_ret = channel.grpc_client_handle.PC_stage_file_download(peer_params)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_sequence_done(channel, sequence_done_params):
    print("api_pa_pc_sequence_done")
    if (api_debug):
        sequence_done_params.display()
    peer_params = api_types.app_to_peer_CmdSequenceDoneParams(sequence_done_params)

    peer_ret = channel.grpc_client_handle.PC_sequence_done(peer_params)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_payload_power_control(channel, payload_power_control_params):
    print("api_pa_pc_payload_power_control")
    if (api_debug):
        payload_power_control_params.display()
    peer_params = api_types.app_to_peer_ReqPayloadPowerControlParams(payload_power_control_params)

    peer_ret = channel.grpc_client_handle.PC_payload_power_control(peer_params)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_response_health_check(channel, response_health_check_params):
    print("api_pa_pc_response_health_check")
    if (api_debug):
        response_health_check_params.display()
    peer_params = api_types.app_to_peer_RespHealthCheckParams(response_health_check_params)

    peer_ret = channel.grpc_client_handle.PC_response_health_check(peer_params)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code

def api_pa_pc_response_shutdown(channel, response_shutdown_params):
    print("api_pa_pc_response_shutdown")
    if (api_debug):
        response_shutdown_params.display()
    peer_params = api_types.app_to_peer_RespShutdownParams(response_shutdown_params)

    peer_ret = channel.grpc_client_handle.PC_response_shutdown(peer_params)

    if (api_debug):
        print("Got return code {} => {}".format(peer_ret.return_code, api_types.AntarisReturnCode.reverse_dict[peer_ret.return_code]))

    return peer_ret.return_code
