/**
 * @author kaiyin
 */

#ifndef IOT_SWITCH_DEVICE_H
#define IOT_SWITCH_DEVICE_H

#include <stdbool.h>
#include <stdint.h>
#include <esp_err.h>
#include "power_sensor.h"

#define DEVICE_NVS_PARTITION_NAME "device_data"

#define DEVICE_INIT_STATUS_CODE 85
#define DEVICE_CONFIG_NAMESPACE "dev_config_ns"
#define DEVICE_STATUS_NAMESPACE "dev_status_ns"

// 断电保护
#define POWER_CUTOFF_PROTECT 0

#define DEVICE_MODEL "KYS-001"
#define DEVICE_HARDWARE_VERSION "1.0.0"
#define DEVICE_SOFTWARE_VERSION "1.0.0"
#define DEVICE_NAME "smart_switch0397"

/**
 * 设备控制
 */
typedef enum {
    SCB_EVENT_LOOP_STOP = 1,
    SCB_EVENT_RESET_NETWORK,
    SCB_EVENT_RESET_PLATFORM,
    SCB_EVENT_RESET_TO_FACTORY,
    SCB_EVENT_INTO_MAINTENANCE_MODE,
    SCB_EVENT_POWER_DATA_READ,
    SCB_EVENT_POWER_USAGE_DAILY_SAVE,
    SCB_EVENT_TEMPERATURE_MEASUREMENT,
    SCB_EVENT_TEMPERATURE_PROTECTION,
    SCB_EVENT_TEMPERATURE_PROTECTION_LIFT,
    SCB_EVENT_POWER_OUTAGE,
    SCB_RTC_TIME_INIT_SYNCED,
    SCB_EVENT_OLED_FLUSH,
} scb_event_t;

typedef struct {
    scb_event_t event;
} scb_event_ctx_t;

// NVS键映射
typedef struct {
    const char* sw_control_mode;
    const char* sw_delay_time;
    const char* power_restore;
    const char* power_protection;
    const char* power_protection_threshold;
    const char* temperature_protection;
    const char* temperature_protection_threshold;
    const char* temperature_protection_lift_threshold;
} config_keys_t;

typedef struct {
    const char* init_start;
    const char* today_power_usage_day;
    const char* today_power_usage_index;
    const char* today_power_usage_consumption;
    const char* daily_on_duration;
    const char* daily_switch_count;
    const char* power_off_count;
    const char* reboot_count;
    const char* temperature_protection_count;
    const char* power_protection_count;
    const char* switch_state_on_power_off;
} status_keys_t;

typedef enum {
    TOGGLE_MODE = 1,
    DELAY_OFF_MODE,
} control_mode_t;

typedef struct {
    control_mode_t mode;    // 开关模式
    bool status;
    uint16_t delay_time;
} switch_control_t;

typedef struct {
    uint8_t power_restore;             // 来电恢复开关状态（恢复: true, 不恢复: false）
    uint8_t power_protection;          // 功率保护开关（启用: true, 禁用: false）
    uint16_t power_protection_threshold;
    uint8_t temperature_protection;    // 温度保护开关（启用: true, 禁用: false）
    uint8_t temperature_protection_threshold;
    uint8_t temperature_protection_lift_threshold;
    switch_control_t switch_control;
} device_config_t;

typedef struct {
    uint8_t init_start;
    bool time_init_synced;
    uint32_t daily_on_duration;       // 每天开启时长（单位：秒）
    uint16_t daily_switch_count;      // 当天开关次数
    uint16_t power_off_count;         // 断电次数
    uint16_t reboot_count;            // 设备重启次数
    uint16_t temperature_protection_count; // 温度保护次数
    uint16_t power_protection_count;       // 功率保护次数
    bool in_power_protection;
    bool in_temperature_protection;
    float temperature;
    uint8_t switch_state_on_power_off; // 断电时开关状态（断电前开启: true, 断电前关闭: false）
    power_data_t power_data;
    uint32_t runtime;
} device_status_t;

typedef struct {
    power_sensor_t *power_sensor;
} device_t;

typedef struct {
    const char* model;
    const char* hw_v;
    const char* fw_v;
    const char* name;
    const char* sn;
    const char* mft;
} device_info_t;

int device_send_event(scb_event_ctx_t scb_event_ctx);

extern device_config_t device_config;
extern device_status_t device_status;
extern device_t device;
extern device_info_t device_info;

extern const config_keys_t nvs_dev_conf_key;
extern const status_keys_t nvs_dev_state_key;

/**
 * 设备参数初始化
 * @return
 */
extern esp_err_t device_param_init(float sensor_value);

/**
 * 重置设备参数
 * @return
 */
extern esp_err_t device_param_reset();

/**
 * 保存参数
 * @param config
 * @return
 */
extern esp_err_t save_device_config_increment(device_config_t* config);


#endif //IOT_SWITCH_DEVICE_H
