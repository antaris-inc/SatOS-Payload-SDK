# Example C++ Payload Application

This directory contains an example Payload Application using the C++.
This is not production-ready code, but it is useful for learning how the platform functions.

NOTE: All the cpp files need to have .cc extention only.


## Build Cpp application

To build application in examples/app-cpp , go to SatOS_Payload_SDK parent directory and run `make build_cpp`.

`make build_cpp` will instantiate a build environment container and compile cpp application present at examples/app-cpp. For other locations please refer the main README iniside SatOS-Payload-SDK.

```
cd ../.. 
make build_cpp
```

## Quickstart

In the Antaris Cloud Platform, create a TrueTwin Satellite with a remote payload and download the associated config.
Place that downloaded zip file in this directory.

Build the app using the following command:

```
docker build --platform=linux/amd64 -t satos-payload-example-app-cpp .
```

Next, we can run the application in a container. The command below assumes that `$CONFIG` is set to the name of the config file (zip) you downloaded from Antaris Cloud Platform. The file must be located in your current working directory:

```
docker run --platform=linux/amd64 -e CONFIG=$CONFIG --privileged -v $(pwd):/workspace -it satos-payload-example-app-cpp
```

You may now use the Antaris Cloud Platform to submit payload sequences. For example, submit a `HelloWorld` payload with
no parameters and you will see it reflected in the container logs.

The sequences supported by the example are described below:
* `HelloWorld`: takes no parameters and simply logs "hello, world!"
* `HelloFriend`: accepts parameter, logging "hello, <parameter>!"
* `LogLocation`: queries the current satellite location and logs it
