#include "antaris_api_i2c.h"
#include "antaris_api_parser.h"
#include <future>
#include <dlfcn.h>

typedef int (*init_qa7_lib_t)(void);
typedef int (*deinit_qa7_lib_t)(void);
typedef unsigned int (*read_i2c_t)(unsigned short, unsigned char, unsigned short, unsigned char *);
typedef unsigned int (*write_i2c_t)(unsigned short, unsigned char, unsigned short, unsigned char *, int);

extern char qa7_lib[32];
char i2c_adapter_type[32] = {0};
extern void *qa7handle;

// Method definition outside class
AntarisReturnCode AntarisApiI2C::api_pa_pc_init_i2c_lib()
{
    AntarisReturnCode ret = An_SUCCESS;
    if ((strncmp(i2c_adapter_type, "QA7", 2) == 0)) {
        ret = init_qa7_lib();
    }
    return ret;
}

AntarisReturnCode AntarisApiI2C::init_qa7_lib() 
{
    init_qa7_lib_t init_qa7_func;
    if (qa7handle == NULL) {
        qa7handle = dlopen(qa7_lib, RTLD_LAZY);
        if (!qa7handle) {
            fprintf(stderr, "dlopen failed: %s\n", dlerror());
            return An_GENERIC_FAILURE;
        }
    }

    // Clear any existing errors
    dlerror();

    *(void **) (&init_qa7_func) = dlsym(qa7handle, QA7_INIT_FUNCTION);
    char *error = dlerror();
    if (error) {
        fprintf(stderr, "dlsym failed: %s\n", error);
        dlclose(qa7handle);
        return An_GENERIC_FAILURE;
    }

    init_qa7_func();

    return (qa7handle != NULL) ? An_SUCCESS : An_GENERIC_FAILURE;
}

AntarisReturnCode AntarisApiI2C::api_pa_pc_deinit_i2c_lib()
{
    AntarisReturnCode ret = An_SUCCESS;
    if ((strncmp(i2c_adapter_type, "QA7", 2) == 0)) {
        ret = deinit_qa7_lib();
    }
    return ret;
}

AntarisReturnCode AntarisApiI2C::deinit_qa7_lib() 
{
    deinit_qa7_lib_t deinit_qa7_func;
    *(void **) (&deinit_qa7_func) = dlsym(qa7handle, QA7_DEINIT_FUNCTION);
    char *error = dlerror();
    if (error) {
        fprintf(stderr, "dlsym failed: %s\n", error);
        dlclose(qa7handle);
        return An_GENERIC_FAILURE;
    }

    deinit_qa7_func();

    if (qa7handle != NULL) {
        dlclose(qa7handle);
    }
    qa7handle = NULL;
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
    read_i2c_t read_i2c_func;

    // Load the shared library
    if (qa7handle == NULL) {
        if (init_qa7_lib() != An_SUCCESS) {
            return An_GENERIC_FAILURE;
        }
    }

    // Load the function symbol
    *(void **) (&read_i2c_func) = dlsym(qa7handle, QA7_I2C_READ_FUNCTION);
    char *error = dlerror();
    if (error) {
        fprintf(stderr, "dlsym failed: %s\n", error);
        dlclose(qa7handle);
        return An_GENERIC_FAILURE;
    }

    // Call the dynamically loaded function
    unsigned int result = read_i2c_func(i2c_dev, i2c_address, index, data);

    if (result) {
        printf("read_i2c() successful, data = 0x%02X\n", *data);
    } else {
        printf("read_i2c() failed\n");
    }

    dlclose(qa7handle);
    return An_SUCCESS;
}

AntarisReturnCode AntarisApiI2C::api_pa_pc_write_i2c_bus(uint16_t i2c_dev, uint8_t i2c_address, uint16_t index, uint8_t *data, int data_length)
{
    AntarisApiParser api_parser;
    AntarisReturnCode ret = An_SUCCESS;

    if ((strncmp(i2c_adapter_type, "QA7", 2) == 0)) {
        ret = write_qa7_i2c(i2c_dev, i2c_address, index, data, data_length);
    }
    return ret;
}

AntarisReturnCode AntarisApiI2C::write_qa7_i2c(uint16_t i2c_dev, uint8_t i2c_address, uint16_t index, uint8_t *data, int data_length)
{
    write_i2c_t write_i2c;

    // Load the shared library
    if (qa7handle == NULL) {
        if (init_qa7_lib() != An_SUCCESS) {
            return An_GENERIC_FAILURE;
        }
    }

    // Load the symbol
    *(void **)(&write_i2c) = dlsym(qa7handle, QA7_I2C_WRITE_FUNCTION);
    char *error = dlerror();
    if (error) {
        fprintf(stderr, "dlsym error: %s\n", error);
        dlclose(qa7handle);
        return An_GENERIC_FAILURE;
    }

    // Call the function
    unsigned int result = write_i2c(i2c_dev, i2c_address, index, data, data_length);
    printf("write_i2c result: %u\n", result);

    return An_SUCCESS;
}
