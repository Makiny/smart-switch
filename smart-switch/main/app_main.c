/**
    MIT License

    Copyright (c) <2024> kaiyin

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 */

#include <sys/cdefs.h>

#include <string.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/timer.h>

#include <esp_wifi.h>
#include <hap.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include "ha_switch.h"

#include "wifi_manage.h"
#include "relay.h"
#include "button.h"
#include "hlw.h"
#include "oled.h"
#include "ntc.h"
#include "device.h"
#include "system_time.h"
#include "power_protection.h"
#include "switch_control.h"
#include "temperature_protection.h"
#include "energy_statistics.h"
#include "led.h"
#include "web_server.h"

static const char * TAG = "smart_switch";

static QueueHandle_t xDeviceQueue;
static bool loop_started;

/**
 * 设备主定时任务
 * @param arg
 */
static void IRAM_ATTR device_main_timer_isr(void *arg) {
    static uint8_t counter = 0;
    static uint8_t sec_counter = 0;
    scb_event_ctx_t scb_event_ctx;

    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);

    ++counter;

    // 200ms
    if (counter % 2 == 0) {
        scb_event_ctx.event = SCB_EVENT_POWER_DATA_READ;
        xQueueSendFromISR(xDeviceQueue, &scb_event_ctx, NULL);
    }

    // 500ms
    if (counter % 5 == 0) {
    }

    // 1000ms
    if (counter % 10 == 0) {
        counter = 0;
        ++sec_counter;

        scb_event_ctx.event = SCB_EVENT_OLED_FLUSH;
        xQueueSendFromISR(xDeviceQueue, &scb_event_ctx, NULL);

        // 设备启动时长
        uint64_t boot_time = esp_timer_get_time();
        device_status.runtime = boot_time / 1000000;
    }

    // 1min
    if(sec_counter >= 60) {
        sec_counter = 0;

        if(!device_status.time_init_synced) {
            system_time_sync_task_create();
        }

        uint16_t today = unix_timestamp_to_days(time(NULL));
        if(device_status.time_init_synced && (today != energy_statistics.today_usage.day)) {
            scb_event_ctx.event = SCB_EVENT_POWER_USAGE_DAILY_SAVE;
            xQueueSendFromISR(xDeviceQueue, &scb_event_ctx, NULL);
        }
    }

    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
}

static void device_main_timer_start() {
    timer_config_t config = {
            .alarm_en = TIMER_ALARM_EN,
            .counter_en = TIMER_PAUSE,
            .intr_type = TIMER_INTR_LEVEL,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = TIMER_AUTORELOAD_EN,
            .divider = 80   // 1 us per tick (80 MHz / 80)
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, device_main_timer_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 100000);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_start(TIMER_GROUP_0, TIMER_0);
}

static void device_main_timer_stop() {
    timer_pause(TIMER_GROUP_0, TIMER_0);
    timer_disable_intr(TIMER_GROUP_0, TIMER_0);
}

int device_send_event(scb_event_ctx_t scb_event_ctx) {
    if (!loop_started) {
        return ESP_FAIL;
    }
    if (!xDeviceQueue) {
        return ESP_FAIL;
    }

    BaseType_t ret;
    if (xPortInIsrContext() == pdTRUE) {
        ret = xQueueSendFromISR(xDeviceQueue, &scb_event_ctx, NULL);
    } else {
        ret = xQueueSend(xDeviceQueue, &scb_event_ctx, 0);
    }
    if (ret == pdTRUE) {
        return ESP_OK;
    }
    return ESP_FAIL;
}

static void update_display_power_data(power_data_t power_data) {
    OLED_ShowString(0, 0, "U:", 8, 1); // 显示电压标签
    OLED_ShowFloat(20, 0, power_data.voltage, 3, 8, 1); // 在(20,0)位置显示电压值
    OLED_ShowString(70, 0, "V", 8, 1); // 在(80,0)位置显示电压单位（伏特）

    OLED_ShowString(0, 12, "I:", 8, 1); // 显示电流标签
    OLED_ShowFloat(20, 12, power_data.current, 3, 8, 1); // 在(20,12)位置显示电流值
    OLED_ShowString(70, 12, "A", 8, 1); // 在(80,12)位置显示电流单位（安培）

    OLED_ShowString(0, 24, "P:", 8, 1); // 显示功率标签
    OLED_ShowFloat(20, 24, power_data.power, 3, 8, 1); // 在(20,24)位置显示功率值
    OLED_ShowString(70, 24, "W", 8, 1); // 在(80,24)位置显示功率单位（瓦特）

    OLED_ShowString(0, 36, "E:", 8, 1); // 显示能量消耗标签
    OLED_ShowFloat(20, 36, get_today_energy_usage(), 3, 8, 1); // 在(20,36)位置显示能量消耗值
    OLED_ShowString(70, 36, "Wh", 8, 1); // 在(80,36)位置显示能量消耗单位（瓦特时）
}

/**
 * 设备主循环
 * @param param
 */
static void device_loop_task(void *param) {
    xDeviceQueue = xQueueCreate( 15, sizeof(scb_event_ctx_t) );
    scb_event_ctx_t scb_event;

    device_main_timer_start();
    while (true) {
//        UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
//        ESP_LOGI("TASK", "剩余堆栈大小: %lu", stack_high_water_mark);
        if (xQueueReceive(xDeviceQueue, &scb_event, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (scb_event.event == SCB_EVENT_LOOP_STOP) {
            break;
        }

        switch (scb_event.event) {
            case SCB_EVENT_RESET_NETWORK:
                ESP_LOGI(TAG, "reset network...");
                vTaskDelay(1000);

                device_main_timer_stop();
                esp_wifi_restore();
                esp_restart();
                break;
            case SCB_EVENT_RESET_PLATFORM:
                ESP_LOGI(TAG, "reset platform...");
                vTaskDelay(1000);

                device_main_timer_stop();
                hap_reset_to_factory();
                break;
            case SCB_EVENT_RESET_TO_FACTORY:
                ESP_LOGI(TAG, "reset to factory...");
                vTaskDelay(1000);

                device_main_timer_stop();
                device_param_reset();
                esp_wifi_restore();
                hap_reset_to_factory();
                break;

            case SCB_EVENT_INTO_MAINTENANCE_MODE:
                start_web_server_task(device_info.name, "");
                web_server_auto_stop();
                break;

            case SCB_EVENT_POWER_DATA_READ:
                device.power_sensor->read_data(&device_status.power_data);
                update_today_energy_usage(device_status.power_data.power_consumption);

                if(power_protection_check(device_status.power_data.power)) {
                    switch_off();
                    hap_switch_status_update(false);
                }

                ntc_read_temperature(&device_status.temperature);
                temperature_protection(device_status.temperature);

                update_display_power_data(device_status.power_data);
                break;

            case SCB_EVENT_POWER_USAGE_DAILY_SAVE:
                save_energy_usage_of_day(device_status.power_data.power_consumption);
                break;

            case SCB_EVENT_TEMPERATURE_MEASUREMENT:
                break;

            case SCB_EVENT_TEMPERATURE_PROTECTION:
                switch_off();

                hap_device_active_update(false);
                hap_switch_status_update(false);

                led_start(BLINK_TEMP_PROTECTING);
                break;
            case SCB_EVENT_TEMPERATURE_PROTECTION_LIFT:
                hap_device_active_update(true);

                led_stop();
                break;

            case SCB_RTC_TIME_INIT_SYNCED:
                device_status.time_init_synced = true;
                ESP_LOGI(TAG, "time synced, timestamp = %d, day = %d", time(NULL), unix_timestamp_to_days(time(NULL)));

                today_energy_usage_calibration();
                break;

            case SCB_EVENT_OLED_FLUSH:
                OLED_ShowString(0, 48, "T:", 8, 1); // 显示温度标签
                OLED_ShowFloat(20, 48, device_status.temperature, 3, 8, 1); // 在(20,48)位置显示温度值
                OLED_ShowString(70, 48, "C", 8, 1); // 在(80,48)位置显示温度单位（摄氏度）
                OLED_Refresh();
                break;

            default:
                break;
        }
    }

    device_main_timer_stop();
    vQueueDelete(xDeviceQueue);
    xDeviceQueue = NULL;
    vTaskDelete(NULL);
}

int device_loop_start() {
    if (!loop_started) {
        loop_started = true;
        xTaskCreate(device_loop_task, "device_loop", (4 * 1024), NULL,
                    7, NULL);
    }
    return ESP_OK;
}


void app_main() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    wifi_init();
    // 配网后才启用其他功能
    bool provisioned = app_prov_is_provisioned();
    if(!provisioned) {
        led_start(BLINK_CONFIGURING);
        softap_provisioning_task(device_info.name, "");
        led_stop();
    }

    device.power_sensor = get_hlw8032_driver();
    device.power_sensor->init();
    device.power_sensor->start_reading();

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    device.power_sensor->read_data(&device_status.power_data);
    device_param_init(device_status.power_data.power_consumption);

    relay_init();
    button_init(GPIO_NUM_6);

    ntc_init();

    OLED_Init();
    OLED_ShowChinese(96, 0, 1, 32, 1);
    OLED_ShowChinese(96, 31, 2, 32, 1);

    device_loop_start();

    // 连接到wifi后才连接平台服务
    if(provisioned) {
        start_wifi_sta_until_got_ip();
    }

    system_time_sync_task_create();

    hap_switch_task_create();
}

