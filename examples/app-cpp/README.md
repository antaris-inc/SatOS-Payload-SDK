# Example C++ Payload Application

This directory contains an example Payload Application using the C++ API.
This is not production-ready code, but it is useful for learning how the platform functions.

## Quickstart

In the Antaris Cloud Platform, create a TrueTwin Satellite with a remote payload and download the associated config.
Place that downloaded zip file in this directory.

Build the C++ app using the following steps:

```bash
cd examples/app-cpp

# Builds your standalone application first 
docker build --platform=linux/amd64 -t satos-payload-example-app-cpp .
```

Next, we can run the payload application in the container. The command below assumes that `$CONFIG` is set to the name of the config file (zip) you downloaded from Antaris Cloud Platform. The file must be located in your current working directory:

```bash
CONFIG=your_remote_config_from_acp.zip

docker run --platform=linux/amd64 -e CONFIG=$CONFIG --privileged -v $(pwd):/workspace -it satos-payload-example-app-cpp
```

You may now use the Antaris Cloud Platform to submit payload sequences. For example, submit a `HelloWorld` payload with
no parameters and you will see it reflected in the container logs.

The sequences supported by the example are described below:
* `HelloWorld`: takes no parameters and simply logs "hello, world!"
* `HelloFriend`: accepts parameter, logging "hello, <parameter>!"
* `LogLocation`: queries the current satellite location and logs it
