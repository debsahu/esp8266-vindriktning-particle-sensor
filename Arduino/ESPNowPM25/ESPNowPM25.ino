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

#define SEND_FREQ_SEC 30 // send every this many seconds
#define SERIAL_DEBUG false // change this to true if you want serial data

#include "../config.h";
#include "SerialCom.h"
#include "Types.h"

particleSensorState_t state;

unsigned long prevSendMillis = 0;

void sendData(void)
{
    sensorData.temperature[0] = (float) state.avgPM25;
    uint8_t bs[sizeof(sensorData)];
    memcpy(bs, &sensorData, sizeof(sensorData));
    // esp_now_send(NULL, bs, sizeof(sensorData)); // NULL means send to all peers
    esp_now_send(mac, bs, sizeof(sensorData)); // mac of peer
}

void setup()
{
    if(SERIAL_DEBUG)
    {
        Serial.begin(115200);
        Serial.println();
    }
    SerialCom::setup();

    WiFi.mode(WIFI_STA); // Station mode for esp-now sensor node
    WiFi.disconnect();

    if(SERIAL_DEBUG)
    {
        Serial.printf("This mac: %s, ", WiFi.macAddress().c_str());
        Serial.printf("target mac: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Serial.printf(", channel: %i\n", WIFI_CHANNEL);
    }

    if (esp_now_init() != 0)
    {
        if(SERIAL_DEBUG)
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
        if(SERIAL_DEBUG) Serial.println("Failed to add peer");
        ESP.restart();
    }

    esp_now_register_send_cb([](const uint8_t *mac, esp_now_send_status_t sendStatus) {
        if(SERIAL_DEBUG)
        {
            Serial.print("\r\nLast Packet Send Status:\t");
            Serial.println(sendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
        }
    });

#elif defined(ESP8266)
    WiFi.setOutputPower(20.5); //max power
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);

    esp_now_register_send_cb([](uint8_t *mac, uint8_t sendStatus) {
        if(SERIAL_DEBUG)
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

    if(currentMillis - prevSendMillis >= SEND_FREQ_SEC * 1000)
    {
        if(state.valid)
        {
            sendData();
        }
        prevSendMillis = millis();
    }
}