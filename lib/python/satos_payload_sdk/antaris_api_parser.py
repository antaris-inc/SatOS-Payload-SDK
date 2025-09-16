import json

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

g_JSON_Key_CAN = "CAN"
g_JSON_Key_Device_Count = "CAN_PORT_COUNT"
g_JSON_Key_CAN_Device_Path = "CAN_Bus_Path_"

g_JSON_Key_I2C = "I2C"
g_JSON_Key_I2C_Adapter_Type = "ADAPTER_TYPE"
g_JSON_Key_I2C_Device_Count = "I2C_PORT_COUNT"
g_JSON_Key_I2C_Device_Path = "I2C_Bus_Path_"
g_JSON_Key__Network = "Network"
g_JSON_Key_Application_Controller_IP_Address = "Application_Controller_IP_Address"

g_JSON_Key_QA7_LIB  = "QA7_LIB"


# Define error code
g_PARSER_ERROR = -1
g_GPIO_AVAILABLE = 1

# Read config info
jsonfile = open('/opt/antaris/app/config.json', 'r')

# returns JSON object as a dictionary
jsfile_data = json.load(jsonfile)

class I2C:
    def __init__(self, port_count, i2c_dev):
        self.i2c_port_count = port_count
        self.i2c_dev = i2c_dev

def api_pa_pc_get_i2c_dev():
    g_total_i2c_port = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_I2C][g_JSON_Key_I2C_Device_Count]
    i2c_dev = []

    i = 0
    for i in range(int(g_total_i2c_port)):
        key = g_JSON_Key_I2C_Device_Path+str(i)
        element = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_I2C][key]
        i2c_dev.append(element)

    i2cObj = I2C(g_total_i2c_port, i2c_dev)
    return i2cObj

def api_pa_pc_get_i2c_adapter():
    i2c_adapter = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_I2C][g_JSON_Key_I2C_Adapter_Type]
    return i2c_adapter

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

def get_ac_ip():
    ac_ip = jsfile_data[g_JSON_Key__Network][g_JSON_Key_Application_Controller_IP_Address]
    return ac_ip

def api_pa_pc_get_gpio_pin_count():
    g_total_gpio_pins = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_GPIO][g_JSON_Key_GPIO_Pin_Count]
    return g_total_gpio_pins

def verify_gpio_pin(input_pin):
    status = g_PARSER_ERROR
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

def api_pa_pc_get_io_interrupt_pin():
    value = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_GPIO][g_JSON_Key_Interrupt_Pin]
    return value

def api_pa_pc_get_gpio_adapter():
    gpio_adapter = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_GPIO][g_JSON_Key_Adapter_Type]
    return gpio_adapter

def api_pa_pc_get_qa7_lib():
    qa7_lib = jsfile_data[g_JSON_Key_QA7_LIB]
    return qa7_lib