idf_component_register(
    SRCS
        "app.cpp"
        "BMP280Sensor.cpp"
        "DHT22Sensor.cpp"
        "AHT20Sensor.cpp"
    INCLUDE_DIRS "."
    REQUIRES bmp280 dht22 aht20 led wifi mqtt_pub
)
