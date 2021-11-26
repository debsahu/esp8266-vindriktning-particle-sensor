#ifdef ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <espnow.h>
#else
#error Platform not supported
#endif

#define SEND_FREQ_SEC 30   // send every this many seconds
#define SERIAL_DEBUG false // change this to true if you want serial data
#define PIN_SDA 4          // SDA pin - GPIO4
#define PIN_SCL 5          // SCL pin - GPIO5

#include "../config.h";
#include "SerialCom.h"
#include "Types.h"

#ifdef SCD4X_ENABLED
#include <SensirionI2CScd4x.h> // https://github.com/Sensirion/arduino-i2c-scd4x
#include <Wire.h>

SensirionI2CScd4x scd4x;
#endif

#ifdef SHT30_ENABLED
#include <SHTSensor.h> //https://github.com/Sensirion/arduino-sht
SHTSensor sht;
#endif

particleSensorState_t state;
unsigned long prevSendMillis = 0;

#ifdef SCD4X_ENABLED
void startSCD41(void)
{
    Wire.begin(PIN_SDA, PIN_SCL);

    uint16_t error;
    char errorMessage[256];

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error && SERIAL_DEBUG)
    {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error && SERIAL_DEBUG)
    {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }
}

bool getSCD41Data(void)
{
    uint16_t error;
    char errorMessage[256];

    uint16_t co2 = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;

    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error)
    {
        if (SERIAL_DEBUG)
        {
            Serial.print("Error trying to execute readMeasurement(): ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
        }
        return false;
    }
    else if (co2 == 0)
    {
        if (SERIAL_DEBUG)
            Serial.println("Invalid sample detected, skipping.");
        return false;
    }
    else
    {
        sensorData.pressure[0] = (float)co2;
        sensorData.temperature[1] = temperature;
        sensorData.humidity[0] = humidity;
        if (SERIAL_DEBUG)
        {
            Serial.print("Co2:");
            Serial.print(co2);
            Serial.print("\t");
            Serial.print("Temperature:");
            Serial.print(temperature);
            Serial.print("\t");
            Serial.print("Humidity:");
            Serial.println(humidity);
        }
    }
    return true;
}
#endif

#ifdef SHT30_ENABLED
bool initSHT30()
{
    Wire.begin();
    if (sht.init())
    {
        if (SERIAL_DEBUG) Serial.print("init(): success\n");
    }
    else
    {
        if (SERIAL_DEBUG) Serial.print("init(): failed\n");
        return false;
    }
    sht.setAccuracy(SHTSensor::SHT_ACCURACY_HIGH);
    return true;
}

bool getSHT30Data(void)
{
    if (sht.readSample())
    {
        float temperature = sht.getTemperature();
        float humidity = sht.getHumidity();
        sensorData.temperature[1] = temperature;
        sensorData.humidity[0] = humidity;
        if (SERIAL_DEBUG)
        {
            Serial.print("Temperature in Celsius : ");
            Serial.println(temperature);
            Serial.print("Relative Humidity : ");
            Serial.println(humidity);
        }
        return true;
    }
    return false;
}
#endif

void sendData(void)
{
    sensorData.temperature[0] = (float)state.avgPM25;
    uint8_t bs[sizeof(sensorData)];
    memcpy(bs, &sensorData, sizeof(sensorData));
    // esp_now_send(NULL, bs, sizeof(sensorData)); // NULL means send to all peers
    esp_now_send(mac, bs, sizeof(sensorData)); // mac of peer
}

void setup()
{
    if (SERIAL_DEBUG)
    {
        Serial.begin(115200);
        Serial.println();
    }

#ifdef SCD4X_ENABLED
    startSCD41();
    getSCD41Data();
#endif
#ifdef SHT30_ENABLED
    initSHT30();
#endif
    SerialCom::setup();

    WiFi.mode(WIFI_STA); // Station mode for esp-now sensor node
    WiFi.disconnect();

    if (SERIAL_DEBUG)
    {
        Serial.printf("This mac: %s, ", WiFi.macAddress().c_str());
        Serial.printf("target mac: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Serial.printf(", channel: %i\n", WIFI_CHANNEL);
    }

    if (esp_now_init() != 0)
    {
        if (SERIAL_DEBUG)
            Serial.println("*** ESP_Now init failed");
        ESP.restart();
    }

#ifdef ESP32
    WiFi.setTxPower(WIFI_POWER_19_5dBm); //max power
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = WIFI_CHANNEL;
    peerInfo.encrypt = false;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        if (SERIAL_DEBUG)
            Serial.println("Failed to add peer");
        ESP.restart();
    }

    esp_now_register_send_cb([](const uint8_t *mac, esp_now_send_status_t sendStatus)
                             {
                                 if (SERIAL_DEBUG)
                                 {
                                     Serial.print("\r\nLast Packet Send Status:\t");
                                     Serial.println(sendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
                                 }
                             });

#elif defined(ESP8266)
    WiFi.setOutputPower(20.5); //max power
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);

    esp_now_register_send_cb([](uint8_t *mac, uint8_t sendStatus)
                             {
                                 if (SERIAL_DEBUG)
                                 {
                                     Serial.printf("send_cb, send done, status = %i\n", sendStatus);
                                     Serial.println(sendStatus == 0 ? "Delivery Success" : "Delivery Fail");
                                 }
                             });
#endif

    memcpy(sensorData.friendlyName, FRIENDLY_NAME, strlen(FRIENDLY_NAME) + 1);
    memcpy(sensorData.deviceName, DEVICE_NAME_FULL, strlen(DEVICE_NAME_FULL) + 1);

    prevSendMillis = millis();
}

void loop()
{
    unsigned long currentMillis = millis();

    SerialCom::handleUart(state);
    if (currentMillis - prevSendMillis >= SEND_FREQ_SEC * 1000)
    {
        if (state.valid)
        {
#ifdef SCD4X_ENABLED
            getSCD41Data();
#endif
#ifdef SHT30_ENABLED
            getSHT30Data();
#endif
            sendData();
        }
        prevSendMillis = millis();
    }
}