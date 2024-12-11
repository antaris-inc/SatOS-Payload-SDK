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

# The sample sequence "CaptureImage" & "RecordVideo" performs following operations
#   - Captures image / Records video 
#   - Downloads file
#   - Matrix counter is maintained for number of photos clicked OR videos recorded

import logging
import os
import sys

from satos_payload_sdk import app_framework
from satos_payload_sdk import antaris_api_gpio as api_gpio

g_ERROR = -1

g_FileDownloadDir = "/opt/antaris/outbound/"    # path for staged file download

g_app = 0

logger = logging.getLogger()

class Controller:

    def is_healthy(self):
        logger.info("Health check succeeded")
        return True

    # Input parameters should be saperated by comma
    # Parameters : 
    #       FileName
    #       Format
    #       Width
    #       Height
    # e.g. To create "OutFile.jpg" in jpeg format with image dimension as 640x480
    # params: OutFile.jpg,jpg,640,480
    def handle_capture_image(self, ctx):
        cameraDev = api_gpio.api_pa_pc_get_camera_dev()
        logger.info("Camera port = %s, port id = %d ", cameraDev.camera_port, cameraDev.camera_port_id)

        # Check if directory is writable or not
        if not os.access(g_FileDownloadDir, os.W_OK):
            logger.error("Output directory is not writable: %s", g_FileDownloadDir)
            return

        params = ctx.params.split(',')
        # Validate the number of parts
        if len(params) != 4:
            logger.error("Input must contain exactly 4 parameters: FileName, Format, Width, Height")
            return 
    
        # Extract parameters
        try:
            g_ImageFileName = params[0]  # FileName as text
            image_format = params[1]     # Format as text, supported image format : "jpg", "jpeg", "png", "bmp", "tiff", "ppm", "pgm", "pbm", "webp"
            width = int(params[2])       # Width as integer
            height = int(params[3])      # Height as integer
        except ValueError:
            logger.error("Width, Height, FPS, and Duration must be integers.")
            return 
        
        # Create full image path
        image_file = os.path.join(g_FileDownloadDir, g_ImageFileName) 
        
        ret = api_gpio.api_capture_image(cameraDev.camera_port_id, image_file, width, height, image_format)

        if ret == g_ERROR:
            logger.error("Error: Can not capture image")
            return 
        
        # Increment counter
        g_app.payload_metrics.inc_counter(0)

        # Staging file for download 
        logger.info("Downloading image %s", image_file)
        ctx.client.stage_file_download(image_file)
        return 

    # Input parameters should be saperated by comma
    # Parameters : 
    #       FileName
    #       Format
    #       Width
    #       Height
    #       FPS
    #       Duration
    # e.g. To create "OutFile.mp4" in mp4 format with image dimension as 640x480 with 30 fps for 60 seconds
    # params: OutFile.mp4,mp4,640,480,30,60
    def handle_capture_video(self, ctx):
        cameraDev = api_gpio.api_pa_pc_get_camera_dev()
        logger.info("Camera port = %s, port id = %d ", cameraDev.camera_port, cameraDev.camera_port_id)

        # Check if directory is writable or not
        if not os.access(g_FileDownloadDir, os.W_OK):
            logger.error("Output directory is not writable: %s", g_FileDownloadDir)
            return

        params = ctx.params.split(',')
        # Validate the number of parts
        if len(params) != 6:
            logger.error("Input must contain exactly 6 parameters: FileName, Format, Width, Height, FPS, Duration.")
            return 
    
        # Extract parameters
        try:
            g_VideoFileName = params[0]  # FileName as text
            video_format = params[1]     # Format as text, Supported format "avi", "mp4", "mkv", "VP80"
            width = int(params[2])       # Width as integer
            height = int(params[3])      # Height as integer
            fps = int(params[4])         # FPS as integer
            duration = int(params[5])    # Duration as integer
        except ValueError:
            logger.error("Width, Height, FPS, and Duration must be integers.")
            return 
    
        # Create full video path
        video_file = os.path.join(g_FileDownloadDir, g_VideoFileName) 

        ret = api_gpio.api_capture_video(cameraDev.camera_port_id, video_file, width, height, video_format, duration, fps)
        if ret == g_ERROR:
            logger.error("Error: Can not record video")
        
        # Increment counter
        g_app.payload_metrics.inc_counter(1)

        # Staging file for download 
        logger.info("Downloading video %s", video_file)
        # Staging file for download 
        ctx.client.stage_file_download(video_file)
        return 

def new():
    ctl = Controller()

    g_app = app_framework.PayloadApplication()
    g_app.set_health_check(ctl.is_healthy)

    # Sample function to add stats counters and names
    set_payload_values()

    # Note : SatOS-Payload-SDK supports sequence upto 16 characters long
    g_app.mount_sequence("CaptureImage", ctl.handle_capture_image)
    g_app.mount_sequence("RecordVideo", ctl.handle_capture_video)
    return g_app

def set_payload_values():
    payload_metrics = g_app.payload_metrics

    # Set used_counter
    payload_metrics.used_counter = 2  

    # Set counter name & values
    payload_metrics.metrics[0].names = "Image" 
    payload_metrics.metrics[0].counter = 0  

    payload_metrics.metrics[1].names = "Video" 
    payload_metrics.metrics[1].counter = 0 
    
    return 
