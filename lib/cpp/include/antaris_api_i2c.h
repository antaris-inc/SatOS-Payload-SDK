#ifndef __ANTARIS_API_I2C_H__
#define __ANTARIS_API_I2C_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fstream>
#include "cJSON.h"
#include <cstdlib>

#include "antaris_api.h"
#include "antaris_api_internal.h"
#include "antaris_sdk_environment.h"

typedef struct i2c {
    int8_t i2c_port_count;
    int8_t i2c_dev[8];
}i2c_s;

class AntarisApiI2C {
    public:
        AntarisReturnCode api_pa_pc_read_i2c_bus(uint16_t i2c_dev, uint8_t i2c_address, uint16_t index, uint8_t *data);

        AntarisReturnCode api_pa_pc_init_i2c_lib();
        AntarisReturnCode init_qa7_lib(); 
    
        AntarisReturnCode api_pa_pc_deinit_i2c_lib();
        AntarisReturnCode deinit_qa7_lib(); 

        AntarisReturnCode read_qa7_i2c(uint16_t i2c_dev, uint8_t i2c_address, uint16_t index, uint8_t *data);

        AntarisReturnCode api_pa_pc_write_i2c_bus(uint16_t i2c_dev, uint8_t i2c_address, uint16_t index, uint8_t *data);

        AntarisReturnCode write_qa7_i2c(uint16_t i2c_dev, uint8_t i2c_address, uint16_t index, uint8_t *data);
        
    private:
};

#endif // __ANTARIS_API_I2C_H__