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

Next, we can run the application in a container. The command below assumes that `CONFIG` is set to the name of the downloaded file in your current working directory:

```
docker run --platform=linux/amd64 -e CONFIG=$CONFIG -v $(pwd):/workspace -it satos-payload-example-app-python 
```

You may now use the Antaris Cloud Platform to submit payload sequences. For example, submit a `HelloWorld` payload with
no parameters and you will see it reflected in the container logs.

The sequences supported by the example are described below:
* `HelloWorld`: takes no parameters and simply logs "hello, world!"
* `HelloFriend`: accepts parameter, logging "hello, <parameter>!"
* `LogLocation`: queries the current satellite location and logs it

## Running Tests

Some local sanity tests are included to confirm the example application is functional.
To run these tests, first build the container image as documented above.
Now, simply run the image and use `pytest` to execute the tests:

```
docker run --platform=linux/amd64 -v $PWD:/workspace -w /workspace -t satos-payload-example-app-python /bin/bash -c "pip3 install pytest && pytest"
```