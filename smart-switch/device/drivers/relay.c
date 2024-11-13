/**
 * @author kaiyin
 */
#include "relay.h"

void relay_init(void) {
    gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << RELAY_PIN),     // 配置控制引脚
            .mode = GPIO_MODE_OUTPUT,                // 设置为输出模式
            .pull_up_en = GPIO_PULLUP_DISABLE,       // 禁用上拉
            .pull_down_en = GPIO_PULLDOWN_DISABLE,   // 禁用下拉
            .intr_type = GPIO_INTR_DISABLE           // 禁用中断
    };
    gpio_config(&io_conf);
}

static void relay_closure(void) {
    gpio_set_level(RELAY_PIN, RELAY_ON_LEVEL);
}

static void relay_release(void) {
    gpio_set_level(RELAY_PIN, RELAY_OFF_LEVEL);
}

int set_relay(const bool value) {
    value ? relay_closure() : relay_release();
    return 0;
}