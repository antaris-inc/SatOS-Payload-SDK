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

""" Wrapper script to pass user sequence/api-responses' configuration to pc-sim """

import os, sys, getopt
import logging
import json

# Creating a logger object
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__.split('.')[0])

g_conf_file="pc-sim/sequence_schedule.json"
g_default_pc_sim_executable="pc-sim/pc-sim"

g_delim1="#"
g_delim2=':'
g_delim3="="
g_delim4='-'

g_conf_key_exec = "executable"

g_conf_key_resps = "responses"
g_conf_key_resp_trigger = "trigger"
g_conf_key_resp_api = "api"
g_conf_key_resp_data = "data"

g_conf_key_seqs = "conops_tasks_schedule"
g_conf_key_seq_id = "sequence_id"
g_conf_key_seq_param = "sequence_params"
g_conf_key_seq_duration = "duration"
g_conf_arg_seq_trigger_first = "registration-request"
g_conf_arg_seq_trigger_not_first = "sequence-done"
g_conf_arg_seq_api = "start-sequence"

def is_valid_resp_conf(resp_conf):
    global g_conf_key_resp_trigger
    global g_conf_key_resp_api
    global g_conf_key_resp_data
    global g_delim1
    global g_delim2
    global g_delim3
    global g_delim4

    if g_conf_key_resp_trigger not in resp_conf or g_conf_key_resp_api not in resp_conf or g_conf_key_resp_data not in resp_conf:
        logger.error("{} must contain {}, {} and {}".format(resp_conf, g_conf_key_resp_trigger, g_conf_key_resp_api, g_conf_key_resp_data))
        return False

    if g_delim1 in resp_conf[g_conf_key_resp_trigger] or g_delim1 in resp_conf[g_conf_key_resp_api] or g_delim1 in resp_conf[g_conf_key_resp_data]:
        logger.error("{} cannot contain special character {}".format(resp_conf, g_delim1))
        return False

    if g_delim2 in resp_conf[g_conf_key_resp_trigger] or g_delim2 in resp_conf[g_conf_key_resp_api] or g_delim2 in resp_conf[g_conf_key_resp_data]:
        logger.error("{} cannot contain special character {}".format(resp_conf, g_delim2))
        return False

    if g_delim3 in resp_conf[g_conf_key_resp_trigger] or g_delim3 in resp_conf[g_conf_key_resp_api] or g_delim3 in resp_conf[g_conf_key_resp_data]:
        logger.error("{} cannot contain special character {}".format(resp_conf, g_delim3))
        return False

    if g_delim4 in resp_conf[g_conf_key_resp_trigger] or g_delim4 in resp_conf[g_conf_key_resp_api] or g_delim4 in resp_conf[g_conf_key_resp_data]:
        logger.error("{} cannot contain special character {}".format(resp_conf, g_delim4))
        return False

    return True

def is_valid_seq_conf(seq_conf):
    global g_conf_key_seq_id
    global g_conf_key_seq_param
    global g_conf_key_seq_duration
    global g_delim1
    global g_delim2
    global g_delim3
    global g_delim4

    if g_conf_key_seq_id not in seq_conf or g_conf_key_seq_param not in seq_conf or g_conf_key_seq_duration not in seq_conf:
        logger.error("{} must contain {}, {} and {}".format(seq_conf, g_conf_key_seq_id, g_conf_key_seq_param, g_conf_key_seq_duration))
        return False

    if g_delim1 in seq_conf[g_conf_key_seq_id] or g_delim1 in seq_conf[g_conf_key_seq_param] or g_delim1 in seq_conf[g_conf_key_seq_duration]:
        logger.error("{} cannot contain special character {}".format(seq_conf, g_delim1))
        return False

    if g_delim2 in seq_conf[g_conf_key_seq_id] or g_delim2 in seq_conf[g_conf_key_seq_param] or g_delim2 in seq_conf[g_conf_key_seq_duration]:
        logger.error("{} cannot contain special character {}".format(seq_conf, g_delim2))
        return False

    if g_delim3 in seq_conf[g_conf_key_seq_id] or g_delim3 in seq_conf[g_conf_key_seq_param] or g_delim3 in seq_conf[g_conf_key_seq_duration]:
        logger.error("{} cannot contain special character {}".format(seq_conf, g_delim3))
        return False

    if g_delim4 in seq_conf[g_conf_key_seq_id] or g_delim4 in seq_conf[g_conf_key_seq_param] or g_delim4 in seq_conf[g_conf_key_seq_duration]:
        logger.error("{} cannot contain special character {}".format(seq_conf, g_delim4))
        return False

    return True

def get_command_line_arg_for_resp(resp_conf):
    global g_conf_key_resp_trigger
    global g_conf_key_resp_api
    global g_conf_key_resp_data

    if not is_valid_resp_conf(resp_conf):
        logger.error("Bad resp-config {}".format(resp_conf))
        sys.exit(-1)

    resp_option = "--{}=\"{}:{}\"".format(resp_conf[g_conf_key_resp_trigger], resp_conf[g_conf_key_resp_api], resp_conf[g_conf_key_resp_data])
    return resp_option

def get_command_line_arg_for_sequence(seq_conf, index):
    global g_conf_key_seq_id
    global g_conf_key_seq_param
    global g_conf_key_seq_duration
    global g_conf_arg_seq_trigger_first
    global g_conf_arg_seq_trigger_not_first
    global g_conf_arg_seq_api

    if not is_valid_seq_conf(seq_conf):
        logger.error("Bad seq-config {}".format(seq_conf))
        sys.exit(-1)

    trigger = g_conf_arg_seq_trigger_not_first

    if index == 0:
        trigger = g_conf_arg_seq_trigger_first

    seq_option = "--{}={}:{}:{}:{}".format(trigger, g_conf_arg_seq_api, seq_conf[g_conf_key_seq_id], seq_conf[g_conf_key_seq_param], seq_conf[g_conf_key_seq_duration])
    return seq_option

def get_command_line():
    global g_conf_file
    global g_conf_key_exec
    global g_conf_key_resps
    global g_conf_key_seqs
    global g_default_pc_sim_executable

    cmd_line = g_default_pc_sim_executable

    conf = None

    with open(g_conf_file, 'r') as f:
        conf = json.load(f)

    if g_conf_key_exec in conf:
        cmd_line = conf[g_conf_key_exec]

    if g_conf_key_resps in conf:
        responses = conf[g_conf_key_resps]
    else:
        responses = []

    if g_conf_key_seqs in conf:
        sequences = conf[g_conf_key_seqs]
    else:
        sequences = []

    for r in responses:
        cmd_line += " " + get_command_line_arg_for_resp(r)

    for idx, s in enumerate(sequences):
        cmd_line += " " + get_command_line_arg_for_sequence(s, idx)

    return cmd_line

def print_usage():
    global g_conf_file

    logger.error("{}: Usage".format(sys.argv[0]))
    logger.error("{} [-c/--conf-file CONF_FILE.json (default {}) ] [-h/--help]".format(sys.argv[0], g_conf_file))

def print_params():
    global g_conf_file

    logger.info("{}: working with parameters".format(sys.argv[0]))
    logger.info("CONF-FILE={}".format(g_conf_file))

def parse_opts():
    global g_conf_file

    argv = sys.argv[1:]
    logger.debug("Got args: {}".format(argv))
    try:
      opts, args = getopt.getopt(argv, "hc:",["help", "conf-file="])
    except getopt.GetoptError:
      logger.critical ('Error parsing arguments')
      print_usage()
      sys.exit(-1)

    for opt, arg in opts:
        print("Parsing option {} with value {}".format(opt, arg))
        if opt in ('-h', "--help"):
            print_usage()
            sys.exit()
        elif opt in ("-c", "--conf-file"):
            g_conf_file = arg

# Main function
if __name__ == '__main__':
    parse_opts()
    print_params()

    command_line = get_command_line()
    logger.info("Invoking PC-SIM with command line:\n{}\n".format(command_line))

    os.system(command_line)
