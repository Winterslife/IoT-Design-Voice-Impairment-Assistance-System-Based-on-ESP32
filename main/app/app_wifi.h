#pragma once

#include <esp_err.h>
#include "esp_wifi.h"
#include "bsp/esp-bsp.h"
#ifdef __cplusplus
extern "C" {
#endif


#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void wifi_connect(void);

#ifdef __cplusplus
}
#endif