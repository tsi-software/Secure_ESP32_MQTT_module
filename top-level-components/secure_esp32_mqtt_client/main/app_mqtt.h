/* app_mqtt.h
   Created: 2019-02-27
   Author: Warren Taylor

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#ifndef _APP_MQTT_H_
#define _APP_MQTT_H_


//-------------------
// class AppMQTT
//-------------------
#ifdef __cplusplus
#include "mqtt_client.h"

class AppMQTT {
public:
    AppMQTT();
    virtual ~AppMQTT() { }

    void connect();

protected:

private:
};

#endif //__cplusplus
//-------------------


//-------------------
// c wrappers
//-------------------
#ifdef __cplusplus
extern "C"
{
#endif

extern void *get_static_app_mqtt(void);
extern esp_err_t app_mqtt_event_handler(esp_mqtt_event_handle_t event);

#ifdef __cplusplus
}
#endif
//-------------------


#endif // _APP_MQTT_H_
