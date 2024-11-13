/**
 * @author kaiyin
 */

#ifndef IOT_SWITCH_ENERGY_STATISTICS_H
#define IOT_SWITCH_ENERGY_STATISTICS_H

#include <stdint.h>
#include <esp_err.h>

#define POWER_USAGE_NAMESPACE "power_data_ns"
#define POWER_USAGE_PREFIX "pwr_use_"
#define POWER_USAGE_STORAGE_SIZE 370

// 编码用电数据
#define POWER_USAGE_ENCODE(day, data) (((day) << 16) | (data))
// 获取用电数据-时间戳
#define POWER_USAGE_DECODE_DAY(encode_usage) ((uint16_t)((encode_usage) >> 16))
// 获取用电数据-用电量
#define POWER_USAGE_DECODE_DATA(encode_usage) ((uint16_t)((encode_usage) & 0xFFFF))

#define POWER_USAGE_CONSUMPTION_ENCODE(data) ((uint16_t)((data)*100.F))
#define POWER_USAGE_CONSUMPTION_DECODE(data) ((float)((data)*0.01F))

typedef struct {
    uint16_t day;         // 时间戳 (UNIX 时间转天)
    uint16_t current_storage_index;     // 存储位置索引
    uint16_t consumption_init;    // 当日初始用电量
    float sensor_init_value;   // 传感器初始测量值
    bool calibrated;
} today_energy_usage_t;

typedef struct {
    uint16_t day;         // 时间戳 (UNIX 时间转天)
    uint16_t consumption; // 用电量
} energy_usage_t;

typedef struct {
    today_energy_usage_t today_usage; // 用电量（单位：Wh）
    energy_usage_t usage_record[POWER_USAGE_STORAGE_SIZE];
} energy_statistics_t;

extern energy_statistics_t energy_statistics;

/**
 * 查询指定时间范围的用电数据
 * @param start_day
 * @param end_day
 */
void query_energy_usage_in_range(uint16_t start_day, uint16_t end_day);

/**
 * 按索引保存电量数据
 * @param index
 * @param day
 * @param power_consumption
 * @return
 */
esp_err_t save_energy_usage_by_index(uint32_t index, uint16_t day, uint16_t power_consumption);

/**
 * 读取所有电量数据
 * @param status
 * @return
 */
esp_err_t read_all_energy_usage(energy_statistics_t* status);

/**
 * 初始化电量存储区域
 * @return
 */
esp_err_t energy_usage_storage_init();

/**
 * 保存当天用电统计数据
 */
esp_err_t save_energy_usage_of_day(float power_consumption_sensor);

/**
 * 更新电量使用数据
 * @param power_consumption_sensor
 */
void update_today_energy_usage(float power_consumption_sensor);

/**
 * 获取当天用电量
 * @return
 */
float get_today_energy_usage();

/**
 * 获取当月用电量
 * @return
 */
float get_monthly_energy_usage();

/**
 * 电量统计状态校正
 */
void today_energy_usage_calibration();

#endif //IOT_SWITCH_ENERGY_STATISTICS_H
