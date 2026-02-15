/**
 * @file BMP280Sensor.cpp
 * @brief C++ wrapper implementation for BMP280
 */

#include "BMP280Sensor.hpp"
#include "esp_log.h"

static const char *TAG = "BMP280Sensor";

BMP280Sensor::BMP280Sensor(i2c_port_t i2c_port,
                           uint8_t i2c_addr,
                           gpio_num_t sda_pin,
                           gpio_num_t scl_pin,
                           uint32_t i2c_freq_hz,
                           bmp280_mode_t mode)
    : m_initialized(false), m_temp_offset(0.0f), m_temp_factor(1.0f), m_press_offset(0.0f), m_press_factor(1.0f)
{
    bmp280_config_t config = {
        .i2c_port = i2c_port,
        .i2c_addr = i2c_addr,
        .sda_pin = sda_pin,
        .scl_pin = scl_pin,
        .i2c_freq_hz = i2c_freq_hz,
        .mode = mode};

    esp_err_t ret = bmp280_init(&m_handle, &config);
    if (ret == ESP_OK)
    {
        m_initialized = true;
        ESP_LOGI(TAG, "BMP280Sensor wrapper initialized");
    }
    else
    {
        ESP_LOGE(TAG, "BMP280Sensor initialization failed");
    }
}

BMP280Sensor::BMP280Sensor(i2c_port_t i2c_port,
                           uint8_t i2c_addr,
                           gpio_num_t sda_pin,
                           gpio_num_t scl_pin,
                           uint32_t i2c_freq_hz,
                           bmp280_mode_t mode,
                           float temp_offset,
                           float temp_factor,
                           float press_offset,
                           float press_factor)
    : m_initialized(false), m_temp_offset(temp_offset), m_temp_factor(temp_factor), m_press_offset(press_offset), m_press_factor(press_factor)
{
    bmp280_config_t config = {
        .i2c_port = i2c_port,
        .i2c_addr = i2c_addr,
        .sda_pin = sda_pin,
        .scl_pin = scl_pin,
        .i2c_freq_hz = i2c_freq_hz,
        .mode = mode};

    esp_err_t ret = bmp280_init(&m_handle, &config);
    if (ret == ESP_OK)
    {
        m_initialized = true;
        ESP_LOGI(TAG, "BMP280Sensor wrapper initialized with calibration");
    }
    else
    {
        ESP_LOGE(TAG, "BMP280Sensor initialization failed");
    }
}

bool BMP280Sensor::read_celsius(float *temp)
{
    if (!m_initialized || temp == nullptr)
    {
        return false;
    }

    float pressure;
    return read_temp_pressure(temp, &pressure);
}

bool BMP280Sensor::read_pressure(float *pressure)
{
    if (!m_initialized || pressure == nullptr)
    {
        return false;
    }

    float temp;
    return read_temp_pressure(&temp, pressure);
}

bool BMP280Sensor::read_temp_pressure(float *temp, float *pressure)
{
    if (!m_initialized || temp == nullptr || pressure == nullptr)
    {
        return false;
    }

    float raw_temp, raw_press;
    esp_err_t ret = bmp280_read(&m_handle, &raw_temp, &raw_press);
    if (ret != ESP_OK)
    {
        return false;
    }

    // Apply calibration: calibrated = (raw * factor) + offset
    *temp = (raw_temp * m_temp_factor) + m_temp_offset;
    *pressure = (raw_press * m_press_factor) + m_press_offset;

    return true;
}
