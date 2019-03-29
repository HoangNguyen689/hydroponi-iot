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
#include "mqttNDH.h"

#include "cJSON.h"
#include "dht22.h"

TaskHandle_t task_wifi_manager = NULL;

const adc_channel_t channel = ADC_CHANNEL_0; //gpio36   
const adc_atten_t atten = ADC_ATTEN_DB_11;

bool mqtt_connected = false;

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

extern xQueueHandle pcnt_evt_queue;
extern pcnt_isr_handle_t user_isr_handle;

void app_main()
{
    nvs_flash_init();

	xTaskCreate(&wifi_manager, "wifi_manager", 4096, NULL, 5, &task_wifi_manager);
	vTaskDelay(5000/portTICK_PERIOD_MS);

    mqtt_app_start();

	/*
	int xx;
	while(1) {
	  xx = read_soil_moisture();
	  printf("%d\n", xx);
	  vTaskDelay(pdMS_TO_TICKS(1000));
	}
	*/

}
