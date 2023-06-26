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

import pylibftdi as ftdi

# Define error code
g_GPIO_ERROR = -1
g_GPIO_AVAILABLE = 1
g_SLEEP_TIME_IN_SEC = 1
g_MASK_BIT_0 = 1
g_MASK_BYTE = 0xFF

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

def api_pa_pc_total_gpio_pins():
    return total_gpio_pins

def api_pa_pc_get_gpio_pins_number(index):
    key = 'GPIO_PIN_'+str(index)
    value = jsfile_data['IO_Access'][key]
    return value

def api_pa_pc_get_io_interface():
    value = jsfile_data['IO_Access']["Interface_Access_Path"]
    return value

def api_pa_pc_get_io_interrupt():
    value = jsfile_data['IO_Access']["GPIO_Interrupt"]
    return value

def api_pa_pc_read_gpio(port, pin):
    status = verify_gpio_pin(pin)
    if status == g_GPIO_ERROR:
        return g_GPIO_ERROR
    
    try:
        DeviceName = ftdi.Driver().list_devices()[0][2]
        if not DeviceName:
            print("FTDI device not connected")
            return g_GPIO_ERROR 
    except Exception as e:
        print("FTDI device not connected")
        return g_GPIO_ERROR 
    
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
    status = verify_gpio_pin(pin)
    if status == g_GPIO_ERROR:
        return g_GPIO_ERROR

    try:
        DeviceName = ftdi.Driver().list_devices()[0][2]
        if not DeviceName:
            print("FTDI device not connected")
            return g_GPIO_ERROR 
    except Exception as e:
        print("FTDI device not connected")
        return g_GPIO_ERROR
    
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
