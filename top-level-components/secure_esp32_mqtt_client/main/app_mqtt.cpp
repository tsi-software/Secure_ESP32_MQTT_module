/*  app_mqtt.cpp
    Created: 2019-02-27
    Author: Warren Taylor

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
//#include "freertos/task.h"
#include "esp_log.h"

#include "app_mqtt.h"
#include "app_queues.h"


static const char *LOG_TAG = "APP_MQTT";
static AppMQTT static_app_mqtt;


//-------------------------------------
// MQTT Events.
//-------------------------------------
esp_err_t AppMQTT::eventHandler(esp_mqtt_event_handle_t event) {
    esp_err_t err_code = ESP_OK;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGV(LOG_TAG, "MQTT_EVENT_CONNECTED");
            //test_subscribe(client);
            err_code = connected(event);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGV(LOG_TAG, "MQTT_EVENT_DISCONNECTED");
            err_code = disconnected(event);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGV(LOG_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            //ESP_LOGI(LOG_TAG, "sent publish successful, msg_id=%d", msg_id);
            err_code = subscribed(event);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGV(LOG_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            err_code = unsubscribed(event);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGV(LOG_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            err_code = published(event);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGV(LOG_TAG, "MQTT_EVENT_DATA");
            ESP_LOGV(LOG_TAG, "TOPIC=%s", event->topic);
            ESP_LOGV(LOG_TAG, "DATA=%s", event->data);
            err_code = dataReceived(event);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(LOG_TAG, "MQTT_EVENT_ERROR");
            err_code = errorOccurred(event);
            break;
    }

    return err_code;
}


esp_err_t AppMQTT::connected(esp_mqtt_event_handle_t event) {
    esp_err_t err_code = ESP_OK;
    return err_code;
}


esp_err_t AppMQTT::disconnected(esp_mqtt_event_handle_t event) {
    esp_err_t err_code = ESP_OK;
    return err_code;
}


esp_err_t AppMQTT::subscribed(esp_mqtt_event_handle_t event) {
    esp_err_t err_code = ESP_OK;
    return err_code;
}


esp_err_t AppMQTT::unsubscribed(esp_mqtt_event_handle_t event) {
    esp_err_t err_code = ESP_OK;
    return err_code;
}


esp_err_t AppMQTT::published(esp_mqtt_event_handle_t event) {
    esp_err_t err_code = ESP_OK;
    return err_code;
}


esp_err_t AppMQTT::dataReceived(esp_mqtt_event_handle_t event) {
    esp_err_t err_code = ESP_OK;
    AppMQTTQueueNode *node = new AppMQTTQueueNode(event->topic, event->data);

    //TODO: perhaps move the following block of code to "app_queues.cpp".
    //portMAX_DELAY
    // 10 milllisecond delay.
    const TickType_t delay = 10 / portTICK_PERIOD_MS;
    BaseType_t result = xQueueSendToBack(mqttReceivedQueue, node, delay);
    if (result == pdFALSE) {
        // The queue was full and timed out.
        delete node;
        node = nullptr;
        ESP_LOGE(
            LOG_TAG,
            "AppMQTT::dataReceived(...): mqttReceivedQueue was full!\ntopic:%s\ndata:%s\n",
            event->topic,
            event->data
        );
        return ESP_ERR_TIMEOUT;
    }

    return err_code;
}

#ifdef IGNORE_THIS //never defined!
typedef struct {
    esp_mqtt_event_id_t event_id;       /*!< MQTT event type */
    esp_mqtt_client_handle_t client;    /*!< MQTT client handle for this event */
    void *user_context;                 /*!< User context passed from MQTT client config */
    char *data;                         /*!< Data asociated with this event */
    int data_len;                       /*!< Lenght of the data for this event */
    int total_data_len;                 /*!< Total length of the data (longer data are supplied with multiple events) */
    int current_data_offset;            /*!< Actual offset for the data asociated with this event */
    char *topic;                        /*!< Topic asociated with this event */
    int topic_len;                      /*!< Length of the topic for this event asociated with this event */
    int msg_id;                         /*!< MQTT messaged id of message */
    int session_present;                /*!< MQTT session_present flag for connection event */
} esp_mqtt_event_t;
#endif //IGNORE_THIS


esp_err_t AppMQTT::errorOccurred(esp_mqtt_event_handle_t event) {
    esp_err_t err_code = ESP_OK;
    return err_code;
}


//-------------------------------------
// c wrappers.
//-------------------------------------
void *get_static_app_mqtt(void) {
    return & static_app_mqtt;
}


esp_err_t app_mqtt_event_handler(esp_mqtt_event_handle_t event) {
    AppMQTT *appMQTT = static_cast<AppMQTT *>(event->user_context);
    //AppMQTT *appMQTT = (AppMQTT *)event->user_context;

    if (!appMQTT) {
        ESP_LOGE(LOG_TAG, "app_mqtt_event_handler(...): event->user_context is NULL!");
        return ESP_ERR_INVALID_ARG;
    }

    return appMQTT->eventHandler(event);
}


/**
static void test_subscribe(esp_mqtt_client_handle_t client) {
    int msg_id;

    msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
    ESP_LOGI(LOG_TAG, "sent subscribe successful, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
    ESP_LOGI(LOG_TAG, "sent subscribe successful, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
    ESP_LOGI(LOG_TAG, "sent unsubscribe successful, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_subscribe(client, "irrigation/zone/on", 0);
    ESP_LOGI(LOG_TAG, "subscribe 'irrigation/zone/on' successful, msg_id=%d", msg_id);
}
**/
