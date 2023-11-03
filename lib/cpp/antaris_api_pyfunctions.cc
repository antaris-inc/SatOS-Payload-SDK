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

#include "antaris_api.h"
#include "antaris_sdk_environment.h"
#include "antaris_api_pyfunctions.h"

AntarisReturnCode AntarisApiPyFunctions::api_pa_pc_staged_file(cJSON *p_cJson, ReqStageFileDownloadParams *download_file_params)
{
    size_t filename_len = 0;
    AntarisReturnCode ret = An_SUCCESS;
    long result;
    int exit_status = An_GENERIC_FAILURE;
    PyObject *pName = NULL;
    PyObject *pModule = NULL;
    PyObject *pFunction = NULL;
    PyObject *pArgs = NULL;
    PyObject *pValue = NULL;
    PyObject *next = NULL;
    PyObject *prev = NULL;
    PyObject *child = NULL;
    PyObject *valuestring = NULL;
    PyObject *string = NULL;

    if (p_cJson->next != NULL)
    {
        next = PyLong_FromVoidPtr(p_cJson->next);
    }
    if (p_cJson->prev != NULL)
    {
        prev = PyLong_FromVoidPtr(p_cJson->prev);
    }

    if (p_cJson->child != NULL)
    {
        child = PyLong_FromVoidPtr(p_cJson->child);
    }
    PyObject *type = PyLong_FromLong(p_cJson->type);
    if (p_cJson->valuestring != NULL)
    {
        valuestring = PyUnicode_DecodeUTF8(p_cJson->valuestring, strlen(p_cJson->valuestring), NULL);
    }
    PyObject *valueint = PyLong_FromLong(p_cJson->valueint);
    PyObject *valuedouble = PyFloat_FromDouble(p_cJson->valuedouble);
    if (p_cJson->string != NULL)
    {
        string = PyUnicode_DecodeUTF8(p_cJson->string, strlen(p_cJson->string), NULL);
    }
    filename_len = strnlen(download_file_params->file_path, MAX_FILE_OR_PROP_LEN_NAME);

    if ((download_file_params->file_path == NULL) || (filename_len > MAX_FILE_OR_PROP_LEN_NAME))
    {
        printf("Error: Filename greater than %d \n", MAX_FILE_OR_PROP_LEN_NAME);
        return An_GENERIC_FAILURE;
    }

    printf("Antaris_SatOS : Uploading file %s \n", download_file_params->file_path);

    pName = PyUnicode_DecodeFSDefault(PYTHON_SCRIPT_FILE);
    pModule = PyImport_Import(pName);

    if (pModule == nullptr)
    {
        printf("Error: Can not upload file %s \n", download_file_params->file_path);
        return An_GENERIC_FAILURE;
    }

    // Create a Python tuple and pack the structure and character array into it
    pArgs = PyTuple_Pack(10, Py_BuildValue("i", download_file_params->correlation_id), Py_BuildValue("s", download_file_params->file_path), next, prev, child, type, valuestring, valueint, valuedouble, string); // Pass arguments

    // Create an instance of the File_Stage class
    PyObject* pInstance = PyObject_CallObject(pModule, pArgs);

    if (pInstance != NULL) {
        pFunction = PyObject_GetAttrString(pInstance, PYTHON_STAGEFILE_MODULE);

        if (pFunction != NULL) {
            // Call the method
            PyObject* pResult = PyObject_CallObject(pFunction, NULL);
            if (pResult != NULL) {
                // Handle the result as needed
                // Note: You should check for exceptions here.

                // Don't forget to decref the result
                Py_DECREF(pResult);
                printf("Success success \n");
            } else {
                PyErr_Print();
                printf("Error:2 Can not upload file %s \n", download_file_params->file_path);
                return An_GENERIC_FAILURE;
            }
        }
    }

/*    if (PyCallable_Check(pFunction) == FALSE)
    {
        printf("Error:2 Can not upload file %s \n", download_file_params->file_path);
        return An_GENERIC_FAILURE;
    }

    pValue = PyObject_CallObject(pFunction, pArgs); // Call the function
    result = PyLong_AsLong(pValue);                 // Convert the result to a C++ type
    exit_status = (int)result;
*/
    // Dereference python objects
    Py_XDECREF(pValue);
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunction);
    Py_XDECREF(pModule);

    // Process and use the Python script's result in your C++ code
    printf("File upload status : %d \n", exit_status);

    return ret;
}
