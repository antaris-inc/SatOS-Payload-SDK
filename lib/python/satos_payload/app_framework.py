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

import os
import threading
import time

import satos_payload.antaris_api_client as api_client
import satos_payload.gen.antaris_api_types as api_types


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
    def params(self):
        return self._handler._seq_params

    @property
    def client(self):
        return self._handler._channel_client

    def deadline_reached(self):
        return self._handler.deadline_reached()

    def is_stopping(self):
        return self._handler.is_stopping()


class SequenceHandler(Stoppable, threading.Thread):

    def __init__(self, seq_id, seq_params, seq_deadline_ms, channel_client, handler_func, callback, logger):
        super().__init__()

        self._seq_id = seq_id
        self._seq_params = seq_params
        self._seq_deadline_ms = seq_deadline_ms

        self._channel_client = channel_client
        self._handler_func = handler_func
        self._callback = callback

        self.logger = logger

    def run(self):
        self.logger.info("sequence execution started: id=%s" % self._seq_id)

        self._handler_func(SequenceContext(self))
        self.logger.info("sequence execution completed: id=%s" % self._seq_id)

        self._callback()
        self.stopped()

    def deadline_reached(self):
        now_ms = time.time() * 1000
        return now_ms >= self._seq_deadline_ms


class ChannelClient:
    def __init__(self, channel):
        self.channel = channel

        self._cond = threading.Condition()
        self._next_cid = 0
        self._responses = {}

    def get_current_location(self):
        #NOTE(bcwaldon): pc-sim currently not able to respond, so we provide a dummy response
        return api_types.RespGetCurrentLocationParams(
                latitude=1.0, longitude=2.0, altitude=3.0,
                correlation_id=0, req_status=0, determined_at=None)

        with self._cond:
            self._next_cid += 1
            params = api_types.ReqGetCurrentLocationParams(self._next_cid)
            resp = api_client.api_pa_pc_get_current_location(self.channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                self.logger.error("get_current_location request failed")
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


    def stage_file_download(self, loc):
        with self._cond:
            self._next_cid += 1
            params = api_types.ReqStageFileDownloadParams(self._next_cid, loc)
            resp = api_client.api_pa_pc_stage_file_download(self.channel, params)
            if resp != api_types.AntarisReturnCode.An_SUCCESS:
                self.logger.error("stage_file_download request failed")
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

    def payload_power_control(self):
        raise NotImplementedError()

    def _handle_resp(self, params):
        with self._cond:
            if not params.correlation_id in self._responses:
                return

            resp_cond = self._responses[params.correlation_id][0]
            with resp_cond:
                self._responses[params.correlation_id][1] = params
                resp_cond.notify_all()


class PayloadApplication(Stoppable):

    def __init__(self, logger):
        super().__init__()

        self.logger = logger

        # used to coordinate local state modifications
        self.lock = threading.Lock()

        # holds active sequence handler
        self.seq_handler = None

        # index of registered sequence handler funcs
        self.sequence_handler_func_idx = dict()

        # abstracts access to channel APIs for sequences
        self.channel_client = ChannelClient(channel=None)

        # used to facilitate communications with payload interface
        self.pi_chan = None
        self.pi_cid = 0
        self.pi_callback_map = {
            'StartSequence': self.handle_start_sequence,
            'Shutdown': self.handle_shutdown,
            'HealthCheck': lambda x: api_types.AntarisReturnCode.An_SUCCESS,
            'RespRegister': lambda x: api_types.AntarisReturnCode.An_SUCCESS,

            'RespGetCurrentLocation': self.channel_client._handle_resp,
            'RespStageFileDownload': self.channel_client._handle_resp,
            'RespPayloadPowerControl': self.channel_client._handle_resp,
        }

        # used to help coordinate shutdown
        self.shutdown_correlation_id = None

    def mount(self, sequence_id, sequence_handler_func):
        self.sequence_handler_func_idx[sequence_id] = sequence_handler_func

    def run(self):
        self.logger.info("payload app starting")

        self.pi_chan = api_client.api_pa_pc_create_channel(self.pi_callback_map)
        if self.pi_chan == None:
            raise Exception("failed establishing payload interface channel")

        self.channel_client.channel = self.pi_chan

        params = api_types.ReqRegisterParams(self.pi_cid, DO_NOTHING_ON_HEALTH_CHECK_FAILURE)
        resp = api_client.api_pa_pc_register(self.pi_chan, params)
        if resp != api_types.AntarisReturnCode.An_SUCCESS:
            raise Exception("failed registering payload app with controller: code=%d" % resp)

        self.logger.info("payload app running")

        self.wait_until_stop_requested()

        self.logger.info("payload app shutdown triggered")

        cid = None
        with self.lock:
            cid = self.shutdown_correlation_id

            if self.seq_handler:
                self.logger.info("stopping active sequence")
                self.seq_handler.request_stop()

        # might hit a race conditions here on shutdown without
        # relying on a lock, so we simply log and move on
        try:
            if self.seq_handler:
                self.seq_handler.wait_until_stopped()
        except Exception as exc:
            self.logger.exception("failed waiting for seq_handler to stop")


        # inform the controller the shutdown has completed, then tear down the channel
        params = api_types.RespShutdownParams(cid, api_types.AntarisReturnCode.An_SUCCESS)
        api_client.api_pa_pc_response_shutdown(self.pi_chan, params)
        api_client.api_pa_pc_delete_channel(self.pi_chan)

        self.wait_until_stopped()

        self.logger.info("payload app shutdown complete")
        return


    def handle_start_sequence(self, params):
        self.logger.info("handling start_sequence request")

        try:
            handler_func = self.sequence_handler_func_idx[params.sequence_id]
        except KeyError:
            self.logger.info("sequence_id not recognized")
            return api_types.AntarisReturnCode.An_GENERIC_FAILURE

        with self.lock:
            if self.seq_handler:
                self.logger.error("sequence already active, unable to start another")
                return api_types.AntarisReturnCode.An_GENERIC_FAILURE

            # spawn a thread to handle the sequence, provding a callback that coordinates
            # shutdown with the payload app
            def callback():
                with self.lock:
                    self.seq_handler = None
                    self._sequence_done(params.sequence_id)

            self.seq_handler = SequenceHandler(params.sequence_id, params.sequence_params, params.scheduled_deadline, self.channel_client, handler_func, callback, self.logger)
            self.seq_handler.start()

            return api_types.AntarisReturnCode.An_SUCCESS

    # communicate to the payload channel a sequence has completed
    def _sequence_done(self, sequence_id):
        params = api_types.CmdSequenceDoneParams(sequence_id)
        resp = api_client.api_pa_pc_sequence_done(self.pi_chan, params)
        if resp != api_types.AntarisReturnCode.An_SUCCESS:
            self.logger.error("sequence_done request failed: resp=%d" % resp)

    def handle_shutdown(self, params):
        self.logger.info("handling shutdown request")

        with self.lock:
            self._shutdown_correlation_id = params.correlation_id

        # non-blocking, as we just want to accept the request and proceed
        # in the background with full shutdown
        self.request_stop()

        return api_types.AntarisReturnCode.An_SUCCESS

