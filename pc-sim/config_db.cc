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
#include <stdio.h>

#include "config_db.h"

#define MAX_NEXT_STEPS      16

/* Individual response/sequence */
typedef struct {
    int api_id; /* what api to invoke next */
    char data[SEQ_CONF_MAX_DATA_LEN]; /* parameters for the api, parsed in an api-specific way */
} user_next_step_t;

#define CONF_TRIGGER_SEPARATOR  '='
#define CONF_DATA_SEPARATOR     ':'
#define CONF_START_STRING       "--"

/* DB-entry of response/sequence configuration */
typedef struct {
    unsigned int        count; /* producer count */
    unsigned int        next_conf; /* consumer count */
    user_next_step_t    next_step_array[MAX_NEXT_STEPS]; /* action array */
} user_trigger_conf_t;

/* DB of all user-confiured actions (=response/sequence) indexed by trigger */
user_trigger_conf_t g_user_trigger_conf[SCENARIO_API_ID_COUNT] = {{0}};

static void scenario_save_conf(char *conf);
static int scenario_get_api_id_from_name(char *api_name);

/* parse interesting parts of command line and save in DB */
void scenario_register_conf(int argc, char *argv[])
{
    int arg_index = 1;

    while (arg_index < argc) {
        printf("%s [%d]: Got arg %s\n", __FUNCTION__, arg_index, argv[arg_index]);
        scenario_save_conf(argv[arg_index]);
        arg_index++;
    }

    return;
}

/*
 * Given a trigger, find out if there is any user-configured action for it.
 * 'consume_conf' parameter indicates whether this call should behave like a 'peek'
 * or consume the returned configuration.
 */
int scenario_get_conf(int api_trigger, int consume_conf, int *out_api_id, char *data)
{
    user_trigger_conf_t *p_trigger;

    if (api_trigger < 0 || api_trigger >= SCENARIO_API_ID_COUNT) {
        return SEQ_CONF_NOT_FOUND;
    }

    p_trigger = &g_user_trigger_conf[api_trigger];

    if (p_trigger->next_conf < p_trigger->count) {
        user_next_step_t *p_step = &(p_trigger->next_step_array[p_trigger->next_conf]);
        
        if (out_api_id) {
            *out_api_id = p_step->api_id;
        }

        if (data) {
            strcpy(data, p_step->data);
        }

        if (consume_conf) {
            p_trigger->next_conf++;
        }

        return SEQ_CONF_FOUND;
    }

    return SEQ_CONF_NOT_FOUND;
}

/* parse a configuration to save it in an organized manner in the DB */
static void scenario_save_conf(char *conf)
{
    char *trigger_loc;
    char *api_loc;
    char *data_loc;
    unsigned int trigger_len = 0;
    unsigned int api_len = 0;
    unsigned int data_len = 0;
    char trigger[SEQ_CONF_MAX_DATA_LEN] = {0};
    char api[SEQ_CONF_MAX_DATA_LEN] = {0};
    char data[SEQ_CONF_MAX_DATA_LEN] = {0};
    int trigger_id;
    int api_id;
    user_next_step_t *p_step;

    if (conf != strstr(conf, CONF_START_STRING)) {
        fprintf(stderr, "%s: %s does not have CONF-START %s, skipping\n", __FUNCTION__, conf, CONF_START_STRING);
        goto done;
    }

    trigger_loc = &conf[strlen(CONF_START_STRING)];

    api_loc = strchr(trigger_loc, CONF_TRIGGER_SEPARATOR);

    if (NULL == api_loc) {
        fprintf(stderr, "%s: %s does not have TRIGGER SEPARATOR %s, skipping\n", __FUNCTION__, conf, CONF_TRIGGER_SEPARATOR);
        goto done;
    }

    trigger_len = api_loc - trigger_loc;

    if (trigger_len >= SEQ_CONF_MAX_DATA_LEN) {
        fprintf(stderr, "%s: %s trigger-len %u too high, skipping\n", __FUNCTION__, conf, trigger_len);
        goto done;
    }

    api_loc++; /* point past the = token */

    data_loc = strchr(api_loc, CONF_DATA_SEPARATOR);

    if (NULL == data_loc) {
        fprintf(stderr, "%s: %s does not have DATA SEPARATOR %s, skipping\n", __FUNCTION__, conf, CONF_DATA_SEPARATOR);
        api_len = strlen(api_loc);
    } else {
        api_len = data_loc - api_loc;
        data_loc++; /* point past the : token */
        data_len = strlen(data_loc);

        if (data_len >= SEQ_CONF_MAX_DATA_LEN) {
            fprintf(stderr, "%s: %s data-len %u too high, skipping\n", __FUNCTION__, conf, data_len);
            goto done;      
        }

        strncpy(data, data_loc, data_len);
    }

    if (api_len >= SEQ_CONF_MAX_DATA_LEN) {
        fprintf(stderr, "%s: %s api-len %u too high, skipping\n", __FUNCTION__, conf, api_len);
        goto done;
    }

    strncpy(trigger, trigger_loc, trigger_len);
    strncpy(api, api_loc, api_len);

    trigger_id = scenario_get_api_id_from_name(trigger);
    api_id = scenario_get_api_id_from_name(api);

    if (SCENARIO_API_ID_INVALID == trigger_id || SCENARIO_API_ID_INVALID == api_id) {
        fprintf(stderr, "%s: %s bad trigger or api, skipping\n", __FUNCTION__, conf);
        goto done;
    }

    if (g_user_trigger_conf[trigger_id].count >= MAX_NEXT_STEPS) {
        fprintf(stderr, "%s: %s bad trigger or api, skipping\n", __FUNCTION__, conf);
        goto done;
    }

    /* ready to save */
    p_step = &(g_user_trigger_conf[trigger_id].next_step_array[g_user_trigger_conf[trigger_id].count]);
    g_user_trigger_conf[trigger_id].count++;

    p_step->api_id = api_id;
    
    if (data_loc) {
        strcpy(p_step->data, data);
    }

    printf("%s: conf %s, saved {api %d, data %s} for trigger %d\n", __FUNCTION__, conf, p_step->api_id, p_step->data, trigger_id);

done:
    return;
}

/* translate a string name for an api/trigger into an integer for easier indexing */
static int scenario_get_api_id_from_name(char *api_name)
{
    if (strcmp(api_name, SCENARIO_API_NAME_REG_RESPONSE) == 0) {
        return SCENARIO_API_ID_REG_RESPONSE;
    }

    if (strcmp(api_name, SCENARIO_API_NAME_START_SEQUENCE) == 0) {
        return SCENARIO_API_ID_START_SEQUENCE;
    }

    if (strcmp(api_name, SCENARIO_API_NAME_SEQUENCE_DONE) == 0) {
        return SCENARIO_API_ID_SEQUENCE_DONE;
    }

    return SCENARIO_API_ID_INVALID;
}
