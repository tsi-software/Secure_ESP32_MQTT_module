/*  app_queues.cpp
    Created: 2019-02-28
    Author: Warren Taylor

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "app_queues.h"

static const char *LOG_TAG = "APP_QUEUES";


//-------------------------------------
// MQTT Received Queue.
//-------------------------------------
#define MQTT_RX_QUEUE_LENGTH 4
#define MQTT_RX_ITEM_SIZE sizeof( AppMQTTQueueNode * )
#if (configSUPPORT_STATIC_ALLOCATION == 1)
static uint8_t mqttRxQueueStorage[ MQTT_RX_QUEUE_LENGTH * MQTT_RX_ITEM_SIZE ];
static StaticQueue_t mqttRxQueueBuffer;
#endif // configSUPPORT_STATIC_ALLOCATION
QueueHandle_t mqttReceivedQueue;


//-------------------------------------
// SPI Received Queue.
//-------------------------------------
#define SPI_RX_QUEUE_LENGTH 4
#define SPI_RX_ITEM_SIZE sizeof( AppSPIQueueNode * )
#if (configSUPPORT_STATIC_ALLOCATION == 1)
static uint8_t spiRxQueueStorage[ SPI_RX_QUEUE_LENGTH * SPI_RX_ITEM_SIZE ];
static StaticQueue_t spiRxQueueBuffer;
#endif // configSUPPORT_STATIC_ALLOCATION
QueueHandle_t spiReceivedQueue;


//-------------------------------------
// SPI Transmit Queue.
//-------------------------------------
#define SPI_TX_QUEUE_LENGTH 4
#define SPI_TX_ITEM_SIZE sizeof( AppSPIQueueNode * )
#if (configSUPPORT_STATIC_ALLOCATION == 1)
static uint8_t spiTxQueueStorage[ SPI_TX_QUEUE_LENGTH * SPI_TX_ITEM_SIZE ];
static StaticQueue_t spiTxQueueBuffer;
#endif // configSUPPORT_STATIC_ALLOCATION
QueueHandle_t spiTransmitQueue;


//-------------------------------------
// app_queues_init()
//-------------------------------------
void app_queues_init(void) {
    //----------------------
    // MQTT Received Queue.
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    mqttReceivedQueue = xQueueCreateStatic(
        MQTT_RX_QUEUE_LENGTH,
        MQTT_RX_ITEM_SIZE,
        mqttRxQueueStorage,
        &mqttRxQueueBuffer
    );
#else
    mqttReceivedQueue = xQueueCreate(
        MQTT_RX_QUEUE_LENGTH,
        MQTT_RX_ITEM_SIZE
    );
#endif
    configASSERT(mqttReceivedQueue);
    ESP_LOGI(LOG_TAG, "mqttReceivedQueue initialized.");

    //----------------------
    // SPI Received Queue.
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    spiReceivedQueue = xQueueCreateStatic(
        SPI_RX_QUEUE_LENGTH,
        SPI_RX_ITEM_SIZE,
        spiRxQueueStorage,
        &spiRxQueueBuffer
    );
#else
    spiReceivedQueue = xQueueCreate(
        SPI_RX_QUEUE_LENGTH,
        SPI_RX_ITEM_SIZE
    );
#endif
    configASSERT(spiReceivedQueue);
    ESP_LOGI(LOG_TAG, "spiReceivedQueue initialized.");

    //----------------------
    // SPI Transmit Queue.
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    spiTransmitQueue = xQueueCreateStatic(
        SPI_TX_QUEUE_LENGTH,
        SPI_TX_ITEM_SIZE,
        spiTxQueueStorage,
        &spiTxQueueBuffer
    );
#else
    spiTransmitQueue = xQueueCreate(
        SPI_TX_QUEUE_LENGTH,
        SPI_TX_ITEM_SIZE
    );
#endif
    configASSERT(spiTransmitQueue);
    ESP_LOGI(LOG_TAG, "spiTransmitQueue initialized.");
}
