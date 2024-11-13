
#ifndef IOT_SWITCH_HLW_H
#define IOT_SWITCH_HLW_H

#include "stdint.h"

#define HLW_OK 0
#define HLW_ERR_INVALID_DATA 1
#define HLW_ERR_INVALID_CHECK_SUM 2
#define HLW_ERR_CORRECTION_FAILURE 3

// 电压系数，根据所采用的分压电阻大小来确定
#define VOLTAGE_COEFFICIENT 1.88F
#define CURRENT_COEFFICIENT 1.0F

#include "power_sensor.h"

power_sensor_t* get_hlw8032_driver(void);

#endif //IOT_SWITCH_HLW_H
