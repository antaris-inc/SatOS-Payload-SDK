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

# Note : Assign proper values of GPIO Pin and Port, as per your hardware interface. 
g_GPIO_ERROR = -1
g_GPIO_Port = 0
g_GPIO_Read_Pin = 5
g_GPIO_Write_Pin = 6

# Note : Assign proper pin numbers for serial port testing, as per your hardware interface.
g_Uart_Port = '/dev/ttyUSB0'
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

    def handle_get_interface_info(self, ctx):
        gpio_count = api_gpio.api_pa_pc_total_gpio_pins()
        logger.info(f"Total gpio pins = %d", gpio_count)
        for i in range(int(gpio_count)):
            pin = api_gpio.api_pa_pc_get_gpio_pins_number(i)
            logger.info("Pin %d. Interface = %d", i, pin)
        logger.info("I/O Interface = %s", api_gpio.api_pa_pc_get_io_interface())

    def handle_get_interrupt_pin(self, ctx):
        interrupt_pin = api_gpio.api_pa_pc_get_io_interrupt()
        logger.info("Interrupt pin = %d", interrupt_pin)

    def handle_gpio_read(self, ctx):
        val = api_gpio.api_pa_pc_read_gpio(g_GPIO_Port, g_GPIO_Read_Pin)
        if val != g_GPIO_ERROR:
            logger.info("Initial Gpio value of pin no %d is %d ", g_GPIO_Read_Pin, val)
        else:
            logger.info("Error in pin no %d", g_GPIO_Read_Pin)

    def handle_gpio_write(self, ctx):
        pin_value = int(ctx.params)
        # All values greater than 0 are considered high
        if pin_value > 0:
            pin_value = 1
        # All values less than 0 are considered low
        if pin_value < 0:
            pin_value = 0
        val = api_gpio.api_pa_pc_write_gpio(g_GPIO_Port, g_GPIO_Write_Pin, pin_value)
        if val != g_GPIO_ERROR:
            logger.info("Written %d successfully to pin no %d", pin_value, g_GPIO_Write_Pin)
        else:
            logger.info("error in pin no %d ", g_GPIO_Write_Pin)

    def handle_uart_loopback(self, ctx):
        data = ctx.params
        if data == "":
            logger.info("Using default string, as input string is empty")
            data = "Default string: Uart Tested working"

        data = data + "\n"
        ser = serial.Serial(g_Uart_Port, g_Uart_Baudrate)  # Replace '/dev/ttyUSB0' with your serial port and '9600' with your baud rate
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

    app.mount_sequence("HelloWorld", ctl.handle_hello_world)
    app.mount_sequence("HelloFriend", ctl.handle_hello_friend)
    app.mount_sequence("LogLocation", ctl.handle_log_location)
    app.mount_sequence("Read_FTDI_GPIO", ctl.handle_gpio_read)
    app.mount_sequence("Write_FTDI_GPIO", ctl.handle_gpio_write)
    app.mount_sequence("UART_Loopback", ctl.handle_uart_loopback)
    app.mount_sequence("Get_Interface_Info", ctl.handle_get_interface_info)
    app.mount_sequence("Get_Interrupt_Pin", ctl.handle_get_interrupt_pin)

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
