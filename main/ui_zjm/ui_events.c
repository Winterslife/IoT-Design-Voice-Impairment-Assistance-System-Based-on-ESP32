// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.4.0
// LVGL version: 8.3.6
// Project name: SquareLine_Project

#include "ui.h"

#include "main.h"
static char *TAG = "app_main";
 void Wifi_Connect(lv_event_t * e)
 {
 	wifi_connect();
	char input[512]="苹果的英文怎么说？"; 
	const char *ai_text = app_ask_tongyi(input);
    ESP_LOGI(TAG,"AI回答:%s",ai_text);
 }

void MicroPhone_Reconfig(lv_event_t * e)
{
	
  sys_param_t *sys_set = settings_get_parameter();
  sys_set->sr_enable = true;
  sys_set->need_hint = true;
  ESP_LOGI(TAG, "demo select:%s, SR Enable:%d", sys_set->demo_gui ? "UI" : "SR", sys_set->sr_enable);
}



void MicroPhone_Reconfig_End(lv_event_t * e)
{
	sys_param_t *sys_set = settings_get_parameter();
    sys_set->sr_enable = false;
    sys_set->need_hint = false;
    ESP_LOGI(TAG, "demo select:%s, SR Enable:%d", sys_set->demo_gui ? "UI" : "SR", sys_set->sr_enable);
}


void Press_Electric(lv_event_t * e)
{
	sys_param_t *sys_set = settings_get_parameter();
    sys_set-> presselectric_enable = true;
    sys_set-> need_hint            = true;
    // ESP_LOGI(TAG, "demo select:%s, SR Enable:%d", sys_set->demo_gui ? "UI" : "SR", sys_set->sr_enable);
}


void Press_Electric_End(lv_event_t * e)
{
		sys_param_t *sys_set = settings_get_parameter();
    sys_set-> presselectric_enable = false;
    sys_set-> need_hint            = false;
}


void MicroPhone_Contant(lv_event_t * e)
{
		sys_param_t *sys_set = settings_get_parameter();
    sys_set-> mqtt_phone_enable    = true;
    sys_set-> need_hint            = true;
    ESP_LOGI(TAG, "MQTT_PHONE_ENABLE");
}

void MicroPhone_Contant_End(lv_event_t * e)
{
	// Your code here
}

void Tcp_Machine_Begin(lv_event_t * e)
{
		sys_param_t *sys_set = settings_get_parameter();
    sys_set-> tcp_machine_translate    = true;
    sys_set-> need_hint            = true;
    ESP_LOGI(TAG, "MQTT_PHONE_ENABLE");
}

void Tcp_Machine_End(lv_event_t * e)
{
		sys_param_t *sys_set = settings_get_parameter();
    sys_set-> tcp_machine_translate    = false;
    sys_set-> need_hint                = false;
    ESP_LOGI(TAG, "Tcp_Machine_Translate_DISABLE");
}

void Tcp_Machine_Clear(lv_event_t * e)
{
		sys_param_t *sys_set = settings_get_parameter();
    if(sys_set->button_machine_clear == false)
      sys_set->button_machine_clear = true;
    else
      sys_set->button_machine_clear = false;
    ESP_LOGI(TAG, "清空问题与回答");
}

void Tcp_Machine_Deter(lv_event_t * e)
{
	sys_param_t *sys_set = settings_get_parameter();
  if(sys_set->button_machine_determine == false)
     sys_set->button_machine_determine = true;
  else
    sys_set->button_machine_determine = false;
  ESP_LOGI(TAG, "确定回答的问题");
}

void Tcp_AI_Q_A_Begin(lv_event_t * e)
{
	sys_param_t *sys_set = settings_get_parameter();
  sys_set-> tcp_AI_Q_A   = true;
  sys_set-> need_hint    = true;
  ESP_LOGI(TAG, "AI智能问答开始");
}

void Tcp_AI_Q_A_End(lv_event_t * e)
{
	sys_param_t *sys_set = settings_get_parameter();
  sys_set-> tcp_AI_Q_A   = false;
  sys_set-> need_hint    = false;
  ESP_LOGI(TAG, "AI智能问答结束");
}

void Tcp_AI_Q_A_Clear(lv_event_t * e)
{
	sys_param_t *sys_set = settings_get_parameter();
    if(sys_set->tcp_AI_Q_A_Clear == false)
      sys_set->tcp_AI_Q_A_Clear = true;
    else
      sys_set->tcp_AI_Q_A_Clear = false;
    ESP_LOGI(TAG, "清空AI问题与回答");
}

void Tcp_AI_Q_A_Deter(lv_event_t * e)
{
	sys_param_t *sys_set = settings_get_parameter();
    if(sys_set->tcp_AI_Q_A_Determine == false)
      sys_set->tcp_AI_Q_A_Determine = true;
    else
      sys_set->tcp_AI_Q_A_Determine = false;
    ESP_LOGI(TAG, "确定");
}