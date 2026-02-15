/**
 * @file DHT22Sensor.cpp
 * @brief C++ wrapper implementation for DHT22
 */

#include "DHT22Sensor.hpp"
#include "esp_log.h"

static const char *TAG = "DHT22Sensor";

DHT22Sensor::DHT22Sensor(gpio_num_t gpio_pin)
    : m_initialized(false), m_temp_offset(0.0f), m_temp_factor(1.0f), m_humidity_offset(0.0f), m_humidity_factor(1.0f)
{
    dht22_config_t config = {
        .gpio_pin = gpio_pin};

    esp_err_t ret = dht22_init(&m_handle, &config);
    if (ret == ESP_OK)
    {
        m_initialized = true;
        ESP_LOGI(TAG, "DHT22Sensor wrapper initialized");
    }
    else
    {
        ESP_LOGE(TAG, "DHT22Sensor initialization failed");
    }
}

DHT22Sensor::DHT22Sensor(gpio_num_t gpio_pin,
                         float temp_offset,
                         float temp_factor,
                         float humidity_offset,
                         float humidity_factor)
    : m_initialized(false), m_temp_offset(temp_offset), m_temp_factor(temp_factor), m_humidity_offset(humidity_offset), m_humidity_factor(humidity_factor)
{
    dht22_config_t config = {
        .gpio_pin = gpio_pin};

    esp_err_t ret = dht22_init(&m_handle, &config);
    if (ret == ESP_OK)
    {
        m_initialized = true;
        ESP_LOGI(TAG, "DHT22Sensor wrapper initialized with calibration");
    }
    else
    {
        ESP_LOGE(TAG, "DHT22Sensor initialization failed");
    }
}

bool DHT22Sensor::read_celsius(float *temp)
{
    if (!m_initialized || temp == nullptr)
    {
        return false;
    }

    float humidity;
    return read_temp_humidity(temp, &humidity);
}

bool DHT22Sensor::read_humidity(float *humidity)
{
    if (!m_initialized || humidity == nullptr)
    {
        return false;
    }

    float temp;
    return read_temp_humidity(&temp, humidity);
}

bool DHT22Sensor::read_temp_humidity(float *temp, float *humidity)
{
    if (!m_initialized || temp == nullptr || humidity == nullptr)
    {
        return false;
    }

    float raw_temp, raw_humidity;
    esp_err_t ret = dht22_read(&m_handle, &raw_temp, &raw_humidity);
    if (ret != ESP_OK)
    {
        return false;
    }

    // Apply calibration: calibrated = (raw * factor) + offset
    *temp = (raw_temp * m_temp_factor) + m_temp_offset;
    *humidity = (raw_humidity * m_humidity_factor) + m_humidity_offset;

    return true;
}
