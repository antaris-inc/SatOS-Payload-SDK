#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <thread>
#include <mutex>

#include "antaris_can_api.h"
#include "antaris_api_common.h"
#include "antaris_sdk_environment.h"

const int GPIO_ERROR = -1;

// Constructor
AntarisApiCAN::AntarisApiCAN() 
{
    for (int i = 0; i < MAX_CAN_DEVICES; i++) {
        receiver_running.push_back(false);    
        can_mutex.emplace_back(std::make_unique<std::mutex>()); // Use unique_ptr for mutex
    }
}

// Destructor
AntarisApiCAN::~AntarisApiCAN() {
    for (auto& t : receiver_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

// Circular buffer functions
bool CircularBuffer::push(const struct can_frame& frame) 
{
    size_t next = (end + 1) % MAX_CAN_MESSAGES;
    
    // Return error if buffer is full
    if (next == start) { 
        return false;
    }
    
    buffer[end] = frame;
    end = next;
    return true;
}

bool CircularBuffer::pop(struct can_frame& frame) 
{
    // return error if buffer is empty
    if (start == end) {  
        return false;
    }

    frame = buffer[start];
    start = (start + 1) % MAX_CAN_MESSAGES;
    return true;
}

size_t CircularBuffer::count() const 
{
    return (end >= start) ? (end - start) : (MAX_CAN_MESSAGES - start + end);
}

// Get available CAN devices
AntarisReturnCode AntarisApiCAN::api_pa_pc_get_can_dev(AntarisApiCAN* can_info) {
    AntarisReturnCode ret = An_SUCCESS;
    cJSON *p_cJson = nullptr;
    cJSON *key_io_access = nullptr;
    cJSON *key_can = nullptr;
    cJSON *pJsonStr = nullptr;
    char *str = nullptr;
    char key[32] = {'\0'};

    read_config_json(&p_cJson);

    if (!p_cJson) {
        std::cerr << "Error: Failed to read the config.json\n";
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }

    key_io_access = cJSON_GetObjectItemCaseSensitive(p_cJson, JSON_Key_IO_Access);
    if (!key_io_access) {
        std::cerr << "Error: " << JSON_Key_IO_Access << " key absent in config.json\n";
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }

    key_can = cJSON_GetObjectItemCaseSensitive(key_io_access, JSON_Key_CAN);
    if (!key_can) {
        std::cerr << "Error: " << JSON_Key_CAN << " key absent in config.json\n";
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }

    // Get CAN port count
    pJsonStr = cJSON_GetObjectItem(key_can, JSON_Key_CAN_Port_Count);
    if (!pJsonStr || !cJSON_IsString(pJsonStr)) {
        std::cerr << "Error: " << JSON_Key_CAN_Port_Count << " value is not a valid string\n";
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }

    str = cJSON_GetStringValue(pJsonStr);
    if (!str || strlen(str) > sizeof(int8_t)) {
        std::cerr << "Failed to read CAN count in JSON, CAN support not added\n";
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }

    can_info->can_port_count = (*str) - '0';

    // Get CAN port names
    for (int i = 0; i < can_info->can_port_count; i++) {
        sprintf(key, "%s%d", JSON_Key_CAN_Bus_Path, i);
        pJsonStr = cJSON_GetObjectItem(key_can, key);
        if (!pJsonStr || !cJSON_IsString(pJsonStr)) {
            std::cerr << "Error: " << key << " value is not a valid string\n";
            ret = An_INVALID_PARAMS;
            goto cleanup_and_exit;
        }

        memset(can_info->can_dev[i], 0, MAX_DEV_NAME_LENGTH);
        strncpy(can_info->can_dev[i], cJSON_GetStringValue(pJsonStr), MAX_DEV_NAME_LENGTH - 2);
        can_info->can_dev[i][MAX_DEV_NAME_LENGTH - 1] = '\0';
    }

cleanup_and_exit:
    if (p_cJson) {
        cJSON_Delete(p_cJson);
    }

    return ret;
}

// Receive CAN message on a specific device
void AntarisApiCAN::api_pa_pc_receive_can_message(int device_index) {
    int sockfd;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    if ((sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket creation failed");
        return;
    }

    strcpy(ifr.ifr_name, can_dev[device_index]);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl failed");
        close(sockfd);
        return;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return;
    }

    while (receiver_running[device_index]) {
        ssize_t nbytes = read(sockfd, &frame, sizeof(struct can_frame));
        if (nbytes > 0) {
            std::lock_guard<std::mutex> lock(*can_mutex[device_index]);
            message_buffers[device_index].push(frame);
        }
    }

    close(sockfd);
}

// Read CAN data from the buffer
struct can_frame AntarisApiCAN::api_pa_pc_read_can_data(int device_index) {
    struct can_frame frame = {0};

    std::lock_guard<std::mutex> lock(*can_mutex[device_index]);
    if (!message_buffers[device_index].pop(frame)) {
        std::cerr << "Error: No message available in buffer\n";
    }

    return frame;
}

// Start CAN receiver thread for a specific device
void AntarisApiCAN::api_pa_pc_start_can_receiver_thread(int device_index) {
    if (device_index < 0 || device_index >= MAX_CAN_DEVICES) return;

    if (!receiver_running[device_index]) {
        receiver_running[device_index] = true;
        receiver_threads.emplace_back(&AntarisApiCAN::api_pa_pc_receive_can_message, this, device_index);
    }
}

// Send a CAN message
int AntarisApiCAN::api_pa_pc_send_can_message(int device_index, int arbitration_id, const uint8_t data[8], uint8_t dlc) {
    int sockfd;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    if ((sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket creation failed");
        return GPIO_ERROR;
    }

    strcpy(ifr.ifr_name, can_dev[device_index]);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl failed");
        close(sockfd);
        return GPIO_ERROR;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return GPIO_ERROR;
    }

    frame.can_id = arbitration_id;
    frame.can_dlc = dlc;
    memcpy(frame.data, data, dlc);

    if (write(sockfd, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        perror("Write failed");
        close(sockfd);
        return GPIO_ERROR;
    }

    close(sockfd);
    return 0;
}

// Get received message count
size_t AntarisApiCAN::api_pa_pc_get_can_message_received_count(int device_index) {
    std::lock_guard<std::mutex> lock(*can_mutex[device_index]);
    return message_buffers[device_index].count();
}