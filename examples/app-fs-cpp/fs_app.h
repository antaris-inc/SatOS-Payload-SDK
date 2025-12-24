#ifndef DATA_FILES_H
#define DATA_FILES_H
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "antaris_api.h"
#define MAX_EXTENSION 8
#define MAX_BASE_NAME 64
#define STAGE_FILE_DOWNLOAD_DIR         "/opt/antaris/outbound/"    // path for staged file download
#define STAGE_FILE_NAME                 "SampleFile.txt"            // name of staged file
#define HM_FILES_PARENT_DIR             "/opt/antaris/hm/"    // path for HM files parent directory
typedef struct
{
    uint8_t sub_id;
    uint8_t queue_id;
    uint16_t data_len;
}__attribute__((__packed__)) file_info;

typedef struct {
    char basename[MAX_BASE_NAME];
    char extension[MAX_EXTENSION];
} DataFile;

AntarisReturnCode write_rotating_queue_file(const char* dir,
                               DataFile file,
                               uint8_t queueId,
                               uint8_t* data,
                               size_t len);

size_t read_rotating_queue_file(const char* dir,
                                DataFile file,
                                uint8_t queueId,
                                const std::string& outFile);
#endif