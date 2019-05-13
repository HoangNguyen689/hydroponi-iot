#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "driver/adc.h"
#include "driver/gpio.h"

#include "cJSON.h"
#include "actuatorNDH.h"

const char* TAG_PUMP = "PUMP";

const adc_channel_t channel = ADC_CHANNEL_0; //gpio36   
const adc_atten_t atten = ADC_ATTEN_DB_11;

const int gpio_to_pump = 33;

int read_soil_moisture() {
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
  //printf("Turn on pump\n");
  //printf("%s\n", command);

  cJSON *json = cJSON_Parse(command);
  const cJSON *item = cJSON_GetObjectItem(json, "additional");
  char *text = item->valuestring;
  int pump;
  sscanf(text,"%d", &pump);
  
  gpio_pad_select_gpio(gpio_to_pump); 
  gpio_set_direction(gpio_to_pump, GPIO_MODE_OUTPUT);
  gpio_set_level(gpio_to_pump,1);
  //  printf("Turn pum in %ds\n",(pump+1)*10);
  ESP_LOGI(TAG_PUMP, "Turn on pump in %d", pump *10);
  vTaskDelay(pdMS_TO_TICKS( pump*10*1000 ));
  ESP_LOGI(TAG_PUMP,"Turn off pump.");
  gpio_set_level(gpio_to_pump,0);  
  
}
