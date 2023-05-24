# Antaris SatOS Payload SDK

This SDK provides libraries, tools and documentation that support developing satellite payloads for use with SatOS (TM) from Antaris, Inc.
Specifically, this enables the development and testing of payload applications: the software that handles communication with core spacecraft services as well as payload-specific devices.

It is recommended to first read through the [SatOS Payload SDK Guide](./docs/Antaris_SatOS_Payload_SDK_Guide.pdf) to familiarize yourself with the concepts and expected operations of payloads within the SatOS platform.

## Local Setup

Before you continue, it is assumed that:
1. You have Docker installed on your local machine
2. You have access to the Antaris Cloud Platform.

If you do not have access to the cloud platform, you can still get started with the SDK but you will be limited in what can be accomplished.

## Quickstart

Visit the [Python example application](./examples/app-python/) to quickly get a working example up and running locally.

## Developing Payload Applications

The development processes available here utilize Docker containers.
It is expected that developers start from one of the supported base images:

* Python applications: `quay.io/antaris-inc/satos-payload-app-python:stable`
* C++ application: quay.io/antaris-inc/satos-payload-app-cpp:stable

A developer does not even need to clone this repository in order to build an application, however there are examples
here that are very helpful when getting started.
Visit the `examples/` directory here for more information.

Additional payload applications are maintained in a separate repository that actually integrate payload hardware for
demonstration purposes: https://github.com/antaris-inc/SatOS-Payload-Demos
