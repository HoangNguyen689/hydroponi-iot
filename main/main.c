#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "esp_spi_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"
#include "driver/spi_master.h"

#include "mdns.h"

#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "wifi_manager.h"

#include "pcntNDH.h"

#include "cJSON.h"
#include "dht22.h"

//static TaskHandle_t task_wifi_manager = NULL;

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t client;

static const adc_channel_t channel = ADC_CHANNEL_0; //gpio36   
static const adc_atten_t atten = ADC_ATTEN_DB_11;

static const char *broker = "mqtt://broker.hivemq.com";

static bool mqtt_connected = false;

bool wifi_connected = false;


int read_soil_moisture()
{
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(channel, atten);

  uint32_t adc_reading = 0;
  for(int i = 0; i < 10000; i++) {
	adc_reading += adc1_get_raw((adc1_channel_t)channel);
  }
  adc_reading /= 10000;
	  
  return adc_reading;
}

void turn_pump_on_with_command(char *command) {
  printf("Turn on pump\n");
  printf("%s\n", command);
  
  cJSON *json = cJSON_Parse(command);
  const cJSON *item = cJSON_GetObjectItem(json, "additional");
  char *text = item->valuestring;
  int pump;
  sscanf(text,"%d", &pump);

  gpio_pad_select_gpio(19); 
  gpio_set_direction(19, GPIO_MODE_INPUT_OUTPUT);
  gpio_set_level(19,1);
  switch(pump) {
  case 0:
	printf("Turn pum in 10s\n");
	vTaskDelay(pdMS_TO_TICKS(10*1000));
	break;
  case 1:
	printf("Turn pum in 20s\n");
	vTaskDelay(pdMS_TO_TICKS(20*1000));
	break;
  case 2:
	printf("Turn pum in 30s\n");
	vTaskDelay(pdMS_TO_TICKS(30*1000));
	break;
  }
  
  gpio_set_level(19,0);
  
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
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
            ESP_LOGI(TAG, "Subscribe successful!");
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
			//char command[event->data_len + 1];
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


void publish_data_to_broker(void *pvParameters)
{
  for( ;; )
	{
	  if(mqtt_connected && wifi_connected) {
		for(int i = 0; i < 10; i++) {
		  cJSON *root = NULL;
		  root = cJSON_CreateObject();
		  
		  cJSON_AddStringToObject(root, "deviceId", "dev1");
		  cJSON_AddNumberToObject(root, "humidity", read_soil_moisture() );
		  char *text = cJSON_PrintUnformatted(root);
		  ESP_LOGI(TAG,"Sent data: %s\n",text);
		  esp_mqtt_client_publish(client,"MQTT_COLLECT_NDH",text,0,0,0);
		  //vTaskDelay(pdMS_TO_TICKS(1000));
		}
		vTaskDelay(pdMS_TO_TICKS(1000*60*2));
	  }
	}

}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = broker,
        .event_handle = mqtt_event_handler,
        .port = 1883,
    };

	client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
	vTaskDelay(5000/portTICK_PERIOD_MS);

	xTaskCreate(publish_data_to_broker,	"Name", 2048, NULL,	5, NULL);
}


void DHT_task(void *pvParameter)
{
   printf("Starting DHT measurement!\n");
   while(1) {
	 readDHT();
	 printf("Temperature reading %f\n",getTemperature());
	 printf("Humidity reading %f\n",getHumidity());
	 vTaskDelay(3000 / portTICK_RATE_MS);
   }
}

extern xQueueHandle pcnt_evt_queue;
extern pcnt_isr_handle_t user_isr_handle;

void app_main()
{
    nvs_flash_init();

	/* task for wifi manager */
	//xTaskCreate(&wifi_manager, "wifi_manager", 4096, NULL, 5, &task_wifi_manager);
	vTaskDelay(5000/portTICK_PERIOD_MS);

    //mqtt_app_start();

    //xTaskCreate(&DHT_task, "DHT_task", 4096, NULL, 5, NULL);

}
