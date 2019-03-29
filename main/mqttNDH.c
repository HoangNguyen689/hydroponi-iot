#include <stdio.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqttNDH.h"

#include "cJSON.h"
#include "actuatorNDH.h"

#include "dht22.h"

const char *TAG = "MQTT";

const char *broker = "mqtt://broker.hivemq.com";

extern bool mqtt_connected;
extern bool wifi_connected;
extern bool identified;

esp_mqtt_client_handle_t client;

esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
  esp_mqtt_client_handle_t client = event->client;
  int msg_id;
  char *command;
  
  switch (event->event_id) {
	
  case MQTT_EVENT_CONNECTED:
	ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
	mqtt_connected = true;
	char *topic = "MQTT_IDENTIFY_NDH";
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "deviceId", "dev1");
	cJSON_AddStringToObject(root, "username", "dev1");
	cJSON_AddStringToObject(root, "password", "dev1");
	char *text = cJSON_PrintUnformatted(root);
	
	msg_id = esp_mqtt_client_publish(client, topic, text, 0, 0, 0);
	ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
	
	esp_mqtt_client_subscribe(client, "MQTT_CONTROL_NDH", 0);
	esp_mqtt_client_subscribe(client, "MQTT_IDENTIFY_REPLY_NDH", 0);
	ESP_LOGI(TAG, "Subscribe successful!");
	//identified = true;
	break;
			
  case MQTT_EVENT_DISCONNECTED:
	ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
	mqtt_connected = false;
	break;
	
  case MQTT_EVENT_SUBSCRIBED:
	ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
	break;
	
  case MQTT_EVENT_UNSUBSCRIBED:
	ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
	break;
	
  case MQTT_EVENT_PUBLISHED:
	ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
	break;
	
  case MQTT_EVENT_DATA:
	ESP_LOGI(TAG, "MQTT_EVENT_DATA");
	printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
	printf("DATA=%.*s\r\n", event->data_len, event->data);
	command =  event->data;
	*(command + event->data_len ) = '\0';
	turn_pump_on_with_command(command);
	break;
	
  case MQTT_EVENT_ERROR:
	ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
	break;
  }
  return ESP_OK;
}

void publish_data_to_broker() {
  while(1) {
	if ( !wifi_connected ) {
	  ESP_LOGI(TAG, "WIFI is not connected!\n");
	  vTaskDelay(pdMS_TO_TICKS(1000*60));
	}
	else if ( !mqtt_connected ) {
	  ESP_LOGI(TAG, "MQTT_Server is not connected!\n");
	  vTaskDelay(pdMS_TO_TICKS(1000*60));
	}
	else if ( !identified ) {
	  ESP_LOGI(TAG, "Not identified yet!\n");
	  vTaskDelay(pdMS_TO_TICKS(1000*60));
	}
	else {
	  for(int i = 0; i < 10; i++) {
		readDHT();
		
		cJSON *root = NULL;
		root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, "deviceId", "dev1");
		cJSON_AddNumberToObject(root, "temperature", getTemperature() );
		cJSON_AddNumberToObject(root, "humidity", getHumidity() );
		cJSON_AddNumberToObject(root, "moisture", read_soil_moisture() );
		char *text = cJSON_PrintUnformatted(root);
		ESP_LOGI(TAG,"Sent data: %s\n",text);
		esp_mqtt_client_publish(client,"MQTT_COLLECT_NDH",text,0,0,0);
		//vTaskDelay(pdMS_TO_TICKS(1000));
	  }
	  vTaskDelay(pdMS_TO_TICKS(1000*60*2));
	}
  }
  
}

void mqtt_app_start(void) {
  esp_mqtt_client_config_t mqtt_cfg = {
	.uri = broker,
	.event_handle = mqtt_event_handler,
	.port = 1883,
  };

  client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_start(client);
  vTaskDelay(5000/portTICK_PERIOD_MS);
  
  xTaskCreate(publish_data_to_broker, "Name", 4096, NULL, 5, NULL);
}
