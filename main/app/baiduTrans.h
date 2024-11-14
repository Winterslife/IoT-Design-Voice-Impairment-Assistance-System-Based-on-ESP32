#include <stdio.h>
#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "app_wifi.h"
#include "esp_http_client.h"
#include "mbedtls/md5.h"
#include "cJSON.h"

void md5(const char *str, char *md5_str);
esp_err_t baiduTranslate_event_handler(esp_http_client_event_t *evt);
char *translate_text(const char *text, const char *from, const char *to);