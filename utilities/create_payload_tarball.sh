#!/bin/bash

set -e  # Exit immediately on error

echo "===== Local Payload App Setup Script ====="

ask_yes_no() {
    local prompt="$1"
    while true; do
        read -rp "$prompt (y/n): " yn
        case "$yn" in
            [Yy]*) return 0 ;;
            [Nn]*) return 1 ;;
            *) echo "Please answer y or n." ;;
        esac
    done
}

generate_payload_script() {

    OUTPUT_FILE="payload_run.sh"

    # -----------------------------
    # Function arguments
    # -----------------------------
    APP_KEYWORD="$1"
    APP_DIR="$2"
    EXTRA_DOCKER_MOUNTS="$3"
    DOCKER_IMAGE="$4"

    NETWORK_CREATE_SCRIPT="/home/${APP_DIR}/app/${APP_KEYWORD}_create_network.sh"
    NETWORK_DELETE_SCRIPT="/home/${APP_DIR}/app/${APP_KEYWORD}_delete_network.sh"

    ENV_VARIABLES="-e RUN_SDK_AGENT="

    # -----------------------------
    # Generate payload_run.sh
    # -----------------------------
    cat > "$OUTPUT_FILE" <<EOF
#!/bin/bash

APP_NAME="\$1"
IMAGE_NAME="\$2"
ACTION="\$3"

APP_NAME_LOWER=\$(echo "\$APP_NAME" | tr '[:upper:]' '[:lower:]')
ACTION_LOWER=\$(echo "\$ACTION" | tr '[:upper:]' '[:lower:]')

if [[ "\$APP_NAME_LOWER" == "${APP_KEYWORD}" ]]; then

    if [[ "\$ACTION_LOWER" == "on" ]]; then

        bash ${NETWORK_CREATE_SCRIPT}

        docker run --privileged \\
            -v /home/${APP_DIR}/inbound:/opt/antaris/inbound \\
            -v /home/${APP_DIR}/config:/opt/antaris/config \\
            -v /home/${APP_DIR}/outbound:/opt/antaris/outbound \\
            -v /home/${APP_DIR}/app/config.json:/opt/antaris/app/config.json \\
            -v /home/${APP_DIR}/app/conf/sdk_env.conf:/opt/antaris/app/conf/sdk_env.conf \\
            ${EXTRA_DOCKER_MOUNTS} \\
            --network "\$5" --ip "\$6" \\
            --platform=linux/amd64 \\
            ${ENV_VARIABLES} \\
            -d --rm -it ${DOCKER_IMAGE}

    elif [[ "\$ACTION_LOWER" == "off" ]]; then

        CONTAINER_ID=\$(docker ps -qf ancestor="\${IMAGE_NAME}")

        if [[ -n "\$CONTAINER_ID" ]]; then
            docker stop "\$CONTAINER_ID"
            docker rm "\$CONTAINER_ID"
            echo "Container stopped and removed."
        else
            echo "No running container found."
        fi

        bash ${NETWORK_DELETE_SCRIPT}
    fi

fi
EOF

    chmod +x "$OUTPUT_FILE"

    echo "payload_run.sh generated successfully."
}

generate_network_create_script() {

    APP_KEYWORD="$1"
    USER_IP="$2"

    OUTPUT_FILE="${APP_KEYWORD}_create_network.sh"

    # Extract network prefix (first 3 octets)
    NETWORK_PREFIX=$(echo "$USER_IP" | awk -F. '{print $1"."$2"."$3}')

    GATEWAY_IP="${NETWORK_PREFIX}.1"
    SUBNET="${NETWORK_PREFIX}.0/24"

    cat > "$OUTPUT_FILE" <<EOF
#!/bin/bash

TARGET_IP="\$1"

if [[ -z "\$TARGET_IP" ]]; then
    echo "Usage: \$0 <host_ip>"
    exit 1
fi

# Find interface associated with the target IP
iface=\$(ip -o -4 addr show | awk -v ip="\$TARGET_IP" '\$4 ~ ip {print \$2}')

if [ -z "\$iface" ]; then
    echo "No interface found with IP \$TARGET_IP"
    exit 1
fi

echo "Interface with IP \$TARGET_IP is: \$iface"

MACVLAN_IFACE="\${iface}.10"

if ip link show "\$MACVLAN_IFACE" &> /dev/null; then
    echo "Macvlan interface \$MACVLAN_IFACE already exists"
else
    echo "Creating macvlan interface \$MACVLAN_IFACE"

    sudo ip link add "\$MACVLAN_IFACE" link "\$iface" type macvlan mode bridge
    sudo ip addr add ${GATEWAY_IP}/32 dev "\$MACVLAN_IFACE"
    sudo ip link set "\$MACVLAN_IFACE" up
    sudo ip route add ${NETWORK_PREFIX}.0/24 dev "\$MACVLAN_IFACE"
fi

# Create Docker macvlan network
if docker network ls | grep -q '${APP_KEYWORD}'; then
    echo "Docker network '${APP_KEYWORD}' already exists"
else
    echo "Creating Docker macvlan network '${APP_KEYWORD}'"

    docker network create \
        --driver=macvlan \
        --subnet=${SUBNET} \
        -o parent="\$MACVLAN_IFACE" \
        ${APP_KEYWORD}
fi
EOF

    chmod +x "$OUTPUT_FILE"

    echo "Network script generated: $OUTPUT_FILE"
    echo "Subnet: $SUBNET"
    echo "Gateway: $GATEWAY_IP"
}

generate_network_delete_script() {

    APP_KEYWORD="$1"
    USER_IP="$2"

    OUTPUT_FILE="${APP_KEYWORD}_delete_network.sh"

    # Extract first 3 octets
    NETWORK_PREFIX=$(echo "$USER_IP" | awk -F. '{print $1"."$2"."$3}')

    MACVLAN_IP="${NETWORK_PREFIX}.1/32"
    ROUTE="${NETWORK_PREFIX}.0/24"

    cat > "$OUTPUT_FILE" <<EOF
#!/bin/bash

TARGET_IP="\$1"

if [[ -z "\$TARGET_IP" ]]; then
    echo "Usage: \$0 <host_ip>"
    exit 1
fi

# Find interface associated with the target IP
iface=\$(ip -o -4 addr show | awk -v ip="\$TARGET_IP" '\$4 ~ ip {print \$2}')

if [ -z "\$iface" ]; then
    echo "No interface found with IP \$TARGET_IP"
    exit 1
fi

MACVLAN_IF="\${iface}.10"
MACVLAN_IP="${MACVLAN_IP}"
ROUTE="${ROUTE}"
DOCKER_NET="${APP_KEYWORD}"

echo "Detected interface: \$iface"
echo "Deleting macvlan interface: \$MACVLAN_IF"

# Delete route if it exists
if ip route show | grep -q "\$ROUTE"; then
    echo "Deleting route \$ROUTE..."
    sudo ip route del "\$ROUTE" dev "\$MACVLAN_IF"
else
    echo "Route \$ROUTE not found. Skipping."
fi

# Bring down and delete macvlan interface
if ip link show "\$MACVLAN_IF" &> /dev/null; then
    echo "Bringing down and deleting interface \$MACVLAN_IF..."
    sudo ip link set "\$MACVLAN_IF" down
    sudo ip addr del "\$MACVLAN_IP" dev "\$MACVLAN_IF"
    sudo ip link delete "\$MACVLAN_IF"
else
    echo "Interface \$MACVLAN_IF does not exist. Skipping."
fi

# Remove Docker network if Docker is running
if systemctl is-active --quiet docker; then
    if docker network ls --format '{{.Name}}' | grep -q "^\$DOCKER_NET\$"; then
        echo "Removing Docker network \$DOCKER_NET..."
        docker network rm "\$DOCKER_NET"
    else
        echo "Docker network \$DOCKER_NET does not exist. Skipping."
    fi
else
    echo "Docker service is not running. Skipping Docker network deletion."
fi

EOF

    chmod +x "$OUTPUT_FILE"

    echo "Delete network script generated: $OUTPUT_FILE"
    echo "Route: $ROUTE"
    echo "Macvlan IP: $MACVLAN_IP"
}

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
echo 
if ask_yes_no "Do you have payload_run.sh ? [y/n]:"; then
    read -rp "Enter full path of payload_run.sh: " payload_run_path

    if [[ ! -f "$payload_run_path" ]]; then
        echo "ERROR: payload_run.sh not found!"
        exit 1
    fi

    cp "$payload_run_path" "$BASE_DIR/app/"
    chmod +x "$BASE_DIR/app/payload_run.sh"
elif ask_yes_no "Do you want to create payload_run.sh ? [y/n]:"; then
    read -rp "Enter Docker image name (e.g. repo/image:tag): " DOCKER_IMAGE
    read -rp "Enter extra docker run command parameters: " EXTRA_MOUNTS
    generate_payload_script ${name} ${NAME} "$EXTRA_MOUNTS" "$DOCKER_IMAGE"
    cp payload_run.sh "$BASE_DIR/app/"
    chmod +x "$BASE_DIR/app/payload_run.sh"
fi 

############################
# Step 6: Copy create-network script
############################
echo 

if ask_yes_no "Do you have create network script ? [y/n]:"; then
    read -rp "Enter full path of create-network script: " create_net_script

    if [[ ! -f "$create_net_script" ]]; then
        echo "ERROR: create-network script not found!"
        exit 1
    fi

    cp "$create_net_script" "$BASE_DIR/app/${name}_create_network.sh"
    chmod +x "$BASE_DIR/app/${name}_create_network.sh"

elif ask_yes_no "Do you want to create network scripts ? [y/n]:"; then
    if [ "$BRIDGE_NAME" != "host" ]; then
        generate_network_create_script ${name} ${VM_IP_ADDRESS}
        cp "${name}_create_network.sh" "$BASE_DIR/app/${name}_create_network.sh"
        chmod +x "$BASE_DIR/app/${name}_create_network.sh"
    fi
fi 

############################
# Step 7: Copy delete-network script
############################
echo 

if ask_yes_no "Do you have delete network script ? [y/n]:"; then
    read -rp "Enter full path of delete-network script: " delete_net_script

    if [[ ! -f "$delete_net_script" ]]; then
        echo "ERROR: delete-network script not found!"
        exit 1
    fi

    cp "$delete_net_script" "$BASE_DIR/app/${name}_delete_network.sh"
    chmod +x "$BASE_DIR/app/${name}_delete_network.sh"
elif ask_yes_no "Do you want to create delete_network scripts ? [y/n]:"; then
    if [[ "$BRIDGE_NAME" != "host" ]]; then
        generate_network_delete_script "${name}" "$VM_IP_ADDRESS"
        cp "${name}_delete_network.sh" "$BASE_DIR/app/${name}_delete_network.sh"
        chmod +x "$BASE_DIR/app/${name}_delete_network.sh"
    fi
fi 

############################
# Step 8: Docker image tarball
############################
echo "Creating tarball of docker image"
if ! docker image inspect "$DOCKER_IMAGE" >/dev/null 2>&1; then
    echo "ERROR: Docker image not found locally!"
    exit 1
fi

docker save "$DOCKER_IMAGE" \
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
echo 
echo "Creating zip file"
ZIP_FILE="./${NAME}.zip"

zip -r "$ZIP_FILE" "$BASE_DIR" >/dev/null

echo "ZIP archive created: $ZIP_FILE"

echo "===== Setup Completed Successfully ====="

