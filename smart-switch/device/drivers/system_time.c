/**
 * @author kaiyin
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <lwip/apps/sntp.h>
#include <esp_sntp.h>
#include "device.h"
#include "system_time.h"

#define TIME_SYNC_MAX_RETRY_TIMES 10

#define DAY_BASE_LINE 19723

static const char * TAG = "system_time";

static TaskHandle_t rtc_sync_task_handle = NULL;
static bool sntp_initialized = false;

static void initialize_sntp(void) {
    if(sntp_initialized) {
        return;
    }
    sntp_initialized = true;

    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

static void system_time_sync_task(void *arg) {
    initialize_sntp();

    time_t now = 0;
    uint8_t retry = 0;

    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < TIME_SYNC_MAX_RETRY_TIMES) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", ++retry, TIME_SYNC_MAX_RETRY_TIMES);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    time(&now);
    struct timeval tv = { .tv_sec = now, .tv_usec = 0 };

    if (now < 1609459200) {
        ESP_LOGE(TAG, "Failed to get current time");
    } else if (settimeofday(&tv, NULL) != 0) {
        ESP_LOGE(TAG, "Failed to set time of day");
    } else {
        scb_event_ctx_t scb_event_ctx;
        scb_event_ctx.event = SCB_RTC_TIME_INIT_SYNCED;
        device_send_event(scb_event_ctx);
    }

    rtc_sync_task_handle = NULL;
    vTaskDelete(NULL);
}

void system_time_sync_task_create() {
    if(rtc_sync_task_handle) {
        return;
    }

    xTaskCreate(system_time_sync_task, "rtc_time_init_sync_task", 4*1024,
            NULL, 5, &rtc_sync_task_handle);
}

uint16_t unix_timestamp_to_days(uint32_t timestamp) {
    // todo 定期更新 DAY_BASE_LINE、TIMESTAMP_BASE_LINE，以解决除法精度问题
    timestamp -= TIMESTAMP_BASE_LINE;
    return DAY_BASE_LINE + timestamp/86400;
}