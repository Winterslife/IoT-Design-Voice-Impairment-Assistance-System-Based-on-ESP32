#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "app_wifi.h"
#include "esp_wifi_default.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static bool is_netif_initialized = false;
static void event_handler(void* arg, esp_event_base_t base,
                                int32_t id, void* event_data)
{
   if(base == WIFI_EVENT && id == WIFI_EVENT_STA_START) 
    esp_wifi_connect();
    else if(base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    esp_wifi_connect();
    else if(base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED)
    ESP_LOGI("WIFI","CONNECTED");
    else if(base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI("WIFI", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI("WIFI","ALL DONE");
    }
}


void wifi_connect(void)
{
    //Initialize NVS
     nvs_flash_init();

     esp_netif_init();

    if (!is_netif_initialized) 
    {
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    is_netif_initialized = true;
    }
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&init_config);

    esp_event_handler_register(WIFI_EVENT ,
                               ESP_EVENT_ANY_ID, 
                               &event_handler,
                               NULL);
    esp_event_handler_register(WIFI_EVENT ,
                               ESP_EVENT_ANY_ID, 
                               &event_handler,
                               NULL);
    esp_wifi_set_mode(WIFI_MODE_STA);
    wifi_config_t wifi_config = {
        // .sta.ssid = "JSC's Huawei Mate 60",
        // .sta.password = "40235678",
        // .sta.ssid = "Winters",
        // .sta.password = "12345678",
        // .sta.ssid = "Wonderu",
        // .sta.password = "12345678910",
        // .sta.ssid = "nova 5 Pro",
        // .sta.password = "60985888",
        // .sta.ssid = "zjo",
        // .sta.password = "123456789",
        .sta.ssid = "iPhone 15 pro",
        .sta.password = "12345678910",
    };
    esp_wifi_set_config(WIFI_IF_STA,&wifi_config);
    esp_wifi_start();
    esp_wifi_set_ps(WIFI_PS_NONE);
    
}
