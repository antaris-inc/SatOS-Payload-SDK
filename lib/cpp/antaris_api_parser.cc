#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fstream>
#include "cJSON.h"
#include <cstdlib>
#include <sys/wait.h>
#include <chrono>
#include <thread>
#include <future>

#include "Python.h"

#include "antaris_api_gpio.h"
#include "antaris_api.h"
#include "antaris_api_internal.h"
#include "antaris_sdk_environment.h"
#include "antaris_api_parser.h"

#define GENERIC_ERROR -1

AntarisReturnCode AntarisApiParser::api_pa_pc_get_gpio_info(gpio_s *gpio)
{
    AntarisReturnCode ret = An_SUCCESS;
    cJSON *p_cJson = NULL;
    cJSON *key_io_access = NULL;
    cJSON *key_gpio = NULL;
    cJSON *pJsonStr = NULL;
    char *str = NULL;
    char key[32] = {'\0'};

    memset(gpio->pins, -1, sizeof(gpio->pins));
    gpio->gpio_port = -1;
    gpio->pin_count = -1;
    gpio->interrupt_pin = -1;

    read_config_json(&p_cJson);
    if (p_cJson == NULL)
    {
        printf("Error: Failed to read the config.json\n");
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    key_io_access = cJSON_GetObjectItemCaseSensitive(p_cJson, JSON_Key_IO_Access);
    if (key_io_access == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_IO_Access);
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }
        
    key_gpio = cJSON_GetObjectItemCaseSensitive(key_io_access, JSON_Key_GPIO);
    if (key_gpio == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_GPIO);
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }
        
    // Check adapter type
    pJsonStr = cJSON_GetObjectItem(key_gpio, JSON_Key_Adapter_Type);
    if (pJsonStr == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_Adapter_Type);
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }
    if (cJSON_IsString(pJsonStr) == cJSON_Invalid) {
        printf("Error: %s value is not a string \n", JSON_Key_Adapter_Type);
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }
    str = cJSON_GetStringValue(pJsonStr);
    if ((str == NULL) ||
        ((strncmp(str, "FTDI", 4) != 0)))
    {
        printf("Only FTDI devices are supported");
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }
    
    // get GPIO pin count
    pJsonStr = cJSON_GetObjectItem(key_gpio, JSON_Key_GPIO_Pin_Count);
    if (pJsonStr == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_GPIO_Pin_Count);
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }
    if (cJSON_IsString(pJsonStr) == cJSON_Invalid) {
        printf("Error: %s value is not a string \n", JSON_Key_GPIO_Pin_Count);
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    str = cJSON_GetStringValue(pJsonStr);
    if ((*str == 0) || (str == NULL) || (strlen(str) > sizeof(int8_t)))
    {
        printf("Failed to read gpio count the json, GPIO support not added \n");
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }
    gpio->pin_count = *str - '0';
    if (gpio->pin_count > MAX_GPIO_PIN_COUNT) {
        printf("Error: GPIO pin count canot be greater than %d \n", MAX_GPIO_PIN_COUNT);
        gpio->pin_count = 0;
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    // Get GPIO port
    pJsonStr = cJSON_GetObjectItem(key_gpio, JSON_Key_GPIO_Port);
    if (pJsonStr == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_GPIO_Port);
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }
    if (cJSON_IsString(pJsonStr) == cJSON_Invalid) {
        printf("Error: %s value is not a string \n", JSON_Key_GPIO_Port);
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    str = cJSON_GetStringValue(pJsonStr);
    if ((*str == 0) || (str == NULL) || (strlen(str) > sizeof(int8_t)))
    {
        printf("Failed to read gpio port the json, GPIO support not added \n");
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }
    gpio->gpio_port = *str - '0';
    if (gpio->gpio_port > MAX_GPIO_PORT_NUMBER) {
        printf("Error: GPIO port canot be greater than %d \n", MAX_GPIO_PORT_NUMBER);
        gpio->gpio_port = -1;
        ret = An_GENERIC_FAILURE;
        goto cleanup_and_exit;
    }

    // get GPIO pins
    for (int i = 0; i < gpio->pin_count; i++)
    {
        sprintf(key, "%s%d", JSON_Key_GPIO_Pin, i);
        pJsonStr = cJSON_GetObjectItem(key_gpio, key);
        if (cJSON_IsString(pJsonStr) == cJSON_Invalid) {
            printf("Error: %s value is not a string \n", key);
            ret = An_GENERIC_FAILURE;
            goto cleanup_and_exit;
        }

        str = cJSON_GetStringValue(pJsonStr);
        if ((*str == 0) || (str == NULL) || (strlen(str) > sizeof(int8_t))) {
            printf("Error: Failed to read gpio pin number %d the json \n", i);
            ret = An_GENERIC_FAILURE;
            goto cleanup_and_exit;
        }
        gpio->pins[i] = *str - '0';
        if ((gpio->pins[i] < MIN_GPIO_PIN_NUMBER) ||
            (gpio->pins[i] > MAX_GPIO_PIN_NUMBER)) {
            printf("Error: GPIO pin number is %d. It should be in range of %d to %d \n", gpio->pins[i], MIN_GPIO_PIN_NUMBER, MAX_GPIO_PIN_NUMBER);
            ret = An_GENERIC_FAILURE;
            goto cleanup_and_exit;
        }
    }

    // get Interrupt pin, it is optional, hence not returning upon failure
    pJsonStr = cJSON_GetObjectItem(key_gpio, JSON_Key_Interrupt_Pin);
    if (cJSON_IsString(pJsonStr) != cJSON_Invalid) {
        str = cJSON_GetStringValue(pJsonStr);
        if ((*str != 0) && (str == NULL)) {
            gpio->interrupt_pin = *str - '0';
            if ((gpio->interrupt_pin < MIN_GPIO_PIN_NUMBER) ||
                (gpio->interrupt_pin > MAX_GPIO_PIN_NUMBER)) {
                printf("Error: Interrupt pin number is %d. It should be in range of %d to %d \n", gpio->interrupt_pin, MIN_GPIO_PIN_NUMBER, MAX_GPIO_PIN_NUMBER);
                ret = An_GENERIC_FAILURE;
                goto cleanup_and_exit;
            }
        }
    } else {
        printf("Error: %s value is not a string, ignoring Interrupt pin \n", JSON_Key_Interrupt_Pin);
    }

cleanup_and_exit:
    cJSON_Delete(p_cJson);

    return ret;
}

AntarisReturnCode AntarisApiParser::api_pa_pc_get_gpio_adapter_type(char *adapter)
{
    AntarisReturnCode ret = An_SUCCESS;
    cJSON *p_cJson = NULL;
    cJSON *key_io_access = NULL;
    cJSON *key_gpio = NULL;
    cJSON *pJsonStr = NULL;
    char *str = NULL;
    char key[32] = {'\0'};

    read_config_json(&p_cJson);
    if (p_cJson == NULL)
    {
        printf("Error: Failed to read the config.json\n");
        return An_GENERIC_FAILURE;
    }

    key_io_access = cJSON_GetObjectItemCaseSensitive(p_cJson, JSON_Key_IO_Access);
    if (key_io_access == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_IO_Access);
        return An_GENERIC_FAILURE;
    }
        
    key_gpio = cJSON_GetObjectItemCaseSensitive(key_io_access, JSON_Key_GPIO);
    if (key_gpio == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_GPIO);
        return An_GENERIC_FAILURE;
    }
        
    // Check adapter type
    pJsonStr = cJSON_GetObjectItem(key_gpio, JSON_Key_Adapter_Type);
    if (pJsonStr == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_Adapter_Type);
        return An_GENERIC_FAILURE;
    }
    if (cJSON_IsString(pJsonStr) == cJSON_Invalid) {
        printf("Error: %s value is not a string \n", JSON_Key_Adapter_Type);
        return An_GENERIC_FAILURE;
    }
    adapter = cJSON_GetStringValue(pJsonStr);
    if ((*adapter == 0) || (adapter == NULL))
    {
        printf("Failed to read gpio count the json, GPIO support not added \n");
        return An_GENERIC_FAILURE;
    }
    return An_SUCCESS;
}

AntarisReturnCode AntarisApiParser::api_pa_pc_get_i2c_dev(i2c_s *i2c_info)
{
    AntarisReturnCode ret = An_SUCCESS;
    cJSON *p_cJson = nullptr;
    cJSON *key_io_access = nullptr;
    cJSON *key_i2c = nullptr;
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

    key_i2c = cJSON_GetObjectItemCaseSensitive(key_io_access, JSON_Key_I2C);
    if (!key_i2c) {
        std::cerr << "Error: " << JSON_Key_I2C << " key absent in config.json\n";
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }

    // Get CAN port count
    pJsonStr = cJSON_GetObjectItem(key_i2c, JSON_Key_I2C_Port_Count);
    if (!pJsonStr || !cJSON_IsString(pJsonStr)) {
        std::cerr << "Error: " << JSON_Key_I2C_Port_Count << " value is not a valid string\n";
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }

    str = cJSON_GetStringValue(pJsonStr);
    if (!str || strlen(str) > sizeof(int8_t)) {
        std::cerr << "Failed to read CAN count in JSON, CAN support not added\n";
        ret = An_INVALID_PARAMS;
        goto cleanup_and_exit;
    }

    i2c_info->i2c_port_count = (*str) - '0';

    // Get CAN port names
    for (int i = 0; i < i2c_info->i2c_port_count; i++) {
        sprintf(key, "%s%d", JSON_Key_I2C_Bus_Path, i);
        pJsonStr = cJSON_GetObjectItem(key_i2c, key);
        if (!pJsonStr || !cJSON_IsString(pJsonStr)) {
            std::cerr << "Error: " << key << " value is not a valid string\n";
            ret = An_INVALID_PARAMS;
            goto cleanup_and_exit;
        }

        memset(i2c_info->i2c_dev[i], 0, MAX_DEV_NAME_LENGTH);
        strncpy(i2c_info->i2c_dev[i], cJSON_GetStringValue(pJsonStr), MAX_DEV_NAME_LENGTH - 2);
        i2c_info->i2c_dev[i][MAX_DEV_NAME_LENGTH - 1] = '\0';
    }

cleanup_and_exit:
    if (p_cJson) {
        cJSON_Delete(p_cJson);
    }

    return ret;
}

AntarisReturnCode AntarisApiParser::api_pa_pc_get_i2c_adapter(char *adapter)
{
    AntarisReturnCode ret = An_SUCCESS;
    cJSON *p_cJson = NULL;
    cJSON *key_io_access = NULL;
    cJSON *key_gpio = NULL;
    cJSON *pJsonStr = NULL;
    char *str = NULL;
    char key[32] = {'\0'};

    read_config_json(&p_cJson);
    if (p_cJson == NULL)
    {
        printf("Error: Failed to read the config.json\n");
        return An_GENERIC_FAILURE;
    }

    key_io_access = cJSON_GetObjectItemCaseSensitive(p_cJson, JSON_Key_IO_Access);
    if (key_io_access == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_IO_Access);
        return An_GENERIC_FAILURE;
    }
        
    key_gpio = cJSON_GetObjectItemCaseSensitive(key_io_access, JSON_Key_I2C);
    if (key_gpio == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_I2C);
        return An_GENERIC_FAILURE;
    }
        
    // Check adapter type
    pJsonStr = cJSON_GetObjectItem(key_gpio, JSON_Key_I2C_Adapter_Type);
    if (pJsonStr == NULL) {
        printf("Error: %s key absent in config.json \n", JSON_Key_I2C_Adapter_Type);
        return An_GENERIC_FAILURE;
    }
    if (cJSON_IsString(pJsonStr) == cJSON_Invalid) {
        printf("Error: %s value is not a string \n", JSON_Key_I2C_Adapter_Type);
        return An_GENERIC_FAILURE;
    }
    adapter = cJSON_GetStringValue(pJsonStr);
    if ((*adapter == 0) || (adapter == NULL))
    {
        printf("Failed to read gpio count the json, GPIO support not added \n");
        return An_GENERIC_FAILURE;
    }
    return An_SUCCESS;
}