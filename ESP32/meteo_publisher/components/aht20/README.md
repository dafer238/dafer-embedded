# AHT20 Sensor Component

High-precision I²C temperature and humidity sensor driver for ESP-IDF.

## Overview

Pure C driver for AHT20 sensor following the clean C/C++ architecture pattern.

## Features

- ✅ Handle-based API (no global state)
- ✅ High precision: ±0.3°C temperature, ±2% RH humidity
- ✅ Fast measurements: ~80ms
- ✅ Automatic calibration check
- ✅ Soft reset support
- ✅ No heap allocation
- ✅ Reusable and portable

## Hardware

- **I²C Address**: 0x38 (fixed)
- **Supply Voltage**: 2.2V to 5.5V (3.3V recommended)
- **Temperature Range**: -40°C to +85°C
- **Humidity Range**: 0-100% RH
- **Measurement Time**: ~80ms

## Usage

```c
#include "aht20.h"

// Allocate handle (stack or static)
aht20_handle_t aht20;

// Configure
aht20_config_t config = {
    .i2c_port = I2C_NUM_0,
    .sda_pin = GPIO_NUM_21,
    .scl_pin = GPIO_NUM_22,
    .i2c_freq_hz = 100000
};

// Initialize
esp_err_t ret = aht20_init(&aht20, &config);
if (ret != ESP_OK) {
    // Handle error
}

// Read measurements
float temp, humidity;
ret = aht20_read(&aht20, &temp, &humidity);
if (ret == ESP_OK) {
    printf("Temperature: %.2f°C, Humidity: %.2f%%\n", temp, humidity);
}
```

## API Reference

### Types

```c
typedef struct {
    i2c_port_t i2c_port;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    uint32_t i2c_freq_hz;
} aht20_config_t;

typedef struct {
    aht20_config_t config;
    bool initialized;
    bool calibrated;
} aht20_handle_t;
```

### Functions

```c
// Initialize sensor
esp_err_t aht20_init(aht20_handle_t *handle, const aht20_config_t *config);

// Read temperature and humidity
esp_err_t aht20_read(aht20_handle_t *handle, float *temp, float *humidity);

// Soft reset sensor
esp_err_t aht20_soft_reset(aht20_handle_t *handle);
```

## Building

This component is automatically built when included in an ESP-IDF project.

```cmake
# In your component's CMakeLists.txt
idf_component_register(
    ...
    REQUIRES aht20
)
```

## Wiring

```
ESP32    AHT20
-----    -----
GPIO21 - SDA
GPIO22 - SCL
3.3V   - VCC
GND    - GND
```

**Note**: Pull-up resistors (4.7kΩ) are recommended on SDA and SCL lines.

## Timing

- **Initialization**: ~50ms
- **Measurement**: ~80ms
- **Soft Reset**: ~20ms

## Error Handling

All functions return `esp_err_t`:
- `ESP_OK`: Success
- `ESP_ERR_INVALID_ARG`: NULL pointer or invalid parameter
- `ESP_ERR_TIMEOUT`: I²C timeout or sensor busy
- `ESP_ERR_INVALID_STATE`: Sensor not ready
- `ESP_FAIL`: General failure

## Thread Safety

The driver is not thread-safe. Caller must ensure:
- Only one thread accesses a handle at a time
- Use FreeRTOS mutex if needed for multi-threaded access

## Porting

To port to another MCU:
1. Replace I²C functions with platform-specific equivalents
2. Replace delay functions (`vTaskDelay`)
3. Replace logging (`ESP_LOGI`, etc.)
4. Keep the same handle-based architecture

## License

See project license.
