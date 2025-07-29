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
RUN apt update && apt install -y build-essential

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
