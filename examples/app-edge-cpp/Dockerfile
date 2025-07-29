# Copyright 2023 Antaris, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Stage 1: Build dependencies.
FROM quay.io/antaris-inc/satos-payload-app-cpp:v25.3 AS dependencies

# Install prerequisites: wget, gnupg2, software-properties-common, and build-essential
RUN apt-get update && apt-get install -y \
    wget \
    gnupg2 \
    ca-certificates \
    software-properties-common \
    build-essential

RUN wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/sbsa/cuda-keyring_1.0-1_all.deb && \
    dpkg -i cuda-keyring_1.0-1_all.deb && \
    rm cuda-keyring_1.0-1_all.deb

RUN echo "deb [signed-by=/usr/share/keyrings/cuda-archive-keyring.gpg] https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/sbsa/ /" > /etc/apt/sources.list.d/cuda.list

RUN apt-get update && apt-get install -y cuda-toolkit-12-2
ENV PATH=/usr/local/cuda/bin:${PATH}

RUN nvcc --version


# Stage 2: Build your payload app.
FROM dependencies AS builder
WORKDIR /workspace
COPY . .
RUN make payload_app

# Stage 3: Runtime image
FROM quay.io/antaris-inc/satos-payload-app-cpp:v25.3
COPY --from=builder /workspace/payload_app /opt/antaris/app/payload_app
COPY entrypoint.sh /opt/antaris/app/entrypoint.sh
RUN mkdir -p /opt/antaris/outbound /opt/antaris/inbound
