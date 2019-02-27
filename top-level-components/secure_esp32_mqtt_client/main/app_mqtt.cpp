/* app_mqtt.c
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
// c wrappers.
//-------------------------------------
void *get_static_app_mqtt(void) {
    return & static_app_mqtt;
}


static void test_subscribe(esp_mqtt_client_handle_t client) {
    int msg_id;

    ESP_LOGI(LOG_TAG, "MQTT_EVENT_CONNECTED");
    msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
    ESP_LOGI(LOG_TAG, "sent subscribe successful, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
    ESP_LOGI(LOG_TAG, "sent subscribe successful, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
    ESP_LOGI(LOG_TAG, "sent unsubscribe successful, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_subscribe(client, "irrigation/zone/on", 0);
    ESP_LOGI(LOG_TAG, "subscribe 'irrigation/zone/on' successful, msg_id=%d", msg_id);
}


esp_err_t app_mqtt_event_handler(esp_mqtt_event_handle_t event) {
    esp_mqtt_client_handle_t client = event->client;
    AppMQTT *appMQTT = (AppMQTT *)event->user_context;
    //int msg_id;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            test_subscribe(client);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(LOG_TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(LOG_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            //ESP_LOGI(LOG_TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(LOG_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(LOG_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(LOG_TAG, "MQTT_EVENT_DATA");
            ESP_LOGI(LOG_TAG, "TOPIC=%s", event->topic);
            ESP_LOGI(LOG_TAG, "DATA=%s", event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(LOG_TAG, "MQTT_EVENT_ERROR");
            break;
    }

    return ESP_OK;
}
