#include <stdio.h>
#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "app_wifi.h"
#include "esp_http_client.h"
#include "cJSON.h"


void parse_json(const char *json_str);
esp_err_t app_http_tongyi_event_handler(esp_http_client_event_t *evt);
char *app_ask_tongyi(char *question);