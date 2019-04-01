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
#include <string>


class AppMQTTQueueNode {
public:
    AppMQTTQueueNode(const char *topic, size_t topicSize, const char *data, size_t dataSize)
        : topic{topic, topicSize}, data{data, dataSize}
    { }

    const std::string & getTopic() const { return topic; }
    const std::string & getData()  const { return data; }

private:
    std::string topic, data;
};


class AppSPIQueueNode {
public:
    AppSPIQueueNode(const char *data)
        : data{data}
    { }

private:
    std::string data;
};


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
