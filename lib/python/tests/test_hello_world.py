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

import datetime
import logging
import time
import threading
import unittest

from satos_payload.examples import hello_world
import satos_payload.gen.antaris_api_types as api_types



class LocalChannelClient:
    def __init__(self):
        self.lock = threading.Lock()
        self.staged_files = []
        self.sequences_done = []

    def _connect(self):
        pass

    def _disconnect(self, cid):
        pass

    def get_current_location(self):
        return api_types.RespGetCurrentLocationParams(
                latitude=1.0, longitude=2.0, altitude=3.0,
                correlation_id=0, req_status=0, determined_at=None)

    def stage_file_download(self, loc):
        with self.lock:
            self.staged_files.append(loc)

        return api_types.RespStageFileDownloadParams(
                correlation_id=0, req_status=0)

    def payload_power_control(self):
        raise NotImplementedError()

    def _sequence_done(self, sequence_id):
        with self.lock:
            self.sequences_done.append(sequence_id)


class TestHelloWorld(unittest.TestCase):

    def test_app(self):
        sequences = [
            ["HelloWorld", ""],
            ["HelloFriend", "Jake"],
            ["HelloFriend", "Finn"],
        ]

        deadline = datetime.datetime.now() + datetime.timedelta(seconds=3)

        app = hello_world.new()
        app.channel_client = LocalChannelClient()

        th = threading.Thread(target=app.run)
        th.start()

        def waitForSequences(n):
            while True:
                if len(app.channel_client.sequences_done) == n:
                    return
                if datetime.datetime.now() > deadline:
                    raise Exception("timed out")
                time.sleep(0.1)

        for (i, seq) in enumerate(sequences):
            try:
                got = app.start_sequence(seq[0], seq[1], deadline)
                self.assertEqual(got, api_types.AntarisReturnCode.An_SUCCESS)
                waitForSequences(i+1)
            except:
                # go ahead and stop if we encounter an issue
                app.request_stop()
                raise

        hc = app.health_check()

        app.request_stop()

        th.join(timeout=5)
        self.assertFalse(th.is_alive())

        self.assertEqual(hc, api_types.AntarisReturnCode.An_SUCCESS)
        self.assertEqual(app.channel_client.sequences_done, ["HelloWorld", "HelloFriend", "HelloFriend"])
