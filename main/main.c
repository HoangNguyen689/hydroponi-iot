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

#include "wifi_manager.h"

#include "pcntNDH.h"
#include "mqttNDH.h"

bool mqtt_connected = false;
bool wifi_connected = false;
bool identified = false;

void app_main()
{
    nvs_flash_init();

	wifi_manager();

	vTaskDelay(5000/portTICK_PERIOD_MS);

    mqtt_app_start();

}
