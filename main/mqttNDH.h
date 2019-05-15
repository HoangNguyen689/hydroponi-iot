#ifndef MQTTNDH_H_INCLUDED
#define MQTTNDH_H_INCLUDED

#include "mqtt_client.h"

esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
void publish_data_to_broker();
void mqtt_app_start();
void publish_iden();
void encrypt();

#endif
