import time, sys, json
import os
from periphery import GPIO

# Define error code
g_GPIO_ERROR = -1
g_GPIO_AVAILABLE = 1
g_MASK_BIT_0 = 1
g_MASK_BYTE = 0xFF


def api_read_gpio(port, pin):
    # Map your QA7 GPIO numbering appropriately
    gpio_number = compute_gpio_number(port, pin)
    try:
        gpio = GPIO(gpio_number, "in")
    except Exception as e:
        print("QA7 GPIO not accessible:", e)
        return g_GPIO_ERROR

    try:
        op = gpio.read()
    except Exception as e:
        print("Error reading QA7 GPIO:", e)
        gpio.close()
        return g_GPIO_ERROR

    gpio.close()
    return op


def compute_gpio_number(port, pin):
    # Replace this with real GPIO mapping for your board
    return port * 8 + pin

def api_write_gpio(port, pin, value):
    gpio_number = compute_gpio_number(port, pin)

    try:
        gpio = GPIO(gpio_number, "out")
    except Exception as e:
        print("QA7 GPIO access error:", e)
        return g_GPIO_ERROR

    try:
        gpio.write(bool(value))  # value should be 0 or 1
        op = gpio.read()
    except Exception as e:
        print("QA7 GPIO write/read error:", e)
        gpio.close()
        return g_GPIO_ERROR

    gpio.close()
    return int(op)

