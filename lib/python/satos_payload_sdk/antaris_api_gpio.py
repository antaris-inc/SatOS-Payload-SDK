#
#   Copyright 2022 Antaris, Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

# This file assumes that, env.json file is present at /opt/antaris/app
# location. The sample file is checked-in in conf directory

import time, sys, json

from satos_payload_sdk import antaris_api_common as api_common
import pylibftdi as ftdi

# Define error code
g_GPIO_ERROR = -1
g_GPIO_AVAILABLE = 1
g_SLEEP_TIME_IN_SEC = 1
g_MASK_BIT_0 = 1
g_MASK_BYTE = 0xFF
g_TRUETWIN_FLAG = api_common.g_KEEPALIVE_ENABLE

# Read config info
jsonfile = open('/opt/antaris/app/config.json', 'r')

# returns JSON object as a dictionary
jsfile_data = json.load(jsonfile)

total_gpio_pins = jsfile_data['IO_Access']['GPIO_PIN_COUNT']

def verify_gpio_pin(input_pin):
    status = g_GPIO_ERROR
    for i in range(int(total_gpio_pins)):
        key = 'GPIO_PIN_'+str(i)
        value = jsfile_data['IO_Access'][key]
        if int(input_pin) == int(value):
            status = g_GPIO_AVAILABLE
    return status

#device specific functions
# To know device, use python3 -m pylibftdi.examples.list_devices
DeviceName = ftdi.Driver().list_devices()[0][2]
Device = ftdi.BitBangDevice(DeviceName)

def api_pa_pc_read_gpio(port, pin):
    # In case of True-twin, no need to check pin configuration in config.json
    if g_TRUETWIN_FLAG != '1':
        status = verify_gpio_pin(pin)
        if status == g_GPIO_ERROR:
            return g_GPIO_ERROR
    
    DeviceName = ftdi.Driver().list_devices()[0][2]
    Device = ftdi.BitBangDevice(device_id=DeviceName, interface_select=port)
    time.sleep(g_SLEEP_TIME_IN_SEC)
    wr_port = g_MASK_BIT_0 << int(pin)
    wr_port = g_MASK_BYTE ^ wr_port
    out = Device.direction & wr_port
    Device.direction = out
    op = (Device.port >> pin) & g_MASK_BIT_0
    Device.close()
    return op

def api_pa_pc_write_gpio(port, pin, value):
    # In case of True-twin, no need to check pin configuration in config.json
    if g_TRUETWIN_FLAG != '1':
        status = verify_gpio_pin(pin)
        if status == g_GPIO_ERROR:
            return g_GPIO_ERROR

    DeviceName = ftdi.Driver().list_devices()[0][2]
    Device = ftdi.BitBangDevice(device_id=DeviceName, interface_select=port)
    time.sleep(g_SLEEP_TIME_IN_SEC)
    wr_port = g_MASK_BIT_0 << int(pin)
    Device.direction = Device.direction | wr_port
    if value == 0:
        wr_val = g_MASK_BYTE ^ wr_port
        Device.port = Device.port & wr_val
    else:
        Device.port = (Device.port | wr_port)
    time.sleep(g_SLEEP_TIME_IN_SEC)
    op = (Device.port >> pin) & g_MASK_BIT_0
    Device.close()
    return op