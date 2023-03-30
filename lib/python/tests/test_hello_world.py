import datetime
import logging
import threading
import unittest

from satos_payload.examples import hello_world
import satos_payload.gen.antaris_api_types as api_types



class LocalChannelClient:
    def __init__(self):
        self.lock = threading.Lock()
        self.staged_files = []

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


class TestHelloWorld(unittest.TestCase):

    def test_app(self):
        cl = LocalChannelClient()
        ctl = hello_world.HelloController()
        app = hello_world.make_app(ctl)

        th = threading.Thread(target=app.run_local, args=(cl,))
        th.start()

        deadline = datetime.datetime.utcnow() + datetime.timedelta(seconds=10)
        params = api_types.StartSequenceParams(0, "HelloWorld", "", deadline)
        app.handle_start_sequence(params)

        app.request_stop()

        th.join(timeout=5)
        self.assertFalse(th.is_alive())

        self.assertEqual(ctl.sequence_count, 1)
