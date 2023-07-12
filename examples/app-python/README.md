# Example Python Payload Application

This directory contains an example Payload Application using the Python.
This is not production-ready code, but it is useful for learning how the platform functions.

## Quickstart

In the Antaris Cloud Platform, create a TrueTwin Satellite with a remote payload and download the associated config.
Place that downloaded zip file in this directory.

Build the app using the following command:

```
docker build --platform=linux/amd64 -t satos-payload-example-app-python .
```

Next, we can run the application in a container. The command below assumes that `$CONFIG` is set to the name of the config file (zip) you downloaded from Antaris Cloud Platform. The file must be located in your current working directory:

```
docker run --platform=linux/amd64 -e CONFIG=$CONFIG --privileged -v $(pwd):/workspace -it satos-payload-example-app-python
```

You may now use the Antaris Cloud Platform to submit payload sequences. For example, submit a `HelloWorld` payload with
no parameters and you will see it reflected in the container logs.

The sequences supported by the example are described below:
* `HelloWorld`: takes no parameters and simply logs "hello, world!"
* `HelloFriend`: accepts parameter, logging "hello, <parameter>!"
* `LogLocation`: queries the current satellite location and logs it

### Configuring I/O interface

#### GPIO
The required GPIO pin and port should be provided by user while adding payload hardware in ACP. The same is exposed to application inside cointainer through SatOS_Payload_SDK api's. 

#### UART
The required UART path (e.g. /dev/ttyUSB0 etc.) should be provided by user while adding Payload hardware in ACP. The same is exposed to application inside cointainer. Expected Baud rate can be configured in example_app.py. Default Baud rate is 9600.

The sequences supported by the example for testing IO are described below:
* `TestGPIO` : The sample program assumes 2 GPIO pins are connected back-to-back. This sequence toggles level of 'Write Pin' and then reads level of 'Read Pin'. 
* `UARTLoopback` : Sequence to test UART loopback. The sample program assumes Tx and Rx are connected in loopback mode.

## Running Tests

Some local sanity tests are included to confirm the example application is functional.
To run these tests, first build the container image as documented above.
Now, simply run the image and use `pytest` to execute the tests:

```
docker run --platform=linux/amd64 -v $PWD:/workspace -w /workspace -t satos-payload-example-app-python /bin/bash -c "pip3 install pytest && pytest"
```
