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

import datetime
import json
import logging
import pathlib
import os
import sys
import time
import serial

from satos_payload_sdk import app_framework
from satos_payload_sdk import antaris_api_gpio as api_gpio
from satos_payload_sdk import antaris_api_can as api_can
from satos_payload_sdk import antaris_api_parser as api_parser
from satos_payload_sdk import antaris_api_i2c as api_i2c
import satos_payload_sdk.gen.antaris_api_types as api_types

g_GPIO_ERROR = -1
g_Uart_Baudrate = 9600
g_FileDownloadDir = "/opt/antaris/outbound/"    # path for staged file download
g_StageFileName = "SampleFile.txt"              # name of staged file

ADCS_start_success = 0
ADCS_start_reconfigured = 1
ADCS_start_failed = 2

Request_success = 0

logger = logging.getLogger()

class Controller:

    def is_healthy(self):
        logger.info("Health check succeeded")
        return True

    def gnss_eph_data_handler(self, ctx):
        logger.info("GNSS EPH data received")
        gnss_data = ctx
        # List of field names mapped to each bit position
        fields = [
            "Time Validity",
            "ECI Position Validity",
            "ECI Velocity Validity",
            "ECEF Position Validity",
            "ECEF Velocity Validity",
            "Angular Rate Validity",
            "Attitude Quaternion Validity",
            "Lat-Lon-Altitude Validity",
            "Nadir Vector Validity",
            "Geodetric Nadir Vector Validity",
            "Beta Angle Validity"
        ]
        if gnss_data.gps_timeout_flag == 1:
            logger.info(f"gps_fix_time: {gnss_data.gps_eph_data.gps_fix_time}")
            logger.info(f"gps_sys_time: {gnss_data.gps_eph_data.gps_sys_time}")
            obc = gnss_data.gps_eph_data.obc_time
            obc_formatted = (
                f"{obc.hour:02d}:{obc.minute:02d}:{obc.millisecond // 1000:02d}."
                f"{obc.millisecond % 1000:03d} "
                f"Date: {obc.date:02d}/{obc.month:02d}/{obc.year}"
            )
            logger.info(f"obc_time : {obc_formatted}")

            for i in {0,1,2}:
                logger.info(f"gps_position_ecef: {gnss_data.gps_eph_data.gps_position_ecef[i]}")
            for i in {0,1,2}:
                logger.info(f"gps_velocity_ecef: {gnss_data.gps_eph_data.gps_velocity_ecef[i]}")
                logger.info(f"gps_validity_flag_pos_vel: {gnss_data.gps_eph_data.gps_validity_flag_pos_vel}")

        elif gnss_data.adcs_timeout_flag == 1:    
            logger.info(f"ADCS Orbit Propagator/System Time = {gnss_data.adcs_eph_data.orbit_time}") 
            logger.info(f"ECI Position X (km) = {gnss_data.adcs_eph_data.eci_position_x}") 
            logger.info(f"ECI Position Y (km) = {gnss_data.adcs_eph_data.eci_position_y}") 
            logger.info(f"ECI Position Z (km) = {gnss_data.adcs_eph_data.eci_position_z}") 
            logger.info(f"ECI Velocity X (km/s) = {gnss_data.adcs_eph_data.eci_velocity_x}") 
            logger.info(f"ECI Velocity Y (km/s) = {gnss_data.adcs_eph_data.eci_velocity_y}") 
            logger.info(f"ECI Velocity Z (km/s) = {gnss_data.adcs_eph_data.eci_velocity_z}") 
            logger.info(f"ECEF Position X (km) = {gnss_data.adcs_eph_data.ecef_position_x}")
            logger.info(f"ECEF Position Y (km) = {gnss_data.adcs_eph_data.ecef_position_y}")
            logger.info(f"ECEF Position Z (km) = {gnss_data.adcs_eph_data.ecef_position_z}")
            logger.info(f"ECEF Velocity X (km/s) = {gnss_data.adcs_eph_data.ecef_velocity_x}")
            logger.info(f"ECEF Velocity Y (km/s) = {gnss_data.adcs_eph_data.ecef_velocity_y}")
            logger.info(f"ECEF Velocity Z (km/s) = {gnss_data.adcs_eph_data.ecef_velocity_z}")
            logger.info(f"X axis Angular rate (deg/s) = {gnss_data.adcs_eph_data.ang_rate_x}")
            logger.info(f"Y axis Angular rate (deg/s) = {gnss_data.adcs_eph_data.ang_rate_y}")
            logger.info(f"Z axis Angular rate (deg/s) = {gnss_data.adcs_eph_data.ang_rate_z}")
            logger.info(f"Attitude Quaternion 1 = {gnss_data.adcs_eph_data.att_quat_1}")
            logger.info(f"Attitude Quaternion 2 = {gnss_data.adcs_eph_data.att_quat_2}")
            logger.info(f"Attitude Quaternion 3 = {gnss_data.adcs_eph_data.att_quat_3}")
            logger.info(f"Attitude Quaternion 4 = {gnss_data.adcs_eph_data.att_quat_4}")
            logger.info(f"Latitude (deg) = {gnss_data.adcs_eph_data.latitude}")
            logger.info(f"Longitude (deg) = {gnss_data.adcs_eph_data.longitude}")
            logger.info(f"Altitude (km) = {gnss_data.adcs_eph_data.altitude}")
            logger.info(f"X Nadir Vector = {gnss_data.adcs_eph_data.nadir_vector_x}") 
            logger.info(f"Y Nadir Vector = {gnss_data.adcs_eph_data.nadir_vector_y}") 
            logger.info(f"Z Nadir Vector = {gnss_data.adcs_eph_data.nadir_vector_z}") 
            logger.info(f"X Geodetic Nadir Vector = {gnss_data.adcs_eph_data.gd_nadir_vector_x}")
            logger.info(f"Y Geodetic Nadir Vector = {gnss_data.adcs_eph_data.gd_nadir_vector_y}")
            logger.info(f"Z Geodetic Nadir Vector = {gnss_data.adcs_eph_data.gd_nadir_vector_z}")
            logger.info(f"Beta Angle (deg) = {gnss_data.adcs_eph_data.beta_angle}")
            # Print each bit's meaning
            for i, name in enumerate(fields):
                bit_value = (gnss_data.adcs_eph_data.validity_flags >> i) & 1
                print(f"{name}: {bit_value}")
        return True
    
    def get_eps_voltage_handler(self, ctx):
        logger.info(f"EPS voltage data received : {float(ctx.eps_voltage):.2f}")
        return True
    
    def handle_hello_world(self, ctx):
        logger.info("Handling sequence: hello, world!")

    def handle_hello_friend(self, ctx):
        name = ctx.params
        logger.info(f"Handling sequence: hello, {name}!")

    def handle_log_location(self, ctx):
        loc = ctx.client.get_current_location()
        logger.info(f"Handling sequence: lat={loc.latitude}, lng={loc.longitude}, alt={loc.altitude} sd_lat={loc.sd_latitude}, sd_lng={loc.sd_longitude}, sd_alt={loc.sd_altitude}")
    
    def handle_gnss_data(self, ctx):
        periodicity_in_ms = 2000    # Periodicity = 0 indicates one time GNSS EPH data. Max is 1 minute
        eph2_enable = 1
        if ctx.params.lower() == "stop":
            logger.info("Sending GNSS EPH data stop request")
            resp = ctx.client.gnss_eph_stop_data_req()
            if (resp.req_status == Request_success):
                logger.info("GNSS EPH data stop request success")
            else:
                logger.info("GNSS EPH data stop request failed")
        elif ctx.params.lower() == "start":
            logger.info("Sending GNSS EPH data start request")
            resp = ctx.client.gnss_eph_start_data_req(periodicity_in_ms, eph2_enable)
            if (resp.req_status == ADCS_start_success):
                logger.info("GNSS EPH data start request success")
            elif (resp.req_status == ADCS_start_reconfigured):
                logger.info("Reconfiguring GNSS EPH data start request success")
            else:
                logger.info("GNSS EPH data start request failed")
        else:
            logger.info("Incorrect parameters. Parameter can be 'stop' or 'start'")

    def handle_eps_voltage_telemetry_request(self, ctx):
        periodicity_in_ms = 2000   # Periodicity = 0 indicates one time EPS voltage data frequency. Max is 1 minute.
        if ctx.params.lower() == "stop":
            logger.info("Sending Get Eps Voltage telemetry stop request")
            resp = ctx.client.get_eps_voltage_stop_req()
            if (resp.req_status == Request_success):
                logger.info("Get Eps Voltage telemetry stop request success")
            else:
                logger.info("Get Eps Voltage telemetry stop request failed")
        elif ctx.params.lower() == "start":
            logger.info("Sending Get Eps Voltage telemetry start request")
            resp = ctx.client.get_eps_voltage_start_req(periodicity_in_ms)
            logger.info(f"Current voltage = {resp}")
        else:
            logger.info("Incorrect parameters. Parameter can be 'stop' or 'start'")

    def handle_ses_therm_mgmnt(self, ctx):
        hardware_id = 0   # 0:SESA , 1:SESB
        duration = 2000   # millisecond
        upper_threshold = 25 # in celsius
        lower_threshold = 20 # in celsius

        if ctx.params.lower() == "stop":
            logger.info("Sending stop SES thermal management request")
            resp = ctx.client.stop_ses_therm_mgmnt_req(hardware_id)
            if (resp.req_status == Request_success):
                logger.info("stop SES thermal management request success")
            else:
                logger.info("stop SES thermal management request failed")
        elif ctx.params.lower() == "start":
            logger.info("Sending start SES thermal management request")
            resp = ctx.client.start_ses_therm_mgmnt_req(hardware_id, duration, upper_threshold, lower_threshold)
            if (resp.req_status == Request_success):
                logger.info("start SES thermal management request success")
            else:
                logger.info("start SES thermal management request failed")

    def handle_ses_temp_req(self, ctx):
        hardware_id = 0   # 0:SESA , 1:SESB
        resp = ctx.client.ses_temp_req(hardware_id)
        logger.info(f"Current temperature = {resp.temp}")
        logger.info(f"Heater power status = {resp.heater_pwr_status}") # 0:OFF, 1:ON

    def ses_thermal_status_ntf(self, ctx):
        logger.info(f"Hardware IS = {ctx.hardware_id}") # 0:SAS-A 1: SAS-B
        logger.info(f"Current temperature = {ctx.temp}")
        logger.info(f"Heater power status = {ctx.heater_pwr_status}") # 0:OFF, 1:ON

    def handle_power_control(self, ctx):
        logger.info("Handling payload power")
        power_state = ctx.params      # 0 = power off, 1 = power on
        hw_id = 0x4001                # If hw_id = 0, then default payload hardware id is send
        if(power_state != 0 and power_state != 1):
            logger.info("invlaid power state. power state can only be 0 or 1")
            return
        resp = ctx.client.payload_power_control(power_state, hw_id)
        logger.info(f"Power control state = {power_state}. Call response is = {resp}")
        
    # The sample program assumes 2 GPIO pins are connected back-to-back. 
    # This sequence toggles level of 'Write Pin' and then reads level of 'Read Pin'
    def handle_test_gpio(self, ctx):
        gpio_info = api_parser.api_pa_pc_get_gpio_info()

        logger.info("Total gpio pins = %d", int(gpio_info.pin_count))
        api_gpio.api_pa_pc_init_gpio_lib()
        i = 0
        # Read initial value of GPIO pins.
        # As GPIO pins are back-to-back connected, their value must be same.
        while (i < int(gpio_info.pin_count)):
            if int(gpio_info.pins[i]) != -1:
                readPin = gpio_info.pins[i];
                i += 1
                writePin = gpio_info.pins[i];

                val = api_gpio.api_pa_pc_read_gpio(int(readPin))
                if val != g_GPIO_ERROR:
                    logger.info("Initial Gpio value of pin no %d is %d ", int(readPin), val)
                else:
                    logger.error("Error in pin no %d", int(readPin))
                    return 
                # Toggle the value
                val = val ^ 1                      
                logger.info("Writing %d to pin no. %d", val, int(writePin))

                # Writing value to WritePin.
                val = api_gpio.api_pa_pc_write_gpio(int(writePin), val)
                if val != g_GPIO_ERROR:
                    logger.info("Written %d successfully to pin no %d", val, int(writePin))
                else:
                    logger.error("error in pin no %d ", int(writePin))
                    return 
                
                logger.info("Reading from pin no. %d", int(readPin))
                # As Read and Write pins are back-to-back connected, 
                # Reading value of Read pin to confirm GPIO success/failure
                val = api_gpio.api_pa_pc_read_gpio(int(readPin))
                if val != g_GPIO_ERROR:
                    logger.info("Final Gpio value of pin no %d is %d ", int(readPin), val)
                else:
                    logger.error("Error in pin no %d", int(readPin))
                    return
            i += 1
        
        api_gpio.api_pa_pc_deinit_gpio_lib()
        return 
    
    # Sequence to test UART loopback. The sample program assumes Tx and Rx are connected in loopback mode.
    def handle_uart_loopback(self, ctx):
        data = ctx.params
        if data == "":
            logger.info("Using default string, as input string is empty")
            data = "Default string: Uart Tested working"

        data = data + "\n"
        uartInfo = api_gpio.api_pa_pc_get_uart_dev()
        logger.info("Total uart ports = %d", int(uartInfo.uart_port_count))

        uartPort = uartInfo.uart_dev[0]
        try: 
            ser = serial.Serial(uartPort, g_Uart_Baudrate)  # Replace '9600' with your baud rate
        except Exception as e:
            logger.error("Error in opening serial port")
            return
        
        logger.info(f"writing data")
        # Write data to the serial port
        ser.write(data.encode('utf-8'))  # Send the data as bytes

        logger.info("Reading data")
        # Read data from the serial port
        read_data = ser.readline()
        logger.info("Data =  %s", read_data)

        # Close the serial port
        ser.close()

    def handle_stage_filedownload(self, ctx):
        logger.info("Staging file for download")
        # creating a sample text file
        new_file = g_FileDownloadDir + g_StageFileName
        with open(new_file, "w") as file:
            file.write("Testing file download with payload")
        
        # Files must be present in "/opt/antaris/outbound/" before staging them for download
        # 'FILE_DL_PRIORITY_LOW': 0,
        # 'FILE_DL_PRIORITY_NORMAL': 1,
        # 'FILE_DL_PRIORITY_HIGH': 2,
        # 'FILE_DL_PRIORITY_IMMEDIATE': 3,
        api_types.FileDlRadioType.file_dl_band =  api_types.FileDlRadioType.FILE_DL_XBAND
        resp = ctx.client.stage_file_download(g_StageFileName, api_types.FilePriorities.FILE_DL_PRIORITY_NORMAL, api_types.FileDlRadioType.file_dl_band)
        if resp == ValueError:
            print("Error in staging file")
        else:
            print("File successfully staged ")

    def handle_test_can_bus(self, ctx):
        logger.info("Test CAN bus")

        # Get Arbitration ID & data
        data = ctx.params
        parts = data.split()

        if len(parts) != 2:
            logger.info("Input format is incorrect. The format is: ")
            logger.info("Arbitration ID data[0],data[1],data[2],data[3]..data[7]") 
            logger.info("Using defaullt arbitration ID and data bytes.")
            data = "0x123 0x11,0x12,0x13,0x14,0x15,0x16,0x17"
            parts = data.split()

        # Extract arbitration ID and data bytes
        arb_id = int(parts[0], 16)
        data_str = parts[1]
        data_bytes = [int(byte, 16) for byte in data_str.split(",")]

        # Get CAN bus info from config file
        canInfo = api_can.api_pa_pc_get_can_dev()
        logger.info("Total CAN bus ports = %d", int(canInfo.can_port_count))

        # Define the CAN channel to use (assuming the first device)
        channel = canInfo.can_dev[0]
        logger.info("Starting CAN receiver port %s", channel)

        # Starting CAN received thread
        api_can.api_pa_pc_start_can_receiver_thread(channel)
        
        # Defining limits for data send and receive
        send_msg_limit = 10

        loopCounter = 0

        # Main loop to send CAN messages
        logger.info("Sending data in CAN bus")
        while loopCounter < send_msg_limit:
            loopCounter = loopCounter + 1
            arb_id = arb_id + 1
            api_can.api_pa_pc_send_can_message(channel, arb_id, data_bytes)
            time.sleep(1)

        logger.info("Data send = %d", api_can.api_pa_pc_get_can_message_received_count())

        while api_can.api_pa_pc_get_can_message_received_count() > 0: 
            received_data = api_can.api_pa_pc_read_can_data()
            if received_data != g_GPIO_ERROR:
                logger.info("received data =", received_data)
            else:
                logger.error("Error in receiving data")
        
        logger.info("Completed reading")

        return 
    
    def handle_test_i2c_bus(self, ctx):
        logger.info("Test I2C bus")

        # Check number of i2c bus 
        i2cInfo = api_parser.api_pa_pc_get_i2c_dev()
        logger.info("Total I2C bus ports = %d", int(i2cInfo.i2c_port_count))

        # Get i2c devide details
        channel = i2cInfo.i2c_dev[0]
        logger.info("Starting CAN receiver port %s", channel)

        api_i2c.api_pa_pc_init_i2c_lib()
        # Write data to i2c bus
        baseAddr = 0xA0
        index= 0
        data = 1
        api_i2c.api_pa_pc_write_i2c_data(i2cInfo.i2c_dev[0], baseAddr, index, data)

        time.sleep(1)

        # Read data from i2c bus
        api_i2c.api_pa_pc_read_i2c(i2cInfo.i2c_dev[0], baseAddr, index, data)

        logger.info(f"Data received = {data}")

        api_i2c.api_pa_pc_deinit_i2c_lib()
        
        return 

    def handle_pa_satos_message(self, ctx):
        command = 1  # UINT16 command ID
        payload_data = bytes([0x12, 0x34, 0x56])  # Can be up to 1020 bytes

        resp = ctx.client.pa_satos_message(command, payload_data)
        print(f"Command id = {resp.command_id} , status = {resp.req_status}")

        return

def new():
    ctl = Controller()

    app = app_framework.PayloadApplication()
    app.set_health_check(ctl.is_healthy)
    app.set_gnss_eph_data_cb(ctl.gnss_eph_data_handler)
    app.set_get_eps_voltage_cb(ctl.get_eps_voltage_handler)
    app.set_ses_thermal_status_ntf(ctl.ses_thermal_status_ntf)

    # Sample function to add stats counters and names
    set_payload_values(app)

    # Note : SatOS-Payload-SDK supports sequence upto 16 characters long
    app.mount_sequence("HelloWorld", ctl.handle_hello_world)
    app.mount_sequence("HelloFriend", ctl.handle_hello_friend)
    app.mount_sequence("LogLocation", ctl.handle_log_location)
    app.mount_sequence("TestGPIO", ctl.handle_test_gpio)
    app.mount_sequence("UARTLoopback", ctl.handle_uart_loopback)
    app.mount_sequence("StageFile",ctl.handle_stage_filedownload)
    app.mount_sequence("PowerControl", ctl.handle_power_control)
    app.mount_sequence("TestCANBus", ctl.handle_test_can_bus)
    app.mount_sequence("GnssDataTm", ctl.handle_gnss_data)
    app.mount_sequence("EpsVoltageTm", ctl.handle_eps_voltage_telemetry_request)
    app.mount_sequence("SesThermMgmnt", ctl.handle_ses_therm_mgmnt)
    app.mount_sequence("SesTempReq", ctl.handle_ses_temp_req)
    app.mount_sequence("TestI2CBus", ctl.handle_test_i2c_bus)
    app.mount_sequence("PaSatOsMsg", ctl.handle_pa_satos_message)
    return app

def set_payload_values(payload_app):
    payload_metrics = payload_app.payload_metrics
    # Set used_counter
    payload_metrics.used_counter = 5  # Example value

    # Set counter values
    for i in range(payload_metrics.used_counter):
        payload_metrics.metrics[i].counter = i + 1 # Example value

    # Set counter_name values
    for i in range(payload_metrics.used_counter):
        payload_metrics.metrics[i].names = f"Counter {i}"  # Example value
    
    # Change counter name
    payload_metrics.define_counter(1, "MetricName_1")
    # Increment counter
    payload_metrics.inc_counter(1)

    return 

if __name__ == '__main__':
    DEBUG = os.environ.get('DEBUG')
    logging.basicConfig(    level=logging.DEBUG if DEBUG else logging.INFO,
                            format="%(asctime)s  %(levelname)s %(message)s",
                            datefmt="%Y-%m-%d %H:%M:%S"
                        )

    app = new()

    try:
        app.run()
    except Exception as exc:
        logger.exception("payload app failed")
        sys.exit(1)
