/*  app_mqtt.c
    Created: 2019-02-27
    Author: Warren Taylor

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_log.h"
#include "app_mqtt.h"

static const char *LOG_TAG = "APP_MQTT";

static AppMQTT static_app_mqtt;


AppMQTT::AppMQTT()
{
}


//-------------------------------------
// MQTT Events.
//-------------------------------------
esp_err_t AppMQTT::eventHandler(esp_mqtt_event_handle_t event) {
    //esp_mqtt_client_handle_t client = event->client;
    esp_err_t err_code = ESP_OK;
    //int msg_id;

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


    return err_code;
}


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
    AppMQTT *appMQTT = (AppMQTT *)event->user_context;

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
