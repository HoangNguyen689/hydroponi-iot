#ifndef WIFI_MANAGER_H_INCLUDED
#define WIFI_MANAGER_H_INCLUDED


#define DEFAULT_AP_SSID        "esp32"
#define DEFAULT_AP_PASSWORD    "esp32pwd"

/**
 * Main task for the wifi_manager
 */
void wifi_manager( void * pvParameters );


/**
 * @brief A standard wifi event manager.
 * The following event are being monitoring and will set/clear group events:
 * SYSTEM_EVENT_STA_START
 * SYSTEM_EVENT_STA_GOT_IP
 * SYSTEM_EVENT_STA_DISCONNECTED
 */
esp_err_t wifi_event_handler(void *ctx, system_event_t *event);

#endif
