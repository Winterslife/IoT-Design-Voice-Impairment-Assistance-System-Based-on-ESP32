#include "baiduTrans.h"

#define BAIDU_TRANSLATE_URL "http://api.fanyi.baidu.com/api/trans/vip/translate"
#define APPID "20240703002091352"          // 替换为你的APPID
#define SECRET_KEY "JChP2VW561adUxkrK0mi" // 替换为你的密钥

static const char *TAG = "BaiduTranslate";
char data[512]={0};

//md5加密
void md5(const char *str, char *md5_str) 
{
    unsigned char md5_result[16];
    char tmp[3] = {0};

    mbedtls_md5_context ctx;
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (const unsigned char *)str, strlen(str));
    mbedtls_md5_finish(&ctx, md5_result);
    mbedtls_md5_free(&ctx);
    for (int i = 0; i < 16; i++) {
        sprintf(tmp, "%02x", md5_result[i]);
        strcat(md5_str, tmp);
    }
}
//解析结果
esp_err_t baiduTranslate_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA)
    {
        ESP_LOGI(TAG, "ss%.*s", evt->data_len, (char *)evt->data);
        // Parse JSON response
        cJSON *root = cJSON_Parse((char *)evt->data);
        if (root == NULL) {
            ESP_LOGE(TAG, "Failed to parse JSON response");
            return ESP_FAIL;
        }
        cJSON *trans_result = cJSON_GetObjectItem(root, "trans_result");
        if (trans_result == NULL || !cJSON_IsArray(trans_result)) {
            ESP_LOGE(TAG, "Invalid JSON structure: trans_result not found or not an array");
            cJSON_Delete(root);
            return ESP_FAIL;
        }
        cJSON *first_result = cJSON_GetArrayItem(trans_result, 0);
        if (first_result == NULL) {
            ESP_LOGE(TAG, "No translation result found in trans_result array");
            cJSON_Delete(root);
            return ESP_FAIL;
        }
        cJSON *dst_item = cJSON_GetObjectItem(first_result, "dst");
        if (dst_item == NULL || !cJSON_IsString(dst_item)) {
            ESP_LOGE(TAG, "Invalid JSON structure: dst not found or not a string");
            cJSON_Delete(root);
            return ESP_FAIL;
        }
        const char *translated_text = dst_item->valuestring;
        ESP_LOGI(TAG, "Translated text: %s", translated_text);
        strncpy(data, translated_text, sizeof(data) - 1);
        cJSON_Delete(root);
    }
    return ESP_OK;
}

char *translate_text(const char *text, const char *from, const char *to) 
{
    int salt = esp_random() % (65536 - 32768) + 32768;
    char salt_str[16];
    snprintf(salt_str, sizeof(salt_str), "%d", salt);

    char sign_str[256] = {0};
    snprintf(sign_str, sizeof(sign_str), "%s%s%s%s", APPID, text, salt_str, SECRET_KEY);

    char sign[33] = {0};
    md5(sign_str, sign);

    char post_data[512];
    snprintf(post_data, sizeof(post_data), "q=%s&from=%s&to=%s&appid=%s&salt=%s&sign=%s",
             text, from, to, APPID, salt_str, sign);
    ESP_LOGI(TAG,"%s",post_data);

    esp_http_client_config_t config = {
        .url = BAIDU_TRANSLATE_URL,
        .event_handler = baiduTranslate_event_handler,
        .buffer_size = 4 * 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if(NULL == client)  //如果没有这个，esp32会重启
        {
            ESP_LOGE(TAG, "Failed to initialize HTTP client");
        }
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

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
    return data;
}