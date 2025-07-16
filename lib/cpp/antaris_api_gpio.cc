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
#include <dlfcn.h>

#include "Python.h"

#include "antaris_api_gpio.h"
#include "antaris_api.h"
#include "antaris_api_internal.h"
#include "antaris_sdk_environment.h"
#include "antaris_api_parser.h"

#define GENERIC_ERROR -1
#define TIMEOUT_PYFINALIZE 5

typedef int (*read_pin_t)(int port, int pin_number);
typedef int (*write_pin_t)(int port, int pin_number, int value);
typedef int (*init_qa7_lib_t)(void);
typedef int (*deinit_qa7_lib_t)(void);

extern char qa7_lib[32];
char gpio_adapter_type[32] = {0};

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
    AntarisApiParser api_parser;

    api_parser.api_pa_pc_get_gpio_info(&gpio_info);

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
    AntarisApiParser api_parser;

    if (An_GENERIC_FAILURE == verify_gpio_pin(pin_number) ) {
        printf("Error: Wrong configuration for GPIO pin %d \n", pin_number);
        return An_GENERIC_FAILURE;
    }

    if ((strncmp(gpio_adapter_type, "QA7", 2) == 0)) {
        int8_t value = 0;
        value = read_qa7_pin(gpio_port, pin_number);
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

    AntarisApiParser api_parser;

    if (An_GENERIC_FAILURE == verify_gpio_pin(pin_number) ) {
        printf("Error: Wrong configuration for GPIO pin %d \n", pin_number);
        return An_GENERIC_FAILURE;
    }

    if ((strncmp(gpio_adapter_type, "QA7", 2) == 0)) {
        int8_t op = FALSE;
        op = write_qa7_pin(gpio_port, pin_number, value);
        if (op == FALSE) {
            return An_GENERIC_FAILURE;
        } 
        return An_SUCCESS;
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
    AntarisApiParser api_parser;
    AntarisReturnCode ret;

    ret = api_parser.api_pa_pc_get_gpio_adapter_type(gpio_adapter_type);
    
    if (ret != An_SUCCESS) {
        printf("Error: json file is not configured properly. Kindly check configurations done in ACP \n");
        return An_GENERIC_FAILURE;
    }
    printf("Adapter type = %s \n", gpio_adapter_type);

    if ((strncmp(gpio_adapter_type, "QA7", 2) == 0)) {
        void *handle;
        init_qa7_lib_t init_func;
        if (qa7_lib[0] == 0) {
            if (api_parser.api_pa_pc_get_qa7_lib() != An_SUCCESS) {
                printf("Error in fetching qA7 lib \n");
                return An_GENERIC_FAILURE;
            }
        }
        printf("qa7 lib %s \n", qa7_lib);
        
        // Load the shared library
        handle = dlopen(qa7_lib, RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "dlopen failed: %s\n", dlerror());
            return An_GENERIC_FAILURE;
        }
    
        // Clear any previous error
        dlerror();
    
        // Lookup the symbol
        *(void **) (&init_func) = dlsym(handle, QA7_INIT_FUNCTION);
        char *error = dlerror();
        if (error) {
            fprintf(stderr, "dlsym failed: %s\n", error);
            dlclose(handle);
            return An_GENERIC_FAILURE;
        }
    
        // Call the function
        int status = init_func();
    
        // Cleanup
        dlclose(handle);
    }
    return An_SUCCESS;
}

AntarisReturnCode AntarisApiGPIO::api_pa_pc_deinit_gpio_lib()
{
    AntarisApiParser api_parser;

    if ((strncmp(gpio_adapter_type, "QA7", 2) == 0)) {
        void *handle;
        deinit_qa7_lib_t deinit_func;
    
        // Load the shared library
        handle = dlopen(qa7_lib, RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "dlopen failed: %s\n", dlerror());
            return An_GENERIC_FAILURE;
        }
    
        // Clear any previous error
        dlerror();
    
        // Lookup the symbol
        *(void **) (&deinit_func) = dlsym(handle, QA7_DEINIT_FUNCTION);
        char *error = dlerror();
        if (error) {
            fprintf(stderr, "dlsym failed: %s\n", error);
            dlclose(handle);
            return An_GENERIC_FAILURE;
        }
    
        // Call the function
        int status = deinit_func();
    
        // Cleanup
        dlclose(handle);
        
        memset(qa7_lib, 0, sizeof(qa7_lib));
    }
    memset(gpio_adapter_type, 0, sizeof(gpio_adapter_type));

    return An_SUCCESS;
}

int8_t AntarisApiGPIO::read_qa7_pin(int8_t gpio_port, int8_t pin_number)
{
    void *handle;
    read_pin_t read_pin_func;
    int8_t value;

    // Load the shared library
    handle = dlopen(qa7_lib, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }

    // Clear any existing errors
    dlerror();

    // Get the symbol
    *(void **) (&read_pin_func) = dlsym(handle, QA7_READ_PIN_FUNCTION);
    char *error = dlerror();
    if (error) {
        fprintf(stderr, "dlsym failed: %s\n", error);
        dlclose(handle);
        return 1;
    }

    value = (int8_t) read_pin_func(gpio_port, pin_number);

    dlclose(handle);
    return value;
}

int8_t AntarisApiGPIO::write_qa7_pin(int8_t gpio_port, int8_t pin_number, int8_t value)
{
    void *handle;
    write_pin_t write_pin_func;
    int8_t result;

    // Load the shared library
    handle = dlopen(qa7_lib, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }

    // Clear any previous error
    dlerror();

    // Lookup symbol
    *(void **) (&write_pin_func) = dlsym(handle, QA7_WRITE_PIN_FUNCTION);
    char *error = dlerror();
    if (error) {
        fprintf(stderr, "dlsym failed: %s\n", error);
        dlclose(handle);
        return 1;
    }

    result = (int8_t) write_pin_func(gpio_port, pin_number, value);

    // Done
    dlclose(handle);
    return result;
}
