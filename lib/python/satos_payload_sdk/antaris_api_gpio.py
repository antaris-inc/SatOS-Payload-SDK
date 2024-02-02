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

g_JSON_Key_IO_Access = "IO_Access"
g_JSON_Key_GPIO = "GPIO"
g_JSON_Key_Adapter_Type = "ADAPTER_TYPE"
g_JSON_Key_GPIO_Pin_Count = "GPIO_PIN_COUNT"
g_JSON_Key_GPIO_Port = "GPIO_Port"
g_JSON_Key_GPIO_Pin = "GPIO_PIN_"
g_JSON_Key_UART = "UART"
g_JSON_Key_Device_Count = "UART_PORT_COUNT"
g_JSON_Key_Device_Path = "Device_Path_"
g_JSON_Key_Interrupt_Pin = "GPIO_Interrupt"

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

class UART:
    def __init__(self, port_count, uart_dev):
        self.uart_port_count = port_count
        self.uart_dev = uart_dev

class GPIO:
    def __init__(self, pin_count, pin, interrupt_pin):
        self.pin_count = pin_count
        self.pins = pin
        self.interrupt_pin = interrupt_pin

def api_pa_pc_get_gpio_info():
    g_total_gpio_pins = api_pa_pc_get_gpio_pin_count()
    pin = [-1, -1, -1, -1, -1, -1, -1, -1]
    i = 0
    for i in range(int(g_total_gpio_pins)):
        key = g_JSON_Key_GPIO_Pin+str(i)
        pin[i] = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_GPIO][key]

    interrupt_pin = api_pa_pc_get_io_interrupt_pin()
    gpio = GPIO(g_total_gpio_pins, pin, interrupt_pin)
    return gpio    

def api_pa_pc_get_gpio_pin_count():
    g_total_gpio_pins = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_GPIO][g_JSON_Key_GPIO_Pin_Count]
    return g_total_gpio_pins

def verify_gpio_pin(input_pin):
    status = g_GPIO_ERROR
    g_total_gpio_pins = api_pa_pc_get_gpio_pin_count()

    for i in range(int(g_total_gpio_pins)):
        key = g_JSON_Key_GPIO_Pin+str(i)
        value = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_GPIO][key]
        if int(input_pin) == int(value):
            status = g_GPIO_AVAILABLE
    return status

def api_pa_pc_get_gpio_port():
    value = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_GPIO][g_JSON_Key_GPIO_Port]
    return value

def api_pa_pc_get_gpio_pins_number(index):
    key = g_JSON_Key_GPIO_Pin+str(index)
    value = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_GPIO][key]
    return value

def api_pa_pc_get_uart_dev():
    g_total_uart_port = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_UART][g_JSON_Key_Device_Count]
    uart_dev = []

    i = 0
    for i in range(int(g_total_uart_port)):
        key = g_JSON_Key_Device_Path+str(i)
        element = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_UART][key]
        uart_dev.append(element)

    uart = UART(g_total_uart_port, uart_dev)
    return uart

def api_pa_pc_get_io_interrupt_pin():
    value = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_GPIO][g_JSON_Key_Interrupt_Pin]
    return value

def api_pa_pc_read_gpio(pin):
    status = verify_gpio_pin(pin)
    if status == g_GPIO_ERROR:
        return g_GPIO_ERROR
    
    adapter_type = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_GPIO][g_JSON_Key_Adapter_Type]

    if adapter_type != "FTDI":
        print("Only FTDI devices are supported")
        return g_GPIO_ERROR
    
    port = api_pa_pc_get_gpio_port()

    op = api_read_gpio(port, pin)
    
    return op

def api_read_gpio(port, pin):
    try:
        DeviceName = ftdi.Driver().list_devices()[0][2]  # Assumptioon: single FTDI device connected.
        if not DeviceName:
            print("FTDI device not connected")
            return g_GPIO_ERROR 
    except Exception as e:
        print("FTDI device not connected")
        return g_GPIO_ERROR 

    Device = ftdi.BitBangDevice(device_id=DeviceName, interface_select=int(port))
    op = (Device.port >> int(pin)) & g_MASK_BIT_0
    Device.close()
    return op

def api_pa_pc_write_gpio(pin, value):
    status = verify_gpio_pin(pin)
    if status == g_GPIO_ERROR:
        return g_GPIO_ERROR

    adapter_type = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_GPIO][g_JSON_Key_Adapter_Type]

    if adapter_type != "FTDI":
        print("Only FTDI devices are supported")
        return g_GPIO_ERROR
    
    port = api_pa_pc_get_gpio_port()
    op = api_write_gpio(port, pin, value)
    
    return op

def api_write_gpio(port, pin, value):
    try:
        DeviceName = ftdi.Driver().list_devices()[0][2] # Assumption : Single FTDI device connected.
        if not DeviceName:
            print("FTDI device not connected")
            return g_GPIO_ERROR 
    except Exception as e:
        print("FTDI device not connected")
        return g_GPIO_ERROR
    
    Device = ftdi.BitBangDevice(device_id=DeviceName, interface_select=int(port))
    wr_port = g_MASK_BIT_0 << int(pin)
    Device.direction = wr_port
    if int(value) == 0:
        wr_val = g_MASK_BYTE ^ wr_port
        Device.port = Device.port & wr_val
    else:
        Device.port = (Device.port | wr_port)

    op = (Device.port >> int(pin)) & g_MASK_BIT_0
    Device.close()
    return op

# Main function is added for standalone testing of GPIO, if needed
if __name__ == "__main__":
    output = g_GPIO_ERROR
    argc = len(sys.argv)

    if argc < 4:
        print("Error: Not enough arguments")
        print("Usage:")
        print(" To read any pin: ")
        print("     python3 antaris_api_gpio.py 0 <port> <pin number>")
        print(" To write any pin: ")
        print("     python3 antaris_api_gpio.py 1 <port> <pin number> <value>")
        sys.exit()

    if sys.argv[1] == "0":
        output = api_read_gpio(int(sys.argv[2]), sys.argv[3])
    elif sys.argv[1] == "1":
        output = api_write_gpio(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        print("Script should use 1 or 0")
    
    sys.exit(output)
