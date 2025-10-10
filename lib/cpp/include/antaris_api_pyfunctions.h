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

// This file contains API's for python functions

#ifndef __ANTARIS_API_PYTHON_CALLS_H__
#define __ANTARIS_API_PYTHON_CALLS_H__

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

#define PYTHON_SCRIPT_FILE         "antaris_file_download"

#define AZURE_STRING                   "FileEndpoint="
#define PYTHON_AZURE_STAGEFILE_MODULE  "azure_file_upload"

#define GCS_STRING                     "upload-file-to-bucket"
#define PYTHON_GCS_STAGEFILE_MODULE    "gcp_file_upload"

#define TRUE        1
#define FALSE       0

class AntarisApiPyFunctions {
    public:
        AntarisReturnCode api_pa_pc_staged_file(cJSON* p_cJson, ReqStageFileDownloadParams *download_file_params);

    private:
};

#endif // __ANTARIS_API_PYTHON_CALLS_H__
