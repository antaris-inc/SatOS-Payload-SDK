# Copyright 2023 Antaris, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import logging
import os
import threading
import time

import satos_payload_sdk.antaris_api_client as api_client
import satos_payload_sdk.gen.antaris_api_types as api_types
from satos_payload_sdk import antaris_api_common as api_common
from satos_payload_sdk import gen as api_gen

logger = logging.getLogger("satos_payload_sdk")


DO_NOTHING_ON_HEALTH_CHECK_FAILURE = 0
REBOOT_ON_HEALTH_CHECK_FAILURE = 1

class Stoppable:
    def __init__(self):
        super().__init__()

        self._stop_requested_cond = threading.Condition()
        self._stop_requested = False
        self._stopped_cond = threading.Condition()
        self._stopped = False

    # Request that the Stoppable begins stopping.
    #TODO(bcwaldon): pass through a timeout
    def request_stop(self):
        with self._stop_requested_cond:
            self._stop_requested = True
            self._stop_requested_cond.notify_all()

    # Returns true if request_stop() has been called.
    def stop_requested(self):
        with self._stop_requested_cond:
            return self._stop_requested

    # Blocks until request_stop() has been called.
    # If a timeout is provided, it must be a number of seconds (partial
    # values OK) to wait for the condition before returning False.
    def wait_until_stop_requested(self, timeout=None):
        with self._stop_requested_cond:
            if self._stop_requested:
                return True
            return self._stop_requested_cond.wait(timeout=timeout)

    # Indicate that the Stoppable stopping process has completed.
    # This is used by the class inheriting Stoppable itself
    # to coordinate a clean shutdown.
    # A caller waiting for stopped() to return will be unblocked
    # after this is called.
    def stopped(self):
        with self._stopped_cond:
            self._stopped = True
            self._stopped_cond.notify_all()

    # Blocks until stopped() has been called.
    # If a timeout is provided, it must be a number of seconds (partial
    # values OK) to wait for the condition before returning False.
    def wait_until_stopped(self, timeout=None):
        with self._stopped_cond:
            if self._stopped:
                return True
            return self._stopped_cond.wait(timeout=timeout)


class SequenceContext:
    def __init__(self, handler):
        self._handler = handler

    @property
    def id(self):
        return self._handler._seq_id

    @property
    def params(self):
        return self._handler._seq_params

    @property
    def deadline_reached(self):
        return self._handler.deadline_reached()

    @property
    def stop_requested(self):
        return self._handler.stop_requested()

    @property
    def client(self):
        return self._handler._channel_client


class SequenceHandler(Stoppable, threading.Thread):

    def __init__(self, seq_id, seq_params, seq_deadline, channel_client, handler_func, callback):
        super().__init__()

        self._seq_id = seq_id
        self._seq_params = seq_params
        self._seq_deadline = seq_deadline

        self._channel_client = channel_client
        self._handler_func = handler_func
        self._callback = callback

    def run(self):
        logger.info("sequence execution started: id=%s" % self._seq_id)

        try:
            self._handler_func(SequenceContext(self))
            logger.info("sequence execution completed: id=%s" % self._seq_id)
        except:
            logger.exception("sequence execution failed: id=%s" % self._seq_id)

        #TODO(bcwaldon): need a way to bubble up failures
        self._callback()

        self.stopped()

    def deadline_reached(self):
        return time.time() >= self._seq_deadline


class ChannelClient:
    def __init__(self, start_sequence_cb, health_check_cb, shutdown_cb, req_payload_metrics_cb, gnss_eph_data_cb, get_eps_voltage_cb, ses_thermal_ntf_cb):
        self._channel = None
        self._cond = threading.Condition()
        self._next_cid = 0
        self._responses = {}

        self._start_sequence_cb = start_sequence_cb
        self._req_payload_metrics_cb = req_payload_metrics_cb
        # used to facilitate communications with payload interface
        self._callback_map = {
            'StartSequence': self._handle_start_sequence,
            'Shutdown': shutdown_cb,
            'HealthCheck': health_check_cb,
            'RespRegister': lambda x: api_types.AntarisReturnCode.An_SUCCESS,
            'RespGetCurrentLocation': self._handle_response,
            'RespStageFileDownload': self._handle_response,
            'RespPayloadPowerControl': self._handle_response,
            'ReqPayloadMetrics':self._handle_payload_metrics,
            'RespGnssEphStopDataReq':self._handle_response,
            'RespGnssEphStartDataReq': self._handle_response,
            'GnssEphData': gnss_eph_data_cb,
            'RespGetEpsVoltageStopReq':self._handle_response,
            'RespGetEpsVoltageStartReq': self._handle_response,
            'GetEpsVoltage': get_eps_voltage_cb,
            'RespStartSesThermMgmntReq': self._handle_response,
            'RespStopSesThermMgmntReq': self._handle_response,
            'RespSesTempReq': self._handle_response,
            'SesThrmlStsNtf': ses_thermal_ntf_cb
        }

    def _get_next_cid(self):
        self._next_cid += 1
        return self._next_cid

    def _connect(self):
        with self._cond:
            self._channel = api_client.api_pa_pc_create_channel(self._callback_map)
            if self._channel == None:
                raise Exception("failed establishing payload interface channel")

            params = api_types.ReqRegisterParams(0, DO_NOTHING_ON_HEALTH_CHECK_FAILURE)
            resp = api_client.api_pa_pc_register(self._channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                raise Exception("failed registering payload app with controller: code=%d" % resp)

    def _disconnect(self, cid):
        with self._cond:
            # inform the controller the shutdown has completed, then tear down the channel
            params = api_types.RespShutdownParams(cid, api_types.AntarisReturnCode.An_SUCCESS)
            api_client.api_pa_pc_response_shutdown(self._channel, params)
            api_client.api_pa_pc_delete_channel(self._channel)

    def get_current_location(self):
        with self._cond:
            params = api_types.ReqGetCurrentLocationParams(self._get_next_cid())
            resp = api_client.api_pa_pc_get_current_location(self._channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                logger.error("get_current_location request failed")
                return None

            resp_cond = threading.Condition()
            resp_cond.acquire()

            self._responses[params.correlation_id] = [resp_cond, None]

        # wait for response trigger
        resp_cond.wait()

        with self._cond:
            resp = self._responses[params.correlation_id][1]
            del self._responses[params.correlation_id]

        return resp

    def gnss_eph_stop_data_req(self):
        with self._cond:
            params = api_types.ReqGnssEphStopDataReq(self._get_next_cid())
            resp = api_client.api_pa_pc_gnss_eph_stop_req(self._channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                logger.error("gnss_eph_stop_data_req request failed")
                return None

            resp_cond = threading.Condition()
            resp_cond.acquire()

            self._responses[params.correlation_id] = [resp_cond, None]

        # wait for response trigger
        resp_cond.wait()

        with self._cond:
            resp = self._responses[params.correlation_id][1]
            del self._responses[params.correlation_id]

        return resp
    
    def gnss_eph_start_data_req(self, periodicity_in_ms, eph2_enable):
        with self._cond:
            params = api_types.ReqGnssEphStartDataReq(self._get_next_cid(), periodicity_in_ms, eph2_enable)
            resp = api_client.api_pa_pc_gnss_eph_start_req(self._channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                logger.error("gnss_eph_start_data_req request failed")
                return None

            resp_cond = threading.Condition()
            resp_cond.acquire()

            self._responses[params.correlation_id] = [resp_cond, None]

        # wait for response trigger
        resp_cond.wait()

        with self._cond:
            resp = self._responses[params.correlation_id][1]
            del self._responses[params.correlation_id]

        return resp

    def get_eps_voltage_stop_req(self):
        with self._cond:
            params = api_types.ReqGetEpsVoltageStopReq(self._get_next_cid())
            resp = api_client.api_pa_pc_get_eps_voltage_stop_req(self._channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                logger.error("api_pa_pc_get_eps_voltage_stop_req request failed")
                return None

            resp_cond = threading.Condition()
            resp_cond.acquire()

            self._responses[params.correlation_id] = [resp_cond, None]

        # wait for response trigger
        resp_cond.wait()

        with self._cond:
            resp = self._responses[params.correlation_id][1]
            del self._responses[params.correlation_id]

        return resp

    def get_eps_voltage_start_req(self, periodicity_in_ms):
        with self._cond:
            params = api_types.ReqGetEpsVoltageStartReq(self._get_next_cid(), periodicity_in_ms)
            resp = api_client.api_pa_pc_get_eps_voltage_start_req(self._channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                logger.error("api_pa_pc_get_eps_voltage_start_req request failed")
                return None

            resp_cond = threading.Condition()
            resp_cond.acquire()

            self._responses[params.correlation_id] = [resp_cond, None]

        # wait for response trigger
        resp_cond.wait()

        with self._cond:
            resp = self._responses[params.correlation_id][1]
            del self._responses[params.correlation_id]

        return resp

    def start_ses_therm_mgmnt_req(self, hardware_id, duration, lower_threshold, upper_threshold):
        with self._cond:
            params = api_types.StartSesThermMgmntReq(self._get_next_cid(), hardware_id, duration, lower_threshold, upper_threshold)
            resp = api_client.api_pa_pc_start_ses_therm_mgmnt_req(self._channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                logger.error("api_pa_pc_start_ses_therm_mgmnt_req request failed")
                return None

            resp_cond = threading.Condition()
            resp_cond.acquire()

            self._responses[params.correlation_id] = [resp_cond, None]

         # wait for response trigger
        resp_cond.wait()

        with self._cond:
            resp = self._responses[params.correlation_id][1]
            del self._responses[params.correlation_id]

        return resp
    
    def stop_ses_therm_mgmnt_req(self, hardware_id):
        with self._cond:
            params = api_types.StopSesThermMgmntReq(self._get_next_cid(), hardware_id)
            resp = api_client.api_pa_pc_stop_ses_therm_mgmnt_req(self._channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                logger.error("api_pa_pc_stop_ses_therm_mgmnt_req request failed")
                return None

            resp_cond = threading.Condition()
            resp_cond.acquire()

            self._responses[params.correlation_id] = [resp_cond, None]

        # wait for response trigger
        resp_cond.wait()

        with self._cond:
            resp = self._responses[params.correlation_id][1]
            del self._responses[params.correlation_id]

        return resp

    def ses_temp_req(self, hardware_id):
        with self._cond:
            params = api_types.SesTempReq(self._get_next_cid(), hardware_id)
            resp = api_client.api_pa_pc_ses_temp_req(self._channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                logger.error("api_pa_pc_ses_temp_req request failed")
                return None

            resp_cond = threading.Condition()
            resp_cond.acquire()

            self._responses[params.correlation_id] = [resp_cond, None]

        # wait for response trigger
        resp_cond.wait()

        with self._cond:
            resp = self._responses[params.correlation_id][1]
            del self._responses[params.correlation_id]

        return resp
    
    def ses_thermal_ntf_cb(self, hardware_id):
        with self._cond:
            params = api_types.SesTempReq(self._get_next_cid(), hardware_id)
            resp = api_client.api_pa_pc_ses_temp_req(self._channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                logger.error("api_pa_pc_ses_temp_req request failed")
                return None

            resp_cond = threading.Condition()
            resp_cond.acquire()

            self._responses[params.correlation_id] = [resp_cond, None]

        # wait for response trigger
        resp_cond.wait()

        with self._cond:
            resp = self._responses[params.correlation_id][1]
            del self._responses[params.correlation_id]

        return resp

    def stage_file_download(self, filename, file_priority):
        with self._cond:
            if (file_priority < api_types.FilePriorities.FILE_DL_PRIORITY_LOW) or (file_priority > api_types.FilePriorities.FILE_DL_PRIORITY_IMMEDIATE):
                return ValueError("Invalid file priority")

            params = api_types.ReqStageFileDownloadParams(self._get_next_cid(), filename, file_priority)
            resp = api_client.api_pa_pc_stage_file_download(self._channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                logger.error("stage_file_download request failed")
                return None

            resp_cond = threading.Condition()
            resp_cond.acquire()

            self._responses[params.correlation_id] = [resp_cond, None]

        # wait for response trigger
        resp_cond.wait()

        with self._cond:
            resp = self._responses[params.correlation_id][1]
            del self._responses[params.correlation_id]

        return resp

    def payload_power_control(self, power_state):
        # In case if True-twin mode, return success
        if api_common.g_KEEPALIVE_ENABLE == '1':
            return api_types.AntarisReturnCode.An_SUCCESS 
        else:
            payload_power_control_params = api_types.ReqPayloadPowerControlParams(self._get_next_cid(), int(power_state))

            resp = api_client.api_pa_pc_payload_power_control(self._channel, payload_power_control_params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                logger.error("sequence_done request failed: resp=%d" % resp)
            return resp
            
    def _sequence_done(self, sequence_id):
        params = api_types.CmdSequenceDoneParams(sequence_id)
        resp = api_client.api_pa_pc_sequence_done(self._channel, params)
        if resp != api_types.AntarisReturnCode.An_SUCCESS:
            logger.error("sequence_done request failed: resp=%d" % resp)

    def _handle_response(self, params):
        with self._cond:
            if not params.correlation_id in self._responses:
                return

            resp_cond = self._responses[params.correlation_id][0]
            with resp_cond:
                self._responses[params.correlation_id][1] = params
                resp_cond.notify_all()

    def _handle_start_sequence(self, params):
        return self._start_sequence_cb(params.sequence_id, params.sequence_params, params.scheduled_deadline)

    def _handle_payload_metrics(self, params):
        logger.info("Inside _handle_payload_metrics")
        return self._req_payload_metrics_cb(params)
    
class PayloadMetrics:
    def __init__(self):
        self.correlation_id = 0
        self.used_counter = 0
        self.timestamp = 0
        self.metrics = [api_gen.antaris_api_pb2.PayloadMetricsInfo() for _ in range(8)]        

    def define_counter(self, idx, name):
        if 0 <= idx < 16:
            self.metrics[idx].names = name
        else:
            raise ValueError("Index out of range. Index must be between 0 and 15.")

    def inc_counter(self, idx, val=1):
        if 0 <= idx < 16:
            self.metrics[idx].counter += val
        else:
            raise ValueError("Index out of range. Index must be between 0 and 15.")
        
class PayloadApplication(Stoppable):

    def __init__(self):
        super().__init__()

        # used to coordinate local state modifications
        self.lock = threading.Lock()

        # holds active sequence handler objects
        self.active_seq_handlers_map = {}

        # index of registered sequence handler funcs
        self.sequence_handler_func_idx = dict()

        # default health check; can be overridden
        self.health_check_handler_func = lambda: True
        
        # default gnss data handler; can be overridden
        self.gnss_eph_data_handler = lambda: True

        # default gnss data handler; can be overridden
        self.get_eps_voltage_handler = lambda: True
        # abstracts access to channel APIs for sequences
        self.channel_client = None

        # used to help coordinate shutdown
        self.shutdown_correlation_id = None

        # Additional payload attributes
        self.payload_metrics = PayloadMetrics()
        
    def mount_sequence(self, sequence_id, sequence_handler_func):
        self.sequence_handler_func_idx[sequence_id] = sequence_handler_func

    # Provided function should expect no arguments and return True or False
    # to represent health check success or failure, respectively
    def set_health_check(self, health_check_handler_func):
        self.health_check_handler_func = health_check_handler_func

    # Provided function should expect no arguments and return True or False
    # to represent gnss eph data handling, respectively
    def set_gnss_eph_data_cb(self, gnss_eph_data_handler):
        self.gnss_eph_data_handler = gnss_eph_data_handler
    # Provided function should expect no arguments and return True or False
    # to represent get eps voltage handling, respectively
    def set_get_eps_voltage_cb(self, get_eps_voltage_handler):
        self.get_eps_voltage_handler = get_eps_voltage_handler

    # Provided function should expect no arguments and return True or False
    # to represent SES thermal status notification, respectively
    def set_ses_thermal_status_ntf(self, ses_thermal_status_ntf):
        self.ses_thermal_status_ntf = ses_thermal_status_ntf
    #TODO(bcwaldon): actually do something with the provided params
    def _handle_health_check(self, params):
        try:
            hv = self.health_check_handler_func()
        except:
            logger.exception("health check failed")
            hv = False

        if hv:
            return api_types.AntarisReturnCode.An_SUCCESS
        else:
            return api_types.AntarisReturnCode.An_GENERIC_FAILURE

    def run(self):
        logger.info("payload app starting")

        if not self.channel_client:
            self.channel_client = ChannelClient(self.start_sequence, self._handle_health_check, self._handle_shutdown, self._req_payload_metrics, self._set_gnss_eph_data_cb, self._set_get_eps_voltage_cb, self._set_ses_thermal_status_ntf)
        
        self.channel_client._connect()

        logger.info("payload app running")

        self.wait_until_stop_requested()

        logger.info("payload app shutdown triggered")

        self._shutdown()

        logger.info("payload app shutdown complete")

    def _shutdown(self):
        cid = None
        with self.lock:
            cid = self.shutdown_correlation_id

            if self.active_seq_handlers_map:
                logger.info("stopping active sequences")
                for handler in self.active_seq_handlers_map.values():
                    handler.request_stop()

        # might hit a race conditions here on shutdown without
        # relying on a lock, so we simply log and move on
        # Wait for all active sequence handlers to stop
        try:
            for handler in self.active_seq_handlers_map.values():
                handler.wait_until_stopped()
        except Exception as exc:
            logger.exception("failed waiting for sequence handlers to stop")

        self.channel_client._disconnect(cid)

        self.stopped()

    def start_sequence(self, seq_id, seq_params, seq_deadline):
        try:
            handler_func = self.sequence_handler_func_idx[seq_id]
        except KeyError:
            logger.error(f"sequence_id not recognized: {seq_id}")
            return api_types.AntarisReturnCode.An_GENERIC_FAILURE

        # Keep track of the active sequences
        with self.lock:
            if seq_id in self.active_seq_handlers_map:
                logger.error("sequence already active, unable to start another")
                return api_types.AntarisReturnCode.An_GENERIC_FAILURE
            
                # spawn a thread to handle the sequence, provding a callback that coordinates
                # shutdown with the payload app
            def callback(sequence_handler):
                with self.lock:
                    sequence_handler.stopped()
                    self.channel_client._sequence_done(seq_id)
                    del self.active_seq_handlers_map[seq_id]

            sequence_handler = SequenceHandler(seq_id, seq_params, seq_deadline, self.channel_client, handler_func, lambda: callback(sequence_handler))

            # If sequence not active then append it to the list
            self.active_seq_handlers_map[seq_id] = sequence_handler
            logger.warning(f"Active Sequences: {self.active_seq_handlers_map}")
            sequence_handler.start()

            return api_types.AntarisReturnCode.An_SUCCESS

    def _handle_shutdown(self, params):
        logger.info("handling shutdown request")

        with self.lock:
            self.shutdown_correlation_id = params.correlation_id

        # non-blocking, as we just want to accept the request and proceed
        # in the background with full shutdown. Response will be sent later.
        self.request_stop()

        return api_types.AntarisReturnCode.An_SUCCESS

    def _req_payload_metrics(self, params):
        logger.info("Handling req_payload_metrics")

        payload_metrics = self.payload_metrics

        # Now, assign params.correlation_id to payload.correlation_id
        payload_metrics.correlation_id = params.correlation_id
    
        resp = api_client.api_pa_pc_response_payload_metrics(self.channel_client._channel, payload_metrics)
        if resp != api_types.AntarisReturnCode.An_SUCCESS:
            logger.error("sequence_done request failed: resp=%d" % resp)

        return api_types.AntarisReturnCode.An_SUCCESS

    def _set_gnss_eph_data_cb(self, params):
        logger.info("Handling GNSS EPH data")
        try:
            hv = self.gnss_eph_data_handler(params)
        except:
            logger.exception("Handling GNSS EPH data failed")
            hv = False

        if hv:
            return api_types.AntarisReturnCode.An_SUCCESS
        else:
            return api_types.AntarisReturnCode.An_GENERIC_FAILURE

    def _set_get_eps_voltage_cb(self, params):
        logger.info("Handling EPS voltage data")
        try:
            hv = self.get_eps_voltage_handler(params)
        except:
            logger.exception("Handling get EPS voltage failed")
            hv = False

        if hv:
            return api_types.AntarisReturnCode.An_SUCCESS
        else:
            return api_types.AntarisReturnCode.An_GENERIC_FAILURE

    def _set_ses_thermal_status_ntf(self, params):
        logger.info("Handling SES thermal notification")
        try:
            hv = self.ses_thermal_status_ntf(params)
        except:
            logger.exception("Handling get EPS voltage failed")
            hv = False

        if hv:
            return api_types.AntarisReturnCode.An_SUCCESS
        else:
            return api_types.AntarisReturnCode.An_GENERIC_FAILURE
