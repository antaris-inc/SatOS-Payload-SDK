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

import time
import datetime
import logging
import threading
import unittest

from satos_payload import app_framework


class assertThread(threading.Thread):
    def __init__(self, fn):
        super().__init__()
        self.got = False
        self.fn = fn

    def run(self):
        self.got = self.fn(timeout=5)


class TestStoppable(unittest.TestCase):

    # confirm that a single client can progress through
    # the state machine relying on timeouts, no async triggers
    def test_linear_state_machine(self):
        st = app_framework.Stoppable()

        self.assertFalse(st.stop_requested())
        self.assertFalse(st.wait_until_stop_requested(timeout=0.01))
        self.assertFalse(st.wait_until_stopped(timeout=0.01))

        st.request_stop()

        self.assertTrue(st.stop_requested())
        self.assertTrue(st.wait_until_stop_requested(timeout=0.01))
        self.assertFalse(st.wait_until_stopped(timeout=0.01))

        st.stopped()

        self.assertTrue(st.stop_requested())
        self.assertTrue(st.wait_until_stop_requested(timeout=0.01))
        self.assertTrue(st.wait_until_stopped(timeout=0.01))

    # ensure the assertThread fixture works as expected
    def test_thread_behavior(self):
        st = app_framework.Stoppable()

        th = assertThread(lambda **kwargs: False)
        th.start()
        th.join()
        self.assertFalse(th.got)

        th = assertThread(lambda **kwargs: True)
        th.start()
        th.join()
        self.assertTrue(th.got)

    def test_notify_request_stop(self):
        st = app_framework.Stoppable()

        th = assertThread(st.wait_until_stop_requested)
        th.start()

        # assert the thread did not close prematurely
        th.join(timeout=0.1)
        self.assertTrue(th.is_alive())

        st.request_stop()

        th.join()
        self.assertTrue(th.got)

    def test_notify_stopped(self):
        st = app_framework.Stoppable()

        th = assertThread(st.wait_until_stopped)
        th.start()

        # assert the thread did not close prematurely
        th.join(timeout=0.1)
        self.assertTrue(th.is_alive())

        st.stopped()

        th.join()
        self.assertTrue(th.got)


class TestSequenceHandler(unittest.TestCase):

    def test_basic(self):
        client = None
        params = "abc"
        deadline_ms = 60 * 1000  # 60sec timeout, will not be hit

        class testSequenceHandler:
            def __init__(self):
                self.callback_func_called = False
                self.handler_func_called = False
                self.handler_func_params = ""

            def handler_func(self, ctx):
                self.handler_func_called = True
                self.handler_func_params = ctx.params

            def callback_func(self):
                self.callback_func_called = True

        tsh = testSequenceHandler()

        sh = app_framework.SequenceHandler("", params, deadline_ms, client, tsh.handler_func, tsh.callback_func)
        sh.start()

        self.assertTrue(sh.wait_until_stopped(timeout=5))

        self.assertTrue(tsh.handler_func_called)
        self.assertEqual(tsh.handler_func_params, params)
        self.assertTrue(tsh.callback_func_called)

    def test_deadline(self):
        client = None
        params = "abc"

        class testSequenceHandler:
            def __init__(self):
                self.handler_func_called = False
                self.handler_func_deadline_reached = False

            def handler_func(self, ctx):
                time.sleep(0.2) # sleep 200ms
                self.handler_func_deadline_reached = ctx.deadline_reached

            def callback_func(self):
                pass

        tsh = testSequenceHandler()

        # confirm a short deadline is triggered
        sh = app_framework.SequenceHandler("", params, 100, client, tsh.handler_func, tsh.callback_func)
        sh.start()
        self.assertTrue(sh.wait_until_stopped(timeout=5))
        self.assertTrue(tsh.handler_func_deadline_reached)

        # confirm a long deadline is not triggered
        sh = app_framework.SequenceHandler("", params, 300, client, tsh.handler_func, tsh.callback_func)
        sh.start()
        self.assertTrue(sh.wait_until_stopped(timeout=5))
        self.assertTrue(tsh.handler_func_deadline_reached)
