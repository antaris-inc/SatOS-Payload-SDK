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

# FROM quay.io/antaris-inc/satos-payload-app-python:stable AS app_image

# ADD example_app.py /opt/antaris/app/example_app.py
# ADD entrypoint.sh /opt/antaris/app/entrypoint.sh
# RUN mkdir -p /opt/antaris/outbound -p /opt/antaris/inbound

FROM quay.io/antaris-inc/satos-payload-app-python:v2.0.6 AS app_image

RUN pip install --no-cache-dir numpy numba cupy-cuda12x

# Add your application code
ADD example_app.py /opt/antaris/app/example_app.py
ADD entrypoint.sh /opt/antaris/app/entrypoint.sh

RUN chmod +x /opt/antaris/app/entrypoint.sh

# Create required directories
RUN mkdir -p /opt/antaris/outbound /opt/antaris/inbound
