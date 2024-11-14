#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"
#include "nvs_flash.h"
#include "ui.h"
#include "driver/i2s_std.h"
#include "app_sr.h"
#include "app_wifi.h"
#include "settings.h"
#include "esp_log.h"
#include "bsp_board_extra.h"
#include "tongyi.h"
#include "baiduTrans.h"



// void MicroPhone_Reconfig(lv_event_t * e);
// void Wifi_Connect(lv_event_t * e);
// void MicroPhone_Reconfig_End(lv_event_t * e);
// extern SemaphoreHandle_t xSemaphore; // 声明信号量
#ifdef __cplusplus
}
#endif