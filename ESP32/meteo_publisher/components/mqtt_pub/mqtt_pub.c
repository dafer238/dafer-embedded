#include "mqtt_pub.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <stdio.h>
#include <time.h>
#include <math.h>

#define MQTT_URI CONFIG_MQTT_BROKER_URI
#define MQTT_USER CONFIG_MQTT_USERNAME
#define MQTT_PASS CONFIG_MQTT_PASSWORD

static const char *TAG = "MQTT";

void mqtt_publish_measurement(const char *device_id, const char *fw,
                              float dht_temp, float dht_rh,
                              float aht20_temp, float aht20_rh,
                              float bmp_temp, float bmp_press,
                              int8_t rssi, float altitude_m,
                              uint32_t free_heap)
{
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_URI,
        .credentials.username = MQTT_USER,
        .credentials.authentication.password = MQTT_PASS,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_start(client);

    vTaskDelay(pdMS_TO_TICKS(2000));

    char payload[512];
    char topic[128];
    int64_t ts = time(NULL);

    snprintf(topic, sizeof(topic), "sensors/%s/environment", device_id);

    // Replace NaN and invalid values with null for valid JSON
    char altitude_str[32];
    if (isnan(altitude_m) || altitude_m < -500.0f || altitude_m > 10000.0f)
    {
        snprintf(altitude_str, sizeof(altitude_str), "null");
    }
    else
    {
        snprintf(altitude_str, sizeof(altitude_str), "%.1f", altitude_m);
    }

    snprintf(payload, sizeof(payload),
             "{"
             "\"device_id\":\"%s\","
             "\"fw\":\"%s\","
             "\"ts_device\":%lld,"
             "\"rssi\":%d,"
             "\"altitude_m\":%s,"
             "\"free_heap\":%lu,"
             "\"dht22\":{\"temperature_c\":%.2f,\"humidity_percent\":%.2f},"
             "\"aht20\":{\"temperature_c\":%.2f,\"humidity_percent\":%.2f},"
             "\"bmp280\":{\"temperature_c\":%.2f,\"pressure_pa\":%.2f}"
             "}",
             device_id, fw, ts, rssi, altitude_str, free_heap, dht_temp, dht_rh, aht20_temp, aht20_rh, bmp_temp, bmp_press);

    ESP_LOGI(TAG, "Payload: %s", payload);

    esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
    ESP_LOGI(TAG, "Published to %s", topic);

    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_mqtt_client_stop(client);
    esp_mqtt_client_destroy(client);
}
