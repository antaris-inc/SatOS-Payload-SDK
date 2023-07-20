Python Application Guide
########################

The SatOS Payload SDK contains assets that greatly simplify development of payload applications in Python.
The python library (named `satos_payload_sdk`) provides Payload Interface client bindings as well as a user-friendly abstraction for applications.
This guide primarily describes how to use this library in pursuit of building a fully functional Payload Application.

Obtaining the Library
*********************

The `satos_payload_sdk` library is not distributed independently, and can only be accessed from within a docker image.
Applications are expected to be built atop this image, which is distributed via public registry: `quay.io/antaris-inc/satos-payload-app-python:stable`

During the development process, it can be helpful to run a local container to manually interact with the library and explore its operation:

  docker run -v $PWD:/workspace -w /workspace -it quay.io/antaris-inc/satos-payload-app-python:stable /bin/bash

Sample Applications
*******************

Fully functional applications using this SDK are available here: https://github.com/antaris-inc/SatOS-Payload-Demos.

Device I/O
**********

Payload device access is not yet abstracted through the `satos_payload_sdk.app_framework` module.  
Instead, developers should use the `satos_payload_sdk.antaris_api_gpio` module.

To identify the location of a connected UART device, call `api_pa_pc_get_uart_dev()`. 
The returned value will be a string containing a your device path (e.g. `"/dev/ttyUSB0"`).
Your application may then interact with the device as necessary.

To understand the available GPIO pin configuration, call `api_pa_pc_get_gpio_info()`.
The function will return an object with three attributes:

* `pin_count` holds the number of pins available
* `pins` holds an array of assigned pin numbers (of length `pin_count`)
* `interrupt` holds the assigned interrupt pin number

To read the current value from a GPIO pin, simply call `api_pa_pc_read_gpio(<pin>)`.
The read value will be returned from the function.
A value of -1 will be returned if the operation fails.

To write to a GPIO pin, call `api_pa_pc_write_gpio(<pin>, <value>)`.
The written value will be returned from the function.
A value of -1 will be returned if the operation fails.

At this time, GPIO support is limited to FTDI devices.
