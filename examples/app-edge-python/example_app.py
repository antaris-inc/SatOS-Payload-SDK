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
import serial

from satos_payload_sdk import app_framework
from satos_payload_sdk import antaris_api_gpio as api_gpio
from satos_payload_sdk import antaris_api_can as api_can
from numba import cuda
import numpy as np


logger = logging.getLogger()

# Try to import CuPy and check for GPU
try:
    import cupy as xp
    _ = xp.zeros((1,))  # Trigger GPU context
    GPU_AVAILABLE = True
    logger.info("CuPy loaded — running on GPU.")
except Exception as e:
    import numpy as xp
    GPU_AVAILABLE = False
    logger.warning("CuPy not available, using NumPy (CPU).")


class Controller:

    def is_healthy(self):
        logger.info("Health check succeeded")
        return True

    def handle_hello_world(self, ctx):
        logger.info("Handling sequence: hello, world!")
        logger.info("Calling run_cuda now...")
        self.run_cuda("hello_world")

    def run_cuda(self, tag):
        if not GPU_AVAILABLE:
            logger.info(f"[{tag}] No CUDA device available. Skipping CUDA demo.")
            return

        try:
            output = np.zeros(1, dtype=np.float32)
            d_output = cuda.to_device(output)
            self._cuda_kernel[1, 1](d_output)
            result = d_output.copy_to_host()
            logger.info(f"[{tag}] Ran CUDA demo. GPU result: {result[0]}")
        except Exception as e:
            logger.error(f"[{tag}] CUDA demo failed: {e}")

    @staticmethod
    @cuda.jit
    def _cuda_kernel(arr):
        idx = cuda.grid(1)
        if idx < arr.size:
            arr[idx] = 10.0
        
    
def new():
    ctl = Controller()

    app = app_framework.PayloadApplication()
    app.set_health_check(ctl.is_healthy)


    # Note : SatOS-Payload-SDK supports sequence upto 16 characters long
    app.mount_sequence("HelloWorld", ctl.handle_hello_world)
    return app

if __name__ == '__main__':
    DEBUG = os.environ.get('DEBUG')
    logging.basicConfig(    level=logging.DEBUG if DEBUG else logging.INFO,
                            format="%(asctime)s  %(levelname)s %(message)s",
                            datefmt="%Y-%m-%d %H:%M:%S"
                        )

    app = new()

    try:
        app.run()
    except Exception as exc:
        logger.exception("payload app failed")
        sys.exit(1)
