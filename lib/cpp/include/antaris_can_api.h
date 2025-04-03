#ifndef ANTARIS_CAN_API_H
#define ANTARIS_CAN_API_H

#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <cstring>
#include <unistd.h>

#include "antaris_api.h"

#define MAX_CAN_DEVICES        4   // Max number of CAN devices
#define MAX_DEV_NAME_LENGTH    32  // Max length for each device name
#define MAX_CAN_MESSAGES       100 // Buffer for received messages

struct CircularBuffer {
    public:
        CircularBuffer() {
            start = 0;
            end = 0;
        }; 
        bool push(const struct can_frame& frame);
        bool pop(struct can_frame& frame);
        size_t count() const;
    private:
        struct can_frame buffer[MAX_CAN_MESSAGES];
        size_t start;
        size_t end;
};

class AntarisApiCAN {
private:
    CircularBuffer message_buffers[MAX_CAN_DEVICES];  // One circular buffer per device
    std::vector<bool> receiver_running;  // Track receiver state
    std::vector<std::thread> receiver_threads;  // One thread per CAN device
    std::vector<std::unique_ptr<std::mutex>> can_mutex; // Mutex for each CAN device

public:
    int can_port_count;
    char can_dev[MAX_CAN_DEVICES][MAX_DEV_NAME_LENGTH];

    // Constructor & Destructor
    AntarisApiCAN();
    ~AntarisApiCAN();

    // Get available CAN devices
    AntarisReturnCode api_pa_pc_get_can_dev(AntarisApiCAN *can_info);

    // Start receiving messages on a separate thread
    void api_pa_pc_start_can_receiver_thread(int device_index);

    // Receive CAN message on a specific device
    void api_pa_pc_receive_can_message(int device_index);

    // Read a message from the buffer
    struct can_frame api_pa_pc_read_can_data(int device_index);

    // Send a CAN message
    int api_pa_pc_send_can_message(int device_index, int arbitration_id, const uint8_t data[8], uint8_t dlc);

    // Get received message count
    size_t api_pa_pc_get_can_message_received_count(int device_index);
};

#endif