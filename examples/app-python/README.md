# Example Python Payload Application

This directory contains an example Payload Application using the Python.
This is not production-ready code, but it is useful for learning how the platform functions.

## Quickstart

# Building sample application
In the Antaris Cloud Platform, create a TrueTwin Satellite with a remote payload and download the associated config.
Place that downloaded zip file in this directory.

Build the app using the following command:

```
docker build --platform=linux/amd64 -t satos-payload-example-app-python .
```

# Setting up environment
Next, we can run the application in a container. The command below assumes that `$CONFIG` is set to the name of the config file (zip) you downloaded from Antaris Cloud Platform. The file must be located in your current working directory:

```
export $CONFIG=<zip file name>
```

# Running sample application
To run sample application, simply do:
```
docker run --platform=linux/amd64 -e CONFIG=$CONFIG -v $(pwd):/workspace -it satos-payload-example-app-python 
```

# Testing sample application
You may now use the Antaris Cloud Platform to submit payload sequences. For example, submit a `HelloWorld` payload with
no parameters and you will see it reflected in the container logs.

The sequences supported by the example are described below:
* `HelloWorld`: takes no parameters and simply logs "hello, world!"
* `HelloFriend`: accepts parameter, logging "hello, <parameter>!"
* `LogLocation`: queries the current satellite location and logs it

## Working with I/O interface

# Configuring GPIO in application
The sample program supports GPIO connected through FTDI interface.
To configure GPIO, assign right GPIO pin numbers in example_app.py.
Kindly note that, GPIO pins used in application are same as pins declared while adding Payload in ACP.

# Configuring UART in application
Assign right UART port number (e.g. /dev/ttyUSB0 etc.) in example_app.py. 
Kindly note that, sample program assumes that, Tx and Rx are connected in loopback mode.

# Building sample application with I/O interface 
Build the app using the following command:

```
docker build --platform=linux/amd64 -t satos-payload-example-app-python .
```

# Running sample application with I/O interface
 To support I/O interface, docker should be run in privileged mode.

```
docker run --platform=linux/amd64 -e CONFIG=$CONFIG --privileged -v $(pwd):/workspace -it satos-payload-example-app-python
```

# Testing I/O interface
You may now use the Antaris Cloud Platform to submit payload sequences. 

The sequences supported by the example are described below:
* `Read_FTDI_GPIO` : Reads level (high/low) of GPIO pin connected through FTDI interface.
* `Write_FTDI_GPIO` : Write <parameter> to GPIO pin connected thrugh FTDI interface. The <parameter> can be `1`(high) or '0' (low)
* `UART_Loopback` : Sequence to test UART loopback.
* `Get_Interface_Info` : Gives details of interface declared in config.json file 

## Running Tests

Some local sanity tests are included to confirm the example application is functional.
To run these tests, first build the container image as documented above.
Now, simply run the image and use `pytest` to execute the tests:

```
docker run --platform=linux/amd64 -v $PWD:/workspace -w /workspace -t satos-payload-example-app-python /bin/bash -c "pip3 install pytest && pytest"
```
