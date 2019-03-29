#ifndef WIFI_MANAGER_H_INCLUDED
#define WIFI_MANAGER_H_INCLUDED

void wifi_manager();

esp_err_t wifi_event_handler(void *ctx, system_event_t *event);

#endif
