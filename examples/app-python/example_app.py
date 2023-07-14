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

g_GPIO_ERROR = -1
g_Uart_Baudrate = 9600

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

    # The sample program assumes 2 GPIO pins are connected back-to-back. 
    # This sequence toggles level of 'Write Pin' and then reads level of 'Read Pin'
    def handle_test_gpio(self, ctx):
        gpio_info = api_gpio.api_pa_pc_get_gpio_info()

        logger.info("Total gpio pins = %d", int(gpio_info.pin_count))

        i = 0
        # Read initial value of GPIO pins.
        # As GPIO pins are back-to-back connected, their value must be same.
        while (i < int(gpio_info.pin_count)):
            if int(gpio_info.pins[i]) != -1:
                readPin = gpio_info.pins[i];
                i += 1
                writePin = gpio_info.pins[i];

                val = api_gpio.api_pa_pc_read_gpio(int(readPin))
                if val != g_GPIO_ERROR:
                    logger.info("Initial Gpio value of pin no %d is %d ", int(readPin), val)
                else:
                    logger.info("Error in pin no %d", int(readPin))
                    return 
                # Toggle the value
                val = val ^ 1                      
                logger.info("Writing %d to pin no. %d", val, int(writePin))

                # Writing value to WritePin.
                val = api_gpio.api_pa_pc_write_gpio(int(writePin), val)
                if val != g_GPIO_ERROR:
                    logger.info("Written %d successfully to pin no %d", val, int(writePin))
                else:
                    logger.info("error in pin no %d ", int(writePin))
                    return 
                # As Read and Write pins are back-to-back connected, 
                # Reading value of Read pin to confirm GPIO success/failure
                val = api_gpio.api_pa_pc_read_gpio(int(readPin))
                if val != g_GPIO_ERROR:
                    logger.info("Final Gpio value of pin no %d is %d ", int(readPin), val)
                else:
                    logger.info("Error in pin no %d", int(readPin))
                    return
            i += 1

    # Sequence to test UART loopback. The sample program assumes Tx and Rx are connected in loopback mode.
    def handle_uart_loopback(self, ctx):
        data = ctx.params
        if data == "":
            logger.info("Using default string, as input string is empty")
            data = "Default string: Uart Tested working"

        data = data + "\n"
        uartPort = api_gpio.api_pa_pc_get_uart_dev()
        try: 
            ser = serial.Serial(uartPort, g_Uart_Baudrate)  # Replace '9600' with your baud rate
        except Exception as e:
            print("Error in opening serial port")
            return
        
        logger.info(f"writing data")
        # Write data to the serial port
        ser.write(data.encode('utf-8'))  # Send the data as bytes

        logger.info("Reading data")
        # Read data from the serial port
        read_data = ser.readline()
        logger.info("Data =  %s", read_data)

        # Close the serial port
        ser.close()


def new():
    ctl = Controller()

    app = app_framework.PayloadApplication()
    app.set_health_check(ctl.is_healthy)

    # Note : SatOS-Payload-SDK supports sequence upto 16 characters long
    app.mount_sequence("HelloWorld", ctl.handle_hello_world)
    app.mount_sequence("HelloFriend", ctl.handle_hello_friend)
    app.mount_sequence("LogLocation", ctl.handle_log_location)
    app.mount_sequence("TestGPIO", ctl.handle_test_gpio)
    app.mount_sequence("UARTLoopback", ctl.handle_uart_loopback)

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
