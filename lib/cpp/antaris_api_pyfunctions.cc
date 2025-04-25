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

#define JSON_Key_FTM            ("FTM")
#define JSON_Key_File_Conn      ("File_Conn_Str")
#define JSON_Key_TrueTwin_Dir   ("Truetwin_Dir")
#define JSON_Key_Share_Name     ("Share_Name")

#define FILE_DOWNLOAD_DIR    "/opt/antaris/outbound/"

AntarisReturnCode AntarisApiPyFunctions::api_pa_pc_staged_file(cJSON *p_cJson, ReqStageFileDownloadParams *download_file_params)
{
    AntarisReturnCode exit_status = An_GENERIC_FAILURE;
    cJSON *key_ftm = NULL;
    cJSON *pJsonStr = NULL;

    PyObject *pArgs = NULL;
    PyObject *pName = NULL;
    PyObject *pModule = NULL;
    PyObject *pFunction = NULL;
    PyObject *pResult = NULL;

    char *g_File_String = NULL;
    char *g_Truetwin_Dir = NULL;
    char *g_Share_Name = NULL;
    char dst_file_name[MAX_FILE_OR_PROP_LEN_NAME] = {'\0'};
    char *position = NULL;
    size_t remaining_length = 0;

    read_config_json(&p_cJson);
    if (p_cJson == NULL)  {
        printf("Error: Failed to read the config.json\n");
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    key_ftm = cJSON_GetObjectItemCaseSensitive(p_cJson, JSON_Key_FTM);
    if (key_ftm == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_FTM);
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }
    
    // get File_Conn_Str
    pJsonStr = cJSON_GetObjectItem(key_ftm, JSON_Key_File_Conn);
    if (pJsonStr == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_File_Conn);
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    if (cJSON_IsString(pJsonStr) == cJSON_Invalid) {
        printf("Error: %s value is not a string \n", JSON_Key_File_Conn);
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    g_File_String = cJSON_GetStringValue(pJsonStr);
    if (g_File_String == NULL) {
        printf("Error: %s string not present \n", JSON_Key_File_Conn);
    }

    // get Truetwin_Dir
    pJsonStr = cJSON_GetObjectItem(key_ftm, JSON_Key_TrueTwin_Dir);
    if (pJsonStr == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_TrueTwin_Dir);
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    if (cJSON_IsString(pJsonStr) == cJSON_Invalid) {
        printf("Error: %s value is not a string \n", JSON_Key_TrueTwin_Dir);
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    g_Truetwin_Dir = cJSON_GetStringValue(pJsonStr);
    if (g_Truetwin_Dir == NULL) {
        printf("Error: %s string not present \n", JSON_Key_TrueTwin_Dir);
    }

    // get  Share_Name
    pJsonStr = cJSON_GetObjectItem(key_ftm, JSON_Key_Share_Name);
    if (pJsonStr == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_Share_Name);
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    if (cJSON_IsString(pJsonStr) == cJSON_Invalid) {
        printf("Error: %s value is not a string \n", JSON_Key_Share_Name);
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    g_Share_Name = cJSON_GetStringValue(pJsonStr);
    if (g_Share_Name == NULL) {
        printf("Error: %s string not present \n", JSON_Key_Share_Name);
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    // Creating destination file path
    position = strstr(download_file_params->file_path, FILE_DOWNLOAD_DIR);
    if ( position == NULL ) {
        printf("Error: source file should be at %s \n", FILE_DOWNLOAD_DIR);
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    position += strlen(FILE_DOWNLOAD_DIR);
    position--;
    remaining_length = strlen(position);
    strcpy(dst_file_name, g_Truetwin_Dir);
    strncat(dst_file_name, position, remaining_length);

    pArgs = PyTuple_New(4);  // Create a tuple with 4 elements

    PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", download_file_params->file_path));  // 's' for string
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", g_File_String));
    PyTuple_SetItem(pArgs, 2, Py_BuildValue("s", g_Share_Name));
    PyTuple_SetItem(pArgs, 3, Py_BuildValue("s", dst_file_name));

    printf("Antaris_SatOS : Uploading file %s \n", download_file_params->file_path);
    printf("Antaris_SatOS : file string %s \n", g_File_String);
    printf("Antaris_SatOS : Share name %s \n", g_Share_Name);
    printf("Antaris_SatOS : dst file name %s \n", dst_file_name);

    pName = PyUnicode_DecodeFSDefault(PYTHON_SCRIPT_FILE);
    pModule = PyImport_Import(pName);
    if (pModule == nullptr)   {
        PyErr_Print();
        printf("Error: Import failed, Can not upload file %s \n", download_file_params->file_path);
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    if (strstr(download_file_params->file_path, AZURE_STRING) != NULL) {
        pFunction = PyObject_GetAttrString(pModule, PYTHON_AZURE_STAGEFILE_MODULE);
        if (pFunction == NULL) {
            PyErr_Print();
            printf("Error: module failed, Can not upload file %s \n", download_file_params->file_path);
            exit_status = An_GENERIC_FAILURE;
            goto cleanup_and_exit;
        }
    } else if (strstr(download_file_params->file_path, GCS_STRING) != NULL) {
        pFunction = PyObject_GetAttrString(pModule, PYTHON_GCS_STAGEFILE_MODULE);
        if (pFunction == NULL) {
            PyErr_Print();
            printf("Error: module failed, Can not upload file %s \n", download_file_params->file_path);
            exit_status = An_GENERIC_FAILURE;
            goto cleanup_and_exit;
        }
    } else {
        printf("Unsupported connection string format");
        goto cleanup_and_exit;
    }

    pResult = PyObject_CallObject(pFunction, pArgs);
    if (pResult != NULL) {
        // Don't forget to decref the result
        long int ret = PyLong_AsLong(pResult);
        if (ret == TRUE) {
            exit_status = An_SUCCESS;
        }
    } else {
        PyErr_Print();
        printf("Error: Can not upload file %s \n", download_file_params->file_path);
        exit_status = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

cleanup_and_exit:
    // Dereference python objects
    Py_XDECREF(pArgs);
    Py_XDECREF(pName);
    Py_XDECREF(pModule);
    Py_XDECREF(pFunction);
    Py_XDECREF(pResult);

    // Process and use the Python script's result in your C++ code
    printf("File upload status : %d \n", exit_status);

    return exit_status;
}
