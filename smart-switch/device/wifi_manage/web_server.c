/**
 * @author kaiyin
 */


#include <sys/_default_fcntl.h>
#include <sys/cdefs.h>

#include "spiffs.h"
#include <esp_wifi.h>
#include <esp_log.h>

#include "http_server.h"
#include "dns_server.h"
#include "wifi_manage.h"

static const char *TAG = "web_server";

static esp_netif_t *netif_ap = NULL;
static esp_timer_handle_t web_server_timer; // 定时器句柄
static const int WEB_SERVER_TIMEOUT_S = 60; // 无连接时定时器的超时时间（单位秒）

static const char *p_ssid;
static const char *p_pass;

static TaskHandle_t web_server_task_handle = NULL;

void stop_web_server_task();

// 定时器回调函数
static void web_server_timeout_callback(void* arg) {
    ESP_LOGI(TAG, "No connection received, stopping web server...");
    stop_web_server_task();
}

void web_server_auto_stop() {
    esp_timer_create_args_t timer_args = {
            .callback = &web_server_timeout_callback,
            .name = "web_server_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &web_server_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(web_server_timer, WEB_SERVER_TIMEOUT_S * 1000000)); // 微秒
}

// 停止并删除定时器
static void stop_web_server_timer() {
    if (web_server_timer != NULL) {
        esp_timer_stop(web_server_timer);
        esp_timer_delete(web_server_timer);
        web_server_timer = NULL;
    }
}

void reset_web_server_auto_stop() {
    if (web_server_timer != NULL) {
        esp_timer_stop(web_server_timer);
        ESP_ERROR_CHECK(esp_timer_start_once(web_server_timer, WEB_SERVER_TIMEOUT_S * 1000000)); // 重置计时器
    }
}

_Noreturn static void web_server_task(void *arg) {
    mount_spiffs();
    start_http_server();
    start_dns_server();

    netif_ap = start_wifi_apsta(p_ssid, p_pass);

    while(true) {
        vTaskDelay(portMAX_DELAY);
    }
}

static void stop_web_server() {

    stop_dns_server();
    stop_http_server();
    unmount_spiffs();

    start_wifi_sta();

    if(netif_ap) {
        esp_netif_destroy(netif_ap);
    }

    netif_ap = NULL;
}

void start_web_server_task(const char *ssid, const char *pass) {
    if(web_server_task_handle) {
        return;
    }

    p_ssid = ssid;
    p_pass = pass;

    xTaskCreate(web_server_task, "web_server_task", 4*1024,
                NULL, 5, &web_server_task_handle);
}

void stop_web_server_task() {
    if(!web_server_task_handle) {
        return;
    }
    stop_web_server_timer();

    stop_web_server();

    vTaskDelay(1000);

    vTaskDelete(web_server_task_handle);
    web_server_task_handle = NULL;
}