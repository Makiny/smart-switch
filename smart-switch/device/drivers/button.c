/**
 * @author kaiyin
 */

#include <iot_button.h>
#include <hap_platform_os.h>
#include <esp_log.h>

#include "button.h"
#include "led.h"
#include "device.h"
#include "switch_control.h"

static const char *TAG = "button";

static int64_t press_start_time = 0;

static void key_pressed_3s_cb(void* arg) {
    press_start_time = esp_timer_get_time();
    led_start(BLINK_SINGLE);
}

static void key_pressed_6s_cb(void* arg) {
    led_start(BLINK_DOUBLE);
}

static void key_pressed_9s_cb(void* arg) {
    led_start(BLINK_TRIPLE);
}

static void key_release_cb(void* arg) {
    int64_t press_duration = (esp_timer_get_time() - press_start_time) / 1000;

    if (press_duration >= 0 && press_duration < 3000) {
        // 3s
        scb_event_ctx_t scb_event_ctx ;
        scb_event_ctx.event = SCB_EVENT_INTO_MAINTENANCE_MODE;
        device_send_event(scb_event_ctx);
    } else if (press_duration >= 3000 && press_duration < 6000) {
        // 6s
        scb_event_ctx_t scb_event_ctx ;
        scb_event_ctx.event = SCB_EVENT_RESET_NETWORK;
        device_send_event(scb_event_ctx);
    } else if (press_duration >= 6000) {
        // 9s
        scb_event_ctx_t scb_event_ctx ;
        scb_event_ctx.event = SCB_EVENT_RESET_TO_FACTORY;
        device_send_event(scb_event_ctx);
    }
}

static void key_tap(void* arg) {
    switch_status_update(!device_config.switch_control.status);
}

void button_init(uint32_t key_gpio_pin) {
    button_handle_t handle = iot_button_create(key_gpio_pin, BUTTON_ACTIVE_LOW);

    iot_button_add_on_press_cb(handle, 3, key_pressed_3s_cb, NULL);
    iot_button_add_on_press_cb(handle, 6, key_pressed_6s_cb, NULL);
    iot_button_add_on_press_cb(handle, 9, key_pressed_9s_cb, NULL);

    iot_button_add_on_release_cb(handle, 3, key_release_cb, NULL);

    iot_button_set_evt_cb(handle, BUTTON_CB_TAP, key_tap, NULL);
}

