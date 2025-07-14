import time, sys, json

import pylibftdi as ftdi

# Define error code
g_GPIO_ERROR = -1
g_GPIO_AVAILABLE = 1
g_MASK_BIT_0 = 1
g_MASK_BYTE = 0xFF


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