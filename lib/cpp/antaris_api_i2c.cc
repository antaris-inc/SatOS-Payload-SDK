#include "antaris_api_i2c.h"
#include "antaris_api_parser.h"
#include <future>
#include <dlfcn.h>

typedef int (*init_qa7_lib_t)(void);
typedef int (*deinit_qa7_lib_t)(void);
typedef unsigned int (*read_i2c_t)(unsigned short, unsigned char, unsigned short, unsigned char *);
typedef unsigned int (*write_i2c_t)(unsigned short, unsigned char, unsigned short, unsigned char *);

extern char qa7_lib[32];
char i2c_adapter_type[32] = {0};

AntarisReturnCode AntarisApiI2C::api_pa_pc_init_i2c_lib()
{
    AntarisApiParser api_parser;
    AntarisReturnCode ret;

    ret = api_parser.api_pa_pc_get_i2c_adapter(i2c_adapter_type);

    if (ret != An_SUCCESS) {
        printf("Error: json file is not configured properly. Kindly check configurations done in ACP \n");
        return An_GENERIC_FAILURE;
    }

    if ((strncmp(i2c_adapter_type, "QA7", 2) == 0)) {
        void *handle;
        init_qa7_lib_t init_func;
    
        if (qa7_lib[0] == 0) {
            if (api_parser.api_pa_pc_get_qa7_lib() != An_SUCCESS) {
                printf("Error in fetching qA7 lib \n");
                return An_GENERIC_FAILURE;
            }
        }

        // Load the shared library
        handle = dlopen(qa7_lib, RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "dlopen failed: %s\n", dlerror());
            return An_GENERIC_FAILURE;
        }
    
        // Clear any previous error
        dlerror();
    
        // Lookup the symbol
        *(void **) (&init_func) = dlsym(handle, QA7_INIT_FUNCTION);
        char *error = dlerror();
        if (error) {
            fprintf(stderr, "dlsym failed: %s\n", error);
            dlclose(handle);
            return An_GENERIC_FAILURE;
        }
    
        // Call the function
        int status = init_func();
        printf("init_qa7_lib returned: %d\n", status);
    
        // Cleanup
        dlclose(handle);
    }
    return An_SUCCESS;
}

AntarisReturnCode AntarisApiI2C::api_pa_pc_deinit_i2c_lib()
{
    AntarisApiParser api_parser;

    if ((strncmp(i2c_adapter_type, "QA7", 2) == 0)) {
        void *handle;
        deinit_qa7_lib_t deinit_func;
    
        // Load the shared library
        handle = dlopen(qa7_lib, RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "dlopen failed: %s\n", dlerror());
            return An_GENERIC_FAILURE;
        }
    
        // Clear any previous error
        dlerror();
    
        // Lookup the symbol
        *(void **) (&deinit_func) = dlsym(handle, QA7_DEINIT_FUNCTION);
        char *error = dlerror();
        if (error) {
            fprintf(stderr, "dlsym failed: %s\n", error);
            dlclose(handle);
            return An_GENERIC_FAILURE;
        }
    
        // Call the function
        int status = deinit_func();
        printf("init_qa7_lib returned: %d\n", status);
    
        // Cleanup
        dlclose(handle);
        memset(qa7_lib, 0, sizeof(qa7_lib));
    }
    memset(i2c_adapter_type, 0, sizeof(i2c_adapter_type));
    return An_SUCCESS;
}

AntarisReturnCode AntarisApiI2C::api_pa_pc_read_i2c_bus(uint16_t i2c_dev, uint8_t i2c_address, uint16_t index, uint8_t *data)
{
    AntarisApiParser api_parser;
    AntarisReturnCode ret = An_SUCCESS;

    if ((strncmp(i2c_adapter_type, "QA7", 2) == 0)) {
        ret = read_qa7_i2c(i2c_dev, i2c_address, index, data);
    }
    return ret;
}

AntarisReturnCode AntarisApiI2C::read_qa7_i2c(uint16_t i2c_dev, uint8_t i2c_address, uint16_t index, uint8_t *data)
{
    void *handle;
    read_i2c_t read_i2c_func;

    // Open the shared object file
    handle = dlopen(qa7_lib, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return An_GENERIC_FAILURE;
    }

    // Clear any existing errors
    dlerror();

    // Load the function symbol
    *(void **) (&read_i2c_func) = dlsym(handle, QA7_I2C_READ_FUNCTION);
    char *error = dlerror();
    if (error) {
        fprintf(stderr, "dlsym failed: %s\n", error);
        dlclose(handle);
        return An_GENERIC_FAILURE;
    }

    // Call the dynamically loaded function
    unsigned int result = read_i2c_func(i2c_dev, i2c_address, index, data);

    if (result) {
        printf("read_i2c() successful, data = 0x%02X\n", *data);
    } else {
        printf("read_i2c() failed\n");
    }

    dlclose(handle);
    return An_SUCCESS;
}

AntarisReturnCode AntarisApiI2C::api_pa_pc_write_i2c_bus(uint16_t i2c_dev, uint8_t i2c_address, uint16_t index, uint8_t *data)
{
    AntarisApiParser api_parser;
    AntarisReturnCode ret = An_SUCCESS;

    if ((strncmp(i2c_adapter_type, "QA7", 2) == 0)) {
        ret = write_qa7_i2c(i2c_dev, i2c_address, index, data);
    }
    return ret;
}

AntarisReturnCode AntarisApiI2C::write_qa7_i2c(uint16_t i2c_dev, uint8_t i2c_address, uint16_t index, uint8_t *data)
{
    void *handle;
    write_i2c_t write_i2c;

    // Load the shared library
    handle = dlopen(qa7_lib, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return An_GENERIC_FAILURE;
    }

    // Load the symbol
    *(void **)(&write_i2c) = dlsym(handle, QA7_I2C_WRITE_FUNCTION);
    char *error = dlerror();
    if (error) {
        fprintf(stderr, "dlsym error: %s\n", error);
        dlclose(handle);
        return An_GENERIC_FAILURE;
    }

    // Call the function
    unsigned int result = write_i2c(i2c_dev, i2c_address, index, data);
    printf("write_i2c result: %u\n", result);

    // Cleanup
    dlclose(handle);
    return An_SUCCESS;
}
