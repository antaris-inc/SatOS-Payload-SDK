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

#ifndef __CONFIG_DB_H__
#define __CONFIG_DB_H__

#define SEQ_CONF_MAX_DATA_LEN           128

#define SCENARIO_API_ID_INVALID         -1

#define SCENARIO_API_ID_REG_RESPONSE    0
#define SCENARIO_API_NAME_REG_RESPONSE  "registration-request"

#define SCENARIO_API_ID_START_SEQUENCE      1
#define SCENARIO_API_NAME_START_SEQUENCE    "start-sequence"

#define SCENARIO_API_ID_SEQUENCE_DONE       2
#define SCENARIO_API_NAME_SEQUENCE_DONE     "sequence-done"

#define SCENARIO_API_ID_COUNT               3

void scenario_register_conf(int argc, char *argv[]);

#define SEQ_CONF_FOUND          0
#define SEQ_CONF_NOT_FOUND      -1

int scenario_get_conf(int api_trigger, int consume_conf, int *out_api_id, char *data);

#endif /* __CONFIG_DB_H__ */
