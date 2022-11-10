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

"""The Python implementation of the Antaris Sample Payload Application """

import logging
import time
import threading
import pdb
import sys

import antaris_api_client as api_client
import antaris_api_types as api_types

debug = 0
channel = 0
shutdown_requested = 0

#
# Following counters should be incremented whenever
# reqeust/response (to PC) API hits error
reqs_to_pc_in_err_cnt = 0
resps_to_pc_in_err_cnt = 0

# Global variable to record application state
application_state = 0

# This sequence should complete within its scheduled_deadline
def sequence_a_fsm(mythread):

    print("sequence_a_fsm")

    print("sequence_a_fsm : state : " + mythread.state + " scheduled_deadline : ", mythread.scheduled_deadline)

    mythread.correlation_id = 0

    # Request PC to get current location
    get_location_params = api_types.ReqGetCurrentLocationParams(mythread.correlation_id)
    ret = api_client.api_pa_pc_get_current_location(channel, get_location_params)

    print("Invoked api_pa_pc_get_current_location, got return-code {} => {}".format(ret, api_types.AntarisReturnCode.reverse_dict[ret]))

    if ret != api_types.AntarisReturnCode.An_SUCCESS:
        print("Error : Get-current-location call failed")
        mythread.state = "FSM_ERROR"
        print("sequence_a_fsm : state : ", mythread.state)
        return

    mythread.correlation_id += 1
    mythread.state = "WAITING_FOR_GET_LOCATION"
    print("sequence_a_fsm : state : ", mythread.state)
    mythread.condition.acquire()
    mythread.condition.wait()
    if shutdown_requested:
        return

    # Request PC to stage file download
    download_file_params = api_types.ReqStageFileDownloadParams(mythread.correlation_id, "teledata/payload_teledata.txt")
    ret = api_client.api_pa_pc_stage_file_download(channel, download_file_params)

    print("Invoked api_pa_pc_stage_file_download, got return-code {} => {}".format(ret, api_types.AntarisReturnCode.reverse_dict[ret]))

    if ret != api_types.AntarisReturnCode.An_SUCCESS:
        print("Error : Download-file call failed")
        mythread.state = "FSM_ERROR"
        print("sequence_a_fsm : state : ", mythread.state)
        return

    mythread.correlation_id += 1
    mythread.state = "WAITING_FOR_STAGE_FILE_DOWNLOAD_COMPLETION"
    print("sequence_a_fsm : state : ", mythread.state)
    mythread.condition.acquire()
    mythread.condition.wait()
    if shutdown_requested:
        return

    # Here Application may want to gracefully shutdown its
    # payload device before asking PC to power it off.
 
    # Request PC to power-off my payload hardware
    payload_power_control_params = api_types.ReqPayloadPowerControlParams(mythread.correlation_id, 0)
    ret = api_client.api_pa_pc_payload_power_control(channel, payload_power_control_params)

    print("Invoked api_pa_pc_payload_power_control, got return-code {} => {}".format(ret, api_types.AntarisReturnCode.reverse_dict[ret]))

    if ret != api_types.AntarisReturnCode.An_SUCCESS:
        print("Error : Payload Power Control call failed")
        mythread.state = "FSM_ERROR"
        print("sequence_a_fsm : state : ", mythread.state)
        return

    mythread.correlation_id += 1
    mythread.state = "WAITING_FOR_PAYLOAD_POWER_OFF"
    print("sequence_a_fsm : state : ", mythread.state)
    mythread.condition.acquire()
    mythread.condition.wait()
    if shutdown_requested:
        return

    # Tell PC that current sequence is done
    sequence_done_params = api_types.CmdSequenceDoneParams("Sequence_A")
    ret = api_client.api_pa_pc_sequence_done(channel, sequence_done_params)

    print("Invoked api_pa_pc_sequence_done, got return-code {} => {}".format(ret, api_types.AntarisReturnCode.reverse_dict[ret]))

    if ret != api_types.AntarisReturnCode.An_SUCCESS:
        print("Error : Sequence_done call failed")
        mythread.state = "FSM_ERROR"
        print("sequence_a_fsm : state : ", mythread.state)
        return

    current_time_in_ms = time.time()*1000
    if (mythread.scheduled_deadline > current_time_in_ms) :
        print("CONGRATULATIONS! :  Sequence A completed at ", current_time_in_ms, "earlier than the deadline", mythread.scheduled_deadline)
    else :
        print("WARNING : Sequence A completed at ", current_time_in_ms, " after the deadline", mythread.scheduled_deadline)

    mythread.state = "EXITING"
    print("sequence_a_fsm : state : ", mythread.state)
    mythread.condition.release()
    return

def sequence_b_fsm(mythread):
    print("sequence_b_fsm")

def sequence_c_fsm(mythread):
    print("sequence_c_fsm")

# Table of Sequence_id : FsmThread
payload_sequences_fsms = dict()

current_sequence_id = "Sequence_A"

g_sequence_id_to_idx_mapping = {"Sequence_A": 0, "Sequence_B": 1, "Sequence_C": 2}

class FsmThread(threading.Thread):
    def __init__(self, threadID, counter, seq_id, seq_params, scheduled_deadline, func):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.counter = counter
        self.func = func
        self.state = "NOT_STARTED"
        self.condition = threading.Condition()
        self.correlation_id = 0
        self.seq_id = seq_id
        self.seq_params = seq_params
        self.scheduled_deadline = scheduled_deadline
    def run(self):
        print("Starting {}".format(self.seq_id))
        self.func(self)
        print("Exiting {}".format(self.seq_id))

# Callback functions (PC => Application)
def start_sequence(start_seq_param):
    global g_sequence_id_to_idx_mapping
    global current_sequence_id

    print("start_sequence for sequence-id {}".format(start_seq_param.sequence_id))

    if (debug):
        start_seq_param.display()
    #<Payload Application Business Logic>
    #Start Sequence FSM Thread
    current_sequence_id = start_seq_param.sequence_id
    seq_params = start_seq_param.sequence_params
    scheduled_deadline = start_seq_param.scheduled_deadline

    # Create FSM threads (arg : channel, thread-id, count, seq_id, fsm-function)
    if start_seq_param.sequence_id == "Sequence_A":
        fsm_thread = payload_sequences_fsms['Sequence_A'] = FsmThread(1, 1, 'Sequence_A', seq_params, scheduled_deadline, sequence_a_fsm)
    elif start_seq_param.sequence_id == "Sequence_B":
        fsm_thread = payload_sequences_fsms['Sequence_B'] = FsmThread(1, 1, 'Sequence_B', seq_params, scheduled_deadline, sequence_a_fsm)
    elif start_seq_param.sequence_id == "Sequence_C":
        fsm_thread = payload_sequences_fsms['Sequence_C'] = FsmThread(1, 1, 'Sequence_C', seq_params, scheduled_deadline, sequence_a_fsm)
    else:
        print("ERROR: Sequence " + start_seq_param.sequence_id + " does not exist.")
        return api_types.AntarisReturnCode.An_ERROR

    fsm_thread.start()

    return api_types.AntarisReturnCode.An_SUCCESS

def wakeup_seq_fsm(seq_fsm):
    seq_fsm.condition.acquire()
    seq_fsm.condition.notify()
    seq_fsm.condition.release()

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

    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_id])

    return api_types.AntarisReturnCode.An_SUCCESS

def process_passthru_tele_cmd(passthru_cmd_param):
    if (debug):
        passthru_cmd_param.display()
    #<Payload Application Business Logic>
    return api_types.AntarisReturnCode.An_SUCCESS

def process_response_register(resp_register_param):
    print("process_response_register")
    if (debug):
        resp_register_param.display()
    #<Payload Application Business Logic>
    return api_types.AntarisReturnCode.An_SUCCESS

def process_response_get_current_location(resp_get_curr_location_param):
    print("process_response_get_current_location")
    if (debug):
        resp_get_curr_location_param.display()
    #<Payload Application Business Logic>
    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_id])
    return api_types.AntarisReturnCode.An_SUCCESS

def process_response_stage_file_download(resp_stage_file_download):
    print("process_response_stage_file_download")
    if (debug):
        resp_stage_file_download.display()
    #<Payload Application Business Logic>
    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_id])
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
    wakeup_seq_fsm(payload_sequences_fsms[current_sequence_id])
    return api_types.AntarisReturnCode.An_SUCCESS

# Main function
if __name__ == '__main__':
    correlation_id = 0

    logging.basicConfig()

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

    # Create Channel to talk to Payload Controller (PC)
    channel = api_client.api_pa_pc_create_channel(callback_func_list)

    if channel == None:
        print("Error : Create Channel failed")
        sys.exit(-1)

    # Register application with PC
    # 2nd parameter decides PC's action on PA's health check failure
    # 0 => No Action, 1 => Reboot PA
    register_params = api_types.ReqRegisterParams(correlation_id, 0)
    ret = api_client.api_pa_pc_register(channel, register_params)

    print("Invoked register, got return-code {} => {}".format(ret, api_types.AntarisReturnCode.reverse_dict[ret]))

    if ret != api_types.AntarisReturnCode.An_SUCCESS:
        print("ERROR: Register failed")
        sys.exit(-1)

    # After registration, simulated PC will ask application to start sequence Sequence_A

    # Wait until shutdown command comes
    while (1):
        time.sleep(5)
        if shutdown_requested:
            print("Main thread detected shutdown")
            break

    # Wait for all FSM threads to complete
    for fsm_thread in payload_sequences_fsms:
        payload_sequences_fsms[fsm_thread].join()

    # Delete Channel
    api_client.api_pa_pc_delete_channel(channel)

    print("Exiting Main Thread")
