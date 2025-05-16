#!/bin/bash
# This script will create docker image for updating in satellite. It will create upgrade_artifact.tar
# that contains docker layers which are changed from base image.
# Input:
#       -i = tarball of new docker image
#       -r = tarball of base docker image
 
DIR_PREFIX="/opt/antaris"
BLOB_DIR="blobs/sha256"
 
function check_docker_version() {
  # Check if Docker is installed
  if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed." >&2
    return 1
  fi
 
  # Extract the Docker version (e.g., 25.0.1)
  docker_version=$(docker --version | grep -oP '\d+\.\d+\.\d+')
  if [[ -z "$docker_version" ]]; then
    echo "Error: Could not parse Docker version." >&2
    return 1
  fi
 
  # Split version into major, minor, patch
  IFS='.' read -r major minor patch <<< "$docker_version"
 
  # Check if version >= 25.0.0
  if (( major > 25 )) ||
     (( major == 25 && minor >= 0 && patch >= 0 )); then
    return 0
  else
    echo "Error: Docker version $docker_version is too old. Minimum required is 25.0.0." >&2
    return 1
  fi
}
 
 
function print_help() {
    echo "$0: Usage is"
    echo "$0 -f <factory image tarball>  -u <update image tarball> "
    exit 1
}
 
if [[ $# -eq 0 ]]; then
    print_help
    exit 1
fi
 
while getopts "u:f:h" arg; do
  case $arg in
    h)
      print_help
      ;;
    f)
      FACTORY_IMAGE=${OPTARG}
      ;;
    u)
      UPDATE_IMAGE=${OPTARG}
      ;;
    *)
      print_help
      ;;
  esac
done
 
# Use the function and exit if requirement is not met
if check_docker_version; then
  echo "Docker version check passed. (Docker version>=25.0.0)"
else
  echo "Docker version must be greater than 25.0.0"
  exit 1
fi
 
TEMP_DIR="/tmp"
 
echo "Deleting old directories and re-creating again"
UPGRADE_DIR="${TEMP_DIR}/upgrade"
FACTORY_DIR="${TEMP_DIR}/ref"
OUTPUT_DIR="${TEMP_DIR}/output"
 
rm -rf ${UPGRADE_DIR} ${FACTORY_DIR} ${OUTPUT_DIR}
mkdir -p ${UPGRADE_DIR} ${FACTORY_DIR} ${OUTPUT_DIR}
 
# Step 1: Extract factory image
echo "Extracting ${FACTORY_IMAGE} in ${FACTORY_DIR} directory"
tar -xf ${FACTORY_IMAGE} -C ${FACTORY_DIR}
 
# Step 2: Extract docker image to upgrade
echo "Extracting ${UPDATE_IMAGE} in ${UPGRADE_DIR} directory"
tar -xf ${UPDATE_IMAGE} -C ${UPGRADE_DIR}/
 
UPGRADE_BLOB=${UPGRADE_DIR}/${BLOB_DIR}
OUTPUT_BLOB=${OUTPUT_DIR}/${BLOB_DIR}
 
mkdir -p ${OUTPUT_BLOB}
 
# Copy non-common files from ref image to output
diff -rq "${UPGRADE_BLOB}" "${FACTORY_DIR}/${BLOB_DIR}" | grep "Only in ${UPGRADE_DIR}" | awk '{print $4}' | xargs -I {} cp -rf "${UPGRADE_BLOB}/{}" "${OUTPUT_BLOB}"
 
# Copy manifest and repositories file
cp ${UPGRADE_DIR}/*.json ${OUTPUT_DIR}/
cp ${UPGRADE_DIR}/repositories ${OUTPUT_DIR}/
 
tar -cf Upgrade_artifact.tar -C ${OUTPUT_DIR} .
echo "Use Upgrade_artifact.tar in your local directory"