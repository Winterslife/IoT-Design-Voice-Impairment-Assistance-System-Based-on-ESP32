/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "main.h"
#include "driver/timer.h"
#include "esp_timer.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
static char *TAG = "app_main";
static TaskHandle_t xHandleTaskTcpClinent = NULL;
// SemaphoreHandle_t xSemaphore = NULL; // 定义信号量
// static TimerHandle_t timer_handle;

bool sr_enable_reg                = false;
bool presselectric_enable_reg     = false;
bool mqtt_phone_enable_reg        = false;
bool tcp_machine_translate_reg    = false;
bool tcp_AI_Q_A_reg               = false;
void app_main(void)
{
  
  ESP_LOGI(TAG, "system start");
  //初始化NVS分区
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  ESP_ERROR_CHECK(settings_read_parameter_from_nvs());
  bsp_spiffs_mount();
 // bsp_extra_led_init();
  bsp_extra_codec_init();

  bsp_i2c_init();        //I2C初始化

  bsp_display_start();
  //检查NVS中的设置的状态

  bsp_display_lock(0);
  ui_init();
  bsp_display_unlock();
  // xSemaphore = xSemaphoreCreateBinary();
    // if (xSemaphore == NULL) {
    //     ESP_LOGE("main", "Failed to create semaphore");
    //     return;
    // }
  /**********************************************控制参数初始化*********************************************/
  sys_param_t *sys_set = settings_get_parameter();
  sys_set->sr_enable             = false;
  sys_set->need_hint             = false;
  sys_set->presselectric_enable  = false;
  sys_set->mqtt_phone_enable     = false;
  sys_set->tcp_machine_translate = false;
  sys_set->button_machine_determine =false;
  sys_set->button_machine_clear     =false;
  sys_set->tcp_AI_Q_A            =false;
  sys_set->tcp_AI_Q_A_Clear      =false;
  sys_set->tcp_AI_Q_A_Determine  =false;  
  ESP_LOGI(TAG,"sr_enable:%d",sys_set->sr_enable);
  /**************************************************************************************************/
while(1)
{
  sys_param_t *sys_set = settings_get_parameter();

  while (false == sys_set->need_hint) 
    {   
        //ESP_LOGI(TAG, "needed_hint:false");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
/**********************************************控制双麦克风阵列的代码*********************************************/
  if ((sys_set->sr_enable == true) && (sr_enable_reg == false)) 
  {
    ESP_LOGI(TAG, "speech recognition Enable");
    sr_enable_reg = sys_set->sr_enable;
    app_speech_init();       //双麦克风阵列

  }
  else if((sys_set->sr_enable == false) && (sr_enable_reg == true))
  { 
    ESP_LOGI(TAG, "speech recognition Disable");
    ESP_LOGI(TAG,"need_hint:%d",sys_set->need_hint);
    ESP_LOGI(TAG,"presselectric:%d",sys_set ->  presselectric_enable);
    ESP_LOGI(TAG,"sr_enable:%d",sys_set ->  sr_enable);
    sr_enable_reg = sys_set->sr_enable;
    app_speech_deinit();       //双麦克风阵列
    
  }
/**************************************压电传感器相关代码**********************************************************/

  else if((sys_set-> presselectric_enable == true) && (presselectric_enable_reg == false))
  {
    ESP_LOGI(TAG, "presselectric recognition enable");
    ESP_LOGI(TAG,"need_hint:%d",sys_set->need_hint);
    ESP_LOGI(TAG,"presselectric:%d",sys_set ->  presselectric_enable);
    // ESP_LOGI(TAG,"sr_enable:%d",sys_set ->  sr_enable);
    presselectric_enable_reg = sys_set -> presselectric_enable;
    // ESP_LOGI(TAG, "presselectric_enable_reg = %s",sys_set -> presselectric_enable);
    app_speech_init_2();      //压电传感器的数据采集
    
  }
  else if((sys_set-> presselectric_enable == false) && (presselectric_enable_reg == true))
  {
    ESP_LOGI(TAG, "presselectric reconfigure Disable");
    presselectric_enable_reg = sys_set -> presselectric_enable;
    // ESP_LOGI(TAG, "presselectric_enable_reg = %s",sys_set -> presselectric_enable);
    app_speech_deinit_2();      //压电传感器的数据采集
  }
/**********************************************语音通话相关的代码**********************************************/

  else if((sys_set-> mqtt_phone_enable == true) && (mqtt_phone_enable_reg == false))
  {
    
    ESP_LOGI(TAG, "mqtt_phone enable");
    mqtt_phone_enable_reg = sys_set -> mqtt_phone_enable;
    // ESP_LOGI(TAG, "presselectric_enable_reg = %s",sys_set -> presselectric_enable);
    mqtt_phone_publish();        //语音通话开启
  }

/**********************************************机器翻译相关的代码**********************************************/

  else if((sys_set-> tcp_machine_translate == true) && (tcp_machine_translate_reg == false))
  {
    
    ESP_LOGI(TAG, "mqtt_phone enable");
    tcp_machine_translate_reg = sys_set -> tcp_machine_translate;
    // ESP_LOGI(TAG, "presselectric_enable_reg = %s",sys_set -> presselectric_enable);
    tcp_machine_translate();     //机器翻译开始
  }

/**********************************************AI智能问答的相关的代码**********************************************/
  else if((sys_set-> tcp_AI_Q_A == true) && (tcp_AI_Q_A_reg == false))
  {
    
    ESP_LOGI(TAG, "mqtt_phone enable");
    tcp_AI_Q_A_reg = sys_set -> tcp_AI_Q_A;
    // ESP_LOGI(TAG, "presselectric_enable_reg = %s",sys_set -> presselectric_enable);
    tcp_AI_Q_A_Begin();     //AI智能问答开始
  }

}

}


/************************************************按键的回调函数***********************************************/

// #include "main.h"
// static char *TAG = "app_main";
/*WIFI连接*/
//  void Wifi_Connect(lv_event_t * e)
//  {
//  	wifi_connect();
//  }

// void MicroPhone_Reconfig(lv_event_t * e)
// {
	
//   sys_param_t *sys_set = settings_get_parameter();
//   sys_set->sr_enable = true;
//   sys_set->need_hint = true;
//   ESP_LOGI(TAG, "demo select:%s, SR Enable:%d", sys_set->demo_gui ? "UI" : "SR", sys_set->sr_enable);
// }



// void MicroPhone_Reconfig_End(lv_event_t * e)
// {
// 	  sys_param_t *sys_set = settings_get_parameter();
//     sys_set->sr_enable = false;
//     sys_set->need_hint = false;
//     ESP_LOGI(TAG, "demo select:%s, SR Enable:%d", sys_set->demo_gui ? "UI" : "SR", sys_set->sr_enable);
// }


// void Press_Electric(lv_event_t * e)
// {
// 		sys_param_t *sys_set = settings_get_parameter();
//     sys_set-> presselectric_enable = true;
//     sys_set-> need_hint            = true;
//     // ESP_LOGI(TAG, "demo select:%s, SR Enable:%d", sys_set->demo_gui ? "UI" : "SR", sys_set->sr_enable);
// }


// void Press_Electric_End(lv_event_t * e)
// {
// 		sys_param_t *sys_set = settings_get_parameter();
//     sys_set-> presselectric_enable = false;
//     sys_set-> need_hint            = false;
// }


// void MicroPhone_Contant(lv_event_t * e)
// {
// 		sys_param_t *sys_set = settings_get_parameter();
//     sys_set-> mqtt_phone_enable    = true;
//     sys_set-> need_hint            = true;
//     ESP_LOGI(TAG, "MQTT_PHONE_ENABLE");
// }

// void MicroPhone_Contant_End(lv_event_t * e)
// {
// 	// Your code here
// }

// void Tcp_Machine_Begin(lv_event_t * e)
// {
// 		sys_param_t *sys_set = settings_get_parameter();
//     sys_set-> tcp_machine_translate    = true;
//     sys_set-> need_hint            = true;
//     ESP_LOGI(TAG, "MQTT_PHONE_ENABLE");
// }

// void Tcp_Machine_End(lv_event_t * e)
// {
// 		sys_param_t *sys_set = settings_get_parameter();
//     sys_set-> tcp_machine_translate    = false;
//     sys_set-> need_hint                = false;
//     ESP_LOGI(TAG, "Tcp_Machine_Translate_DISABLE");
// }

// void Tcp_Machine_Clear(lv_event_t * e)
// {
// 		sys_param_t *sys_set = settings_get_parameter();
//     if(sys_set->button_machine_clear == false)
//       sys_set->button_machine_clear = true;
//     else
//       sys_set->button_machine_clear = false;
//     ESP_LOGI(TAG, "清空问题与回答");
// }

// void Tcp_Machine_Deter(lv_event_t * e)
// {
// 	sys_param_t *sys_set = settings_get_parameter();
//   if(sys_set->button_machine_determine == false)
//      sys_set->button_machine_determine = true;
//   else
//     sys_set->button_machine_determine = false;
//   ESP_LOGI(TAG, "确定回答的问题");
// }

// void Tcp_AI_Q_A_Begin(lv_event_t * e)
// {
// 	sys_param_t *sys_set = settings_get_parameter();
//   sys_set-> tcp_AI_Q_A   = true;
//   sys_set-> need_hint    = true;
//   ESP_LOGI(TAG, "AI智能问答开始");
// }

// void Tcp_AI_Q_A_End(lv_event_t * e)
// {
// 	sys_param_t *sys_set = settings_get_parameter();
//   sys_set-> tcp_AI_Q_A   = false;
//   sys_set-> need_hint    = false;
//   ESP_LOGI(TAG, "AI智能问答结束");
// }

// void Tcp_AI_Q_A_Clear(lv_event_t * e)
// {
// 		sys_param_t *sys_set = settings_get_parameter();
//     if(sys_set->tcp_AI_Q_A_Clear == false)
//       sys_set->tcp_AI_Q_A_Clear = true;
//     else
//       sys_set->tcp_AI_Q_A_Clear = false;
//     ESP_LOGI(TAG, "清空AI问题与回答");
// }

// void Tcp_AI_Q_A_Deter(lv_event_t * e)
// {
// 		sys_param_t *sys_set = settings_get_parameter();
//     if(sys_set->tcp_AI_Q_A_Determine == false)
//       sys_set->tcp_AI_Q_A_Determine = true;
//     else
//       sys_set->tcp_AI_Q_A_Determine = false;
//     ESP_LOGI(TAG, "确定");
// }