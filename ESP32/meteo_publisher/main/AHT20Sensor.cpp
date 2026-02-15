/**
 * @file AHT20Sensor.cpp
 * @brief C++ wrapper implementation for AHT20
 */

#include "AHT20Sensor.hpp"
#include "esp_log.h"

static const char *TAG = "AHT20Sensor";

AHT20Sensor::AHT20Sensor(i2c_port_t i2c_port,
                         gpio_num_t sda_pin,
                         gpio_num_t scl_pin,
                         uint32_t i2c_freq_hz)
    : m_initialized(false), m_temp_offset(0.0f), m_temp_factor(1.0f), m_humidity_offset(0.0f), m_humidity_factor(1.0f)
{
    aht20_config_t config = {
        .i2c_port = i2c_port,
        .sda_pin = sda_pin,
        .scl_pin = scl_pin,
        .i2c_freq_hz = i2c_freq_hz};

    esp_err_t ret = aht20_init(&m_handle, &config);
    if (ret == ESP_OK)
    {
        m_initialized = true;
        ESP_LOGI(TAG, "AHT20Sensor wrapper initialized");
    }
    else
    {
        ESP_LOGE(TAG, "AHT20Sensor initialization failed");
    }
}

AHT20Sensor::AHT20Sensor(i2c_port_t i2c_port,
                         gpio_num_t sda_pin,
                         gpio_num_t scl_pin,
                         uint32_t i2c_freq_hz,
                         float temp_offset,
                         float temp_factor,
                         float humidity_offset,
                         float humidity_factor)
    : m_initialized(false), m_temp_offset(temp_offset), m_temp_factor(temp_factor), m_humidity_offset(humidity_offset), m_humidity_factor(humidity_factor)
{
    aht20_config_t config = {
        .i2c_port = i2c_port,
        .sda_pin = sda_pin,
        .scl_pin = scl_pin,
        .i2c_freq_hz = i2c_freq_hz};

    esp_err_t ret = aht20_init(&m_handle, &config);
    if (ret == ESP_OK)
    {
        m_initialized = true;
        ESP_LOGI(TAG, "AHT20Sensor wrapper initialized with calibration");
    }
    else
    {
        ESP_LOGE(TAG, "AHT20Sensor initialization failed");
    }
}

bool AHT20Sensor::read_celsius(float *temp)
{
    if (!m_initialized || temp == nullptr)
    {
        return false;
    }

    float humidity;
    return read_temp_humidity(temp, &humidity);
}

bool AHT20Sensor::read_humidity(float *humidity)
{
    if (!m_initialized || humidity == nullptr)
    {
        return false;
    }

    float temp;
    return read_temp_humidity(&temp, humidity);
}

bool AHT20Sensor::read_temp_humidity(float *temp, float *humidity)
{
    if (!m_initialized || temp == nullptr || humidity == nullptr)
    {
        return false;
    }

    float raw_temp, raw_humidity;
    esp_err_t ret = aht20_read(&m_handle, &raw_temp, &raw_humidity);
    if (ret != ESP_OK)
    {
        return false;
    }

    // Apply calibration: calibrated = (raw * factor) + offset
    *temp = (raw_temp * m_temp_factor) + m_temp_offset;
    *humidity = (raw_humidity * m_humidity_factor) + m_humidity_offset;

    return true;
}

bool AHT20Sensor::soft_reset()
{
    if (!m_initialized)
    {
        return false;
    }

    return aht20_soft_reset(&m_handle) == ESP_OK;
}
