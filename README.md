# Antaris SatOS SDK

This SDK provides libraries, tools and documentation that support developing satellite on-board applications for use with SatOS (TM) from Antaris, Inc.
Specifically, this enables the development and testing of payload applications: the software that handles communication with core spacecraft services as well as payload-specific devices.

[Comprehensive documentation](https://antaris-inc.github.io/SatOS-Payload-SDK/index.html) is available online. Please read through the [Developer Guide](https://antaris-inc.github.io/SatOS-Payload-SDK/developer-guide.html) if this is your first time working with this SDK, as it will help you learn the terminology and concepts used herein.

## Prerequisites

Before you continue, it is assumed that:
1. You have Docker installed on your local machine
2. You have access to the Antaris Cloud Platform.

If you do not have access to the cloud platform, you can still get started with the SDK but you will be limited in what can be accomplished.

## Quickstart

If you have read through the Developer Guide, simply visit the [Python example application](./examples/app-python/) to quickly get a sample Payload Application up and running locally.

## Developing Applications

The development processes available here utilize Docker containers.
It is expected that developers start from one of the supported base images:

* Python applications: `quay.io/antaris-inc/satos-payload-app-python:stable`
* C++ application: quay.io/antaris-inc/satos-payload-app-cpp:stable

A developer does not even need to clone this repository in order to build an application, however there are examples
here that are very helpful when getting started.
Visit the `examples/` directory here for more information.

Additional sample payload applications are maintained in a separate repository that actually integrate payload hardware for
demonstration purposes: https://github.com/antaris-inc/SatOS-Payload-Demos

### Build Example C++ Application

  `make build_cpp` will instantiate a build environment container and compile c++ application present at examples/app-cpp.

```
make build_cpp
```
NOTE: This step also builds the requisite Docker image which requires internet access to install some system packages. 
If your application's source location is different then you need to provide the path in the variable **SRC_DIR** inside `MakeFile`. Please note that this path must exist within SatOS-Payload-SDK directory only.

		
## Tools Image

This repo utilizes a container image containing a number of tools (e.g. scripts, software, etc).
The tools image is used to provide a consistent environment for automation of tasks in CI.
For example, the image is used to generate API bindings from protobuf schemas and package those bindings into client libraries.
The image is automatically built and published at `quay.io/antaris-inc/satos-payload-sdk-tools:stable`.

Typically, one can run the tools image with a command like the following. It is expected that the root of the SDK repo is mounted in to `/workspace`:

```
docker run -v $PWD:/workspace -it quay.io/antaris-inc/satos-payload-sdk-tools:stable
```

If you are interested in making changes to the image, the Dockerfile resides at `images/sdk-tools/Dockerfile`.
A `stable` tag is managed by hand in the image repository.
