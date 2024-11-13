/**
 * @author kaiyin
 */

#ifndef IOT_SWITCH_LED_H
#define IOT_SWITCH_LED_H

/**
 * @brief Define blinking type and priority.
 *
 */
enum {
    BLINK_NET_CONNECTING = 0,
    BLINK_TEMP_PROTECTING,
    BLINK_POWER_PROTECTING,
    BLINK_CONFIGURING,
    BLINK_SINGLE,
    BLINK_DOUBLE,
    BLINK_TRIPLE,
    BLINK_MAX,
};

void led_start(int blink_type);
void led_stop();

#endif //IOT_SWITCH_LED_H
