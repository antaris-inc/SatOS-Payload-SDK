#ifndef __ANTARIS_API_PARSER_H__
#define __ANTARIS_API_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fstream>
#include "cJSON.h"
#include <cstdlib>
#include <sys/wait.h>
#include <chrono>
#include <iostream>

#include "antaris_api.h"
#include "antaris_api_internal.h"
#include "antaris_sdk_environment.h"
#include "antaris_api_gpio.h"
#include "antaris_api_i2c.h"

#define JSON_Key_GPIO_Pin_Count   ("GPIO_PIN_COUNT")
#define JSON_Key_IO_Access        ("IO_Access")
#define JSON_Key_GPIO             ("GPIO")
#define JSON_Key_Adapter_Type     ("ADAPTER_TYPE")
#define JSON_Key_GPIO_Port        ("GPIO_Port")
#define JSON_Key_GPIO_Pin         ("GPIO_PIN_")
#define JSON_Key_UART             ("UART")
#define JSON_Key_Device_Path      ("Device_Path")
#define JSON_Key_Interrupt_Pin    ("GPIO_Interrupt")
#define JSON_Key_CAN              ("CAN")
#define JSON_Key_CAN_Port_Count   ("CAN_PORT_COUNT")
#define JSON_Key_CAN_Bus_Path     ("CAN_Bus_Path_")
#define JSON_Key_I2C              ("I2C")
#define JSON_Key_I2C_Adapter_Type ("ADAPTER_TYPE")
#define JSON_Key_I2C_Port_Count   ("I2C_PORT_COUNT")
#define JSON_Key_I2C_Bus_Path     ("I2C_Bus_Path_")
#define JSON_Key_QA7_LIB          ("QA7_LIB")
#define JSON_Key_Network          ("Network")
#define JSON_Key_Application_Controller_IP_Address  ("Application_Controller_IP_Address")

#define MAX_DEV_NAME_LENGTH       32  // Max length for each device name

#define QA7_INIT_FUNCTION         "init_qa7_lib"
#define QA7_DEINIT_FUNCTION       "deinit_qa7_lib"
#define QA7_READ_PIN_FUNCTION     "read_pin"
#define QA7_WRITE_PIN_FUNCTION    "write_pin"
#define QA7_I2C_READ_FUNCTION     "read_i2c"
#define QA7_I2C_WRITE_FUNCTION    "write_i2c"

class AntarisApiParser {
    public:
        AntarisReturnCode api_pa_pc_get_gpio_info(gpio_s *gpio);
        AntarisReturnCode api_pa_pc_get_gpio_adapter_type(char *adapter);

        AntarisReturnCode api_pa_pc_get_i2c_dev(i2c_s *i2c_info);
        AntarisReturnCode api_pa_pc_get_i2c_adapter(char *adapter);
        
        AntarisReturnCode api_pa_pc_get_qa7_lib();
        AntarisReturnCode api_pa_pc_get_ac_ip(char *ac_ip);
    private:
};

#endif // __ANTARIS_API_PARSER_H__
