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

Use `make build_env` to instantiate a build environment container and open a new shell within it.

NOTE: This step also builds the requisite Docker image which requires internet access to install some system packages.

### Compile SDK Assets

Run `make all` within the build environment to compile all dependent libraries and sample applications.

## Quickstart: Run Sample Application

Note that the following steps are executed from inside the build environment.

### Start Payload Controller

The simulated Payload Controller must be running to provide applications an upstream source of sequences/commands.
Start it with the following command:

```bash
$ PYTHONPATH=/workspace/lib/python/satos_payload:/workspace/lib/python/satos_payload/gen python3 pc-sim/invoke_pc_sim.py
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

## HOWTO: Run Custom Application

Note that you may configure the sequences sent from the controller to connected applications via `pc-sim/sequence_schedule.json`.

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

## HOWTO: Package & Verify Application

After you have completed development of your payload application, you must package it for
integration into a SatOS-powered satellite.

This packaging step should be taken from within the build container:

```bash
$ make payload_app_pkg
```

Upon completion, a debian package containing your payload application will be available in `/workspace/output`.

### Verify Packaged Application

To verify your application was packaged appropriately, you can test installation into a payload app VM environment like

that which will be available in the production satellite.

Assuming you have already packaged your application, the only additional step needed is to package the SDK itself.
Run the following command in the build container to produce another debian package in `/workspace/output`:

```bash
$ make sdk_pkg
```

Now, take the following steps to confirm proper packaging:
1. Provision a VM using Ubuntu 22.04 on your choice of virtualization platforms
2. Copy the debian packages from the build container's workspace to the VM
2. Install the debian packages using `apt install`

## Reference

### SDK Configuration

The SDK allows some control over its behavior using a configuration file.
By default, the configuration file is located at `/opt/antaris/sdk/lib/conf/sdk_env.conf`.
The location can be overridden via the `ANTARIS_ENV_CONF_FILE` environment variable.

The currently supported environment variables that can be overridden using the config file are:

| Name                  | Default |
|-----------------------|---------|
| PAYLOAD_CONTROLLER_IP | 127.0.0.1 |
| PAYLOAD_APP_IP        | 127.0.0.1 |
