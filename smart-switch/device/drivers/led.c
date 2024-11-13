/**
 * @author kaiyin
 */

#include <esp_log.h>
#include "led_indicator.h"
#include "led.h"

static const char * TAG = "led_indicate";

static const blink_step_t ordinary_blink[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 200},
        {LED_BLINK_HOLD, LED_STATE_OFF, 200},
        {LED_BLINK_HOLD, LED_STATE_ON, 200},
        {LED_BLINK_HOLD, LED_STATE_OFF, 200},
        {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t single_blink_cycle[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 200},
        {LED_BLINK_HOLD, LED_STATE_OFF, 1000},
        {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t double_blink_cycle[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 100},
        {LED_BLINK_HOLD, LED_STATE_OFF, 100},
        {LED_BLINK_HOLD, LED_STATE_ON, 100},
        {LED_BLINK_HOLD, LED_STATE_OFF, 1000},
        {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t breath_blink[] = {
        {LED_BLINK_HOLD, LED_STATE_OFF, 0},
        {LED_BLINK_BREATHE, LED_STATE_ON, 1000},
        {LED_BLINK_BREATHE, LED_STATE_OFF, 1000},
        {LED_BLINK_LOOP, 0, 0},
};

static const blink_step_t single_blink[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 200},
        {LED_BLINK_HOLD, LED_STATE_OFF, 100},
        {LED_BLINK_STOP, 0, 0},
};

static const blink_step_t double_blink[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 100},
        {LED_BLINK_HOLD, LED_STATE_OFF, 100},
        {LED_BLINK_HOLD, LED_STATE_ON, 100},
        {LED_BLINK_HOLD, LED_STATE_OFF, 100},
        {LED_BLINK_STOP, 0, 0},
};

static const blink_step_t triple_blink[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 100},
        {LED_BLINK_HOLD, LED_STATE_OFF, 100},
        {LED_BLINK_HOLD, LED_STATE_ON, 100},
        {LED_BLINK_HOLD, LED_STATE_OFF, 100},
        {LED_BLINK_HOLD, LED_STATE_ON, 100},
        {LED_BLINK_HOLD, LED_STATE_OFF, 100},
        {LED_BLINK_STOP, 0, 0},
};

blink_step_t const *led_mode[] = {
        [BLINK_NET_CONNECTING] = ordinary_blink,
        [BLINK_TEMP_PROTECTING] = single_blink_cycle,
        [BLINK_POWER_PROTECTING] = double_blink_cycle,
        [BLINK_CONFIGURING] = breath_blink,
        [BLINK_SINGLE] = single_blink,
        [BLINK_DOUBLE] = double_blink,
        [BLINK_TRIPLE] = triple_blink,
        [BLINK_MAX] = NULL,
};

static led_indicator_handle_t led_handle = NULL;
static int led_current_blink_type = -1;


void led_init() {
    led_indicator_ledc_config_t ledc_config = {
            .is_active_level_high = false,
            .timer_inited = false,
            .timer_num = LEDC_TIMER_0,
            .gpio_num = 7,
            .channel = LEDC_CHANNEL_0,
    };

    const led_indicator_config_t config = {
            .mode = LED_LEDC_MODE,
            .led_indicator_ledc_config = &ledc_config,
            .blink_lists = led_mode,
            .blink_list_num = BLINK_MAX,
    };

    led_handle = led_indicator_create(&config);
}

void led_start(int blink_type) {

// todo LED闪烁 优先级设置 保护>其他状态

    if(NULL == led_handle) {
        led_init();
    } else if(blink_type == led_current_blink_type) {
        return;
    } else if(-1 != led_current_blink_type) {
        led_indicator_stop(led_handle, led_current_blink_type);
    }
    led_indicator_set_on_off(led_handle, false);
    led_current_blink_type = blink_type;
    led_indicator_start(led_handle, led_current_blink_type);

    if(blink_type == BLINK_SINGLE || blink_type== BLINK_DOUBLE || blink_type == BLINK_TRIPLE) {
        led_current_blink_type = -1;
    }
}

void led_stop() {
    if(NULL == led_handle || led_current_blink_type == -1) {
        return;
    }
    led_indicator_stop(led_handle, led_current_blink_type);
    led_indicator_set_on_off(led_handle, false);
    led_indicator_delete(led_handle);

    led_handle = NULL;
}