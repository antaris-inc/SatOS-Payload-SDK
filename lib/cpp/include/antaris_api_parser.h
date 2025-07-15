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


class AntarisApiParser {
    public:
        AntarisReturnCode api_pa_pc_get_gpio_info(gpio_s *gpio);
        AntarisReturnCode api_pa_pc_get_gpio_adapter_type(char *adapter);

        AntarisReturnCode api_pa_pc_get_i2c_dev(i2c_s *i2c_info);
        AntarisReturnCode api_pa_pc_get_i2c_adapter(char *adapter);
        
    private:
};