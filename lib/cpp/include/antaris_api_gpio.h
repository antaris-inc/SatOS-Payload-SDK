/*
 * Copyright 2022 Antaris, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// This file assumes that, config.json file is present at /opt/antaris/app
// location. The sample file is checked-in in conf directory

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fstream>
#include "cJSON.h"
#include <cstdlib>

#include "antaris_api.h"
#include "antaris_api_internal.h"
#include "antaris_sdk_environment.h"

#define PYTHON_SCRIPT             "/workspace/lib/cpp//access_gpio.py"
#define JSON_Key_GPIO_Pin_Count   ("GPIO_PIN_COUNT")
#define JSON_Key_IO_Access        ("IO_Access")
#define JSON_Key_GPIO             ("GPIO")
#define JSON_Key_Adapter_Type     ("ADAPTER_TYPE")
#define JSON_Key_GPIO_Port        ("GPIO_Port")
#define JSON_Key_GPIO_Pin         ("GPIO_PIN_")
#define JSON_Key_UART             ("UART")
#define JSON_Key_Device_Path      ("Device_Path")
#define JSON_Key_Interrupt_Pin    ("GPIO_Interrupt")

typedef struct gpio {
    int8_t pin_count;
    int8_t gpio_port;
    int8_t pins[8];
    int8_t interrupt_pin;
}gpio_s;

class AntarisApiGPIO {
    public:
        AntarisReturnCode api_pa_pc_get_gpio_info(gpio_s *gpio);

        int8_t api_pa_pc_read_gpio(int8_t gpio_port, int8_t pin_number);

        AntarisReturnCode api_pa_pc_write_gpio(int8_t gpio_port, int8_t pin_number, int8_t value);

    private:

};