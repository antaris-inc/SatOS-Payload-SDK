FROM ubuntu:22.04

RUN apt-get update

# install dependencies required to run pc-sim, PC, other servers, clients and proxy
RUN apt-get install -y python3 

ARG PROXY_AGENT=${WORKSPACE_ROOT}/tools/proxy-agent

ARG CONTAINER_ASSET_ROOT=/assets

RUN mkdir -p ${CONTAINER_ASSET_ROOT}/proxy-agent 

ADD ${PROXY_AGENT} ${CONTAINER_ASSET_ROOT}/proxy-agent

ENV PYTHONPATH=/assets/lib/python:/assets/lib/python/gen

WORKDIR ${CONTAINER_ASSET_ROOT}

SHELL [ "/bin/bash", "-c" ]

CMD ["sleep", "infinity"]

# ENTRYPOINT ["/usr/bin/python3", ]
# ENTRYPOINT [ "/usr/bin/python3", "${PROXY_AGENT}/agent.py", "-i", "172.155.155.2", "-p", "19051", "-s", "127.0.0.1", "-l", "127.0.0.1" "-o", "50051" ]
ENTRYPOINT [ "/usr/bin/python3", "/assets/proxy-agent/agent.py", "-m", "user", "-i", "0.0.0.0", "-p", "19051", "-l", "127.0.0.1", "-s", "127.0.0.1", "-t", "50051", "-o", "50053"]
