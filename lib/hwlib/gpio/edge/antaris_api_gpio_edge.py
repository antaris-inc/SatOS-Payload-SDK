# Define error code
g_GPIO_ERROR = -1
g_GPIO_AVAILABLE = 1
g_MASK_BIT_0 = 1
g_MASK_BYTE = 0xFF

import Jetson.GPIO as GPIO
import time

def api_read_gpio(port,pin):
    try:
        GPIO.setmode(GPIO.BOARD)  # Or GPIO.BCM, depending on your pin numbering scheme
        GPIO.setup(pin, GPIO.IN)
        value = GPIO.input(pin)
        GPIO.cleanup(pin)
        return value
    except Exception as e:
        print("Jetson GPIO read error:", e)
        return g_GPIO_ERROR

def api_write_gpio(pin, value):
    try:
        GPIO.setmode(GPIO.BOARD)  # or GPIO.BCM depending on how you reference the pin
        GPIO.setup(pin, GPIO.OUT)
        GPIO.output(pin, GPIO.HIGH if value else GPIO.LOW)
        op = GPIO.input(pin)  # read back the pin value (0 or 1)
        GPIO.cleanup(pin)
        return op
    except Exception as e:
        print("Jetson GPIO write error:", e)
        return g_GPIO_ERROR
