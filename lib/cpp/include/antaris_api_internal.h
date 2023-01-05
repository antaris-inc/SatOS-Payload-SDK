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

#ifndef __ANTARIS_API_INTERNAL_H__
#define __ANTARIS_API_INTERNAL_H__

#include "antaris_api.h"
#include "antaris_sdk_environment.h"

extern char g_LISTEN_IP[];
extern char g_PAYLOAD_CONTROLLER_IP[];
extern char g_PAYLOAD_APP_IP[];
extern unsigned short g_PC_GRPC_SERVER_PORT;
extern char g_PC_GRPC_SERVER_PORT_STR[];
extern unsigned short g_PA_GRPC_SERVER_PORT;
extern char g_PA_GRPC_SERVER_PORT_STR[];

extern char g_PC_GRPC_LISTEN_ENDPOINT[];
extern char g_PC_GRPC_CONNECT_ENDPOINT[];
extern char g_APP_GRPC_LISTEN_ENDPOINT[];
extern char g_APP_GRPC_CONNECT_ENDPOINT[];

#define REGISTRATION_AUTH_TOKEN         "good-token"

#endif
