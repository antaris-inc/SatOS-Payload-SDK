# Antaris SatOS Payload SDK

This SDK provides libraries, tools and documentation that support developing satellite payloads for use with SatOS (TM) from Antaris, Inc.
Specifically, this enables the development and testing of payload applications: the software that handles communication with core spacecraft services as well as payload-specific devices.

It is recommended to first read through the [SatOS Payload SDK Guide](./docs/Antaris_SatOS_Payload_SDK_Guide.pdf) to familiarize yourself with the concepts and expected operations of payloads within the SatOS platform.

## Quickstart: SDK Dependencies

Before you continue, the following depdencies must be available on your local machine:

- docker (tested with docker version 20.10.12)
- make (tested with version GNU Make 4.2.1)

The SDK has been tested on both Linux- and OSX-based machines.

## Quickstart: Environment Setup

Before application development can begin, you must first create a build environment.
The SDK utilizes Docker to provide a reusable, portable build environment via containerization.

### Launch Build Environment

Use `make build_env` to instantiate a build environment container and open a new shell within it:

NOTE: This step also builds the requisite Docker image, and requires internet access to install some system packages.

### Compile SDK Assets

Run `make all` within the build environment to compile all dependent libraries and sample applications:

## Quickstart: Run Sample Application

Note that the following steps are executed from inside the build environment.

### Start Payload Controller

The simulated Payload Controller must be running to provide applications an upstream source of sequences/commands.
Start it with the following command:

```bash
$ python3 pc-sim/invoke_pc_sim.py
```

### Start Sample Application

Before starting an application, you will likely need to open another shell within your active build environment:

```bash
$ make build_env_shell
```

Now from within the container, execute the following command to start the Python-based payload application:

```bash
$ python3 apps/samples/python/payload_app.py
```

Alternatively, the C++ sample application can also be run:

```bash
$ apps/samples/cpp/payload/payload_app
```

## Quickstart: Run Custom Application

Note that you may configure the sequences sent from the controller to connceted applications via `pc-sim/sequence_schedule.json`.

### Rebuild an Application

To build sample application or your own application, following commands can be run in the build environment:

1. To rebuild an application:

```bash
$ make sample_app LANGUAGE=cpp
```

2. To delete and clean up application :

```bash
$ make sample_app_clean
```
