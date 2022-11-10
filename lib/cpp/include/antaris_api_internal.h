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

#define STRINGIFY(_X) __INTERNAL_STRINGIFY(_X)
#define __INTERNAL_STRINGIFY(_X) #_X

#define LISTEN_IP                               "0.0.0.0"
#define PEER_IP                                 "127.0.0.1"
#define SERVER_GRPC_PORT                        50051
#define SERVER_GRPC_PORT_STR                    STRINGIFY(SERVER_GRPC_PORT)
#define APP_GRPC_CALLBACK_PORT                  50053
#define APP_GRPC_CALLBACK_PORT_STR              STRINGIFY(APP_GRPC_CALLBACK_PORT)

#define MAKE_ENDPOINT(_ip_, _port_) _ip_ ":" _port_

#define SERVER_GRPC_LISTEN_ENDPOINT            MAKE_ENDPOINT(LISTEN_IP, SERVER_GRPC_PORT_STR)
#define SERVER_GRPC_CONNECT_ENDPOINT           MAKE_ENDPOINT(PEER_IP, SERVER_GRPC_PORT_STR)
#define APP_CALLBACK_GRPC_LISTEN_ENDPOINT      MAKE_ENDPOINT(LISTEN_IP, APP_GRPC_CALLBACK_PORT_STR)
#define APP_CALLBACK_GRPC_CONNECT_ENDPOINT     MAKE_ENDPOINT(PEER_IP, APP_GRPC_CALLBACK_PORT_STR)

#define REGISTRATION_AUTH_TOKEN         "good-token"

#endif
