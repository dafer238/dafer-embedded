#include "led.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define LED_GPIO CONFIG_LED_GPIO

static const char *TAG = "LED";

esp_err_t led_init(void)
{
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << LED_GPIO),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  esp_err_t ret = gpio_config(&io_conf);
  if (ret == ESP_OK)
  {
    gpio_set_level(LED_GPIO, 0); // Start with LED off
    ESP_LOGI(TAG, "LED initialized on GPIO %d", LED_GPIO);
  }
  return ret;
}

void led_on(void) { gpio_set_level(LED_GPIO, 1); }

void led_off(void) { gpio_set_level(LED_GPIO, 0); }

void led_blink(int duration_ms)
{
  led_on();
  vTaskDelay(pdMS_TO_TICKS(duration_ms));
  led_off();
}

void led_blink_success(int count)
{
  for (int i = 0; i < count; i++)
  {
    led_on();
    vTaskDelay(pdMS_TO_TICKS(100));
    led_off();
    if (i < count - 1)
    {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

void neopixel_off(int gpio_num)
{
  // WS2812 timing: 0=0.4µs/0.85µs, 1=0.8µs/0.45µs @ 10MHz RMT clock
  rmt_bytes_encoder_config_t encoder_cfg = {
      .bit0 = {
          .level0 = 1,
          .duration0 = 3, // 0.3µs high
          .level1 = 0,
          .duration1 = 9 // 0.9µs low
      },
      .bit1 = {
          .level0 = 1,
          .duration0 = 9, // 0.9µs high
          .level1 = 0,
          .duration1 = 3 // 0.3µs low
      },
      .flags = {.msb_first = 1}};

  rmt_tx_channel_config_t tx_cfg = {
      .gpio_num = (gpio_num_t)gpio_num,
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = 10000000, // 10 MHz
      .mem_block_symbols = 64,
      .trans_queue_depth = 1};

  rmt_channel_handle_t tx_chan = NULL;
  rmt_encoder_handle_t encoder = NULL;
  uint8_t black[3] = {0, 0, 0}; // GRB format: all off

  if (rmt_new_tx_channel(&tx_cfg, &tx_chan) == ESP_OK &&
      rmt_enable(tx_chan) == ESP_OK &&
      rmt_new_bytes_encoder(&encoder_cfg, &encoder) == ESP_OK)
  {

    rmt_transmit_config_t tx_config = {};
    rmt_transmit(tx_chan, encoder, black, sizeof(black), &tx_config);
    rmt_tx_wait_all_done(tx_chan, 100);

    rmt_disable(tx_chan);
    rmt_del_encoder(encoder);
    rmt_del_channel(tx_chan);
  }
}

void neopixel_set_color(int gpio_num, uint8_t r, uint8_t g, uint8_t b)
{
  rmt_bytes_encoder_config_t encoder_cfg = {
      .bit0 = {
          .level0 = 1,
          .duration0 = 3,
          .level1 = 0,
          .duration1 = 9},
      .bit1 = {.level0 = 1, .duration0 = 9, .level1 = 0, .duration1 = 3},
      .flags = {.msb_first = 1}};

  rmt_tx_channel_config_t tx_cfg = {
      .gpio_num = (gpio_num_t)gpio_num,
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = 10000000,
      .mem_block_symbols = 64,
      .trans_queue_depth = 1};

  rmt_channel_handle_t tx_chan = NULL;
  rmt_encoder_handle_t encoder = NULL;
  uint8_t grb[3] = {g, r, b}; // WS2812 uses GRB format

  if (rmt_new_tx_channel(&tx_cfg, &tx_chan) == ESP_OK &&
      rmt_enable(tx_chan) == ESP_OK &&
      rmt_new_bytes_encoder(&encoder_cfg, &encoder) == ESP_OK)
  {
    rmt_transmit_config_t tx_config = {};
    rmt_transmit(tx_chan, encoder, grb, sizeof(grb), &tx_config);
    rmt_tx_wait_all_done(tx_chan, 100);

    rmt_disable(tx_chan);
    rmt_del_encoder(encoder);
    rmt_del_channel(tx_chan);
  }
}

void neopixel_blink(int gpio_num, uint8_t r, uint8_t g, uint8_t b, int duration_ms)
{
  neopixel_set_color(gpio_num, r, g, b);
  vTaskDelay(pdMS_TO_TICKS(duration_ms));
  neopixel_off(gpio_num);
}

void neopixel_blink_success(int gpio_num, uint8_t r, uint8_t g, uint8_t b, int count)
{
  for (int i = 0; i < count; i++)
  {
    neopixel_set_color(gpio_num, r, g, b);
    vTaskDelay(pdMS_TO_TICKS(100));
    neopixel_off(gpio_num);
    if (i < count - 1)
    {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}
