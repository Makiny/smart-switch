/**
 * @author kaiyin
 */

#ifndef IOT_SWITCH_POWER_SENSOR_H
#define IOT_SWITCH_POWER_SENSOR_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERROR
} sensor_status_t;

typedef struct {
    float voltage;
    float current;
    float power;
    float power_consumption;
} power_data_t;

typedef struct {
    sensor_status_t (*init)(void);
    sensor_status_t (*read_data)(power_data_t *data);
    void (*deinit)(void);
    void (*start_reading)(void);
} power_sensor_t;

#endif //IOT_SWITCH_POWER_SENSOR_H
