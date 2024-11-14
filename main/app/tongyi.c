#include "tongyi.h"
static const char *TAG = "HTTP_TONGYI";
esp_http_client_handle_t client;
QueueHandle_t xQueue;
char *tongyi_url = "https://dashscope.aliyuncs.com/api/v1/services/aigc/text-generation/generation";
char *key = "sk-5d62f937da0148b9918a9c46856a5856";
char *format = "{\"model\": \"qwen-turbo\",\"input\": {\"messages\": [{\"role\": \"system\",\"content\": \"你是智能助手！\"},{\"role\": \"user\",\"content\": \"%s\"}]},\"parameters\": {\"result_format\": \"message\"}}";


char parsed_result[1024] = {0};//保存最终结果

//解析json
void parse_json(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) 
    {
        ESP_LOGE("JSON", "解析 JSON 失败");
        return;
    }
    // 提取 "content" 字段
    cJSON *output = cJSON_GetObjectItem(root, "output");
    if (output != NULL) 
    {
        cJSON *choices = cJSON_GetObjectItem(output, "choices");
        if (choices != NULL && cJSON_IsArray(choices)) 
        {
            cJSON *choice = cJSON_GetArrayItem(choices, 0);
            if (choice != NULL) 
            {
                cJSON *message = cJSON_GetObjectItem(choice, "message");
                if (message != NULL) 
                {
                    cJSON *content = cJSON_GetObjectItem(message, "content");
                    if (content != NULL) 
                    {
                        ESP_LOGI("JSON", "解析结果: %s", content->valuestring);
                        strncpy(parsed_result, content->valuestring, sizeof(parsed_result) - 1);
                        //存储结果
                    }
                }
            }
        }
    }
    // 释放 cJSON 对象
    cJSON_Delete(root);
}

//获得回答
esp_err_t app_http_tongyi_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA)
    {
        //ESP_LOGI(TAG, "ss%.*s", evt->data_len, (char *)evt->data);
        parse_json(evt->data);//解析获得的json数据，然后打印出来
        //printf("result:%s",parsed_result);
    }
    return ESP_OK;
}


char *app_ask_tongyi(char *question)
{
    char *data = heap_caps_calloc(1, strlen(format) + strlen(question) + 1, MALLOC_CAP_DMA);
    sprintf(data, format, question);
    ESP_LOGI(TAG, "ss%s dd%d", question, strlen(question));
    //配置http客户端
    esp_http_client_config_t config = {
        .url = tongyi_url,
        .event_handler = app_http_tongyi_event_handler,
        .buffer_size = 4 * 1024,
    };
    client = esp_http_client_init(&config);
    
    if(NULL == client)  //如果没有这个，esp32会重启
    {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        free(data);
        //return;
    }
    
    /**********/
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", key);
    esp_http_client_set_post_field(client, data, strlen(data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", 
        esp_http_client_get_status_code(client), 
        (int)esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGI(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    free(data);
    return parsed_result;
}