# Copyright 2023 Antaris, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import sys

from satos_payload_sdk import antaris_api_gpio as api_gpio

g_GPIO_ERROR = -1

if __name__ == "__main__":
    print("Sample program to test gpio pins")
    print("Description : Connect gpio pins in loopback mode. \n \t   Value assigned to one gpio pin gets reflected to other gpio pin connected in loopback mode \n")

    #Total arguments
    argc = len(sys.argv)
    if argc != 5:
        print("Usage = python3 example_gpio.py <port> <Pin no to Read> <Pin no to write> <Value>")
        exit()

    print("GPIO Port = ", int(sys.argv[1]))

    # Read value of gpio pin
    val = api_gpio.api_pa_pc_read_gpio(int(sys.argv[1]), int(sys.argv[2]))
    if val != g_GPIO_ERROR:
        print("Initial Gpio value of pin no ", int(sys.argv[2])," is", val)
    else:
        print("Error in pin no ", int(sys.argv[2]))

    # Write value of gpio pin
    val = api_gpio.api_pa_pc_write_gpio(int(sys.argv[1]), int(sys.argv[3]), int(sys.argv[4]))
    if val != g_GPIO_ERROR:
        print("Written ", int(sys.argv[4]), " successfully to pin no ", int(sys.argv[3]))
    else:
        print("error in pin no ", int(sys.argv[3]))

    # Read value of gpio pin
    val = api_gpio.api_pa_pc_read_gpio(int(sys.argv[1]), int(sys.argv[2]))
    if val != g_GPIO_ERROR:
        print("Final Gpio value of pin no ", int(sys.argv[2]), " is = ", val)
    else:
        print("Error in pin no ", int(sys.argv[2]))