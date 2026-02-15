/**
 * @file aht20.c
 * @brief AHT20 sensor driver implementation
 */

#include "aht20.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "AHT20";

// AHT20 Commands
#define AHT20_CMD_INIT 0xBE       // Initialization command
#define AHT20_CMD_TRIGGER 0xAC    // Trigger measurement
#define AHT20_CMD_SOFT_RESET 0xBA // Soft reset
#define AHT20_CMD_STATUS 0x71     // Read status

// AHT20 Parameters
#define AHT20_INIT_PARAM1 0x08
#define AHT20_INIT_PARAM2 0x00
#define AHT20_TRIGGER_PARAM1 0x33
#define AHT20_TRIGGER_PARAM2 0x00

// Status bits
#define AHT20_STATUS_BUSY (1 << 7)       // Bit 7: busy indication
#define AHT20_STATUS_CALIBRATED (1 << 3) // Bit 3: calibration enabled

// Timing
#define AHT20_MEASUREMENT_DELAY_MS 80
#define AHT20_RESET_DELAY_MS 20
#define AHT20_INIT_DELAY_MS 10
#define I2C_TIMEOUT_MS 1000

/**
 * Write command to AHT20
 */
static esp_err_t aht20_write_cmd(aht20_handle_t *handle, uint8_t cmd, uint8_t param1, uint8_t param2)
{
    uint8_t write_buf[3] = {cmd, param1, param2};
    return i2c_master_write_to_device(
        handle->config.i2c_port,
        AHT20_I2C_ADDR,
        write_buf, 3,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

/**
 * Read data from AHT20
 */
static esp_err_t aht20_read_data(aht20_handle_t *handle, uint8_t *data, size_t len)
{
    return i2c_master_read_from_device(
        handle->config.i2c_port,
        AHT20_I2C_ADDR,
        data, len,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

/**
 * Read status register
 */
static esp_err_t aht20_read_status(aht20_handle_t *handle, uint8_t *status)
{
    return aht20_read_data(handle, status, 1);
}

/**
 * Wait for sensor to be ready (not busy)
 */
static esp_err_t aht20_wait_ready(aht20_handle_t *handle, uint32_t timeout_ms)
{
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

    while (1)
    {
        uint8_t status;
        esp_err_t ret = aht20_read_status(handle, &status);
        if (ret != ESP_OK)
        {
            return ret;
        }

        if ((status & AHT20_STATUS_BUSY) == 0)
        {
            return ESP_OK; // Not busy
        }

        uint32_t elapsed = (xTaskGetTickCount() * portTICK_PERIOD_MS) - start_time;
        if (elapsed > timeout_ms)
        {
            ESP_LOGE(TAG, "Timeout waiting for sensor ready");
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t aht20_soft_reset(aht20_handle_t *handle)
{
    if (handle == NULL || !handle->initialized)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t cmd = AHT20_CMD_SOFT_RESET;
    esp_err_t ret = i2c_master_write_to_device(
        handle->config.i2c_port,
        AHT20_I2C_ADDR,
        &cmd, 1,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS));

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Soft reset failed");
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(AHT20_RESET_DELAY_MS));
    ESP_LOGI(TAG, "Soft reset completed");

    return ESP_OK;
}

esp_err_t aht20_init(aht20_handle_t *handle, const aht20_config_t *config)
{
    if (handle == NULL || config == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Clear handle
    memset(handle, 0, sizeof(aht20_handle_t));

    // Copy configuration
    memcpy(&handle->config, config, sizeof(aht20_config_t));

    // Configure and install I2C driver (if not already done)
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = config->i2c_freq_hz,
    };

    esp_err_t ret = i2c_param_config(config->i2c_port, &i2c_conf);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "I2C config failed: %d", ret);
        return ret;
    }

    ret = i2c_driver_install(config->i2c_port, i2c_conf.mode, 0, 0, 0);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "I2C driver installed");
    }
    else if (ret == ESP_ERR_INVALID_STATE)
    {
        ESP_LOGI(TAG, "I2C driver already installed, reusing");
        ret = ESP_OK; // Clear error, driver is available
    }
    else
    {
        ESP_LOGE(TAG, "I2C driver install failed: %d", ret);
        return ret;
    }

    // Wait for sensor to be ready after power-up
    vTaskDelay(pdMS_TO_TICKS(40));

    // Check status
    uint8_t status;
    ret = aht20_read_status(handle, &status);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read status");
        return ret;
    }

    ESP_LOGI(TAG, "AHT20 status: 0x%02X", status);

    // Check if calibrated
    if ((status & AHT20_STATUS_CALIBRATED) == 0)
    {
        ESP_LOGW(TAG, "AHT20 not calibrated, initializing...");

        // Send initialization command
        ret = aht20_write_cmd(handle, AHT20_CMD_INIT, AHT20_INIT_PARAM1, AHT20_INIT_PARAM2);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Initialization command failed");
            return ret;
        }

        vTaskDelay(pdMS_TO_TICKS(AHT20_INIT_DELAY_MS));

        // Check status again
        ret = aht20_read_status(handle, &status);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to read status after init");
            return ret;
        }

        if ((status & AHT20_STATUS_CALIBRATED) == 0)
        {
            ESP_LOGE(TAG, "AHT20 calibration failed");
            return ESP_FAIL;
        }
    }

    handle->calibrated = true;
    handle->initialized = true;

    ESP_LOGI(TAG, "AHT20 initialized and calibrated");

    return ESP_OK;
}

esp_err_t aht20_read(aht20_handle_t *handle, float *temp, float *humidity)
{
    if (handle == NULL || !handle->initialized || temp == NULL || humidity == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Trigger measurement
    esp_err_t ret = aht20_write_cmd(handle, AHT20_CMD_TRIGGER,
                                    AHT20_TRIGGER_PARAM1, AHT20_TRIGGER_PARAM2);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to trigger measurement");
        return ret;
    }

    // Wait for measurement to complete
    vTaskDelay(pdMS_TO_TICKS(AHT20_MEASUREMENT_DELAY_MS));

    // Wait for sensor to be ready
    ret = aht20_wait_ready(handle, 100);
    if (ret != ESP_OK)
    {
        return ret;
    }

    // Read measurement data (7 bytes: status + data)
    uint8_t data[7];
    ret = aht20_read_data(handle, data, 7);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read measurement data");
        return ret;
    }

    // Check status
    if (data[0] & AHT20_STATUS_BUSY)
    {
        ESP_LOGW(TAG, "Sensor still busy after wait");
        return ESP_ERR_INVALID_STATE;
    }

    // Extract humidity data (20 bits)
    uint32_t raw_humidity = ((uint32_t)data[1] << 12) |
                            ((uint32_t)data[2] << 4) |
                            ((uint32_t)data[3] >> 4);

    // Extract temperature data (20 bits)
    uint32_t raw_temp = (((uint32_t)data[3] & 0x0F) << 16) |
                        ((uint32_t)data[4] << 8) |
                        (uint32_t)data[5];

    // Convert to physical values
    // Humidity: RH% = (raw / 2^20) * 100
    *humidity = ((float)raw_humidity / 1048576.0f) * 100.0f;

    // Temperature: T(°C) = (raw / 2^20) * 200 - 50
    *temp = ((float)raw_temp / 1048576.0f) * 200.0f - 50.0f;

    // Sanity check
    if (*temp < -40.0f || *temp > 85.0f)
    {
        ESP_LOGW(TAG, "Temperature out of range: %.2f°C", *temp);
    }
    if (*humidity < 0.0f || *humidity > 100.0f)
    {
        ESP_LOGW(TAG, "Humidity out of range: %.2f%%", *humidity);
    }

    ESP_LOGI(TAG, "Temperature: %.2f°C, Humidity: %.2f%%", *temp, *humidity);

    return ESP_OK;
}
