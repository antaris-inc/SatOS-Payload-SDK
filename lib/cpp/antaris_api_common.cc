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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "antaris_api.h"
#include "antaris_api_internal.h"

#include "antaris_api.grpc.pb.h"
#include "antaris_api.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::Status;

void app_to_peer_UINT16(void *ptr_src_app, void *ptr_dst_peer)
{
    *(INT32 *)ptr_dst_peer = *(UINT16 *)ptr_src_app;
}

void peer_to_app_UINT16(void *ptr_src_peer, void *ptr_dst_app)
{
    *(UINT16 *)ptr_dst_app = (UINT16)(*(INT32 *)ptr_src_peer);
}

void app_to_peer_UINT32(void *ptr_src_app, void *ptr_dst_peer)
{
    *(INT32 *)ptr_dst_peer = *(UINT32 *)ptr_src_app;
}

void peer_to_app_UINT32(void *ptr_src_peer, void *ptr_dst_app)
{
    *(UINT32 *)ptr_dst_app = (UINT32)(*(INT32 *)ptr_src_peer);
}

void app_to_peer_UINT64(void *ptr_src_app, void *ptr_dst_peer)
{
    *(INT64 *)ptr_dst_peer = *(UINT64 *)ptr_src_app;
}

void peer_to_app_UINT64(void *ptr_src_peer, void *ptr_dst_app)
{
    *(UINT64 *)ptr_dst_app = (UINT64)(*(INT64 *)ptr_src_peer);
}

void app_to_peer_INT32(void *ptr_src_app, void *ptr_dst_peer)
{
    *(INT32 *)ptr_dst_peer = *(INT32 *)ptr_src_app;
}

void peer_to_app_INT32(void *ptr_src_peer, void *ptr_dst_app)
{
    *(INT32 *)ptr_dst_app = *(INT32 *)ptr_src_peer;
}

void app_to_peer_FLOAT(void *ptr_src_app, void *ptr_dst_peer)
{
    *(FLOAT *)ptr_dst_peer = *(FLOAT *)ptr_src_app;
}

void peer_to_app_FLOAT(void *ptr_src_peer, void *ptr_dst_app)
{
    *(FLOAT *)ptr_dst_app = *(FLOAT *)ptr_src_peer;
}
