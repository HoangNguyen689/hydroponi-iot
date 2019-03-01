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

#include "driver/gpio.h"
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

static TaskHandle_t task_wifi_manager = NULL;

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t client;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_publish(client, "topic061196", "connected", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "topic061196", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
			
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
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
	  esp_mqtt_client_publish(client,"topic061196","huhi",0,0,0);
	  vTaskDelay(2*1000/portTICK_PERIOD_MS);
	}

}



void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://iot.eclipse.org",
        .event_handle = mqtt_event_handler,
        .port = 1883,
    };

	client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
	vTaskDelay(5000/portTICK_PERIOD_MS);

	xTaskCreate(publish_data_to_broker,
				"Name",
				2048,
				NULL,
				4,
				NULL);
}


void app_main()
{
    nvs_flash_init();

/* task for wifi manager */
xTaskCreate(&wifi_manager, "wifi_manager", 4096, NULL, 5, &task_wifi_manager);
	vTaskDelay(5000/portTICK_PERIOD_MS);

    mqtt_app_start();
}
