//
// Created by makiny on 2024/7/27.
//
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include <esp_wifi.h>
#include "esp_netif.h"
#include "esp_http_server.h"
#include <cJSON.h>
#include "http_server.h"
#include "switch_control.h"
#include "device.h"
#include "energy_statistics.h"
#include "web_server.h"
#include "wifi_manage.h"
#include "system_time.h"

#define BSSID_STR_LEN 18  // BSSID字符串长度 (包含 '\0')

static const char *TAG = "http_server";

static httpd_handle_t http_server_handler = NULL;

/**
 * 处理静态资源文件
 * @param req
 * @param filepath
 * @param content_type
 * @return
 */
static esp_err_t send_file(httpd_req_t *req, const char *filepath, const char *content_type, bool is_gzip) {
    char file_to_send[32];
    strcpy(file_to_send, filepath);
    if (is_gzip) {
        strcat(file_to_send, ".gz");
    }

    FILE* f = fopen(file_to_send, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", file_to_send);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, content_type);
    if(is_gzip) {
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    }

    char buffer[256];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        httpd_resp_send_chunk(req, buffer, (ssize_t)bytes_read);
    }
    httpd_resp_sendstr_chunk(req, NULL);
    fclose(f);

    return ESP_OK;
}

/**
 * 处理 index.html
 */
static esp_err_t portal_handler(httpd_req_t *req) {
    return send_file(req, "/spiffs/index.html", "text/html", false);
}

/**
 * 处理 index.css
 */
static esp_err_t css_handler(httpd_req_t *req) {
    return send_file(req, "/spiffs/index.css", "text/css", true);
}

/**
 * 处理 index.js
 */
static esp_err_t js_handler(httpd_req_t *req) {
    return send_file(req, "/spiffs/index.js", "application/javascript", true);
}

/**
 * 获取可用 Wi-Fi 网络列表
 * @param req
 * @return
 */
static esp_err_t wifi_list(httpd_req_t *req) {
    wifi_scan_config_t scan_config = {
            .ssid = NULL,
            .bssid = NULL,
            .channel = 0,
            .show_hidden = false
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_num = 0;
    esp_wifi_scan_get_ap_num(&ap_num);
    wifi_ap_record_t *ap_records = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_num);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));

    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < ap_num; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ssid", (char *)ap_records[i].ssid);
        char bssid_str[BSSID_STR_LEN];
        snprintf(bssid_str, BSSID_STR_LEN, MACSTR, MAC2STR(ap_records[i].bssid));
        cJSON_AddStringToObject(item, "bssid", bssid_str);
        cJSON_AddNumberToObject(item, "rssi", ap_records[i].rssi);
        cJSON_AddNumberToObject(item, "auth", ap_records[i].authmode == 0 ? 0 : 1);
        cJSON_AddItemToArray(root, item);
    }
    free(ap_records);

    const char *json_response = cJSON_Print(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    free((void *)json_response);

    return ESP_OK;
}

/**
 * 连接到 Wi-Fi 网络
 * @param req
 * @return
 */
static esp_err_t wifi_connect(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;

    // 读取请求体
    while (remaining > 0) {
        if (remaining > sizeof(buf) - 1) {
            ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
        } else {
            ret = httpd_req_recv(req, buf, remaining);
        }

        if (ret < 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(TAG, "Failed to receive request payload");
            return ESP_FAIL;
        }

        buf[ret] = '\0';
        remaining -= ret;
    }

    // 使用 cJSON 解析 JSON 请求体
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // 获取 ssid 和 cred
    cJSON *ssid_json = cJSON_GetObjectItem(json, "ssid");
    cJSON *cred_json = cJSON_GetObjectItem(json, "cred");

    if (!cJSON_IsString(ssid_json) || !cJSON_IsString(cred_json)) {
        ESP_LOGE(TAG, "Invalid JSON format");
        cJSON_Delete(json);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    const char *ssid = ssid_json->valuestring;
    const char *password = cred_json->valuestring;

    ESP_LOGI(TAG, "SSID: %s", ssid);
    ESP_LOGI(TAG, "Password: %s", password);

    free(g_ssid);
    free(g_password);

    g_ssid = malloc(strlen(ssid) + 1);
    g_password = malloc(strlen(password) + 1);

    if (g_ssid == NULL || g_password == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }
    strcpy(g_ssid, ssid);
    strcpy(g_password, password);

    // 响应客户端
    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddStringToObject(response_json, "status", "OK");
    const char *resp_str = cJSON_Print(response_json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "get wifi config");
    xEventGroupSetBits(get_wifi_prov_event_group(), EVENT_WIFI_CONFIG_READY);

    cJSON_Delete(json);
    cJSON_Delete(response_json);
    free((void *)resp_str);

    return ESP_OK;
}

/**
 * 查询 Wi-Fi 配网状态
 * @param req
 * @return
 */
static esp_err_t wifi_prov_status(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();

    // 添加配网状态
    switch (g_wifi_prov_status.prov_status) {
        case WIFI_PROV_CONNECTING:
            cJSON_AddStringToObject(root, "status", "connecting");
            ESP_LOGI(TAG, "Provisioning status: %s", "connecting");
            break;
        case WIFI_PROV_CONNECTED:
            cJSON_AddStringToObject(root, "status", "connected");
            ESP_LOGI(TAG, "Provisioning status: %s", "connected");
            break;
        case WIFI_PROV_ERROR:
            cJSON_AddStringToObject(root, "status", "error");
            ESP_LOGI(TAG, "Provisioning status: %s", "error");
            break;
        default:
            cJSON_AddStringToObject(root, "status", "unknown");
            ESP_LOGI(TAG, "Provisioning status: %s", "unknown");
            break;
    }

    // 添加失败原因
    if (g_wifi_prov_status.prov_status == WIFI_PROV_ERROR) {
        switch (g_wifi_prov_status.fail_reason) {
            case WIFI_PROV_AUTH_ERROR:
                cJSON_AddStringToObject(root, "reason", "auth_error");
                break;
            case WIFI_PROV_AP_NOT_FOUND:
                cJSON_AddStringToObject(root, "reason", "ap_not_found");
                break;
            default:
                cJSON_AddStringToObject(root, "reason", "unknown");
                break;
        }
    }

    const char *json_response = cJSON_Print(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    free((void *)json_response);

    return ESP_OK;
}

/**
 * 处理 Wi-Fi 相关请求
 * @param req
 * @return
 */
static esp_err_t wifi_handler(httpd_req_t *req) {

    reset_web_server_auto_stop();

    // 存储查询参数
    char query_string[50];

    // 获取查询参数
    if (httpd_req_get_url_query_str(req, query_string, sizeof(query_string)) != ESP_OK) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Query string: %s", query_string);

    // 从查询字符串中解析 "action" 参数
    char action[10];
    if (httpd_query_key_value(query_string, "action", action, sizeof(action)) != ESP_OK) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Action parameter: %s", action);

    // 根据 action 参数执行不同的逻辑
    if (strcmp(action, "list") == 0) {
        return wifi_list(req);
    } else if (strcmp(action, "connect") == 0) {
        return wifi_connect(req);
    } else if (strcmp(action, "status") == 0) {
        return wifi_prov_status(req);
    }

    httpd_resp_send_404(req);
    return ESP_FAIL;
}

/**
 * 获取设备配置
 * @param req
 * @return
 */
static esp_err_t device_config_get(httpd_req_t *req) {
    // 获取配置
    cJSON *root = cJSON_CreateObject();

    // 添加字段到 JSON 对象
    cJSON_AddNumberToObject(root, "sw_mode", device_config.switch_control.mode);
    cJSON_AddNumberToObject(root, "sw_delay_time", device_config.switch_control.delay_time);
    cJSON_AddNumberToObject(root, "pwr_pro", device_config.power_protection);
    cJSON_AddNumberToObject(root, "pwr_pro_thr", device_config.power_protection_threshold);
    cJSON_AddNumberToObject(root, "tmp_pro", device_config.temperature_protection);
    cJSON_AddNumberToObject(root, "tmp_pro_thr", device_config.temperature_protection_threshold);

    // 创建 JSON 响应
    const char *json_response = cJSON_Print(root);
    cJSON_Delete(root);

    // 设置响应类型并发送响应
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);

    free((void *)json_response);

    return ESP_OK;
}

/**
 * 更新设备配置
 * @param req
 * @return
 */
static esp_err_t device_config_update(httpd_req_t *req) {
    // 更新配置
    char buf[100];
    int ret;
    size_t remaining = req->content_len;

    // 读取请求体
    while (remaining > 0) {
        if (remaining > sizeof(buf) - 1) {
            ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
        } else {
            ret = httpd_req_recv(req, buf, remaining);
        }

        if (ret < 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(TAG, "Failed to receive request payload");
            return ESP_FAIL;
        }

        buf[ret] = '\0';
        remaining -= ret;
    }

    // 使用 cJSON 解析 JSON 请求体
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    device_config_t config;
    memcpy(&config, &device_config, sizeof(device_config_t));

    // 更新配置
    cJSON *sw_mode_json = cJSON_GetObjectItem(json, "sw_mode");
    if (cJSON_IsNumber(sw_mode_json)) {
        config.switch_control.mode = sw_mode_json->valueint;
    }

    cJSON *sw_delay_time_json = cJSON_GetObjectItem(json, "sw_delay_time");
    if (cJSON_IsNumber(sw_delay_time_json)) {
        config.switch_control.delay_time = sw_delay_time_json->valueint;
    }

    cJSON *pwr_pro_json = cJSON_GetObjectItem(json, "pwr_pro");
    if (cJSON_IsNumber(pwr_pro_json)) {
        config.power_protection = pwr_pro_json->valueint;
    }

    cJSON *pwr_pro_thr_json = cJSON_GetObjectItem(json, "pwr_pro_thr");
    if (cJSON_IsNumber(pwr_pro_thr_json)) {
        config.power_protection_threshold = pwr_pro_thr_json->valueint;
    }

    cJSON *tmp_pro_json = cJSON_GetObjectItem(json, "tmp_pro");
    if (cJSON_IsNumber(tmp_pro_json)) {
        config.temperature_protection = tmp_pro_json->valueint;
    }

    cJSON *tmp_pro_thr_json = cJSON_GetObjectItem(json, "tmp_pro_thr");
    if (cJSON_IsNumber(tmp_pro_thr_json)) {
        config.temperature_protection_threshold = tmp_pro_thr_json->valueint;
    }

    save_device_config_increment(&config);

    // 创建响应 JSON
    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddStringToObject(response_json, "status", "OK");
    const char *resp_str = cJSON_Print(response_json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    // 清理
    cJSON_Delete(json);
    cJSON_Delete(response_json);
    free((void *)resp_str);

    return ESP_OK;
}

/**
 * 设备配置（获取和更新）
 * @param req
 * @return
 */
static esp_err_t device_config_handler(httpd_req_t *req) {
    reset_web_server_auto_stop();

    // 获取URL参数
    char query_string[20];
    if (httpd_req_get_url_query_str(req, query_string, sizeof(query_string)) != ESP_OK) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Query string: %s", query_string);

    // 解析 action 参数
    char action[10];
    if (httpd_query_key_value(query_string, "action", action, sizeof(action)) != ESP_OK) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Action parameter: %s", action);

    if (strcmp(action, "get") == 0) {
        return device_config_get(req);
    } else if (strcmp(action, "update") == 0) {
        return device_config_update(req);
    }

    httpd_resp_send_404(req);
    return ESP_FAIL;
}

/**
 * 获取设备信息
 * @param req
 * @return
 */
static esp_err_t device_info_get(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();

    // 添加字段到 JSON 对象
    cJSON_AddStringToObject(root, "model", device_info.model);
    cJSON_AddStringToObject(root, "hw_v", device_info.hw_v);
    cJSON_AddStringToObject(root, "fw_v", device_info.fw_v);
    cJSON_AddNumberToObject(root, "runtime", device_status.runtime);

    // 创建 JSON 响应
    const char *json_response = cJSON_Print(root);
    cJSON_Delete(root);

    // 设置响应类型并发送响应
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    free((void *) json_response);

    return ESP_OK;
}

/**
 * 设备重置
 * @param req
 * @return
 */
static esp_err_t device_reset(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;

    // 读取请求体
    while (remaining > 0) {
        if (remaining > sizeof(buf) - 1) {
            ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
        } else {
            ret = httpd_req_recv(req, buf, remaining);
        }

        if (ret < 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(TAG, "Failed to receive request payload");
            return ESP_FAIL;
        }

        buf[ret] = '\0';
        remaining -= ret;
    }

    // 使用 cJSON 解析 JSON 请求体
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    cJSON *rst_mode_json = cJSON_GetObjectItem(json, "rstMode");
    if(cJSON_IsString(rst_mode_json)) {
        if(strcmp(rst_mode_json->valuestring, "rstNet") == 0) {
            ESP_LOGE(TAG, "rstNet");
            scb_event_ctx_t scb_event_ctx ;
            scb_event_ctx.event = SCB_EVENT_RESET_NETWORK;
            device_send_event(scb_event_ctx);
        } else if(strcmp(rst_mode_json->valuestring, "rstPlf") == 0) {
            ESP_LOGE(TAG, "rstPlf");
            scb_event_ctx_t scb_event_ctx ;
            scb_event_ctx.event = SCB_EVENT_RESET_PLATFORM;
            device_send_event(scb_event_ctx);
        } else if(strcmp(rst_mode_json->valuestring, "rstFat") == 0) {
            ESP_LOGE(TAG, "rstFat");
            scb_event_ctx_t scb_event_ctx ;
            scb_event_ctx.event = SCB_EVENT_RESET_TO_FACTORY;
            device_send_event(scb_event_ctx);
        }
    }
    cJSON_Delete(json);

    // 设置响应类型并发送响应
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "OK");
    const char *json_response = cJSON_Print(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    free((void *) json_response);

    return ESP_OK;
}

/**
 * 设备控制
 * @param req
 * @return
 */
static esp_err_t device_control(httpd_req_t *req) {
    char buf[50];
    int ret, remaining = req->content_len;

    // 读取请求体
    while (remaining > 0) {
        if (remaining > sizeof(buf) - 1) {
            ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
        } else {
            ret = httpd_req_recv(req, buf, remaining);
        }

        if (ret < 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(TAG, "Failed to receive request payload");
            return ESP_FAIL;
        }

        buf[ret] = '\0';
        remaining -= ret;
    }

    // 使用 cJSON 解析 JSON 请求体
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    cJSON *rst_mode_json = cJSON_GetObjectItem(json, "ctrlCmd");
    if(cJSON_IsNumber(rst_mode_json)) {
        if(0 == rst_mode_json->valueint) {
            switch_status_update(false);
        } else if (1 == rst_mode_json->valueint){
            switch_status_update(true);
        }
    }
    cJSON_Delete(json);


    cJSON *root = cJSON_CreateObject();
    // 添加字段到 JSON 对象
    cJSON_AddStringToObject(root, "status", "OK");

    // 创建 JSON 响应
    const char *json_response = cJSON_Print(root);
    cJSON_Delete(root);

    // 设置响应类型并发送响应
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    free((void *) json_response);

    return ESP_OK;
}

/**
 * 设备管理
 * @param req
 * @return
 */
static esp_err_t device_handler(httpd_req_t *req) {
    // 存储查询参数
    char query_string[50];

    reset_web_server_auto_stop();

    // 获取查询参数
    if (httpd_req_get_url_query_str(req, query_string, sizeof(query_string)) != ESP_OK) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Query string: %s", query_string);

    // 从查询字符串中解析 "action" 参数
    char action[10];
    if (httpd_query_key_value(query_string, "action", action, sizeof(action)) != ESP_OK) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Action parameter: %s", action);

    if (strcmp(action, "info") == 0) {
        return device_info_get(req);
    } else if (strcmp(action, "reset") == 0) {
        return device_reset(req);
    } else if (strcmp(action, "ctrl") == 0) {
        return device_control(req);
    }

    httpd_resp_send_404(req);
    return ESP_FAIL;
}

/**
 * 设备状态
 * @param req
 * @return
 */
static esp_err_t device_status_handler(httpd_req_t *req) {
    char buf[150];
    int ret, remaining = req->content_len;

    reset_web_server_auto_stop();

    // 读取请求体
    while (remaining > 0) {
        if (remaining > sizeof(buf) - 1) {
            ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
        } else {
            ret = httpd_req_recv(req, buf, remaining);
        }

        if (ret < 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(TAG, "Failed to receive request payload");
            return ESP_FAIL;
        }

        buf[ret] = '\0';
        remaining -= ret;
    }

    // 使用 cJSON 解析 JSON 请求体
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    cJSON *query = cJSON_GetObjectItem(json, "query");
    if (!cJSON_IsArray(query)) {
        ESP_LOGE(TAG, "Invalid query format");
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    cJSON *response_json = cJSON_CreateObject();

    // 遍历 "query" 数组，根据需要构建响应
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, query) {
        if (cJSON_IsString(item)) {
            const char *query_str = cJSON_GetStringValue(item);

            if (strcmp(query_str, "pwr_pro") == 0) {
                cJSON_AddNumberToObject(response_json, "pwr_pro", device_status.in_power_protection);
            } else if (strcmp(query_str, "tmp_pro") == 0) {
                cJSON_AddNumberToObject(response_json, "tmp_pro", device_status.in_temperature_protection);
            } else if (strcmp(query_str, "cur_tmp") == 0) {
                cJSON_AddNumberToObject(response_json, "cur_tmp", device_status.temperature);
            } else if (strcmp(query_str, "sw_status") == 0) {
                cJSON_AddNumberToObject(response_json, "sw_status", device_config.switch_control.status);
            } else if (strcmp(query_str, "wifi_con") == 0) {
                wifi_ap_record_t ap_info;
                esp_wifi_sta_get_ap_info(&ap_info);

                cJSON *wifi_con_response = cJSON_CreateObject();
                cJSON_AddStringToObject(wifi_con_response, "ssid", (char *)ap_info.ssid);
                cJSON_AddNumberToObject(wifi_con_response, "rssi", ap_info.rssi);
                char bssid_str[BSSID_STR_LEN];
                snprintf(bssid_str, BSSID_STR_LEN, MACSTR, MAC2STR(ap_info.bssid));
                cJSON_AddStringToObject(wifi_con_response, "bssid", bssid_str);
                cJSON_AddNumberToObject(wifi_con_response, "auth", ap_info.authmode==0 ? 0 : 1);
                cJSON_AddItemToObject(response_json, "wifi_con", wifi_con_response);


            } else if (strcmp(query_str, "eng_today_usage") == 0) {
                cJSON_AddNumberToObject(response_json, "eng_today_usage", get_today_energy_usage());
            } else if (strcmp(query_str, "eng_month_usage") == 0) {
                cJSON_AddNumberToObject(response_json, "eng_month_usage", get_monthly_energy_usage());
            } else if (strcmp(query_str, "power") == 0) {
                cJSON_AddNumberToObject(response_json, "power", device_status.power_data.power);
            }
        }
    }

    const char *resp_str = cJSON_Print(response_json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    // 清理
    cJSON_Delete(json);
    cJSON_Delete(response_json);
    free((void *)resp_str);

    return ESP_OK;
}

esp_err_t energy_statistics_query_handler(httpd_req_t *req) {
    cJSON *response_json = cJSON_CreateObject();
    cJSON *data_array = cJSON_CreateArray();

    // 遍历 energy_usage_t 数据并构建 JSON 响应
    for (int i = 0; i < POWER_USAGE_STORAGE_SIZE; i++) {
        cJSON *item = cJSON_CreateObject();
//        ESP_LOGI(TAG, "T = %d, U = %f", energy_statistics.usage_record[i].day, ((float)energy_statistics.usage_record[i].consumption / 100.0f));
        cJSON_AddNumberToObject(item, "time", (energy_statistics.usage_record[i].day * 86400));
        cJSON_AddNumberToObject(item, "data", energy_statistics.usage_record[i].consumption);
        cJSON_AddItemToArray(data_array, item);
    }

    cJSON_AddNumberToObject(response_json, "timestamp_base", TIMESTAMP_BASE_LINE);
    cJSON_AddItemToObject(response_json, "data", data_array);

    // 返回 JSON 响应
    const char *resp_str = cJSON_Print(response_json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    // 清理
    cJSON_Delete(response_json);
    free((void *)resp_str);

    return ESP_OK;
}

/**
 * 重定向到portal
 * @param req
 * @param err
 * @return
 */
static esp_err_t redirect_2_captive_portal_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // 重定向到配置页面
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to portal");
    return ESP_OK;
}

void start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = 80;
    config.ctrl_port = 32768;
    config.max_uri_handlers = 16;
    config.lru_purge_enable = true;
    config.max_open_sockets = 13;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&http_server_handler, &config) == ESP_OK) {
        // 前端
        httpd_uri_t uri_portal = {
                .uri = "/",
                .method = HTTP_GET,
                .handler = portal_handler,
                .user_ctx = NULL
        };
        httpd_uri_t uri_css = {
                .uri = "/index.css",
                .method = HTTP_GET,
                .handler = css_handler,
                .user_ctx = NULL
        };
        httpd_uri_t uri_js = {
                .uri = "/index.js",
                .method = HTTP_GET,
                .handler = js_handler,
                .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handler, &uri_portal);
        httpd_register_uri_handler(http_server_handler, &uri_css);
        httpd_register_uri_handler(http_server_handler, &uri_js);

        // wifi
        httpd_uri_t uri_wifi = {
                .uri = "/api/wifi",
                .method = HTTP_POST,
                .handler = wifi_handler,
                .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handler, &uri_wifi);

        // 设备配置
        httpd_uri_t uri_device_config = {
                .uri = "/api/config",
                .method = HTTP_POST,
                .handler = device_config_handler,
                .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handler, &uri_device_config);

        // 设备管理
        httpd_uri_t uri_device = {
                .uri = "/api/device",
                .method = HTTP_POST,
                .handler = device_handler,
                .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handler, &uri_device);

        // 设备状态
        httpd_uri_t uri_device_status = {
                .uri = "/api/status",
                .method = HTTP_POST,
                .handler = device_status_handler,
                .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handler, &uri_device_status);

        // 用电统计
        httpd_uri_t energy_statistics_query = {
                .uri = "/api/energy/statistics/get",
                .method = HTTP_POST,
                .handler = energy_statistics_query_handler,
                .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handler, &energy_statistics_query);

        httpd_register_err_handler(http_server_handler, HTTPD_404_NOT_FOUND, redirect_2_captive_portal_handler);
    }
}

void stop_http_server(void) {
    if (http_server_handler != NULL) {
        ESP_LOGI(TAG, "Stopping server");
        httpd_stop(http_server_handler);
        http_server_handler = NULL;
    }
}
