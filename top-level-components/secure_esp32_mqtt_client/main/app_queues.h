/*  app_queues.h
    Created: 2019-02-28
    Author: Warren Taylor

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/
#ifndef _APP_QUEUES_H_
#define _APP_QUEUES_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"


//-------------------
#ifdef __cplusplus
#include <sstream>
#include <string>


//*************************************
class AppMQTTQueueNode {
public:
    AppMQTTQueueNode() = default;
    explicit AppMQTTQueueNode(const char *topic, size_t topicSize, const char *data, size_t dataSize)
        : topic{topic, topicSize}, data{data, dataSize}
    { }
    ~AppMQTTQueueNode() = default;

     // Disallow Copy.
    AppMQTTQueueNode(const AppMQTTQueueNode&) = delete;
    AppMQTTQueueNode& operator=(const AppMQTTQueueNode&) = delete;

    // Implement Move.
    AppMQTTQueueNode(AppMQTTQueueNode&&) = default;
    AppMQTTQueueNode& operator=(AppMQTTQueueNode&&) = default;

    const std::string & getTopic() const { return topic; }
    const std::string & getData()  const { return data; }

    esp_err_t queueSendToBack(QueueHandle_t queueHandle);
    esp_err_t queueSendToBack(QueueHandle_t queueHandle, TickType_t queueReceiveDelay);
    esp_err_t queueReceive(QueueHandle_t queueHandle);
    esp_err_t queueReceive(QueueHandle_t queueHandle, TickType_t queueReceiveDelay);

    std::string toString() const {
        std::stringstream sstr;
        sstr << "topic:" << topic << ", data:" << data;
        return sstr.str();
    }

private:
    std::string topic, data;
};


//*************************************
class AppSPIQueueNode {
public:
    AppSPIQueueNode() = default;
    explicit AppSPIQueueNode(const char *data) : data{data}
    { }
    ~AppSPIQueueNode() = default;

    //AppSPIQueueNode(const AppSPIQueueNode&) = default;
    AppSPIQueueNode(AppSPIQueueNode&&) = default;
    //AppSPIQueueNode& operator=(const AppSPIQueueNode&) = default;
    AppSPIQueueNode& operator=(AppSPIQueueNode&&) = default;

    const std::string & getData()  const { return data; }

    esp_err_t queueSendToBack(QueueHandle_t queueHandle);
    esp_err_t queueSendToBack(QueueHandle_t queueHandle, TickType_t queueReceiveDelay);
    esp_err_t queueReceive(QueueHandle_t queueHandle);
    esp_err_t queueReceive(QueueHandle_t queueHandle, TickType_t queueReceiveDelay);

    std::string toString() const {
        return std::string( "data:" + data);
        //std::stringstream sstr;
        //sstr << "data:" << data;
        //return sstr.str();
    }

private:
    std::string data;
};


//class AppSPIReceiveQueueNode : public AppSPIQueueNode {
//};
//class AppSPITransmitQueueNode : public AppSPIQueueNode {
//};


#endif //__cplusplus
//-------------------


#ifdef __cplusplus
extern "C"
{
#endif

extern QueueHandle_t mqttReceivedQueue;
extern QueueHandle_t spiReceivedQueue;
extern QueueHandle_t spiTransmitQueue;

// c wrapper.
extern void app_queues_init(void);

#ifdef __cplusplus
}
#endif


#endif // _APP_QUEUES_H_
