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

"""The Python implementation of a Test Payload App to test SDK APIs"""

import logging
import time
import threading
import pdb
import sys
import json
import os

import antaris_api_client as api_client
import antaris_api_types as api_types

debug = 1
channel = 0
g_condition = threading.Condition()

# Callback functions (PC => Application)
def start_sequence(start_seq_param):
    global g_sequence_id_to_idx_mapping
    global current_sequence_id

    print("start_sequence for sequence-id {}".format(start_seq_param.sequence_id))

    if (debug):
        start_seq_param.display()
    #<Payload Application Business Logic>

    return api_types.AntarisReturnCode.An_SUCCESS

def wakeup_app_to_send_next_api():
    g_condition.acquire()
    g_condition.notify()
    g_condition.release()

def shutdown_app(shutdown_param):
    global shutdown_requested

    if (debug):
        shutdown_param.display()

    print("shutdown_app : Got shutdown request from PC")

    #<Payload Application Business Logic>
    # 1. Shutdown Payload Hardware
    # 2. Do whatever is required to be done before shutting down the application

    shutdown_requested = 1

    # Respond back to Payload Controller about Shutdown completion
    print("Completed shutdown, responding back to PC")
    resp_shutdown_params = api_types.RespShutdownParams(shutdown_param.correlation_id, api_types.AntarisReturnCode.An_SUCCESS)
    api_client.api_pa_pc_response_shutdown(channel, resp_shutdown_params)

    wakeup_app_to_send_next_api()

    return api_types.AntarisReturnCode.An_SUCCESS

def process_response_register(resp_register_param):
    print("process_response_register")
    if (debug):
        resp_register_param.display()
    #<Payload Application Business Logic>
    wakeup_app_to_send_next_api()
    return api_types.AntarisReturnCode.An_SUCCESS

def process_response_get_current_location(resp_get_curr_location_param):
    print("process_response_get_current_location")
    if (debug):
        resp_get_curr_location_param.display()
    #<Payload Application Business Logic>
    wakeup_app_to_send_next_api()
    return api_types.AntarisReturnCode.An_SUCCESS

def process_response_stage_file_download(resp_stage_file_download):
    print("process_response_stage_file_download")
    if (debug):
        resp_stage_file_download.display()
    #<Payload Application Business Logic>
    wakeup_app_to_send_next_api()
    return api_types.AntarisReturnCode.An_SUCCESS

def process_health_check(health_check_param):
    print("process_health_check")
    if (debug):
        health_check_param.display()

    resp_health_check_params = api_types.RespHealthCheckParams(health_check_param.correlation_id, application_state , reqs_to_pc_in_err_cnt, resps_to_pc_in_err_cnt)
    api_client.api_pa_pc_response_health_check(channel, resp_health_check_params)
    return api_types.AntarisReturnCode.An_SUCCESS

def process_response_payload_power_control(resp_payload_power_control):
    print("process_response_payload_power_control")
    if (debug):
        resp_payload_power_control.display()
    #<Payload Application Business Logic>
    wakeup_app_to_send_next_api()
    return api_types.AntarisReturnCode.An_SUCCESS

# Functions (PA => PC)
func_list = {
        'Register': { 'param_types': api_types.ReqRegisterParams, 'func': api_client.api_pa_pc_register },
        'GetCurrentLocation': { 'param_types': api_types.ReqGetCurrentLocationParams, 'func': api_client.api_pa_pc_get_current_location },
        'StageFileDownload': { 'param_types': api_types.ReqStageFileDownloadParams, 'func': api_client.api_pa_pc_stage_file_download },
        'PayloadPowerControl': { 'param_types': api_types.ReqPayloadPowerControlParams, 'func': api_client.api_pa_pc_payload_power_control },
        'SequenceDone': { 'param_types': api_types.CmdSequenceDoneParams, 'func': api_client.api_pa_pc_sequence_done }
}

# Callback functions (PC => PA)
callback_func_list = {
        'StartSequence' : start_sequence,
        'Shutdown' : shutdown_app,
        'HealthCheck' : process_health_check,
        'RespRegister' : process_response_register,
        'RespGetCurrentLocation' : process_response_get_current_location,
        'RespStageFileDownload' : process_response_stage_file_download,
        'RespPayloadPowerControl' : process_response_payload_power_control,
}

# Main function
if __name__ == '__main__':

    logging.basicConfig()

    # Create Channel to talk to Payload Controller (PC)
    channel = api_client.api_pa_pc_create_channel(callback_func_list)

    if channel == None:
        print("Error : Create Channel failed")
        sys.exit(-1)

    f = open(os.path.join(sys.path[0], "test_app_input.json"))

    test_input = json.load(f)

    # After registration, simulated PC will ask application to start sequence Sequence_A

    for record in test_input['api_list']:
        api_name = record['api']
        # Find the function and initialize params for this api_name
        func = func_list[api_name]['func']
        param_type = func_list[api_name]['param_types']
        params = param_type(**record['args'])
        print("For input api : " + api_name + " found func : " + str(func) + " input param : " + str(params))
        ret = func(channel, params)
        if ret != api_types.AntarisReturnCode.An_SUCCESS:
           print("API " + api_name + "failed with error {} => {}".format(ret, api_types.AntarisReturnCode.reverse_dict[ret]))
           f.close()
           sys.ext(ret)

        g_condition.acquire()
        g_condition.wait()
        print("API " + api_name + "successful with response ")

    f.close()

    # Delete Channel
    api_client.api_pa_pc_delete_channel(channel)

    print("Exiting Main Thread")
