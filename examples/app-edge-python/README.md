# Example Python SatOS Application for Edge Server

This directory contains an example Payload Application using the Python.
This is not production-ready code, but it is useful for learning how the platform functions.

## Quickstart

In the Antaris Cloud Platform, create a TrueTwin Satellite with a remote payload and download the associated config.
Place that downloaded zip file in this directory.

Build the app using the following command:

```
docker build -t satos-payload-example-app-python-cuda .
```

Next, we can run the application in a container. The command below assumes that `$CONFIG` is set to the name of the config file (zip) you downloaded from Antaris Cloud Platform. The file must be located in your current working directory. The below command mount the host cuda inside the container and grant the container gpu access if available:

```
docker run --rm -e CONFIG=$CONFIG --gpus all --privileged   -v $(pwd):/workspace   -v /usr/local/cuda:/usr/local/cuda:ro   -e LD_LIBRARY_PATH=/usr/local/cuda/lib64:/usr/local/cuda/extras/CUPTI/lib64:$LD_LIBRARY_PATH   -it satos-payload-example-app-python-cuda
```

You may now use the Antaris Cloud Platform to submit payload sequences. For example, submit a `HelloWorld` payload with
no parameters and you will see it reflected in the container logs.

The sequences supported by the example are described below:
* `HelloWorld`: takes no parameters and simply logs "hello, world! and after that it checks if gpu is available and if available it runs a simple code "cuda_kernel on gpu"

```
