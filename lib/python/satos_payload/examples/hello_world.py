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

from satos_payload import app_framework


logger = logging.getLogger()


class HelloController:

    def is_healthy(self):
        logger.info("Ran health check")
        return True

    def handle_hello_world(self, ctx):
        logger.info("Hello, world!")

    def handle_hello_friend(self, ctx):
        name = ctx.params
        logger.info(f"Hello, {name}!")


def new():
    ctl = HelloController()

    app = app_framework.PayloadApplication()
    app.set_health_check(ctl.is_healthy)
    app.mount_sequence("HelloWorld", ctl.handle_hello_world)
    app.mount_sequence("HelloFriend", ctl.handle_hello_friend)

    return app


if __name__ == '__main__':
    DEBUG = os.environ.get('DEBUG')
    logging.basicConfig(level=logging.DEBUG if DEBUG else logging.INFO)

    ctl = HelloController()
    app = make_app(ctl)

    try:
        app.run()
    except Exception as exc:
        logger.exception("payload app failed")
        sys.exit(1)
