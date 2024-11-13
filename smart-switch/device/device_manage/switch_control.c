/**
 * @author kaiyin
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_log.h>
#include "relay.h"
#include "device.h"
#include "ha_switch.h"
#include "switch_control.h"

static TimerHandle_t delay_timer = NULL;


static TimerHandle_t action_timer = NULL;
static int action_count = 0;
static const int ACTION_THRESHOLD = 6;
static const int TIME_WINDOW = 1500;

static void action_timer_callback(TimerHandle_t xTimer) {
    action_count = 0;
}

static void switch_action_check() {
    if (action_timer == NULL) {
        action_timer = xTimerCreate(
                "action_timer",
                pdMS_TO_TICKS(TIME_WINDOW),
                pdFALSE,
                0,
                action_timer_callback
        );
    }

    ++action_count;

    if (action_count >= ACTION_THRESHOLD) {
        xTimerStop(action_timer, 0);

        scb_event_ctx_t scb_event_ctx ;
        scb_event_ctx.event = SCB_EVENT_INTO_MAINTENANCE_MODE;
        device_send_event(scb_event_ctx);

        action_count = 0;
    } else {
        if (xTimerIsTimerActive(action_timer) == pdTRUE) {
            xTimerReset(action_timer, 0);
        } else {
            xTimerStart(action_timer, 0);
        }
    }
}

void switch_off() {
    device_config.switch_control.status = false;
    set_relay(false);
}

static void switch_action() {
    device_config.switch_control.status ? set_relay(true) : set_relay(false);

    // 同步开关状态
    hap_switch_status_update(device_config.switch_control.status);
}

static esp_err_t switch_status_set(const bool value) {
    if(!value) {
        device_config.switch_control.status = false;
    } else if(device_config.temperature_protection && device_status.in_temperature_protection) {
        return ESP_FAIL;
    } else {
        device_config.switch_control.status = true;
    }

    switch_action();

    return ESP_OK;
}

static void delay_off_timer_callback(TimerHandle_t xTimer) {
    switch_status_set(false);
}

esp_err_t switch_status_update(const bool value) {
    static uint16_t last_delay_time = -1;
    esp_err_t err;

    switch (device_config.switch_control.mode) {
        case TOGGLE_MODE:
            err = switch_status_set(value);
            break;
        case DELAY_OFF_MODE:
            err = switch_status_set(value);
            if (err == ESP_OK) {
                if (delay_timer == NULL) {
                    delay_timer = xTimerCreate(
                            "switch_timer",
                            pdMS_TO_TICKS((device_config.switch_control.delay_time * 1000)),
                            pdFALSE,
                            0,
                            delay_off_timer_callback
                    );
                } else if(device_config.switch_control.delay_time != last_delay_time) {
                    last_delay_time = device_config.switch_control.delay_time;
                    xTimerChangePeriod(delay_timer, pdMS_TO_TICKS((device_config.switch_control.delay_time * 1000)), 0);
                }
                xTimerStart(delay_timer, 0);
            }
            break;
        default:
            err = switch_status_set(value);
    }

    if (err == ESP_OK) {
        switch_action_check();
    }

    return err;
}