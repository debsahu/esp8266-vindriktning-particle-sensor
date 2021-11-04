#include <pgmspace.h>

const char FRIENDLY_NAME[] = "pm25_sensor1"; // do not uses spaces
const char DEVICE_NAME_FULL[] = "PM2.5 sensor 1";

#define WIFI_CHANNEL 1
/////// Make sure WiFi channel is same as 'wifi_ssid' /////////
const char WIFI_SSID[] = "WIFI_SSID"; // not used by sender
const char WIFI_PASS[] = "WIFI_PASS"; // not used by sender

const char ESPSSID[] = "ESPNOWNETWORK";
const char ESPPASS[] = "ESP826632";

const char MQTT_HOST[] = "MQTT_IP_ADDRESS"; // not used by sender
int MQTT_PORT = 1883; // not used by sender
const char MQTT_USER[] = "MQTT_USER"; // not used by sender
const char MQTT_PASS[] = "MQTT_PASS"; // not used by sender

struct __attribute__((packed)) SENSOR_DATA
{
    bool motion;
    char friendlyName[20];
    char deviceName[20];
    uint8_t battery_percent;
    float temperature[10];
    float humidity[10];
    float pressure[10];
} sensorData;

uint8_t mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // This is mac of ESPNow Server
