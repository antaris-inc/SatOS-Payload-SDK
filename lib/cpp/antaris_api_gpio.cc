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
#include <chrono>
#include <thread>
#include <future>

#include "Python.h"

#include "antaris_api_gpio.h"
#include "antaris_api.h"
#include "antaris_api_internal.h"
#include "antaris_sdk_environment.h"
#include "an_pa_pc_qa5.h"

#define GENERIC_ERROR -1
#define TIMEOUT_PYFINALIZE 5

AntarisReturnCode init_satos_lib()
{
    Py_Initialize();
    PyObject* sysPath = PySys_GetObject("path");

    if (sysPath == nullptr) {
        PyErr_Print();
        printf("Error: Can not initialize SatOS library \n");
        return An_GENERIC_FAILURE;
    } 
    
    PyObject* directoryPath = PyUnicode_DecodeFSDefault("/lib/antaris/tools/");
    if (directoryPath == nullptr)  {
        PyErr_Print();
        printf("Error: Can not initialize SatOS library \n");
        return An_GENERIC_FAILURE;
    }

    PyList_Append(sysPath, directoryPath);
    Py_XDECREF(directoryPath);

    printf("Completed initialization of SatOS \n");
    return An_SUCCESS;
}

void deinit_satos_lib()
{
    Py_Finalize(); // Finalize the Python interpreter
}


void with_timeout_deinit_satos_lib()
{
    // Run Py_Finalize() in a separate thread
    std::chrono::milliseconds timeout(TIMEOUT_PYFINALIZE*1000);
    std::future<void> finalize_result = std::async(std::launch::async, deinit_satos_lib);

    // Wait for the result with a timeout
    if (finalize_result.wait_for(timeout) == std::future_status::timeout) {
        // Timeout occurred, Py_Finalize() did not complete in time
        printf("After Py_Finalize() closed forcefully\n");
        _exit(0);
    } else {
        printf("Py_Finalize() completed within the timeout");
        return;
    }
}

AntarisReturnCode AntarisApiGPIO::verify_gpio_pin(int8_t pin_number) 
{
    gpio_s gpio_info;
    int i = 0;

    api_pa_pc_get_gpio_info(&gpio_info);

    while (i < gpio_info.pin_count) {
        if (gpio_info.pins[i] == pin_number) {
            return An_SUCCESS;
        }
        i += 1;
    }

    return An_GENERIC_FAILURE; 
}
int8_t AntarisApiGPIO::api_pa_pc_read_gpio(int8_t gpio_port, int8_t pin_number)
{
    int exit_status = An_GENERIC_FAILURE;
    long result;
    PyObject *pName = NULL;
    PyObject *pModule = NULL;
    PyObject *pFunction = NULL;
    PyObject *pArgs = NULL;
    PyObject *pValue = NULL;
    char adapter_type[32] = {0};
    AntarisApiParser api_parser;

    if (An_GENERIC_FAILURE == verify_gpio_pin(pin_number) ) {
        printf("Error: Wrong configuration for GPIO pin %d \n", pin_number);
        return An_GENERIC_FAILURE;
    }

    api_parser.api_pa_pc_get_gpio_adapter_type(adapter_type);

    if (strcmp(adapter_type, "QA7") == 0) {
        int value = 0;
        read_pin(gpio_port, pin_number);
        return value;
    }
    pName = PyUnicode_DecodeFSDefault(PYTHON_GPIO_MODULE);
    pModule = PyImport_Import(pName);

    if (pModule == nullptr)  {
        PyErr_Print();
        printf("Error: Module import error. Can not read GPIO pin %d \n", pin_number);
        return An_GENERIC_FAILURE;
    }
    
    pFunction = PyObject_GetAttrString(pModule, PYTHON_GPIO_READ_FUNCTION); 
       
    if (PyCallable_Check(pFunction) == FALSE) {
        PyErr_Print();
        printf("Error: Python function not found. Can not read GPIO pin %d \n", pin_number);
        return An_GENERIC_FAILURE;
    }
    
    pArgs = PyTuple_Pack(2, PyLong_FromLong((long) gpio_port), PyLong_FromLong((long) pin_number)); // Pass arguments
    pValue = PyObject_CallObject(pFunction, pArgs);                  // Call the function
    result = PyLong_AsLong(pValue);                                       // Convert the result to a C++ type
    exit_status = (int) result;
          
    // Dereference python objects
    Py_XDECREF(pValue);
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunction);
    Py_XDECREF(pModule);
    
    // Process and use the Python script's result in your C++ code
    printf("Pin level : %d \n", exit_status);

    return exit_status;
}

AntarisReturnCode AntarisApiGPIO::api_pa_pc_write_gpio(int8_t gpio_port, int8_t pin_number, int8_t value)
{
    long pystatus = An_GENERIC_FAILURE;
    PyObject *pName = NULL;
    PyObject *pModule = NULL;
    PyObject *pFunction = NULL;
    PyObject *pArgs = NULL;
    PyObject *pValue = NULL;

    char adapter_type[32] = {0};
    AntarisApiParser api_parser;

    if (An_GENERIC_FAILURE == verify_gpio_pin(pin_number) ) {
        printf("Error: Wrong configuration for GPIO pin %d \n", pin_number);
        return An_GENERIC_FAILURE;
    }

    api_parser.api_pa_pc_get_gpio_adapter_type(adapter_type);

    if (strcmp(adapter_type, "QA7") == 0) {
        write_pin(gpio_port, pin_number, value);
        return value;
    }

    pName = PyUnicode_DecodeFSDefault(PYTHON_GPIO_MODULE);
    pModule = PyImport_Import(pName);
    if (pModule == nullptr) {
        PyErr_Print();
        printf("Error: Module not found. Can not write GPIO pin %d \n", pin_number);
        return An_GENERIC_FAILURE;
    }
    
    pFunction = PyObject_GetAttrString(pModule, PYTHON_GPIO_WRITE_FUNCTION);
       
    if (PyCallable_Check(pFunction) == FALSE) {
        PyErr_Print();
        printf("Error: Function not found. Can not write GPIO pin %d \n", pin_number);
        return An_GENERIC_FAILURE;
    }
    
    pArgs = PyTuple_Pack(3, PyLong_FromLong((long) gpio_port), PyLong_FromLong((long) pin_number), PyLong_FromLong((long) value));                            // Pass arguments
    pValue = PyObject_CallObject(pFunction, pArgs);            // Call the function
    pystatus = PyLong_AsLong(pValue);                          // Convert the result to a C++ type

    // Dereference all objects
    Py_XDECREF(pValue);
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunction);
    Py_XDECREF(pModule);

    if (pystatus == An_GENERIC_FAILURE) {
        printf("Error: Can not write GPIO pin %d \n", pin_number);
        return An_GENERIC_FAILURE;
    }

    return An_SUCCESS;
}

AntarisReturnCode AntarisApiGPIO::api_pa_pc_init_gpio_lib()
{
    char adapter_type[32] = {0};
    AntarisApiParser api_parser;

    api_parser.api_pa_pc_get_gpio_adapter_type(adapter_type);

    if (strcmp(adapter_type, "QA7") == 0) {
        return init_qa7_lib();
    }
    return An_SUCCESS;
}

AntarisReturnCode AntarisApiGPIO::api_pa_pc_deinit_gpio_lib()
{
    char adapter_type[32] = {0};
    AntarisApiParser api_parser;

    api_parser.api_pa_pc_get_gpio_adapter_type(adapter_type);

    if (strcmp(adapter_type, "QA7") == 0) {
        return deinit_qa7_lib();
    }
    return An_SUCCESS;
}