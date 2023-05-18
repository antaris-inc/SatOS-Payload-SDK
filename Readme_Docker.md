## Description

This document describes 'antaris_payload' docker image with sample payload application that executes 'get_location'

## Launch container
```bash
$ docker run --name remote_payload  -it antaris_payload /bin/bash
```
It will start container.

## Copy true-twin remote payload package in container
```bash
$ docker cp <Package Name> remote_payload:/root/
```

## Running remote application
Following command should be run from container
# Step 1: Unzip package
```docker bash
root<container>:/# unzip <zip name>
```

# Step 2: Install payload package
```docker bash
root<container>:/# dpkg -i /root/<Package Name>
```

# Step 3: Start proxy
```docker bash
root<container>:/# bash /opt/antaris/proxy/start_proxy.sh
```

# Step 4: Run sample application
```docker bash
root<container>:/# python3 /root/payload_app.py
```
