/**
 * @file bmp280.c
 * @brief BMP280 sensor driver implementation
 */

#include "bmp280.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "BMP280";

// BMP280 Register addresses
#define BMP280_REG_TEMP_XLSB 0xFC
#define BMP280_REG_TEMP_LSB 0xFB
#define BMP280_REG_TEMP_MSB 0xFA
#define BMP280_REG_PRESS_XLSB 0xF9
#define BMP280_REG_PRESS_LSB 0xF8
#define BMP280_REG_PRESS_MSB 0xF7
#define BMP280_REG_CONFIG 0xF5
#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_STATUS 0xF3
#define BMP280_REG_RESET 0xE0
#define BMP280_REG_ID 0xD0
#define BMP280_REG_CALIB 0x88

#define BMP280_CHIP_ID 0x58
#define I2C_TIMEOUT_MS 1000

/**
 * Write a single byte to BMP280 register
 */
static esp_err_t bmp280_write_reg(bmp280_handle_t *handle, uint8_t reg, uint8_t data)
{
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_write_to_device(
        handle->config.i2c_port,
        handle->config.i2c_addr,
        write_buf, 2,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

/**
 * Read bytes from BMP280 register
 */
static esp_err_t bmp280_read_reg(bmp280_handle_t *handle, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(
        handle->config.i2c_port,
        handle->config.i2c_addr,
        &reg, 1,
        data, len,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

/**
 * Compensate temperature using calibration data
 * Returns temperature in 0.01 degrees Celsius
 */
static int32_t bmp280_compensate_temp(bmp280_handle_t *handle, int32_t adc_T)
{
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)handle->calib.dig_T1 << 1))) *
            ((int32_t)handle->calib.dig_T2)) >>
           11;
    var2 = (((((adc_T >> 4) - ((int32_t)handle->calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)handle->calib.dig_T1))) >>
             12) *
            ((int32_t)handle->calib.dig_T3)) >>
           14;
    handle->calib.t_fine = var1 + var2;
    return (handle->calib.t_fine * 5 + 128) >> 8;
}

/**
 * Compensate pressure using calibration data
 * Returns pressure in Pa * 256
 */
static uint32_t bmp280_compensate_press(bmp280_handle_t *handle, int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)handle->calib.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)handle->calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)handle->calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)handle->calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)handle->calib.dig_P3) >> 8) +
           ((var1 * (int64_t)handle->calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)handle->calib.dig_P1) >> 33;

    if (var1 == 0)
    {
        return 0;
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)handle->calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)handle->calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)handle->calib.dig_P7) << 4);

    return (uint32_t)p;
}

esp_err_t bmp280_init(bmp280_handle_t *handle, const bmp280_config_t *config)
{
    if (handle == NULL || config == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Clear handle
    memset(handle, 0, sizeof(bmp280_handle_t));

    // Copy configuration
    memcpy(&handle->config, config, sizeof(bmp280_config_t));

    // Configure mode-specific settings
    switch (config->mode)
    {
    case BMP280_MODE_WEATHER_MONITORING:
        // Ultra low power: osrs_t=001 (×1), osrs_p=001 (×1), mode=01 (forced)
        handle->mode_config.ctrl_meas_value = 0x25; // 00100101
        handle->mode_config.meas_time_ms = 10;
        bmp280_write_reg(handle, BMP280_REG_CONFIG, 0x00); // Filter off
        break;

    case BMP280_MODE_HIGH_RESOLUTION:
        // High resolution: osrs_t=010 (×2), osrs_p=101 (×16), mode=01 (forced)
        handle->mode_config.ctrl_meas_value = 0x55; // 01010101
        handle->mode_config.meas_time_ms = 50;
        bmp280_write_reg(handle, BMP280_REG_CONFIG, 0x00); // Filter off
        break;

    case BMP280_MODE_METEO_ULTRA_PRECISION:
        // Ultra precision: osrs_t=111 (×16), osrs_p=101 (×16), mode=01 (forced)
        handle->mode_config.ctrl_meas_value = 0xB5; // 10110101
        handle->mode_config.meas_time_ms = 100;
        bmp280_write_reg(handle, BMP280_REG_CONFIG, 0x10); // Filter=16
        break;

    default:
        return ESP_ERR_INVALID_ARG;
    }

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
    else if (ret == ESP_ERR_INVALID_STATE || ret == ESP_FAIL)
    {
        ESP_LOGI(TAG, "I2C driver already installed, reusing");
        ret = ESP_OK; // Clear error, driver is available
    }
    else
    {
        ESP_LOGE(TAG, "I2C driver install failed: %d", ret);
        return ret;
    }

    // Check chip ID
    uint8_t chip_id;
    ret = bmp280_read_reg(handle, BMP280_REG_ID, &chip_id, 1);
    if (ret != ESP_OK || chip_id != BMP280_CHIP_ID)
    {
        ESP_LOGE(TAG, "BMP280 not found (ID: 0x%02X)", chip_id);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "BMP280 detected (ID: 0x%02X)", chip_id);

    // Read calibration data
    uint8_t calib_data[24];
    ret = bmp280_read_reg(handle, BMP280_REG_CALIB, calib_data, 24);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read calibration data");
        return ret;
    }

    // Parse calibration coefficients
    handle->calib.dig_T1 = (calib_data[1] << 8) | calib_data[0];
    handle->calib.dig_T2 = (calib_data[3] << 8) | calib_data[2];
    handle->calib.dig_T3 = (calib_data[5] << 8) | calib_data[4];
    handle->calib.dig_P1 = (calib_data[7] << 8) | calib_data[6];
    handle->calib.dig_P2 = (calib_data[9] << 8) | calib_data[8];
    handle->calib.dig_P3 = (calib_data[11] << 8) | calib_data[10];
    handle->calib.dig_P4 = (calib_data[13] << 8) | calib_data[12];
    handle->calib.dig_P5 = (calib_data[15] << 8) | calib_data[14];
    handle->calib.dig_P6 = (calib_data[17] << 8) | calib_data[16];
    handle->calib.dig_P7 = (calib_data[19] << 8) | calib_data[18];
    handle->calib.dig_P8 = (calib_data[21] << 8) | calib_data[20];
    handle->calib.dig_P9 = (calib_data[23] << 8) | calib_data[22];

    // Put sensor in sleep mode initially
    uint8_t sleep_mode = handle->mode_config.ctrl_meas_value & 0xFC;
    bmp280_write_reg(handle, BMP280_REG_CTRL_MEAS, sleep_mode);

    const char *mode_name =
        (config->mode == BMP280_MODE_WEATHER_MONITORING) ? "Weather monitoring" : (config->mode == BMP280_MODE_HIGH_RESOLUTION) ? "High resolution"
                                                                                                                                : "Meteo ultra precision";

    ESP_LOGI(TAG, "BMP280 initialized - Mode: %s", mode_name);

    handle->initialized = true;
    return ESP_OK;
}

esp_err_t bmp280_read(bmp280_handle_t *handle, float *temp, float *press)
{
    if (handle == NULL || !handle->initialized || temp == NULL || press == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Trigger forced mode measurement
    esp_err_t ret = bmp280_write_reg(handle, BMP280_REG_CTRL_MEAS,
                                     handle->mode_config.ctrl_meas_value);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to trigger measurement");
        return ret;
    }

    // Wait for measurement to complete
    vTaskDelay(pdMS_TO_TICKS(handle->mode_config.meas_time_ms));

    // Check if measurement is done
    uint8_t status;
    for (int i = 0; i < 10; i++)
    {
        bmp280_read_reg(handle, BMP280_REG_STATUS, &status, 1);
        if ((status & 0x08) == 0)
            break; // measuring bit cleared
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Read sensor data
    uint8_t data[6];
    ret = bmp280_read_reg(handle, BMP280_REG_PRESS_MSB, data, 6);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read sensor data");
        return ret;
    }

    // Parse ADC values
    int32_t adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    int32_t adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);

    // Compensate and convert
    int32_t T = bmp280_compensate_temp(handle, adc_T);
    uint32_t P = bmp280_compensate_press(handle, adc_P);

    *temp = T / 100.0f;
    *press = P / 256.0f;

    ESP_LOGI(TAG, "Temperature: %.2f°C, Pressure: %.2f Pa", *temp, *press);

    return ESP_OK;
}
