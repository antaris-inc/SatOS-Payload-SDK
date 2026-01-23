#!/bin/bash

set -e  # Exit immediately on error

echo "===== Local Payload App Setup Script ====="

############################
# Step 1: Get config.json
############################
read -rp "Enter full path of config.json: " config_json_path

if [[ ! -f "$config_json_path" ]]; then
    echo "ERROR: config.json not found!"
    exit 1
fi

command -v jq >/dev/null 2>&1 || {
    echo "ERROR: jq is required but not installed"
    exit 1
}

############################
# Step 2: Extract values
############################
export APP_ID=$(jq -r '.ID.App_Id' "$config_json_path")
export NAME=$(jq -r '.ID.Name' "$config_json_path")
export PAYLOAD_HW_ID=$(jq -r '.ID.Payload_HW_Id' "$config_json_path")

export VM_NAME=$(jq -r '.VM_Info.VM_Name' "$config_json_path")

export BRIDGE_NAME=$(jq -r '.Network.Bridge_Name' "$config_json_path")
export VM_IP_ADDRESS=$(jq -r '.Network.VM_IP_Address' "$config_json_path")
export APP_CONTROLLER_IP=$(jq -r '.Network.Application_Controller_IP_Address' "$config_json_path")
export COOKIE=$(jq -r '.cookie' "$config_json_path")

# Delete last 3 characters of COOKIE
COOKIE="${COOKIE::-3}"

# NAME in lowercase
name=$(echo "$NAME" | tr '[:upper:]' '[:lower:]')

BASE_DIR="./$NAME"

echo "Application directory: $BASE_DIR"

############################
# Step 3: Create folders
############################
mkdir -p \
    "$BASE_DIR/app/conf" \
    "$BASE_DIR/config" \
    "$BASE_DIR/inbound" \
    "$BASE_DIR/outbound" \
    "$BASE_DIR/factory_restore" \
    "$BASE_DIR/RecycleBin"

echo "Directories created."

############################
# Step 4: Create sdk_env.conf
############################
cat > "$BASE_DIR/app/conf/sdk_env.conf" <<EOF
# see documentation for supported variables
PAYLOAD_CONTROLLER_IP=$APP_CONTROLLER_IP
SSL_FLAG=0
KEEPALIVE=0
EOF

echo "sdk_env.conf created."

############################
# Step 5: Copy payload_run.sh
############################
read -rp "Enter full path of payload_run.sh: " payload_run_path

if [[ ! -f "$payload_run_path" ]]; then
    echo "ERROR: payload_run.sh not found!"
    exit 1
fi

cp "$payload_run_path" "$BASE_DIR/app/"
chmod +x "$BASE_DIR/app/payload_run.sh"

############################
# Step 6: Copy create-network script
############################
read -rp "Enter full path of create-network script: " create_net_script

if [[ ! -f "$create_net_script" ]]; then
    echo "ERROR: create-network script not found!"
    exit 1
fi

cp "$create_net_script" "$BASE_DIR/app/${name}_create_network.sh"
chmod +x "$BASE_DIR/app/${name}_create_network.sh"

############################
# Step 7: Copy delete-network script
############################
read -rp "Enter full path of delete-network script: " delete_net_script

if [[ ! -f "$delete_net_script" ]]; then
    echo "ERROR: delete-network script not found!"
    exit 1
fi

cp "$delete_net_script" "$BASE_DIR/app/${name}_delete_network.sh"
chmod +x "$BASE_DIR/app/${name}_delete_network.sh"

############################
# Step 8: Docker image tarball
############################
read -rp "Enter Docker image name (e.g. repo/image:tag): " docker_image

if ! docker image inspect "$docker_image" >/dev/null 2>&1; then
    echo "ERROR: Docker image not found locally!"
    exit 1
fi

docker save "$docker_image" \
    -o "$BASE_DIR/factory_restore/${name}_image.tar"

echo "Docker image tarball created."

############################
# Step 9: Create insert.sql
############################
SQL_FILE="$BASE_DIR/app/insert.sql"

cat > "$SQL_FILE" <<EOF
INSERT INTO app_config
(name, platform, short_app_id, hw_id, image, dir_prefix, network, ip, api_port, auth_key)
VALUES
('$NAME', 'docker', $APP_ID, $PAYLOAD_HW_ID, '$VM_NAME', '/home/$NAME/', '$BRIDGE_NAME', '$VM_IP_ADDRESS', 50053, '$COOKIE');
EOF

echo "insert.sql created at $SQL_FILE"

############################
# Step 10: Zip <NAME> folder
############################
cp "$config_json_path" "$BASE_DIR/app/"

############################
# Step 10: Zip <NAME> folder
############################
ZIP_FILE="./${NAME}.zip"

zip -r "$ZIP_FILE" "$BASE_DIR" >/dev/null

echo "ZIP archive created: $ZIP_FILE"

echo "===== Setup Completed Successfully ====="

