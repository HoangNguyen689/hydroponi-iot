#ifndef MQTTNDH_H_INCLUDED
#define MQTTNDH_H_INCLUDED

esp_err_t mqtt_event_hendler(esp_mqtt_event_handle_t event);
void publish_data_to_broker();
void mqtt_app_start();

#endif
