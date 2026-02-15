/**
 * @file app.cpp
 * @brief Main application logic - refactored to C++
 *
 * Uses sensor interfaces for interchangeability.
 * Depends only on interfaces, not concrete implementations.
 */

#include "SensorInterface.hpp"
#include "BMP280Sensor.hpp"
#include "DHT22Sensor.hpp"
#include "AHT20Sensor.hpp"

extern "C"
{
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "led.h"
#include "mqtt_pub.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include <math.h>
}

// ESP32-S3 NeoPixel RGB LED GPIO (from Kconfig or default)
#ifdef CONFIG_NEOPIXEL_GPIO
#define NEOPIXEL_GPIO CONFIG_NEOPIXEL_GPIO
#else
#define NEOPIXEL_GPIO 48
#endif

static const char *TAG = "APP";

/**
 * Calculate altitude from pressure using standard barometric formula
 * @param pressure_pa Pressure in Pascals
 * @param sea_level_pa Sea level pressure (default 101325 Pa)
 * @return Altitude in meters
 */
static float calculate_altitude(float pressure_pa, float sea_level_pa = 101325.0f)
{
    return 44330.0f * (1.0f - powf(pressure_pa / sea_level_pa, 1.0f / 5.225f));
}

// Wrapper functions for LED signaling based on configuration
__attribute__((unused)) static void signal_led_on()
{
#ifdef CONFIG_LED_SIGNALING_ENABLED
#ifdef CONFIG_LED_TYPE_RGB
    neopixel_set_color(NEOPIXEL_GPIO, 0, 0, 255); // Blue
#else
    led_on();
#endif
#endif
}

static void signal_led_off()
{
#ifdef CONFIG_LED_SIGNALING_ENABLED
#ifdef CONFIG_LED_TYPE_RGB
    neopixel_off(NEOPIXEL_GPIO);
#else
    led_off();
#endif
#endif
}

static void signal_led_blink(int duration_ms)
{
#ifdef CONFIG_LED_SIGNALING_ENABLED
#ifdef CONFIG_LED_TYPE_RGB
    neopixel_blink(NEOPIXEL_GPIO, 0, 0, 255, duration_ms); // Blue
#else
    led_blink(duration_ms);
#endif
#endif
}

static void signal_led_blink_success(int count)
{
#ifdef CONFIG_LED_SIGNALING_ENABLED
#ifdef CONFIG_LED_TYPE_RGB
    neopixel_blink_success(NEOPIXEL_GPIO, 0, 0, 255, count); // Blue
#else
    led_blink_success(count);
#endif
#endif
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Boot %s FW %s", CONFIG_NODE_NAME, CONFIG_FW_VERSION);

    // Turn off the NeoPixel RGB LED immediately (always turn off at boot)
    neopixel_off(NEOPIXEL_GPIO);

    // Initialize LED and signal activity
#ifdef CONFIG_LED_SIGNALING_ENABLED
#ifndef CONFIG_LED_TYPE_RGB
    // Only initialize GPIO LED if not using RGB
    ESP_ERROR_CHECK(led_init());
#endif
    signal_led_on();
#else
    // Initialize GPIO LED but don't turn it on
    ESP_ERROR_CHECK(led_init());
#endif

    // Initialize system
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_and_connect();

    // Initialize sensors using C++ wrappers
    ESP_LOGI(TAG, "Initializing sensors...");

    // Sensor pointer for temperature/pressure sensor
    TempPressureSensor *temp_pressure_sensor = nullptr;

#ifdef CONFIG_DHT22_ENABLED
    // Create DHT22 sensor - no calibration applied
    DHT22Sensor dht22(static_cast<gpio_num_t>(CONFIG_DHT22_GPIO),
                      0.0f, 1.0f,  // temp: offset=0, factor=1
                      0.0f, 1.0f); // humidity: offset=0, factor=1

    if (!dht22.is_initialized())
    {
        ESP_LOGE(TAG, "DHT22 initialization failed");
    }
    else
    {
        ESP_LOGI(TAG, "DHT22 sensor enabled");
    }
#endif

#ifdef CONFIG_AHT20_ENABLED
    // Create AHT20 sensor - no calibration applied
    AHT20Sensor aht20(I2C_NUM_0,
                      static_cast<gpio_num_t>(CONFIG_I2C_SDA_GPIO),
                      static_cast<gpio_num_t>(CONFIG_I2C_SCL_GPIO),
                      100000,
                      0.0f, 1.0f,  // temp: offset=0, factor=1
                      0.0f, 1.0f); // humidity: offset=0, factor=1

    if (!aht20.is_initialized())
    {
        ESP_LOGE(TAG, "AHT20 initialization failed");
    }
    else
    {
        ESP_LOGI(TAG, "AHT20 sensor enabled");
    }
#endif

#ifdef CONFIG_BMP280_ENABLED
    // Create BMP280 sensor - apply -1.2°C offset for module heating compensation
    BMP280Sensor bmp280(I2C_NUM_0,
                        CONFIG_BMP280_I2C_ADDR,
                        static_cast<gpio_num_t>(CONFIG_I2C_SDA_GPIO),
                        static_cast<gpio_num_t>(CONFIG_I2C_SCL_GPIO),
                        100000,
                        BMP280_MODE_METEO_ULTRA_PRECISION,
                        0.0f, 1.0f,  // temp: offset=0, factor=1 (applied later if needed)
                        0.0f, 1.0f); // pressure: offset=0, factor=1

    if (!bmp280.is_initialized())
    {
        ESP_LOGE(TAG, "BMP280 initialization failed");
    }
    else
    {
        temp_pressure_sensor = &bmp280;
        ESP_LOGI(TAG, "BMP280 sensor enabled");
    }
#endif

    // Blink to indicate sensor initialization complete
    signal_led_blink(200);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Read sensors using interfaces
    float dht_temp = 0.0f, dht_humidity = 0.0f;
    float aht20_temp = 0.0f, aht20_humidity = 0.0f;
    float bmp_temp = 0.0f, bmp_pressure = 0.0f;

    // Read DHT22 sensor if enabled
#ifdef CONFIG_DHT22_ENABLED
    if (dht22.is_initialized())
    {
        if (!dht22.read_temp_humidity(&dht_temp, &dht_humidity))
        {
            ESP_LOGW(TAG, "Failed to read DHT22 sensor");
            dht_temp = -999.0f;
            dht_humidity = -999.0f;
        }
    }
    else
#endif
    {
        dht_temp = -999.0f;
        dht_humidity = -999.0f;
    }

    // Read AHT20 sensor if enabled
#ifdef CONFIG_AHT20_ENABLED
    if (aht20.is_initialized())
    {
        if (!aht20.read_temp_humidity(&aht20_temp, &aht20_humidity))
        {
            ESP_LOGW(TAG, "Failed to read AHT20 sensor");
            aht20_temp = -999.0f;
            aht20_humidity = -999.0f;
        }
    }
    else
#endif
    {
        aht20_temp = -999.0f;
        aht20_humidity = -999.0f;
    }

    // Read temperature and pressure sensor (BMP280)
    if (temp_pressure_sensor != nullptr)
    {
        if (!temp_pressure_sensor->read_temp_pressure(&bmp_temp, &bmp_pressure))
        {
            ESP_LOGW(TAG, "Failed to read temperature/pressure sensor");
            bmp_temp = -999.0f;
            bmp_pressure = -999.0f;
        }
    }
    else
    {
        ESP_LOGW(TAG, "No temperature/pressure sensor enabled");
        bmp_temp = -999.0f;
        bmp_pressure = -999.0f;
    }

    // Get WiFi signal strength
    int8_t rssi = wifi_get_rssi();

    // Calculate altitude from pressure
    float altitude_m = calculate_altitude(bmp_pressure);

    // Get free heap memory
    uint32_t free_heap = esp_get_free_heap_size();

    ESP_LOGI(TAG, "Altitude: %.1f m, Free heap: %lu bytes", altitude_m, free_heap);

    // Publish measurements
    mqtt_publish_measurement(CONFIG_NODE_NAME, CONFIG_FW_VERSION,
                             dht_temp, dht_humidity,
                             aht20_temp, aht20_humidity,
                             bmp_temp, bmp_pressure,
                             rssi, altitude_m, free_heap);

    // Success indication
    signal_led_blink_success(3);

    ESP_LOGI(TAG, "Sleeping %d ms (%.1f sec)",
             CONFIG_PUBLISH_INTERVAL,
             CONFIG_PUBLISH_INTERVAL / 1000.0f);

    // Turn off LED before deep sleep
    signal_led_off();

    // Enter deep sleep
    esp_sleep_enable_timer_wakeup(CONFIG_PUBLISH_INTERVAL * 1000ULL);
    esp_deep_sleep_start();
}
