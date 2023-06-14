#
#   Copyright 2022 Antaris, Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import pdb
import socket

from satos_payload_sdk import antaris_sdk_environment as environment

g_LISTEN_IP=None
g_PAYLOAD_CONTROLLER_IP=None
g_PAYLOAD_APP_IP=None
g_PC_GRPC_SERVER_PORT=None
g_PC_GRPC_SERVER_PORT_STR=None
g_PA_GRPC_SERVER_PORT=None
g_PA_GRPC_SERVER_PORT_STR =None
g_SSL_ENABLE=None
g_TRUETWIN_ENABLE=None

def init_vars():
    global g_LISTEN_IP
    global g_PAYLOAD_CONTROLLER_IP
    global g_PC_GRPC_SERVER_PORT
    global g_PC_GRPC_SERVER_PORT_STR
    global g_PA_GRPC_SERVER_PORT
    global g_PA_GRPC_SERVER_PORT_STR
    global g_SSL_ENABLE
    global g_TRUETWIN_ENABLE

    g_LISTEN_IP=environment.get_conf(environment.g_LISTEN_IP_CONF_KEY)
    g_PAYLOAD_CONTROLLER_IP=environment.get_conf(environment.g_PC_IP_CONF_KEY)
    g_PAYLOAD_APP_IP=environment.get_conf(environment.g_APP_IP_CONF_KEY)
    g_SSL_ENABLE=environment.get_conf(environment.g_SSL_ENABLE_KEY)
    g_TRUETWIN_ENABLE=environment.get_conf(environment.g_TRUETWIN_MODE_KEY)
    g_PC_GRPC_SERVER_PORT=environment.get_conf(environment.g_PC_API_PORT_CONF_KEY)
    g_PA_GRPC_SERVER_PORT=environment.get_conf(environment.g_APP_API_PORT_CONF_KEY)
    g_PC_GRPC_SERVER_PORT_STR="{}".format(g_PC_GRPC_SERVER_PORT)
    g_PA_GRPC_SERVER_PORT_STR ="{}".format(g_PA_GRPC_SERVER_PORT)

#pdb.set_trace()
init_vars()

def is_server_endpoint_available(ip, port):
    temp_socket = socket.socket()

    try:
        temp_socket.bind((ip, port))
    except socket.error as e:
        print(e)
        return False

    temp_socket.close()
    return True
