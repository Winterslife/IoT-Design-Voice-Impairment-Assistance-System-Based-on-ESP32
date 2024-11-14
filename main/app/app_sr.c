/*7.11 17:37发送版*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "app_sr.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "esp_mn_speech_commands.h"
#include "esp_process_sdkconfig.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_mn_iface.h"
#include "app_sr_handler.h"
#include "model_path.h"

#include "bsp_board_extra.h"
#include "bsp/esp-bsp.h"
#include "settings.h"
#include "esp_timer.h"
#include "driver/timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "mqtt_client.h"
#include "ui.h"
#include "main.h"
#include "esp_rom_sys.h"






static const char *TAG = "app_sr";

extern const uint8_t server_cert_pem_start[] asm("_binary_mosquitto_org_crt_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_mosquitto_org_crt_end");
static TaskHandle_t m_nnTaskHandle = NULL;

static esp_afe_sr_iface_t *afe_handle = NULL; // 音频前端处理接口指针
static srmodel_list_t *models = NULL;         // 语音识别模型列表的指针

static int sock; // TCP的变量

static uint8_t ADC_Value[2] = {0};
static uint16_t data_number = 0;

#define I2S_CHANNEL_NUM (2)
#define NEED_DELETE BIT0
#define FEED_DELETED BIT1
#define DETECT_DELETED BIT2

#define SAMPLE_RATE 8000
#define CHANNELS 1
#define BYTES_PER_SAMPLE 2
#define CHUNK_DURATION_MS 340
#define CHUNK_SIZE (SAMPLE_RATE * CHANNELS * BYTES_PER_SAMPLE * CHUNK_DURATION_MS / 1000)

/*****************************************此处为双麦克风阵列的数据传输***********************************/

void tcp_client_task(void) // 用于连接python端TCP服务器
{
    char rx_buffer[128];
    int addr_family;
    int ip_protocol;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(TCP_SERVER_ADRESS);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TCP_SERVER_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
    }
    else
        ESP_LOGI(TAG, "Socket connect!");
    return;
}

// int16_t *audio_buffer;//全局变量
static void audio_feed_task(void *arg)
{
    
    size_t bytes_read = 0;

    int audio_chunksize = 1024;
    int feed_channel = 2;
    ESP_LOGI(TAG, "audio_chunksize=%d, feed_channel=%d", audio_chunksize, feed_channel);
    // int16_t *audio_buffer = (int16_t *)malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    int16_t *audio_buffer = (int16_t *)malloc(1920*2);
    if (NULL == audio_buffer) 
    {
        esp_system_abort("No mem for audio buffer");
    }
    // 将音频缓冲区指针存储到全局变量 g_sr_data 中
    tcp_client_task();
    while (true)
    {  
        sys_param_t *sys_set = settings_get_parameter();
        if(sys_set->sr_enable == false) break;

        // 如果需要删除音频输入任务，则删除任务并设置标志位
        // 从 I2S 总线读取音频数据
        // bsp_extra_i2s_read((char *)audio_buffer, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), &bytes_read, portMAX_DELAY);
        bsp_extra_i2s_read((char *)audio_buffer, 1920*2, &bytes_read, portMAX_DELAY);
        int16_t data[1920] ={0};
        for (int  i = 0; i <1920; i++) 
        {  
           data[i] = audio_buffer[i*2];
        }

        // ESP_LOGI(TAG, "byte_read =%d ", bytes_read/2);
        send(sock, data, bytes_read/2, 0);
    }
    free(audio_buffer);
    vTaskDelete(NULL);
}

char *rx_buffer_reg[12800];
uint8_t reg_total_len = 0;
static void tcp_receive_task(void *arg)
{
    char rx_buffer[128];
    while (1) 
    {  
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) 
        {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        } else if (len == 0) 
        {
            ESP_LOGI(TAG, "Connection closed");
            break;
        } 
        else 
        {    
            rx_buffer[len] = 0;
            if(reg_total_len+len <=12700)
            { 
               reg_total_len = reg_total_len+len;
               strcat(rx_buffer_reg, rx_buffer);
            }
            else
            {
                reg_total_len = len;
                memset(rx_buffer_reg, 0, sizeof(rx_buffer_reg)); 
                strcat(rx_buffer_reg, rx_buffer);
            }
            ESP_LOGI(TAG, "Received: %s", rx_buffer);
            lv_textarea_set_text(ui_TextArea1, rx_buffer_reg);
        }    
    }
    vTaskDelete(NULL);
}

void app_speech_init(void)
{
    esp_err_t ret = ESP_OK;
    xTaskCreatePinnedToCore(&audio_feed_task, "feed_task", 6*1024, NULL, 5, NULL, 0);   
    xTaskCreatePinnedToCore(&tcp_receive_task, "receive_task", 4*1024, NULL, 5, NULL, 1); 
    ESP_LOGI(TAG, "Free heap size after creating tasks: %d", xPortGetFreeHeapSize());
}

void app_speech_deinit(void)
{
    vTaskDelete(&audio_feed_task);
    vTaskDelete(&tcp_receive_task);
    close(sock);
    ESP_LOGI(TAG, "Tasks deleted successfully");
}
/************************************************************************************************************/

/**************************************此处为压电传感器的语音数据的传输****************************************/
#define SAMPLE 1920 // 存储数据的数组大小

void tcp_client_task_press(void) // 用于连接python端TCP服务器
{
    char rx_buffer[128];
    int addr_family;
    int ip_protocol;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(TCP_SERVER_ADRESS);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TCP_SERVER_PORT_press);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
    }
    else
        ESP_LOGI(TAG, "Socket connect!");
    return;
}

static void get_sensor_datagroup()
{
    tcp_client_task_press();
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
    adc_oneshot_chan_cfg_t config = {
        // .channel = ADC1_CHANNEL_3,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));
    int i = 0;
    uint16_t buffer[SAMPLE];
    uint32_t voltage = 0;
    

    while (true)
    {
        int raw_data;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &raw_data));
        buffer[i++] = raw_data;
        esp_rom_delay_us(30);
        if (i == 1920)
        {
            int ret = send(sock, buffer, 1920 * sizeof(uint16_t), 0);
            i = 0;
        }
    }

    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    vTaskDelete(NULL);
}

char *rx_buffer_reg_press[12800];
uint8_t reg_total_len_press = 0;
static void tcp_receive_task_press(void *arg)
{
    char rx_buffer[128];
    while (1) 
    {  
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) 
        {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            // break;
        } else if (len == 0) 
        {
            ESP_LOGI(TAG, "Connection closed");
            break;
        } 
        else 
        {    
            rx_buffer[len] = 0;
            if(reg_total_len_press+len <=12700)
            { 
               reg_total_len_press = reg_total_len_press+len;
               strcat(rx_buffer_reg_press, rx_buffer);
            }
            else
            {
                reg_total_len_press = len;
                memset(rx_buffer_reg_press, 0, sizeof(rx_buffer_reg_press)); 
                strcat(rx_buffer_reg_press, rx_buffer);
            }
            ESP_LOGI(TAG, "Received: %s", rx_buffer);
            lv_textarea_set_text(ui_TextArea2, rx_buffer_reg_press);
        }    
    }
    vTaskDelete(NULL);
}



void app_speech_init_2(void)
{
    esp_err_t ret = ESP_OK;

    xTaskCreatePinnedToCore(&get_sensor_datagroup, "senor_task", 6 * 1024, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&tcp_receive_task_press, "senor_receive", 4 * 1024, NULL, 5, NULL, 1);


}

void app_speech_deinit_2(void)
{
    vTaskDelete(&get_sensor_datagroup);
    close(sock);
    ESP_LOGI(TAG, "Tasks deleted successfully");
}

/********************************************************************************************************/

/**************************************此处的代码为收取TCP端传来的.wav文件的代码****************************/
static esp_err_t sr_echo_play()
{
    typedef struct
    {
        // The "RIFF" chunk descriptor
        uint8_t ChunkID[4];
        int32_t ChunkSize;
        uint8_t Format[4];
        // The "fmt" sub-chunk
        uint8_t Subchunk1ID[4];
        int32_t Subchunk1Size;
        int16_t AudioFormat;
        int16_t NumChannels;
        int32_t SampleRate;
        int32_t ByteRate;
        int16_t BlockAlign;
        int16_t BitsPerSample;
        // The "data" sub-chunk
        uint8_t Subchunk2ID[4];
        int32_t Subchunk2Size;
    } wav_header_t;
    size_t chunk_size = 4096;
    uint8_t *audio_buffer = NULL;
    audio_buffer = malloc(chunk_size);
    // audio_buffer = receive_data;
    if (NULL == audio_buffer)
    {
        ESP_LOGE(TAG, "audio data audio_buffer malloc failed");
    }
    /**
     * read head of WAV file
     */
    wav_header_t wav_head = {
        .ChunkID = "RIFF",
        .ChunkSize = 44 + chunk_size,
        .Format = "WAVE",
        .Subchunk1ID = "fmt ",
        .Subchunk1Size = 16,
        .AudioFormat = 1,
        .NumChannels = 2,
        .SampleRate = 16000,
        .ByteRate = 16000 * 2 * 2,
        .BlockAlign = 4,
        .BitsPerSample = 16,
        .Subchunk2ID = "data",
        .Subchunk2Size = chunk_size

    };
    bsp_extra_codec_set_fs(wav_head.SampleRate, wav_head.BitsPerSample, wav_head.NumChannels);
    ESP_LOGD(TAG, "frame_rate= %" PRIi32 ", ch=%d, width=%d", wav_head.SampleRate, wav_head.NumChannels, wav_head.BitsPerSample);
    bsp_extra_codec_mute_set(true);
    bsp_extra_codec_mute_set(false);
    bsp_extra_codec_volume_set(80, NULL);
    size_t cnt;
    if (bsp_extra_i2s_write(audio_buffer, chunk_size, &cnt, 1000) != ESP_OK)
    {
        ESP_LOGI(TAG, "Write Task: i2s write failed");
    }
    return ESP_OK;
}

typedef struct
{
    // The "RIFF" chunk descriptor
    uint8_t ChunkID[4];
    int32_t ChunkSize;
    uint8_t Format[4];
    // The "fmt" sub-chunk
    uint8_t Subchunk1ID[4];
    int32_t Subchunk1Size;
    int16_t AudioFormat;
    int16_t NumChannels;
    int32_t SampleRate;
    int32_t ByteRate;
    int16_t BlockAlign;
    int16_t BitsPerSample;
    // The "data" sub-chunk
    uint8_t Subchunk2ID[4];
    int32_t Subchunk2Size;
} wav_header_t;

static void get_wav_file()
{
    uint32_t wav_chunksize = 0;
    uint8_t receive_data[2048];
    uint32_t total_byte = 204288;
    size_t chunk_size = 2048;
    tcp_client_task();
    wav_header_t wav_head = {
        .ChunkID = "RIFF",
        .ChunkSize = 44 + chunk_size,
        .Format = "WAVE",
        .Subchunk1ID = "fmt ",
        .Subchunk1Size = 16,
        .AudioFormat = 1,
        .NumChannels = 1,
        .SampleRate = 30000,
        .ByteRate = 30000 * 2 * 1,
        .BlockAlign = 4,
        .BitsPerSample = 16,
        .Subchunk2ID = "data",
        .Subchunk2Size = chunk_size

    };
    bsp_extra_codec_set_fs(wav_head.SampleRate, wav_head.BitsPerSample, wav_head.NumChannels);
    ESP_LOGD(TAG, "frame_rate= %" PRIi32 ", ch=%d, width=%d", wav_head.SampleRate, wav_head.NumChannels, wav_head.BitsPerSample);
    bsp_extra_codec_mute_set(true);
    bsp_extra_codec_mute_set(false);
    bsp_extra_codec_volume_set(100, NULL);
    // vTaskDelay(pdMS_TO_TICKS(1));

    while (true)
    {
        if (wav_chunksize < total_byte)
        {
            int16_t len = recv(sock, receive_data, sizeof(receive_data), 0);
            // ESP_LOGI(TAG,"receive data %d",len);
            wav_chunksize = wav_chunksize + len;
            ESP_LOGI(TAG, "receive data %d", len);
            size_t cnt;
            if (bsp_extra_i2s_write(receive_data, chunk_size, &cnt, 1000) != ESP_OK)
            {
                ESP_LOGI(TAG, "Write Task: i2s write failed");
            }
        }
        else if (wav_chunksize >= 204288)
            wav_chunksize = 0;
    }
}

void app_speech_init_3(void)
{
    esp_err_t ret = ESP_OK;

    xTaskCreatePinnedToCore(&get_wav_file, "wav_task", 6 * 1024, NULL, 4, NULL, 1);
}

/*************************************************************************************************************************/

/****************************************************此处为MQTT实现语音通话的过程******************************************/
size_t data_len2;
// 队列句柄
static QueueHandle_t mqtt_data_queue;
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    int16_t *audio_buffer = (int16_t *)malloc(CHUNK_SIZE);

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // 发送订阅
        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 1);
        // msg_id = esp_mqtt_client_subscribe(client, "/in4nzoV26KT/esp32pro2/user/sub", 0);//订阅自己
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        // 取消订阅
        // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA: 
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");

        memcpy(audio_buffer, event->data, event->data_len);
        size_t bytes_written;
        ESP_LOGI(TAG, "i2s write called");
        esp_err_t err = bsp_extra_i2s_write(audio_buffer, event->data_len, &bytes_written, portMAX_DELAY);
        free(audio_buffer);

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static esp_mqtt_client_handle_t mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtts://xfe1f355.ala.cn-hangzhou.emqxsl.cn:8883",
        .broker.verification.certificate = (const char *)server_cert_pem_start,
        .credentials.username = "esp321",
        // .credentials.username = "esp32",
        .credentials.authentication.password = "123456",
        .session.last_will.qos = 1,//原本没有
        .network.timeout_ms = 10000,
        .network.reconnect_timeout_ms = 1000,
        .session.keepalive = 120,
    };
    bsp_extra_codec_set_fs(SAMPLE_RATE, 16, CHANNELS);
    bsp_extra_codec_mute_set(true);
    bsp_extra_codec_mute_set(false);
    bsp_extra_codec_volume_set(100, NULL);
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    return client;
}

    


void mqtt_published(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    char *audio_buffer_mqtt = malloc(CHUNK_SIZE);
    if (NULL == audio_buffer_mqtt)
    {
        esp_system_abort("No mem for audio buffer");
    }
    esp_mqtt_client_handle_t client;
    client = mqtt_app_start();

    size_t bytes_read;
    while (1) { 
    bsp_extra_i2s_read(audio_buffer_mqtt, CHUNK_SIZE, &bytes_read, portMAX_DELAY);

    if (bytes_read > 0) {
        ESP_LOGI(TAG, "bytes_read = %d", bytes_read);
        // esp_mqtt_client_publish(client, "/in4nzoV26KT/esp32name/user/pub", (char *)audio_buffer_mqtt, CHUNK_SIZE, 0, 0);
        esp_mqtt_client_publish(client, "/topic/qos1", audio_buffer_mqtt, bytes_read, 1, 0);

    } 

    // 每 340 毫秒（即 CHUNK_DURATION_MS）发送一次数据，每秒发送 5 次
    vTaskDelay(CHUNK_DURATION_MS / portTICK_PERIOD_MS);
}
    free(audio_buffer_mqtt);

}

void mqtt_phone_publish(void)
{
    esp_err_t ret = ESP_OK;
    xTaskCreatePinnedToCore(&mqtt_published, "mqtt_phone_task", 8 * 1024, NULL, 4, NULL, 1);
}

/**********************************************************************************************************************************/

/****************************************************此处为机器翻译实现的过程********************************************************/

void tcp_client_task_translate(void) // 用于连接python端TCP服务器
{
    char rx_buffer[128];
    int addr_family;
    int ip_protocol;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(TCP_SERVER_ADRESS);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TCP_SERVER_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
    }
    else
        ESP_LOGI(TAG, "Socket connect!");
    return;
}


static void audio_feed_task_translate(void *arg)
{
    size_t bytes_read = 0;

    int audio_chunksize = 1024;
    int feed_channel = 2;
    ESP_LOGI(TAG, "audio_chunksize=%d, feed_channel=%d", audio_chunksize, feed_channel);
    // int16_t *audio_buffer = (int16_t *)malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    int16_t *audio_buffer = (int16_t *)malloc(1920*2);
    if (NULL == audio_buffer) 
    {
        esp_system_abort("No mem for audio buffer");
    }
    // 将音频缓冲区指针存储到全局变量 g_sr_data 中
    tcp_client_task_translate();
    while (true)
    {  
        sys_param_t *sys_set = settings_get_parameter();
        if(sys_set->tcp_machine_translate == false) break;
        bsp_extra_i2s_read((char *)audio_buffer, 1920*2, &bytes_read, portMAX_DELAY);
        int16_t data[1920] ={0};
        for (int  i = 0; i <1920; i++) 
        {  
           data[i] = audio_buffer[i*2];
        }

        // ESP_LOGI(TAG, "byte_read =%d ", bytes_read/2);
        send(sock, data, bytes_read/2, 0);
    }
    free(audio_buffer);
    vTaskDelete(NULL);
}

char *rx_buffer_reg_translate[6400];
char *machine_answer[6400];
uint8_t reg_total_len_translate = 0;
static void tcp_receive_task_translate(void *arg)
{   bool tcp_machine_clear_reg   = false;
    char rx_buffer[128];
    while (1) 
    {   

        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) 
        {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        } else if (len == 0) 
        {
            ESP_LOGI(TAG, "Connection closed");
            break;
        } 
        else 
        {    
            rx_buffer[len] = 0;
            if(reg_total_len_translate+len <=12700)
            { 
               reg_total_len_translate = reg_total_len_translate+len;
               strcat(rx_buffer_reg_translate, rx_buffer);
            }
            else
            {
                reg_total_len_translate = len;
                memset(rx_buffer_reg_translate, 0, sizeof(rx_buffer_reg_translate)); 
                strcat(rx_buffer_reg_translate, rx_buffer);
            }
            ESP_LOGI(TAG, "Received: %s", rx_buffer);
            lv_textarea_set_text(ui_TextArea8, rx_buffer_reg_translate);
        }    
    }
    vTaskDelete(NULL);
}

static void tcp_machine_clear(void *arg)
{  
   bool tcp_machine_clear_reg   = false;
   while(1)
   {
      sys_param_t *sys_set = settings_get_parameter();
      if(sys_set->button_machine_clear != tcp_machine_clear_reg)
      {
         tcp_machine_clear_reg = sys_set->button_machine_clear;
         memset(rx_buffer_reg_translate, 0, sizeof(rx_buffer_reg_translate));
         memset(machine_answer, 0, sizeof(machine_answer));
         lv_textarea_set_text(ui_TextArea8, rx_buffer_reg_translate);
         lv_textarea_set_text(ui_TextArea4, machine_answer);
      }

   }
}

static void tcp_machine_determine(void *arg)
{  
   bool tcp_machine_determine_reg   = false;
   while(1)
   {
      sys_param_t *sys_set = settings_get_parameter();
      if(sys_set->button_machine_determine != tcp_machine_determine_reg)
      {
         tcp_machine_determine_reg = sys_set->button_machine_determine;
         const char *translated_text = translate_text(rx_buffer_reg_translate, "auto", "en");
         ESP_LOGI(TAG,"翻译结果:%s",translated_text);
         lv_textarea_set_text(ui_TextArea4, translated_text);
      }

   }
}

void tcp_machine_translate(void)
{
    esp_err_t ret = ESP_OK;
    xTaskCreatePinnedToCore(&audio_feed_task_translate, "feed_task", 6*1024, NULL, 5, NULL, 0);   
    xTaskCreatePinnedToCore(&tcp_receive_task_translate, "receive_task", 4*1024, NULL, 5, NULL, 1); 
    xTaskCreatePinnedToCore(&tcp_machine_clear, "machine_clear_task", 2*1024, NULL, 5, NULL, 1); 
    xTaskCreatePinnedToCore(&tcp_machine_determine, "machine_clear_task", 4*1024, NULL, 5, NULL, 1); 
    ESP_LOGI(TAG, "Free heap size after creating tasks: %d", xPortGetFreeHeapSize());
}

/**********************************************************************************************************************************/

/****************************************************此处为AI智能问答的过程********************************************************/

static void audio_feed_task_AI(void *arg)
{
    size_t bytes_read = 0;

    int audio_chunksize = 1024;
    int feed_channel = 2;
    ESP_LOGI(TAG, "audio_chunksize=%d, feed_channel=%d", audio_chunksize, feed_channel);
    // int16_t *audio_buffer = (int16_t *)malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    int16_t *audio_buffer = (int16_t *)malloc(1920*2);
    if (NULL == audio_buffer) 
    {
        esp_system_abort("No mem for audio buffer");
    }
    // 将音频缓冲区指针存储到全局变量 g_sr_data 中
    tcp_client_task_translate();
    while (true)
    {  
        sys_param_t *sys_set = settings_get_parameter();
        if(sys_set->tcp_AI_Q_A == false) break;
        bsp_extra_i2s_read((char *)audio_buffer, 1920*2, &bytes_read, portMAX_DELAY);
        int16_t data[1920] ={0};
        for (int  i = 0; i <1920; i++) 
        {  
           data[i] = audio_buffer[i*2];
        }

        // ESP_LOGI(TAG, "byte_read =%d ", bytes_read/2);
        send(sock, data, bytes_read/2, 0);
    }
    free(audio_buffer);
    vTaskDelete(NULL);
}
static void tcp_receive_AI_Question_Answer(void *arg)
{   
    bool tcp_machine_clear_reg   = false;
    char rx_buffer[128];
    while (1) 
    {   

        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) 
        {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        } else if (len == 0) 
        {
            ESP_LOGI(TAG, "Connection closed");
            break;
        } 
        else 
        {    
            rx_buffer[len] = 0;
            if(reg_total_len_translate+len <=12700)
            { 
               reg_total_len_translate = reg_total_len_translate+len;
               strcat(rx_buffer_reg_translate, rx_buffer);
            }
            else
            {
                reg_total_len_translate = len;
                memset(rx_buffer_reg_translate, 0, sizeof(rx_buffer_reg_translate)); 
                strcat(rx_buffer_reg_translate, rx_buffer);
            }
            ESP_LOGI(TAG, "Received: %s", rx_buffer);
            lv_textarea_set_text(ui_TextArea9, rx_buffer_reg_translate);
        }    
    }
    vTaskDelete(NULL);
}

static void tcp_AI_Q_A_clear(void *arg)
{  
   bool tcp_AI_Q_A_clear_reg   = false;
   while(1)
   {
      sys_param_t *sys_set = settings_get_parameter();
      if(sys_set->tcp_AI_Q_A_Clear != tcp_AI_Q_A_clear_reg)
      {
         tcp_AI_Q_A_clear_reg = sys_set->tcp_AI_Q_A_Clear;
         memset(rx_buffer_reg_translate, 0, sizeof(rx_buffer_reg_translate));
         memset(machine_answer, 0, sizeof(machine_answer));
         lv_textarea_set_text(ui_TextArea9, rx_buffer_reg_translate);
         lv_textarea_set_text(ui_TextArea3, machine_answer);
      }

   }
}

static void tcp_AI_Q_A_Determine(void *arg)
{  
   bool tcp_AI_Q_A_Determine_reg   = false;
   while(1)
   {
      sys_param_t *sys_set = settings_get_parameter();
      if(sys_set->tcp_AI_Q_A_Determine != tcp_AI_Q_A_Determine_reg)
      {
         tcp_AI_Q_A_Determine_reg = sys_set->tcp_AI_Q_A_Determine;
         const char *ai_text = app_ask_tongyi(rx_buffer_reg_translate);
         lv_textarea_set_text(ui_TextArea3, ai_text);
      }

   }
}

void tcp_AI_Q_A_Begin(void)
{
    esp_err_t ret = ESP_OK;
    xTaskCreatePinnedToCore(&audio_feed_task_AI, "feed_task", 6*1024, NULL, 5, NULL, 0);   
    xTaskCreatePinnedToCore(&tcp_receive_AI_Question_Answer, "receive_task", 4*1024, NULL, 5, NULL, 1); 
    xTaskCreatePinnedToCore(&tcp_AI_Q_A_clear, "machine_clear_task", 2*1024, NULL, 5, NULL, 1); 
    xTaskCreatePinnedToCore(&tcp_AI_Q_A_Determine, "machine_clear_task", 4*1024, NULL, 5, NULL, 1); 
    ESP_LOGI(TAG, "Free heap size after creating tasks: %d", xPortGetFreeHeapSize());
}



