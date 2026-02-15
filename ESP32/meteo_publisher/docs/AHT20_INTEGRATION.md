# AHT20 Sensor Integration

This document shows how to use the AHT20 temperature and humidity sensor in your application.

## Overview

The AHT20 is an I²C-based temperature and humidity sensor with high precision:
- **Temperature**: -40°C to +85°C, accuracy ±0.3°C
- **Humidity**: 0-100% RH, accuracy ±2% RH
- **Interface**: I²C (fixed address 0x38)
- **Measurement time**: ~80ms

## Hardware Connection

```
ESP32          AHT20
-----          -----
GPIO 21 (SDA)  SDA
GPIO 22 (SCL)  SCL
3.3V           VCC
GND            GND
```

**Note**: The AHT20 uses a fixed I²C address (0x38), so you can only connect one AHT20 per I²C bus.

## Software Integration

### Option 1: Using AHT20 Instead of DHT22

If you want to replace DHT22 with AHT20 in your application:

**File: `main/app.cpp`**
```cpp
#include "AHT20Sensor.hpp"

// Replace DHT22Sensor with AHT20Sensor
AHT20Sensor temp_humidity_sensor(
    I2C_NUM_0,
    static_cast<gpio_num_t>(CONFIG_I2C_SDA_GPIO),
    static_cast<gpio_num_t>(CONFIG_I2C_SCL_GPIO),
    100000  // 100kHz I²C frequency
);

// Use the same interface
TempHumiditySensor *sensor = &temp_humidity_sensor;
float temp, humidity;
if (sensor->read_temp_humidity(&temp, &humidity)) {
    ESP_LOGI(TAG, "Temp: %.2f°C, Humidity: %.2f%%", temp, humidity);
}
```

### Option 2: Using Both DHT22 and AHT20

If you want to use both sensors:

**File: `main/app.cpp`**
```cpp
#include "DHT22Sensor.hpp"
#include "AHT20Sensor.hpp"

// DHT22 on GPIO 4
DHT22Sensor dht22(GPIO_NUM_4);

// AHT20 on I²C
AHT20Sensor aht20(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22, 100000);

// Read from both
float dht_temp, dht_humidity;
float aht_temp, aht_humidity;

if (dht22.read_temp_humidity(&dht_temp, &dht_humidity)) {
    ESP_LOGI(TAG, "DHT22 - Temp: %.2f°C, RH: %.2f%%", dht_temp, dht_humidity);
}

if (aht20.read_temp_humidity(&aht_temp, &aht_humidity)) {
    ESP_LOGI(TAG, "AHT20 - Temp: %.2f°C, RH: %.2f%%", aht_temp, aht_humidity);
}

// Average the readings
float avg_temp = (dht_temp + aht_temp) / 2.0f;
float avg_humidity = (dht_humidity + aht_humidity) / 2.0f;
```

### Option 3: Using AHT20 with Calibration

If you need to apply calibration factors:

```cpp
// AHT20 with calibration
// temp_calibrated = (raw * factor) + offset
AHT20Sensor aht20(
    I2C_NUM_0,
    GPIO_NUM_21,
    GPIO_NUM_22,
    100000,
    -0.5f,   // temp offset: -0.5°C
    1.0f,    // temp factor: 1.0
    0.0f,    // humidity offset: 0%
    1.0f     // humidity factor: 1.0
);
```

## Complete Example

Here's a complete example replacing DHT22 with AHT20:

**File: `main/app_with_aht20.cpp`**
```cpp
#include "AHT20Sensor.hpp"
#include "BMP280Sensor.hpp"
#include "SensorInterface.hpp"

extern "C" {
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "led.h"
#include "mqtt_pub.h"
#include "nvs_flash.h"
#include "wifi.h"
#include <math.h>
}

static const char *TAG = "APP";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Boot %s FW %s", CONFIG_NODE_NAME, CONFIG_FW_VERSION);

    ESP_ERROR_CHECK(led_init());
    led_on();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_and_connect();

    ESP_LOGI(TAG, "Initializing sensors...");
    
    // Create AHT20 sensor (I²C)
    AHT20Sensor aht20(I2C_NUM_0,
                      static_cast<gpio_num_t>(CONFIG_I2C_SDA_GPIO),
                      static_cast<gpio_num_t>(CONFIG_I2C_SCL_GPIO),
                      100000);
    
    if (!aht20.is_initialized()) {
        ESP_LOGE(TAG, "AHT20 initialization failed");
    } else if (!aht20.is_calibrated()) {
        ESP_LOGW(TAG, "AHT20 not calibrated");
    }
    
    // Create BMP280 sensor (I²C) - note: different I²C address
    BMP280Sensor bmp280(I2C_NUM_0,
                        CONFIG_BMP280_I2C_ADDR,
                        static_cast<gpio_num_t>(CONFIG_I2C_SDA_GPIO),
                        static_cast<gpio_num_t>(CONFIG_I2C_SCL_GPIO),
                        100000,
                        BMP280_MODE_METEO_ULTRA_PRECISION,
                        0.0f, 1.0f, 0.0f, 1.0f);
    
    if (!bmp280.is_initialized()) {
        ESP_LOGE(TAG, "BMP280 initialization failed");
    }

    led_blink(200);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Read sensors
    float aht_temp = 0.0f, aht_humidity = 0.0f;
    float bmp_temp = 0.0f, bmp_pressure = 0.0f;
    
    // Use polymorphic interface
    TempHumiditySensor *temp_humidity_sensor = &aht20;
    if (!temp_humidity_sensor->read_temp_humidity(&aht_temp, &aht_humidity)) {
        ESP_LOGW(TAG, "Failed to read AHT20");
        aht_temp = -999.0f;
        aht_humidity = -999.0f;
    }
    
    TempPressureSensor *temp_pressure_sensor = &bmp280;
    if (!temp_pressure_sensor->read_temp_pressure(&bmp_temp, &bmp_pressure)) {
        ESP_LOGW(TAG, "Failed to read BMP280");
        bmp_temp = -999.0f;
        bmp_pressure = -999.0f;
    }

    int8_t rssi = wifi_get_rssi();
    float altitude_m = 44330.0f * (1.0f - powf(bmp_pressure / 101325.0f, 1.0f / 5.225f));
    uint32_t free_heap = esp_get_free_heap_size();
    
    ESP_LOGI(TAG, "Altitude: %.1f m, Free heap: %lu bytes", altitude_m, free_heap);

    // Publish measurements
    mqtt_publish_measurement(CONFIG_NODE_NAME, CONFIG_FW_VERSION,
                           aht_temp, aht_humidity,
                           bmp_temp, bmp_pressure,
                           rssi, altitude_m, free_heap);

    led_blink_success(3);

    ESP_LOGI(TAG, "Sleeping %d ms (%.1f sec)",
             CONFIG_PUBLISH_INTERVAL,
             CONFIG_PUBLISH_INTERVAL / 1000.0f);
    
    led_off();
    
    esp_sleep_enable_timer_wakeup(CONFIG_PUBLISH_INTERVAL * 1000ULL);
    esp_deep_sleep_start();
}
```

## API Reference

### Constructor

```cpp
// Basic constructor
AHT20Sensor(i2c_port_t i2c_port,
            gpio_num_t sda_pin,
            gpio_num_t scl_pin,
            uint32_t i2c_freq_hz);

// Constructor with calibration
AHT20Sensor(i2c_port_t i2c_port,
            gpio_num_t sda_pin,
            gpio_num_t scl_pin,
            uint32_t i2c_freq_hz,
            float temp_offset,
            float temp_factor,
            float humidity_offset,
            float humidity_factor);
```

### Methods

```cpp
// Read temperature only (°C)
bool read_celsius(float *temp);

// Read humidity only (%)
bool read_humidity(float *humidity);

// Read both temperature and humidity
bool read_temp_humidity(float *temp, float *humidity);

// Perform soft reset
bool soft_reset();

// Check initialization status
bool is_initialized() const;

// Check calibration status
bool is_calibrated() const;
```

## Troubleshooting

### AHT20 Not Detected

**Symptom**: "AHT20 initialization failed"

**Solutions**:
1. Check I²C connections (SDA, SCL, VCC, GND)
2. Verify I²C address is 0x38 (use `i2cdetect` if available)
3. Check pull-up resistors on I²C lines (typically 4.7kΩ)
4. Verify power supply is 3.3V

### AHT20 Not Calibrated

**Symptom**: "AHT20 not calibrated"

**Solutions**:
1. The driver automatically attempts calibration
2. Try soft reset: `aht20.soft_reset()`
3. Power cycle the sensor
4. Check sensor datasheet for calibration requirements

### Readings Out of Range

**Symptom**: Temperature or humidity outside expected range

**Solutions**:
1. Check if sensor is properly calibrated
2. Verify electrical connections
3. Try soft reset
4. Apply calibration factors if needed

### I²C Bus Conflict

**Symptom**: BMP280 and AHT20 not working together

**Solutions**:
1. Both sensors use I²C - check they have different addresses
   - BMP280: 0x76 or 0x77 (configurable)
   - AHT20: 0x38 (fixed)
2. Ensure both sensors are initialized on same I²C bus
3. Check I²C bus speed is compatible (100kHz recommended)

## Advantages vs DHT22

| Feature              | DHT22                  | AHT20                  |
| -------------------- | ---------------------- | ---------------------- |
| Interface            | 1-wire (bit-banging)   | I²C (hardware)         |
| Timing critical      | Yes                    | No                     |
| Temperature accuracy | ±0.5°C                 | ±0.3°C                 |
| Humidity accuracy    | ±2-5% RH               | ±2% RH                 |
| Measurement time     | ~2s                    | ~80ms                  |
| Multiple sensors     | Easy (different GPIOs) | Harder (fixed address) |
| CPU usage            | High (bit-banging)     | Low (hardware I²C)     |
| Reliability          | Good                   | Excellent              |

## Configuration Options

Add to `sdkconfig` or use `idf.py menuconfig`:

```
CONFIG_AHT20_ENABLED=y
```

Then in code:
```cpp
#ifdef CONFIG_AHT20_ENABLED
    AHT20Sensor aht20(...);
#else
    DHT22Sensor dht22(...);
#endif
```

## Performance Notes

- **Initialization**: ~50ms including calibration check
- **Measurement**: ~80ms per reading
- **I²C Speed**: 100kHz recommended (400kHz supported)
- **Memory**: ~100 bytes for handle

## Further Reading

- [AHT20 Datasheet](http://www.aosong.com/en/products-40.html)
- [ESP-IDF I²C Driver Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html)
- See `ARCHITECTURE.md` for overall design
- See `QUICK_REFERENCE.md` for adding more sensors
