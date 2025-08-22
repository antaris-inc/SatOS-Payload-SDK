import ctypes

from satos_payload_sdk import antaris_api_parser as api_parser

# Load the shared library
qa7lib = 0

def api_pa_pc_write_i2c_data(port, baseAddr, index, data, length):
    global qa7lib
    adapter_type = api_parser.api_pa_pc_get_i2c_adapter()

    if adapter_type == "QA7":
        if qa7lib == 0:
            qa7lib_path = api_parser.api_pa_pc_get_qa7_lib()
            qa7lib = ctypes.CDLL(qa7lib_path)
        qa7lib.write_i2c.argtypes = [ctypes.c_int16, ctypes.c_byte, ctypes.c_int16, ctypes.c_byte, ctypes,c_int32]
        qa7lib.write_i2c.argtypes = [ c_uint16,   # i2c_dev
                                      c_uint8,    # i2c_address
                                      c_uint16,   # index
                                      POINTER(c_uint8),  # data pointer
                                      c_int       # data_length
                                    ]
        qa7lib.write_i2c.restype = ctypes.c_int32
        if qa7lib.write_i2c( c_uint16(port), c_uint8(baseAddr), c_uint16(index), data, c_int(length)) == True:
            return True
        else:
            return False
    else:
        return False

def api_pa_pc_read_i2c_data(port, baseAddr, index, data):
    global qa7lib
    adapter_type = api_parser.api_pa_pc_get_i2c_adapter()

    if adapter_type == "QA7":
        if qa7lib == 0:
            qa7lib_path = api_parser.api_pa_pc_get_qa7_lib()
            qa7lib = ctypes.CDLL(qa7lib_path)
        qa7lib.read_i2c.argtypes = [ ctypes.c_uint16,          # i2c_dev
                                     ctypes.c_uint8,           # i2c_address
                                     ctypes.c_uint16,          # index
                                     ctypes.POINTER(ctypes.c_uint8)   # data (output buffer)
                                    ]
        qa7lib.read_i2c.restype = ctypes.c_int32
        if qa7lib.read_i2c(c_uint16(port), c_uint8(baseAddr), c_uint16(index), data) == True:
            return data
        else:
            return False
    else:
        return False
    
def api_pa_pc_deinit_i2c_lib():
    global qa7lib
    
    adapter_type = api_parser.api_pa_pc_get_gpio_adapter()

    if adapter_type == "QA7":
        if qa7lib != 0:
            qa7lib.deinit_qa7_lib()

    return True
 
def api_pa_pc_init_i2c_lib():
    global qa7lib
   
    adapter_type = api_parser.api_pa_pc_get_gpio_adapter()

    if adapter_type == "QA7":
        if qa7lib == 0:
            qa7lib_path = api_parser.api_pa_pc_get_qa7_lib()
            qa7lib = ctypes.CDLL(qa7lib_path)
            qa7lib.init_qa7_lib()
    elif adapter_type == "FTDI":
        print("FTDI init done")
    else:
        print("Device not supported")
        return False
   
    return True