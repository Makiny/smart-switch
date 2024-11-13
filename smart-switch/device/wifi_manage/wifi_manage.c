/* SoftAP based Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event.h>

#include "wifi_manage.h"
#include "led.h"
#include "web_server.h"

// 最大重连次数
#define PROV_MAX_RETRY_COUNT 5
#define INITIAL_RECONNECT_INTERVAL_MS 100 // 初始重连间隔（毫秒）
#define MAX_RECONNECT_INTERVAL_MS 120000 // 最大重连间隔（毫秒）

static const char *TAG = "app_prov";

static int prov_retry_count = 1; // 重连计数器
static int current_reconnect_interval = INITIAL_RECONNECT_INTERVAL_MS;
esp_timer_handle_t reconnect_timer;

EventGroupHandle_t wifi_prov_event_group = NULL;

char *g_ssid = NULL;
char *g_password = NULL;

struct wifi_prov_status g_wifi_prov_status = {
        false,
        WIFI_PROV_IDLE,
        WIFI_PROV_AUTH_ERROR,
};

EventGroupHandle_t get_wifi_prov_event_group() {
    if(NULL == wifi_prov_event_group) {
        wifi_prov_event_group = xEventGroupCreate();
    }
    return wifi_prov_event_group;
}

static void init_wifi_prov_status() {
    g_wifi_prov_status.in_prov = true;
    prov_retry_count = 1;
    g_wifi_prov_status.prov_status = WIFI_PROV_CONNECTING;
}

static void clear_wifi_prov_status() {
    g_wifi_prov_status.in_prov = false;

    free(g_ssid);
    g_ssid=NULL;
    free(g_password);
    g_password=NULL;
}

static void reconnect_timer_callback(void* arg) {
    esp_wifi_connect();
    current_reconnect_interval = MIN(current_reconnect_interval * 2, MAX_RECONNECT_INTERVAL_MS);
    ESP_LOGI(TAG, "Trying to reconnect to Wi-Fi... (interval %d ms)", current_reconnect_interval);
}

static void start_reconnect_timer() {
    esp_timer_create_args_t timer_args = {
            .callback = &reconnect_timer_callback,
            .name = "reconnect_timer"
    };
    esp_timer_create(&timer_args, &reconnect_timer);
    esp_timer_start_once(reconnect_timer, current_reconnect_interval * 1000); // 微秒
}

static void stop_reconnect_timer() {
    if (reconnect_timer != NULL) {
        esp_timer_stop(reconnect_timer);
        esp_timer_delete(reconnect_timer);
        reconnect_timer = NULL;
    }
}

/* Event handler for starting/stopping provisioning */
static void app_prov_event_handler(void* handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "STA Start");
        if(g_wifi_prov_status.in_prov) {
            g_wifi_prov_status.prov_status = WIFI_PROV_CONNECTING;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "STA Got IP");
        if(g_wifi_prov_status.in_prov) {
            prov_retry_count = 0;
            g_wifi_prov_status.prov_status = WIFI_PROV_CONNECTED;
        } else {
            current_reconnect_interval = INITIAL_RECONNECT_INTERVAL_MS;
            stop_reconnect_timer();

            led_stop();
        }
        xEventGroupSetBits(get_wifi_prov_event_group(), EVENT_WIFI_PROV_CONNECTED);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "STA Disconnected");

        wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGI(TAG, "Disconnect reason : %d", disconnected->reason);

        if(g_wifi_prov_status.in_prov) {
            switch (disconnected->reason) {
                case WIFI_REASON_AUTH_EXPIRE:
                case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                case WIFI_REASON_BEACON_TIMEOUT:
                case WIFI_REASON_ASSOC_FAIL:
                case WIFI_REASON_HANDSHAKE_TIMEOUT:
                case WIFI_REASON_AUTH_FAIL:
                    ESP_LOGI(TAG, "WiFi auth error");
                    if (prov_retry_count < PROV_MAX_RETRY_COUNT) {
                        esp_wifi_connect();
                        ++prov_retry_count;
                    } else {
                        g_wifi_prov_status.prov_status = WIFI_PROV_ERROR;
                        g_wifi_prov_status.fail_reason = WIFI_PROV_AUTH_ERROR;
                        prov_retry_count = 1;
                    }
                    break;
                case WIFI_REASON_NO_AP_FOUND:
                    ESP_LOGI(TAG, "AP Not found");
                    g_wifi_prov_status.prov_status = WIFI_PROV_ERROR;
                    g_wifi_prov_status.fail_reason = WIFI_PROV_AP_NOT_FOUND;
                    break;
                default:
                    esp_wifi_connect();
            }
        } else {
            ESP_LOGI(TAG, "reconnect to WiFi");
            start_reconnect_timer();
            led_start(BLINK_NET_CONNECTING);
        }
    }
}

bool app_prov_is_provisioned()
{
    ESP_LOGI(TAG, "Get WiFi Station configuration");

    wifi_config_t wifi_cfg;
    esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg);

    if (strlen((const char*) wifi_cfg.sta.ssid)) {

        ESP_LOGI(TAG, "Found ssid %s",     (const char*) wifi_cfg.sta.ssid);
        ESP_LOGI(TAG, "Found password %s", (const char*) wifi_cfg.sta.password);
        return true;
    }

    ESP_LOGI(TAG, "Found password is empty");
    return false;
}

void wifi_init()
{
    /* Initialize networking stack */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Create default event loop needed by the
     * main app and the provisioning service */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, app_prov_event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, app_prov_event_handler, NULL));
}

/**
 * Wi-Fi设置为STA模式
 * @param wifi_cfg
 * @return
 */
esp_err_t start_wifi_sta() {
    /* Configure WiFi as both AP and Station */
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode");
        return ESP_FAIL;
    }

    wifi_config_t wifi_cfg;
    esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg);
    /* Configure WiFi station with host credentials
     * provided during provisioning */

    if (esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi configuration");
        return ESP_FAIL;
    }
    /* Restart WiFi */
    if (esp_wifi_start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to restart WiFi");
        return ESP_FAIL;
    }
    /* Connect to AP */
    if (esp_wifi_connect() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect WiFi");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void start_wifi_sta_until_got_ip() {
    start_wifi_sta();
    EventGroupHandle_t x_wifi_prov_event_group = get_wifi_prov_event_group();
    xEventGroupWaitBits(
            x_wifi_prov_event_group,
            EVENT_WIFI_PROV_CONNECTED,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
    );
}

/**
 * Wi-Fi设置为AP+STA模式
 * @param ssid
 * @param pass
 * @return
 */
esp_netif_t* start_wifi_apsta(const char *ssid, const char *pass) {

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_t *netif_ap = esp_netif_create_default_wifi_ap();

    /* Build WiFi configuration for AP mode */
    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = 5,
        },
    };

    strlcpy((char *) wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));

    if (strlen(pass) == 0) {
        memset(wifi_config.ap.password, 0, sizeof(wifi_config.ap.password));
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        strlcpy((char *) wifi_config.ap.password, pass, sizeof(wifi_config.ap.password));
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }

    /* Start WiFi in AP mode with configuration built above */
    esp_err_t  err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode : %d", err);
    }
    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config : %d", err);
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi : %d", err);
    }

    return netif_ap;
}

esp_err_t softap_provisioning_task(const char *ssid, const char *pass) {
    if(g_wifi_prov_status.in_prov) {
        return ESP_OK;
    }

    start_web_server_task(ssid, pass);

    EventGroupHandle_t x_wifi_prov_event_group = get_wifi_prov_event_group();

    while(true) {

        EventBits_t bits = xEventGroupWaitBits(
                x_wifi_prov_event_group,
                EVENT_WIFI_CONFIG_READY | EVENT_WIFI_PROV_CONNECTED,
                pdTRUE,
                pdFALSE,
                portMAX_DELAY
        );

        if(bits & EVENT_WIFI_CONFIG_READY) {
            init_wifi_prov_status();

            ESP_LOGI(TAG, "Starting to connect to Wi-Fi...");
            wifi_config_t wifi_config = {
                    .sta = {
                            .ssid = "",
                            .password = "",
                            .bssid_set = 0,
                    },
            };

            strlcpy((char *) wifi_config.sta.ssid, g_ssid, sizeof(wifi_config.sta.ssid));
            strlcpy((char *) wifi_config.sta.password, g_password, sizeof(wifi_config.sta.password));

            // Set Wi-Fi configuration
            esp_wifi_set_storage(WIFI_STORAGE_RAM);
            esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set WiFi config : %d", err);
                return ESP_OK;
            }

            // Connect to the specified Wi-Fi network
            err = esp_wifi_connect();
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to connect to WiFi : %d", err);
                return ESP_OK;
            }

            ESP_LOGI(TAG, "Attempting to connect to Wi-Fi...");
        }

        if (bits & EVENT_WIFI_PROV_CONNECTED) {
            wifi_config_t wifi_config;
            esp_wifi_get_config(WIFI_IF_STA, &wifi_config);

            esp_wifi_set_storage(WIFI_STORAGE_FLASH);
            esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set WiFi config : %d", err);
                return ESP_OK;
            }

            ESP_LOGI(TAG, "Device connected to AP!!!");
            break;
        }
    }

    web_server_auto_stop();
    clear_wifi_prov_status();

    return ESP_OK;
}



