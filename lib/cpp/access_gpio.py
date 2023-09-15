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

# sys.argv[0] = '1' indicates GPIO write and '0' indicates GPIO read
# sys.argv[1] = Port number
# sys.argv[2] = Pin number

import time, sys, json

import pylibftdi as ftdi

# Define error code
g_GPIO_ERROR = -1
g_GPIO_AVAILABLE = 1
g_SLEEP_TIME_IN_SEC = 1
g_MASK_BIT_0 = 1
g_MASK_BYTE = 0xFF

def api_pa_pc_read_gpio(pin, port):
    try:
        DeviceName = ftdi.Driver().list_devices()[0][2]  # Assumptioon: single FTDI device connected.
        if not DeviceName:
            print("FTDI device not connected")
            return g_GPIO_ERROR 
    except Exception as e:
        print("FTDI device not connected")
        return g_GPIO_ERROR 
    
    Device = ftdi.BitBangDevice(device_id=DeviceName, interface_select=int(port))
    time.sleep(g_SLEEP_TIME_IN_SEC)
    wr_port = g_MASK_BIT_0 << int(pin)
    wr_port = g_MASK_BYTE ^ wr_port
    out = Device.direction & wr_port
    Device.direction = out
    op = (Device.port >> pin) & g_MASK_BIT_0
    Device.close()
    return op

def api_pa_pc_write_gpio(port, pin, value):
        
    try:
        DeviceName = ftdi.Driver().list_devices()[0][2] # Assumption : Single FTDI device connected.
        if not DeviceName:
            print("FTDI device not connected")
            return g_GPIO_ERROR 
    except Exception as e:
        print("FTDI device not connected")
        return g_GPIO_ERROR
    
    Device = ftdi.BitBangDevice(device_id=DeviceName, interface_select=int(port))
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

if __name__ == "__main__":
    argc = len(sys.argv)
    if argc < 4:
        print("Error: Not enough arguments")
        print("Usage:")
        print(" To read any pin: ")
        print("     python3 access_gpio.py 0 <port> <pin number>")
        print(" To write any pin: ")
        print("     python3 access_gpio.py 1 <port> <pin number> <value>")
        sys.exit() 


    if sys.argv[1] == "0":
        api_pa_pc_read_gpio(sys.argv[2], sys.argv[3])
    elif sys.argv[1] == "1":
        api_pa_pc_write_gpio(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        print("Script should use 1 or 0")
    