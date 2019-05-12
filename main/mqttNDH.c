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
  char subcribe_topic[1024] ;
  char command[2048] ;

  switch (event->event_id) {
	
  case MQTT_EVENT_CONNECTED:
	ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
	mqtt_connected = true;
  
	esp_mqtt_client_subscribe(client, "MQTT_IDENTIFY_REPLY_NDH", 0);
	ESP_LOGI(TAG, "Subscribe identify server successful!");
	esp_mqtt_client_subscribe(client, "MQTT_CONTROL_NDH", 0);
	ESP_LOGI(TAG, "Subscribe control server successful!");

	ESP_LOGI(TAG, "Check identify ...");
	
	char *topic = "MQTT_IDENTIFY_NDH";
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "deviceId", "dev1");
	cJSON_AddStringToObject(root, "username", "dev1");
	cJSON_AddStringToObject(root, "password", "dev1");
	  
	char *text = cJSON_PrintUnformatted(root);
		
	msg_id = esp_mqtt_client_publish(client, topic, text, 0, 0, 0);
	ESP_LOGI(TAG, "Sent identify message successful - first time!");	
	
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
	
	sprintf(subcribe_topic, "%.*s", event->topic_len, event->topic);
	sprintf(command, "%.*s", event->data_len, event->data);

	printf("%s\n",subcribe_topic);
	printf("%s\n", command);

	while (1) {

	  printf("in loop\n");
	  if (strcmp(subcribe_topic, "MQTT_IDENTIFY_REPLY_NDH") == 0) {
		printf("Check identify!\n");
		identified = true;
	  }
	  
	  if (identified == true) break;

	  
	  char *topic = "MQTT_IDENTIFY_NDH";
	  cJSON *root = cJSON_CreateObject();
	  cJSON_AddStringToObject(root, "deviceId", "dev1");
	  cJSON_AddStringToObject(root, "username", "dev1");
	  cJSON_AddStringToObject(root, "password", "dev1");
	  
	  char *text = cJSON_PrintUnformatted(root);
		
	  msg_id = esp_mqtt_client_publish(client, topic, text, 0, 0, 0);
	  ESP_LOGI(TAG, "Sent identify message successful, msg_id=%d, %s", msg_id, text);	
	  vTaskDelay(pdMS_TO_TICKS(10 * 1000));
	}

	  
	if (strcmp(subcribe_topic, "MQTT_CONTROL_NDH") == 0 && identified == true) {
	  turn_pump_on_with_command(command);
	}
	
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

/*
void mqtt_publish_identify_message() {
  esp_mqtt_client_handle_t cli;
  esp_mqtt_client_config_t cfg;
  config.port = 1883;
  config.uri = broker;
  cli = esp_mqtt_client_init(&cfg); 
}
*/
