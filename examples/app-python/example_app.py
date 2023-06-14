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
import json
import logging
import pathlib
import os
import sys
import time

from satos_payload_sdk import app_framework


logger = logging.getLogger()


class Controller:

    def is_healthy(self):
        logger.info("Health check succeeded")
        return True

    def handle_hello_world(self, ctx):
        logger.info("Handling sequence: hello, world!")

    def handle_hello_friend(self, ctx):
        name = ctx.params
        logger.info(f"Handling sequence: hello, {name}!")

    def handle_log_location(self, ctx):
        loc = ctx.client.get_current_location()
        logger.info(f"Handling sequence: lat={loc.latitude}, lng={loc.longitude}, alt={loc.altitude}")


    def handle_stage_filedownload(self, ctx):
        logger.info("Staging file for upload")
        resp = ctx.client.stage_file_download("SampleFile.txt")

def new():
    ctl = Controller()

    app = app_framework.PayloadApplication()
    app.set_health_check(ctl.is_healthy)

    app.mount_sequence("HelloWorld", ctl.handle_hello_world)
    app.mount_sequence("HelloFriend", ctl.handle_hello_friend)
    app.mount_sequence("LogLocation", ctl.handle_log_location)
    app.mount_sequence("StageFile",ctl.handle_stage_filedownload)

    return app


if __name__ == '__main__':
    DEBUG = os.environ.get('DEBUG')
    logging.basicConfig(level=logging.DEBUG if DEBUG else logging.INFO)

    app = new()

    try:
        app.run()
    except Exception as exc:
        logger.exception("payload app failed")
        sys.exit(1)
