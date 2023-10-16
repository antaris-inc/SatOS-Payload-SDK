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
#include <sys/wait.h>
#include "Python.h"

#include "antaris_api_gpio.h"
#include "antaris_api.h"
#include "antaris_api_internal.h"
#include "antaris_sdk_environment.h"

#define GENERIC_ERROR -1

AntarisReturnCode AntarisApiGPIO::api_pa_pc_get_gpio_info(gpio_s *gpio)
{
    cJSON *p_cJson = NULL;
    cJSON *key_io_access = NULL;
    cJSON *key_gpio = NULL;
    cJSON *pJsonStr = NULL;
    char *str = NULL;
    char key[32] = {'\0'};

    gpio->pins[8] = {-1};
    gpio->gpio_port = -1;
    gpio->pin_count = -1;
    gpio->interrupt_pin = -1;

    read_config_json(&p_cJson);
    if (p_cJson == NULL)
    {
        printf("Failed to read the config.json\n");
    }
    else
    {
        key_io_access = cJSON_GetObjectItemCaseSensitive(p_cJson, JSON_Key_IO_Access);
        if (key_io_access)
        {
            key_gpio = cJSON_GetObjectItemCaseSensitive(key_io_access, JSON_Key_GPIO);
            if (key_gpio)
            {
                // Check adapter type
                pJsonStr = cJSON_GetObjectItem(key_gpio, JSON_Key_Adapter_Type);
                if (cJSON_IsString(pJsonStr))
                {
                    str = cJSON_GetStringValue(pJsonStr);
                    if ((str == NULL) ||
                        ((strncmp(str, "FTDI", 4) != 0)))
                    {
                        printf("Only FTDI devices are supported");
                        return An_GENERIC_FAILURE;
                    }
                }

                // get GPIO pin count
                pJsonStr = cJSON_GetObjectItem(key_gpio, JSON_Key_GPIO_Pin_Count);
                if (cJSON_IsString(pJsonStr))
                {
                    str = cJSON_GetStringValue(pJsonStr);
                    if ((str != NULL) && (strlen(str) <= sizeof(int8_t)))
                    {
                        gpio->pin_count = *str - '0';
                    }
                    else
                    {
                        printf("Failed to read gpio count the json");
                    }
                }

                // Get GPIO port
                pJsonStr = cJSON_GetObjectItem(key_gpio, JSON_Key_GPIO_Port);
                if (cJSON_IsString(pJsonStr))
                {
                    str = cJSON_GetStringValue(pJsonStr);
                    if ((str != NULL) && (strlen(str) <= sizeof(int8_t)))
                    {
                        gpio->gpio_port = *str - '0';
                    }
                    else
                    {
                        printf("Failed to read gpio count the json");
                    }
                }

                // get GPIO pins
                for (int i = 0; i < gpio->pin_count; i++)
                {
                    sprintf(key, "%s%d", JSON_Key_GPIO_Pin, i);
                    pJsonStr = cJSON_GetObjectItem(key_gpio, key);
                    if (cJSON_IsString(pJsonStr))
                    {
                        str = cJSON_GetStringValue(pJsonStr);
                        if ((str != NULL) && (strlen(str) <= sizeof(int8_t)))
                        {
                            gpio->pins[i] = *str - '0';
                        }
                        else
                        {
                            printf("Failed to read gpio pin number %d the json", i);
                        }
                    }
                }

                // get Interrupt pin
                pJsonStr = cJSON_GetObjectItem(key_gpio, JSON_Key_Interrupt_Pin);
                if (cJSON_IsString(pJsonStr))
                {
                    char *str = cJSON_GetStringValue(pJsonStr);
                    if ((str != NULL) && (strlen(str) <= sizeof(int8_t)))
                    {
                        gpio->interrupt_pin = *str - '0';
                    }
                    else
                    {
                        printf("Failed to read interrupt pin number from the json");
                    }
                }
            }
        }

        cJSON_Delete(p_cJson);
    }
    return An_SUCCESS;
}

int8_t AntarisApiGPIO::api_pa_pc_read_gpio(int8_t gpio_port, int8_t pin_number)
{
    int exit_status;
    long result;

    Py_Initialize();
    PyObject* sysPath = PySys_GetObject("path");

    if (sysPath == nullptr) {
        PyErr_Print();
    } else {
        PyObject* directoryPath = PyUnicode_DecodeFSDefault("/lib/antaris/tools/");
        if (directoryPath != nullptr)  {
            PyList_Append(sysPath, directoryPath);
            Py_XDECREF(directoryPath);
            printf("rahul 11 \n");
        } else  {
            PyErr_Print();
        }
    }

    PyObject *pName = PyUnicode_DecodeFSDefault("antaris_api_gpio");
    PyObject *pModule = PyImport_Import(pName);
    if (pModule != nullptr)
    {
        PyObject *pFunction = PyObject_GetAttrString(pModule, "api_read_gpio"); // Replace with your function name
        if (PyCallable_Check(pFunction)) {
            PyObject *pArgs = PyTuple_Pack(2, PyLong_FromLong((long) gpio_port), PyLong_FromLong((long) pin_number)); // Pass arguments
            PyObject *pValue = PyObject_CallObject(pFunction, pArgs);                  // Call the function
            result = PyLong_AsLong(pValue);                                       // Convert the result to a C++ type
            exit_status = (int) result;
            // Use 'result' as needed
            Py_XDECREF(pValue);
            Py_XDECREF(pArgs);
            Py_XDECREF(pFunction);
        } else {
            PyErr_Print();
        }
        Py_XDECREF(pModule);
    } else  {
        PyErr_Print();
    }

    Py_Finalize();

    // Process and use the Python script's result in your C++ code
    printf("Pin level : %d \n", exit_status);

    return exit_status;
}

AntarisReturnCode AntarisApiGPIO::api_pa_pc_write_gpio(int8_t gpio_port, int8_t pin_number, int8_t value)
{
    long pystatus = -1;

    Py_Initialize();
    PyObject* sysPath = PySys_GetObject("path");

    if (sysPath == nullptr) {
        PyErr_Print();
    } else {
        PyObject* directoryPath = PyUnicode_DecodeFSDefault("/lib/antaris/tools/");
        if (directoryPath != nullptr)  {
            PyList_Append(sysPath, directoryPath);
            Py_XDECREF(directoryPath);
            printf("rahul 11 \n");
        } else  {
            PyErr_Print();
        }
    }

    PyObject *pName = PyUnicode_DecodeFSDefault("antaris_api_gpio");
    PyObject *pModule = PyImport_Import(pName);
    if (pModule != nullptr)
    {
        printf("rahul 12 \n");
        PyObject *pFunction = PyObject_GetAttrString(pModule, "api_write_gpio"); // Replace with your function name
        if (PyCallable_Check(pFunction)) {
            PyObject *pArgs = PyTuple_Pack(3, PyLong_FromLong((long) gpio_port), PyLong_FromLong((long) pin_number), PyLong_FromLong((long) value)); // Pass arguments
            PyObject *pValue = PyObject_CallObject(pFunction, pArgs);                  // Call the function
            pystatus = PyLong_AsLong(pValue);                                       // Convert the result to a C++ type
            printf("rahul 13 %ld \n", pystatus);
            Py_XDECREF(pValue);
            Py_XDECREF(pArgs);
            Py_XDECREF(pFunction);
        } else {
            PyErr_Print();
        }
        Py_XDECREF(pModule);
    } else  {
        PyErr_Print();
    }
    Py_Finalize();

    if (pystatus == -1)
    {
        printf("Error \n");
        return An_GENERIC_FAILURE;
    }

    return An_SUCCESS;
}
