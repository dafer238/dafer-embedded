/**
 * @file dht22.c
 * @brief DHT22 sensor driver implementation
 */

#include "dht22.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"
#include <string.h>

static const char *TAG = "DHT22";

/**
 * Wait for GPIO to reach specified state with timeout
 * Returns elapsed time in microseconds, or -1 on timeout
 */
static int wait_for_state(gpio_num_t gpio, int state, int timeout_us)
{
    int elapsed = 0;
    while (gpio_get_level(gpio) != state)
    {
        if (elapsed++ > timeout_us)
        {
            return -1;
        }
        ets_delay_us(1);
    }
    return elapsed;
}

esp_err_t dht22_init(dht22_handle_t *handle, const dht22_config_t *config)
{
    if (handle == NULL || config == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Clear handle
    memset(handle, 0, sizeof(dht22_handle_t));

    // Copy configuration
    memcpy(&handle->config, config, sizeof(dht22_config_t));

    // Configure GPIO with internal pullup
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->gpio_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "GPIO config failed");
        return ret;
    }

    gpio_set_level(config->gpio_pin, 1);

    ESP_LOGI(TAG, "DHT22 initialized on GPIO %d", config->gpio_pin);

    handle->initialized = true;
    return ESP_OK;
}

esp_err_t dht22_read(dht22_handle_t *handle, float *temp, float *humidity)
{
    if (handle == NULL || !handle->initialized || temp == NULL || humidity == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[5] = {0};
    gpio_num_t gpio = handle->config.gpio_pin;

    // Disable interrupts during timing-critical section
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    // Send start signal - pull low for at least 1ms
    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio, 0);
    ets_delay_us(1200);
    gpio_set_level(gpio, 1);
    ets_delay_us(30);

    // Switch to input mode with pullup
    gpio_set_direction(gpio, GPIO_MODE_INPUT);
    ets_delay_us(10);

    // Wait for sensor response
    if (wait_for_state(gpio, 0, 100) < 0)
    {
        portEXIT_CRITICAL(&mux);
        ESP_LOGE(TAG, "Timeout waiting for sensor response");
        return ESP_ERR_TIMEOUT;
    }
    if (wait_for_state(gpio, 1, 100) < 0)
    {
        portEXIT_CRITICAL(&mux);
        ESP_LOGE(TAG, "Timeout waiting for sensor ready");
        return ESP_ERR_TIMEOUT;
    }
    if (wait_for_state(gpio, 0, 100) < 0)
    {
        portEXIT_CRITICAL(&mux);
        ESP_LOGE(TAG, "Timeout waiting for data start");
        return ESP_ERR_TIMEOUT;
    }

    // Read 40 bits of data
    bool read_success = true;
    for (int i = 0; i < 40; i++)
    {
        if (wait_for_state(gpio, 1, 70) < 0)
        {
            ESP_LOGE(TAG, "Timeout reading bit %d", i);
            read_success = false;
            break;
        }

        int duration = wait_for_state(gpio, 0, 90);
        if (duration < 0)
            duration = 80;

        data[i / 8] <<= 1;
        if (duration > 40)
        {
            data[i / 8] |= 1;
        }
    }

    portEXIT_CRITICAL(&mux);

    if (!read_success)
    {
        return ESP_ERR_TIMEOUT;
    }

    // Verify checksum
    uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if (data[4] != checksum)
    {
        ESP_LOGE(TAG, "Checksum error: expected 0x%02X, got 0x%02X", checksum, data[4]);
        return ESP_ERR_INVALID_CRC;
    }

    // Parse data
    uint16_t rh_raw = (data[0] << 8) | data[1];
    uint16_t temp_raw = (data[2] << 8) | data[3];

    float raw_humidity = rh_raw / 10.0f;
    float raw_temp = temp_raw / 10.0f;

    // Handle negative temperatures
    if (temp_raw & 0x8000)
    {
        raw_temp = -(temp_raw & 0x7FFF) / 10.0f;
    }

    // Sanity check: DHT22 range is -40 to 80°C, 0-100% RH
    if (raw_temp < -40.0f || raw_temp > 80.0f)
    {
        ESP_LOGE(TAG, "Temperature out of range: %.1f°C", raw_temp);
        return ESP_ERR_INVALID_RESPONSE;
    }
    if (raw_humidity < 0.0f || raw_humidity > 100.0f)
    {
        ESP_LOGE(TAG, "Humidity out of range: %.1f%%", raw_humidity);
        return ESP_ERR_INVALID_RESPONSE;
    }

    *temp = raw_temp;
    *humidity = raw_humidity;

    ESP_LOGI(TAG, "Temperature: %.1f°C, Humidity: %.1f%%", *temp, *humidity);

    return ESP_OK;
}
