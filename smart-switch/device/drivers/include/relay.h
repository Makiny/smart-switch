/**
 * @author kaiyin
 */

#ifndef IOT_SWITCH_RELAY_H
#define IOT_SWITCH_RELAY_H

#include "driver/gpio.h"

// 定义继电器控制引脚
#define RELAY_PIN GPIO_NUM_5

// 定义高电平和低电平
#define RELAY_ON_LEVEL 1
#define RELAY_OFF_LEVEL 0

/**
 * @brief 初始化继电器控制引脚
 */
void relay_init(void);

int set_relay(bool value);

#endif //IOT_SWITCH_RELAY_H
