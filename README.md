# ESPNow connectivity for the Ikea VINDRIKTNING

Most of the code is bowwored from [Hypfer/esp8266-vindriktning-particle-sensor](https://github.com/Hypfer/esp8266-vindriktning-particle-sensor) that uses an ESP8266 to read from GPIO14(RX) and send via ESPNow to a EspNow Server (check [debsahu/ESPNowMQTT](https://github.com/debsahu/ESPNowMQTT)), which sends it out to a MQTT broker.

## Hardware

- [IKEA VINDRIKTNING](https://www.ikea.com/us/en/p/vindriktning-air-quality-sensor-60515911/) ($11.99)
- [ESP8266](https://www.espressif.com/products/esp8266-esp-module/) ($2.00)
- Dupount cables ($0.50)
- Soldering Iron & solder
- [SCD41 sensor](https://www.sensirion.com/en/environmental-sensors/carbon-dioxide-sensors/carbon-dioxide-sensor-scd4x/) ($50)

## Connections

- 5V on VINDRIKTNING to 5V on ESP8266 VIN
- GND on VINDRIKTNING to GND on ESP8266 GND
- REST on VINDRIKTNING to GPIO14 on ESP8266
- 3.3 of ESP8266 to VCC on SCD41
- GND of ESP8266 to GND of SCD41
- GPIO4 of ESP8266 to SDA of SCD41
- GPIO5 of ESP8266 to SCL of SCD41

![Solder_Points](https://github.com/debsahu/esp8266-vindriktning-particle-sensor/blob/master/img/solder_points.jpg)
![ESP8266](https://github.com/debsahu/esp8266-vindriktning-particle-sensor/blob/master/img/esp8266.jpg)

## Software

- Use platformio to build this sketch
- Edit [`config.h`](https://github.com/debsahu/esp8266-vindriktning-particle-sensor/blob/master/Arduino/config.h)
- Follow ESPNow server code here: [debsahu/ESPNowMQTT](https://github.com/debsahu/ESPNowMQTT)
- Home Assistant

```
sensor:
  - platform: mqtt
    name: "PM 2.5 Sensor 1"
    state_topic: "home/espnow/pm25_sensor1"
    value_template: "{{ value_json.temperature[0] }}"
    device_class: aqi
    icon: mdi:molecule
    unit_of_measurement: "µg/m³"
  
  - platform: mqtt
    name: "Temperature PM 2.5 Sensor 1"
    state_topic: "home/espnow/pm25_sensor1"
    value_template: "{{ value_json.temperature[1] }}"
    device_class: temperature
    icon: mdi:thermometer
    unit_of_measurement: "°C"
  
  - platform: mqtt
    name: "Humidity PM 2.5 Sensor 1"
    state_topic: "home/espnow/pm25_sensor1"
    value_template: "{{ value_json.humidity[0] }}"
    device_class: humidity
    icon: mdi:water-percent
    unit_of_measurement: "%"
  
  - platform: mqtt
    name: "CO2 PM 2.5 Sensor 1"
    state_topic: "home/espnow/pm25_sensor1"
    value_template: "{{ value_json.pressure[0] }}"
    device_class: carbon_dioxide
    icon: mdi:molecule-co2
    unit_of_measurement: "ppm"
  
  - platform: template
    sensors:
     vindriktning_color_1:
        friendly_name: "Vindriktning Color 1"
        value_template: >-
          {% if states('sensor.pm_2_5_sensor_1')|int <= 35 %}
            green
          {% elif states('sensor.pm_2_5_sensor_1')|int >= 36 and states('sensor.pm_2_5_sensor_1')|int < 86 %}
            yellow
          {% else %}
            red
          {% endif %}
```

- Home Assistant Lovelace card (source: [ledhed-jgh/Vindriktning-Card](https://github.com/ledhed-jgh/Vindriktning-Card) - obtain the png images from this repo - check www folder)

```
type: vertical-stack
title: Kitchen Air Quality
cards:
  - type: picture-entity
    entity: sensor.vindriktning_color_1
    show_name: false
    show_state: false
    state_image:
      green: /local/AQI-green.png
      yellow: /local/AQI-yellow.png
      red: /local/AQI-red.png
  - type: entities
    entities:
      - entity: sensor.pm_2_5_sensor_1
        name: Indoor Air Quality
    show_header_toggle: false
    state_color: false
```

## References and sources

- [@Hypfer](https://github.com/Hypfer/esp8266-vindriktning-particle-sensor)
- [Home Assistant Thread](https://community.home-assistant.io/t/ikea-vindriktning-air-quality-sensor/324599)
- [ledhed-jgh/Vindriktning-Card](https://github.com/ledhed-jgh/Vindriktning-Card)
- [@haxfleisch](https://twitter.com/haxfleisch) for their teardown of the device.
- [Gabriel Valky](https://github.com/gabonator) for the incredibly useful [LA104 custom firmware + tools](https://github.com/gabonator/LA104)
