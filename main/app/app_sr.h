
#pragma once

#include <stdbool.h>
#include <sys/queue.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"


#ifdef __cplusplus
extern "C" {
#endif

#define SR_CONTINUE_DET 1
#define SR_RUN_TEST 0 /**< Just for sr experiment in laboratory >*/
#if SR_RUN_TEST
#ifdef SR_CONTINUE_DET
#undef SR_CONTINUE_DET
#define SR_CONTINUE_DET 0
#endif
#endif

#define SR_CMD_STR_LEN_MAX 64
#define SR_CMD_PHONEME_LEN_MAX 64


#define TCP_SERVER_ADRESS       "192.168.43.79"     //作为client，要连接TCP服务器地址
#define TCP_SERVER_PORT          8888              //服务端端口
#define TCP_CLIENT_PORT          8888               //TCP客户端端口

#define TCP_SERVER_PORT_press          8080              //服务端端口
#define TCP_CLIENT_PORT_press          8080               //TCP客户端端口


typedef struct {
    wakenet_state_t wakenet_mode;
    esp_mn_state_t state;
    int command_id;
    bool beep_enable;
} sr_result_t;

/**
 * @brief User defined command list
 *
 */
typedef enum {
    SR_CMD_SET_RED = 0,
    SR_CMD_SET_WHITE,
    SR_CMD_SET_YELLOW,
    SR_CMD_SET_BLUE,
    SR_CMD_LIGHT_ON,
    SR_CMD_LIGHT_OFF,
    SR_CMD_CUSTOMIZE_COLOR,
    SR_CMD_NEXT,
    SR_CMD_PLAY,
    SR_CMD_PAUSE,

    SR_CMD_SET_GREEN,
    SR_CMD_AC_SET_ON,
    SR_CMD_AC_SET_OFF,
    SR_CMD_AC_TEMP_ADD,
    SR_CMD_AC_TEMP_DEC,

    SR_CMD_MAX,
} sr_user_cmd_t;

typedef enum {
    SR_LANG_EN,
    SR_LANG_CN,
    SR_LANG_MAX,
} sr_language_t;

typedef struct sr_cmd_t {
    sr_user_cmd_t cmd;
    sr_language_t lang;
    uint32_t id;
    char str[SR_CMD_STR_LEN_MAX];
    char phoneme[SR_CMD_PHONEME_LEN_MAX];
    SLIST_ENTRY(sr_cmd_t) next;
} sr_cmd_t;

esp_err_t app_sr_start();
esp_err_t app_sr_stop(void);
esp_err_t app_sr_get_result(sr_result_t *result, TickType_t xTicksToWait);
esp_err_t app_sr_set_language(sr_language_t new_lang);
esp_err_t app_sr_add_cmd(const sr_cmd_t *cmd);
esp_err_t app_sr_modify_cmd(uint32_t id, const sr_cmd_t *cmd);
esp_err_t app_sr_remove_cmd(uint32_t id);
esp_err_t app_sr_remove_all_cmd(void);
const sr_cmd_t *app_sr_get_cmd_from_id(uint32_t id);
uint8_t app_sr_search_cmd_from_user_cmd(sr_user_cmd_t user_cmd, uint8_t *id_list, uint16_t max_len);
uint8_t app_sr_search_cmd_from_phoneme(const char *phoneme, uint8_t *id_list, uint16_t max_len);
esp_err_t app_sr_update_cmds(void);
esp_err_t sr_handle_info(sr_result_t *result);

void app_speech_init(void);
void app_speech_deinit(void);
void app_speech_init_2(void);
void app_speech_deinit_2(void);
void tcp_client_task(void);
void tcp_client_task_press(void);
void app_speech_init_3(void);
void mqtt_receive(void);
void tcp_machine_translate(void);
void mqtt_phone_publish(void);
void tcp_AI_Q_A_Begin(void);
#ifdef __cplusplus
}
#endif