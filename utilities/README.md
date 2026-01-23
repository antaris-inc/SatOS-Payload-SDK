# Overview

List of utilities provided:
1. Creating satos_sdk application tarball
2. Create docker artifact for upgrading satos_sdk payload docker image

##  Creating satos_sdk application tarball

This utility creates tarball of satos_sdk payload application which can be installed in Satellite 
for add new payload application.

# Suggested Directory Structure
```bash
.
.
├── app
│   └── config.json
├── factory_restore
│   ├── docker image tarball
├── inbound
├── outbound
└── config
```
5 directories

# Running a script
```bash 
HOST $ bash create_payload_tarball.sh
```
##  Creating upgrade artifact for upgrading satos_sdk application 

This utility creates upgrade_artifact of satos_sdk payload application which can be used in Satellite 
for updating payload application.

This utility requires factory image of satos_sdk payload application docker image to be saved at 
factory_restore folder.

Note: Docker version should be >= 25.0.0

# Running a script
```bash 
HOST $ bash create_upgrade_docker_artifact_tar.sh
```