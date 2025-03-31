#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "antaris_can_api.h"
#include "antaris_api_common.h"
#include "antaris_sdk_environment.h"

const int GPIO_ERROR = -1;

// Constructor
AntarisApiCAN::AntarisApiCAN() {}

// Destructor
AntarisApiCAN::~AntarisApiCAN() {
    pthread_mutex_destroy(&queue_mutex);
}

// Get available CAN devices
AntarisReturnCode AntarisApiCAN::api_pa_pc_get_can_dev(AntarisApiCAN *can_info) 
{
    AntarisReturnCode ret = An_SUCCESS;
    cJSON *p_cJson = NULL;
    cJSON *key_io_access = NULL;
    cJSON *key_can = NULL;
    cJSON *pJsonStr = NULL;
    char *str = NULL;
    char key[32] = {'\0'};

    read_config_json(&p_cJson);

    if (p_cJson == NULL) {
        printf("Error: Failed to read the config.json\n");
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }
    
    key_io_access = cJSON_GetObjectItemCaseSensitive(p_cJson, JSON_Key_IO_Access);
    if (key_io_access == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_IO_Access);
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }
   
    key_can = cJSON_GetObjectItemCaseSensitive(key_io_access, JSON_Key_CAN);
    if (key_can == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_CAN);
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }
    
    // get CAN port count
    pJsonStr = cJSON_GetObjectItem(key_can, JSON_Key_CAN_Port_Count);
    if (pJsonStr == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_CAN_Port_Count);
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }
    if (cJSON_IsString(pJsonStr) == cJSON_Invalid) {
        printf("Error: %s value is not a string \n", JSON_Key_CAN_Port_Count);
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }

    str = cJSON_GetStringValue(pJsonStr);
    if ((*str == 0) || (str == NULL) || (strlen(str) > sizeof(int8_t)))
    {
        printf("Failed to read CAN count the json, CAN support not added \n");
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }

    can_info->can_port_count = (*str) - '0';

    // get CAN ports
    for (int i = 0; i < can_info->can_port_count; i++)
    {
        sprintf(key, "%s%d", JSON_Key_CAN_Bus_Path, i);
        pJsonStr = cJSON_GetObjectItem(key_can, key);
        if (cJSON_IsString(pJsonStr) == cJSON_Invalid) {
            printf("Error: %s value is not a string \n", key);
            ret = An_INVALID_PARAMS;
            goto cleanup_and_exit;
        }

        memset(can_info->can_dev[i], 0, MAX_CAN_PATH_LEN);
        strncpy(can_info->can_dev[i], cJSON_GetStringValue(pJsonStr), MAX_CAN_PATH_LEN - 1);
    }

cleanup_and_exit:
    cJSON_Delete(p_cJson);

    return ret;
}

// Receive CAN message on a specific device
void AntarisApiCAN::api_pa_pc_receive_can_message(int device_index) {
    int sockfd;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    if ((sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        return;
    }

    strcpy(ifr.ifr_name, can_dev[device_index]);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        return;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        return;
    }

    while (receiver_running[device_index]) {
        ssize_t nbytes = read(sockfd, &frame, sizeof(struct can_frame));
        if (nbytes > 0) {
            std::lock_guard<std::mutex> lock(can_mutex[device_index]);
            received_messages[device_index] = frame;
        }
    }
    close(sockfd);
}

struct can_frame AntarisApiCAN::api_pa_pc_read_can_data(int device_index) 
{
    std::lock_guard<std::mutex> lock(can_mutex[device_index]);
    return received_messages[device_index];
}

void AntarisApiCAN::api_pa_pc_start_can_receiver_thread(int device_index) 
{
    if (device_index < 0 || device_index >= MAX_CAN_DEVICES) return;
    if (!receiver_running[device_index]) {
        receiver_running[device_index] = true;
        receiver_threads[device_index] = std::thread(&AntarisApiCAN::api_pa_pc_receive_can_message, this, device_index);
    }
}

int AntarisApiCAN::api_pa_pc_send_can_message(int device_index, int arbitration_id, const uint8_t data[8], uint8_t dlc) 
{
    int sockfd;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    if ((sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        return GPIO_ERROR;
    }

    strcpy(ifr.ifr_name, can_dev[device_index]);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        return -1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        return GPIO_ERROR;
    }

    frame.can_id = arbitration_id;
    frame.can_dlc =dlc;
    memcpy(frame.data, data, dlc);

    if (write(sockfd, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        perror("Write");
        return GPIO_ERROR;
    }

    close(sockfd);
    return 0;
}

// Get received message count
size_t AntarisApiCAN::api_pa_pc_get_can_message_received_count(int device_index) 
{
    std::lock_guard<std::mutex> lock(can_mutex[device_index]);
    return received_messages[device_index].can_id != 0 ? 1 : 0;
    /*
    pthread_mutex_lock(&queue_mutex);
    size_t count = (buffer_end >= buffer_start)
                       ? (buffer_end - buffer_start)
                       : (MAX_CAN_MESSAGES - buffer_start + buffer_end);
    pthread_mutex_unlock(&queue_mutex);
    return count; */
}