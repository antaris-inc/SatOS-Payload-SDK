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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cJSON.h"

#include "antaris_sdk_environment.h"

#define ENV_FILENAME_VARIABLE       "ANTARIS_ENV_CONF_FILE"


static char g_CONF_FILE[MAX_FILE_OR_PROP_LEN_NAME]="/opt/antaris/app/conf/sdk_env.conf";
static char g_CONF_JSON[MAX_FILE_OR_PROP_LEN_NAME]="/opt/antaris/app/config.json" ;

#define PC_IP_CONF_KEY                      "PAYLOAD_CONTROLLER_IP"
#define SSL_ENABLE_KEY                      "SSL_FLAG"
#define APP_IP_CONF_KEY                     "PAYLOAD_APP_IP"
#define LISTEN_IP_CONF_KEY                  "LISTEN_IP"
#define PC_API_PORT_CONF_KEY                "PC_API_PORT"
#define APP_API_PORT_CONF_KEY               "APP_API_PORT"
#define KEEPALIVE_ENABLE_KEY                "KEEPALIVE"

char g_LISTEN_IP[MAX_IP_OR_PORT_LENGTH] = "0.0.0.0";
char g_PAYLOAD_CONTROLLER_IP[MAX_IP_OR_PORT_LENGTH] = "127.0.0.1";
char g_PAYLOAD_APP_IP[MAX_IP_OR_PORT_LENGTH] = "127.0.0.1";
unsigned short g_PC_GRPC_SERVER_PORT = 50051;
char g_PC_GRPC_SERVER_PORT_STR[MAX_IP_OR_PORT_LENGTH] = "50051";
unsigned short g_PA_GRPC_SERVER_PORT = 50053;
char g_PA_GRPC_SERVER_PORT_STR[MAX_IP_OR_PORT_LENGTH] = "50053";
char g_SSL_ENABLE = '1';              // SSL is enabled by default
char g_KEEPALIVE_ENABLE = '1';        // TrueTwin is disabled by default

char g_PC_GRPC_LISTEN_ENDPOINT[2 * MAX_IP_OR_PORT_LENGTH] = "0.0.0.0:50051";
char g_PC_GRPC_CONNECT_ENDPOINT[2 * MAX_IP_OR_PORT_LENGTH] = "127.0.0.1:50051";
char g_APP_GRPC_LISTEN_ENDPOINT[2 * MAX_IP_OR_PORT_LENGTH] = "0.0.0.0:50053";
char g_APP_GRPC_CONNECT_ENDPOINT[2 * MAX_IP_OR_PORT_LENGTH] = "127.0.0.1:50053";

typedef struct _conf {
    char prop[MAX_IP_OR_PORT_LENGTH];
    char value[MAX_IP_OR_PORT_LENGTH];
} conf_t;

static void determine_conf_file(void)
{
    char *conf_file_name;

    if ((conf_file_name = getenv(ENV_FILENAME_VARIABLE)) != NULL) {
        strcpy(g_CONF_FILE, conf_file_name);
    }

    return;
}

static void parse_a_conf(char *conf_line, conf_t *out_conf)
{
    char *comment_token;
    char *new_line_token;
    char *equal_token;

    comment_token = strchr(conf_line, '#');

    if (comment_token) {
        // terminate at first comment
        *comment_token = '\0';
    }

    new_line_token = strchr(conf_line, '\n');

    if (new_line_token) {
        // drop new line, we also don't expect multi-lines here
        *new_line_token = '\0';
    }

    equal_token = strchr(conf_line, '=');

    if (!equal_token) {
        goto not_found;
    }

    strcpy(&out_conf->value[0], equal_token + 1);
    *equal_token = '\0';
    strcpy(&out_conf->prop[0], conf_line);

    return;

not_found:
    strcpy(&out_conf->prop[0], "");
    strcpy(&out_conf->value[0], "");

    return;
}

static void update_a_conf(char *conf_line)
{
    conf_t a_conf = {0};
    size_t length;

    parse_a_conf(conf_line, &a_conf);
    length = strnlen(a_conf.value, MAX_IP_OR_PORT_LENGTH);
    if(length == MAX_IP_OR_PORT_LENGTH) {
        printf("Property value is greater than %d characters is not supported. Going with default value \n", MAX_IP_OR_PORT_LENGTH);
        return;
    }

    if (strcmp(a_conf.prop, PC_IP_CONF_KEY) == 0) {
        strcpy(g_PAYLOAD_CONTROLLER_IP, a_conf.value);
    } else if (strcmp(a_conf.prop, APP_IP_CONF_KEY) == 0) {
        strcpy(g_PAYLOAD_APP_IP, a_conf.value);
    } else if (strcmp(a_conf.prop, LISTEN_IP_CONF_KEY) == 0) {
        strcpy(g_LISTEN_IP, a_conf.value);
    } else if (strcmp(a_conf.prop, PC_API_PORT_CONF_KEY) == 0) {
        strcpy(g_PC_GRPC_SERVER_PORT_STR, a_conf.value);
    } else if (strcmp(a_conf.prop, APP_API_PORT_CONF_KEY) == 0) {
        strcpy(g_PA_GRPC_SERVER_PORT_STR, a_conf.value);
    } else if (strcmp(a_conf.prop, SSL_ENABLE_KEY) == 0) {
        g_SSL_ENABLE = a_conf.value[0];
    } else if (strcmp(a_conf.prop, KEEPALIVE_ENABLE_KEY) == 0) {
        g_KEEPALIVE_ENABLE = a_conf.value[0];
    }

    return;
}

static void refresh_config_vars(void)
{
    /* now populate all the derived globals */
    g_PC_GRPC_SERVER_PORT = (unsigned short)atoi(g_PC_GRPC_SERVER_PORT_STR);
    g_PA_GRPC_SERVER_PORT = (unsigned short)atoi(g_PA_GRPC_SERVER_PORT_STR);
    sprintf(g_PC_GRPC_LISTEN_ENDPOINT, "%s:%s", g_LISTEN_IP, g_PC_GRPC_SERVER_PORT_STR);
    sprintf(g_PC_GRPC_CONNECT_ENDPOINT, "%s:%s", g_PAYLOAD_CONTROLLER_IP, g_PC_GRPC_SERVER_PORT_STR);
    sprintf(g_APP_GRPC_LISTEN_ENDPOINT, "%s:%s", g_LISTEN_IP, g_PA_GRPC_SERVER_PORT_STR);
    sprintf(g_APP_GRPC_CONNECT_ENDPOINT, "%s:%s", g_PAYLOAD_APP_IP, g_PA_GRPC_SERVER_PORT_STR);
    
    return;
}

void sdk_environment_read_config(void)
{
    FILE *conf_file;
    char line[MAX_FILE_OR_PROP_LEN_NAME];
    char *fgets_ret;

    determine_conf_file();

    if ((conf_file = fopen(g_CONF_FILE, "r")) != NULL) {

        while ((fgets_ret = fgets(line, sizeof(line), conf_file)) != NULL) {
            update_a_conf(line);
        }

        fclose(conf_file);
    }

    refresh_config_vars();

    return;
}


#define BUFF_SIZE       (4096)   // Max data size from json file

void read_config_json( cJSON ** pp_cJson)
{
    FILE *conf_file;
    long size = 0 , size_read = 0;
    char buffer[BUFF_SIZE] = {0};
    *pp_cJson = NULL;       /* Original value discarded. */
    if ((conf_file = fopen(g_CONF_JSON, "r")) != NULL) {
        fseek(conf_file , 0 , SEEK_END);
        long size = ftell(conf_file);
        fseek(conf_file , 0 , SEEK_SET);
        if ( size > (BUFF_SIZE-1) ){
            return;
        }
        size_read = fread(buffer , sizeof(char) , size , conf_file);
        fclose(conf_file);
        if (size_read != size ){
            /* All data was not read.*/
            return;
        }
    };
    *pp_cJson = cJSON_Parse(buffer);
};

