#ifndef LED_H
#define LED_H

#include "esp_err.h"

/**
 * @brief Initialize the LED GPIO
 */
esp_err_t led_init(void);

/**
 * @brief Turn LED on
 */
void led_on(void);

/**
 * @brief Turn LED off
 */
void led_off(void);

/**
 * @brief Blink LED once
 * @param duration_ms Duration in milliseconds
 */
void led_blink(int duration_ms);

/**
 * @brief Blink LED multiple times quickly
 * @param count Number of times to blink
 */
void led_blink_success(int count);

/**
 * @brief Turn off the NeoPixel RGB LED on ESP32-S3 DevKit
 * Sends black (0,0,0) via WS2812/RMT protocol
 * @param gpio_num GPIO number for the NeoPixel (typically 47 or 48)
 */
void neopixel_off(int gpio_num);

/**
 * @brief Set NeoPixel RGB LED to a specific color
 * @param gpio_num GPIO number for the NeoPixel
 * @param r Red value (0-255)
 * @param g Green value (0-255)
 * @param b Blue value (0-255)
 */
void neopixel_set_color(int gpio_num, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Blink NeoPixel RGB LED with specified color
 * @param gpio_num GPIO number for the NeoPixel
 * @param r Red value (0-255)
 * @param g Green value (0-255)
 * @param b Blue value (0-255)
 * @param duration_ms Duration in milliseconds
 */
void neopixel_blink(int gpio_num, uint8_t r, uint8_t g, uint8_t b, int duration_ms);

/**
 * @brief Blink NeoPixel RGB LED multiple times with specified color
 * @param gpio_num GPIO number for the NeoPixel
 * @param r Red value (0-255)
 * @param g Green value (0-255)
 * @param b Blue value (0-255)
 * @param count Number of blinks
 */
void neopixel_blink_success(int gpio_num, uint8_t r, uint8_t g, uint8_t b, int count);

#endif // LED_H
