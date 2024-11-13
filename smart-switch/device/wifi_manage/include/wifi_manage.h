/* SoftAP based Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once
#include "freertos/FreeRTOS.h"
#include <freertos/event_groups.h>
#include <esp_event.h>

#define EVENT_WIFI_CONFIG_READY       (1 << 0) // Bit 0: Wi-Fi 配置准备就绪
#define EVENT_WIFI_PROV_CONNECTED     (1 << 1) // Bit 1: 设备已连接到 Wi-Fi

typedef enum {
    WIFI_PROV_IDLE,
    WIFI_PROV_CONNECTING,
    WIFI_PROV_CONNECTED,
    WIFI_PROV_ERROR
} wifi_prov_state_t;

typedef enum {
    WIFI_PROV_AUTH_ERROR,
    WIFI_PROV_AP_NOT_FOUND
} wifi_prov_fail_reason_t;

struct wifi_prov_status {
    bool in_prov;
    wifi_prov_state_t prov_status;
    wifi_prov_fail_reason_t fail_reason;
};

extern struct wifi_prov_status g_wifi_prov_status;

extern char *g_ssid;
extern char *g_password;

EventGroupHandle_t get_wifi_prov_event_group();


/**
 * @brief   Checks if device is provisioned
 * *
 * @param[out] provisioned  True if provisioned, else false
 *
 * @return
 *  - ESP_OK      : Retrieved provision state successfully
 *  - ESP_FAIL    : Failed to retrieve provision state
 */
bool app_prov_is_provisioned(void);

/**
 * @brief   Start provisioning via softAP
 *
 * Starts the WiFi softAP with specified ssid and pass, provisioning
 * security mode and proof of possession (if any).
 *
 * @param[in] ssid      SSID for SoftAP
 * @param[in] pass      Password for SoftAP
 * @param[in] security  Security mode
 * @param[in] pop       Pointer to proof of possession (NULL if not present)
 *
 * @return
 *  - ESP_OK      : Provisioning started successfully
 *  - ESP_FAIL    : Failed to start
 */
esp_err_t softap_provisioning_task(const char *ssid, const char *pass);

esp_err_t start_wifi_sta();

void start_wifi_sta_until_got_ip();

void wifi_init();

esp_netif_t* start_wifi_apsta(const char *ssid, const char *pass);

