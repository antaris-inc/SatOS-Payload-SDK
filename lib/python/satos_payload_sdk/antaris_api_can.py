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

# This file assumes that, env.json file is present at /opt/antaris/app
# location. The sample file is checked-in in conf directory
import can
import threading
import time
import json

g_JSON_Key_IO_Access = "IO_Access"
g_JSON_Key_CAN = "CAN"
g_JSON_Key_Device_Count = "CAN_PORT_COUNT"
g_JSON_Key_CAN_Device_Path = "CAN_Bus_Path_"

# Read config info
jsonfile = open('/opt/antaris/app/config.json', 'r')

g_GPIO_ERROR = -1

# threading lock
threadLock = -1
data_array = []

# returns JSON object as a dictionary
jsfile_data = json.load(jsonfile)

class CAN:
    def __init__(self, port_count, can_dev):
        self.can_port_count = port_count
        self.can_dev = can_dev

def api_pa_pc_get_can_dev():
    g_total_can_port = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_CAN][g_JSON_Key_Device_Count]
    can_dev = []

    i = 0
    for i in range(int(g_total_can_port)):
        key = g_JSON_Key_CAN_Device_Path+str(i)
        element = jsfile_data[g_JSON_Key_IO_Access][g_JSON_Key_CAN][key]
        can_dev.append(element)

    canObj = CAN(g_total_can_port, can_dev)
    return canObj

def api_pa_pc_receive_can_message(channel, data_array, lock):
    try:
        # Initialize a CAN bus interface
        bus = can.interface.Bus(channel=channel, bustype='socketcan')

        # Continuously receive CAN messages
        while True:
            # Receive a CAN message
            message = bus.recv(timeout=1)
            if message is not None:
                with lock:
                    data_array.append(message)  # Append the received message to the data array

    except can.CanError as e:
        print("Error receiving message:", e)
        return g_GPIO_ERROR
    
    finally:
        # Clean up and close the bus
        bus.shutdown()

def api_pa_pc_read_can_data():
    with threadLock:
        if data_array:
            data = data_array.pop(0)
            return data 
        else:
            return g_GPIO_ERROR

def api_pa_pc_start_can_receiver_thread(channel):
    # Create shared data array and lock
    threadLock = threading.Lock()
    receive_thread = threading.Thread(target=api_pa_pc_receive_can_message, args=(channel, data_array, threadLock))
    receive_thread.daemon = True
    receive_thread.start()

def api_pa_pc_send_can_message(channel, arbitration_id, data):
    try:
        # Initialize a CAN bus interface
        bus = can.interface.Bus(channel=channel, bustype='socketcan')

        # Send a CAN message
        message = can.Message(arbitration_id=arbitration_id, data=data, is_extended_id=False)
        bus.send(message)
        return 0

    except can.CanError as e:
        print("Error sending message:", e)
        return g_GPIO_ERROR
    
    finally:
        # Clean up and close the bus
        bus.shutdown()

def api_pa_pc_get_can_message_received_count():
    return len(data_array)

if __name__ == "__main__":
    send_msg_limit = 10
    start_msg_read = 5
    loop_reset_limit = 12

    loopCounter = 0

     # Get CAN devices from configuration
    can_obj = api_pa_pc_get_can_dev()

    # Define the CAN channel to use (assuming the first device)
    channel = can_obj.can_dev[0]

    # Start a thread to continuously receive CAN messages
    api_pa_pc_start_can_receiver_thread(channel)

    # Main loop to send CAN messages
    while True:
        loopCounter = loopCounter + 1

        if loopCounter < send_msg_limit:
            # Example message: arbitration_id=0x123, data=[0x01, 0x02, 0x03, loopCounter]
            api_pa_pc_send_can_message(channel, 0x123, [0x01, 0x02, 0x03, loopCounter])

        if loopCounter >  start_msg_read:
            print("Start reading")
            while api_pa_pc_get_can_message_received_count() > 0: 
                received_data = api_pa_pc_read_can_data()
                if received_data != g_GPIO_ERROR:
                    print("received data =", received_data)
                else:
                    print("Error in receiving data")
            print("Completed reading")

        if loopCounter == loop_reset_limit:
            loopCounter = 0

        time.sleep(1) 