/**
 * @author kaiyin
 */

#include <esp_log.h>
#include <time.h>
#include <esp_random.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "device.h"

#include "energy_statistics.h"
#include "system_time.h"

static const char *TAG = "energy_statistics";

energy_statistics_t energy_statistics = {
        {0, 0, 0, 0.0f, false},
        {{0, 0}},
};

/**
 * 保存电量统计状态
 * @param handle
 * @return
 */
static void save_power_usage_status(const nvs_handle_t *handle) {
    // 设备首次上电不保存
    if(energy_statistics.today_usage.day == 0) {
        return;
    }

    esp_err_t err = nvs_set_u16(*handle, nvs_dev_state_key.today_power_usage_day, energy_statistics.today_usage.day);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save today_power_usage_day, error: %d", err);
    }

    err = nvs_set_u16(*handle, nvs_dev_state_key.today_power_usage_index, energy_statistics.today_usage.current_storage_index);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save today_power_usage_index, error: %d", err);
    }
}

esp_err_t save_energy_usage_by_index(const uint32_t index, const uint16_t day, const uint16_t power_consumption) {
    nvs_handle_t handle;

    esp_err_t err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME, POWER_USAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to open NVS status namespace, error: %d", err);
        return err;
    }

    char key[15];
    snprintf(key, sizeof(key), "%s%u", POWER_USAGE_PREFIX, index);

    err = nvs_set_u32(handle, key, POWER_USAGE_ENCODE(day, power_consumption));
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error writing power data to NVS: %d", err);
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to commit write_power_usage_single, error: %d", err);
    }

    nvs_close(handle);

    energy_statistics.usage_record[index].day = day;
    energy_statistics.usage_record[index].consumption = power_consumption;

    return err;
}

esp_err_t save_energy_usage_of_day(const float power_consumption_sensor) {

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME, POWER_USAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS power_data_ns namespace, error: %d", err);
        ESP_LOGE(TAG, "Failed to save_power_usage_of_day");
        return err;
    }

    // 保存用电数据
    char key[15];
    snprintf(key, sizeof(key), "%s%u", POWER_USAGE_PREFIX, energy_statistics.today_usage.current_storage_index);
    err = nvs_set_u32(handle, key,
                      POWER_USAGE_ENCODE(energy_statistics.today_usage.day,
                                         energy_statistics.usage_record[energy_statistics.today_usage.current_storage_index].consumption));
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error save last day power usage to NVS: %d", err);
    }

    uint16_t today = unix_timestamp_to_days(time(NULL));
    if(today != energy_statistics.today_usage.day) {
        // 更新用电统计状态
        energy_statistics.today_usage.current_storage_index = (energy_statistics.today_usage.current_storage_index + 1) % POWER_USAGE_STORAGE_SIZE;
        energy_statistics.today_usage.consumption_init = 0;
        energy_statistics.today_usage.day = today;
        energy_statistics.today_usage.sensor_init_value = power_consumption_sensor;

        // 更新新的一天的用电统计数据
        energy_statistics.usage_record[energy_statistics.today_usage.current_storage_index].consumption = 0;
        energy_statistics.usage_record[energy_statistics.today_usage.current_storage_index].day = today;
#if !POWER_CUTOFF_PROTECT
        // 保存新的一天的用电统计数据
        snprintf(key, sizeof(key), "%s%u", POWER_USAGE_PREFIX, energy_statistics.today_usage.current_storage_index);
        err = nvs_set_u32(handle, key, POWER_USAGE_ENCODE(energy_statistics.today_usage.day, 0));
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error save new day power usage to NVS: %d", err);
        }
#endif
    }

#if !POWER_CUTOFF_PROTECT
    // 保存电量统计状态
    save_power_usage_status(&handle);
#endif

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to commit save_power_usage_of_day, error: %d", err);
    }

    nvs_close(handle);

    return ESP_OK;
}

void update_today_energy_usage(const float power_consumption_sensor) {
    // todo 需要处理传感器测量溢出场景
    /**
     * 未重启：= init_consumption(0) + power_consumption_sensor - sensor_init_value
     * 非断电重启：= init_consumption(0) + power_consumption_sensor - sensor_init_value
     * 断电重启：= init_consumption(非0) + power_consumption_sensor - sensor_init_value
     */
     if(!energy_statistics.today_usage.calibrated) {
         return;
     }

    energy_statistics.usage_record[energy_statistics.today_usage.current_storage_index].consumption =
            energy_statistics.today_usage.consumption_init
            + POWER_USAGE_CONSUMPTION_ENCODE(power_consumption_sensor - energy_statistics.today_usage.sensor_init_value);

//    ESP_LOGI(TAG, "today power usage = %f", POWER_USAGE_CONSUMPTION_DECODE(device_status.power_usage[device_status.today_power_usage.current_storage_index].power_consumption));
}

float get_today_energy_usage() {
//    ESP_LOGI(TAG, "get_today_energy_usage index = %d, day = %d, com = %d",
//             energy_statistics.today_usage.current_storage_index,
//             energy_statistics.usage_record[energy_statistics.today_usage.current_storage_index].day,
//             energy_statistics.usage_record[energy_statistics.today_usage.current_storage_index].consumption);
//    return POWER_USAGE_CONSUMPTION_DECODE(energy_statistics.usage_record[energy_statistics.today_usage.current_storage_index].consumption);
    int day = unix_timestamp_to_days(1730598285);
    for(int i=0; i< POWER_USAGE_STORAGE_SIZE; ++i) {
        if(day == energy_statistics.usage_record[i].day) {
            return POWER_USAGE_CONSUMPTION_DECODE(energy_statistics.usage_record[i].consumption);
        }
    }
    return 0;
}

float get_monthly_energy_usage() {
    uint32_t current_timestamp = time(NULL);
    struct tm *timeinfo;
    timeinfo = localtime((time_t *)&current_timestamp);

    uint16_t target_month = timeinfo->tm_mon + 1; // tm_mon 从0开始
    uint16_t target_year = timeinfo->tm_year + 1900;
    uint16_t current_day = timeinfo->tm_mday; // 当前日期

    float total_usage = 0.0f;

    for (int i = 0; i < POWER_USAGE_STORAGE_SIZE; ++i) {
        uint16_t record_day = energy_statistics.usage_record[i].day;

        // 计算记录对应的年份和月份
        time_t record_time = (record_day * 86400) + 0; // 将天数转换为秒
        struct tm *record_tm = gmtime(&record_time); // 使用 gmtime 确保时间计算正确

        // 检查记录是否为当月，并且日期不超过当前日期
        if (record_tm->tm_year + 1900 == target_year &&
            record_tm->tm_mon + 1 == target_month &&
            record_tm->tm_mday <= current_day) {
            total_usage += POWER_USAGE_CONSUMPTION_DECODE(energy_statistics.usage_record[i].consumption);
        }
    }

    return total_usage;
}

void today_energy_usage_calibration() {
    ESP_LOGI(TAG, "Staring today_energy_usage_calibration");

    energy_statistics.today_usage.consumption_init =
            POWER_USAGE_CONSUMPTION_DECODE(energy_statistics.usage_record[energy_statistics.today_usage.current_storage_index].consumption);

    uint16_t today = unix_timestamp_to_days(time(NULL));
    if(today == energy_statistics.today_usage.day) {
        ESP_LOGI(TAG, "in same day, today_energy_usage_calibration skipped");
        return;
    }

    // day=0时为设备初次上电，无需更新索引
    if(energy_statistics.today_usage.day != 0) {
        energy_statistics.today_usage.current_storage_index =
                (energy_statistics.today_usage.current_storage_index + 1) % POWER_USAGE_STORAGE_SIZE;
    }
    energy_statistics.today_usage.day = today;
    energy_statistics.today_usage.consumption_init = 0;

    ESP_LOGI(TAG, "device_status.today_power_usage.day                      = %d", energy_statistics.today_usage.day);
    ESP_LOGI(TAG, "device_status.today_power_usage.current_storage_index    = %d", energy_statistics.today_usage.current_storage_index);
    ESP_LOGI(TAG, "device_status.today_power_usage.consumption_init         = %d", energy_statistics.today_usage.consumption_init);

#if !POWER_CUTOFF_PROTECT
    ESP_LOGI(TAG, "save today_energy_usage_calibration result");
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME, POWER_USAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to open NVS power_data_ns namespace, error: %d", err);
        return;
    }
    save_power_usage_status(&handle);
    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to commit save_power_usage_status, error: %d", err);
    }

    nvs_close(handle);
#endif

    energy_statistics.today_usage.calibrated = true;
}

esp_err_t read_all_energy_usage(energy_statistics_t* status) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME, POWER_USAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to open NVS power data namespace, error: %d", err);
        return err;
    }

    err = nvs_get_u16(handle, nvs_dev_state_key.today_power_usage_day, &status->today_usage.day);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read today_power_usage_day, error: %d", err);
    }

    err = nvs_get_u16(handle, nvs_dev_state_key.today_power_usage_index, &status->today_usage.current_storage_index);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read today_power_usage_index, error: %d", err);
    }

    for(uint16_t i = 0; i < POWER_USAGE_STORAGE_SIZE; ++i) {
        char key[32];
        snprintf(key, sizeof(key), "%s%u", POWER_USAGE_PREFIX, i);

        uint32_t combined_data;
        err = nvs_get_u32(handle, key, &combined_data);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error read power data from NVS: %D", err);
        } else {
            status->usage_record[i].day = POWER_USAGE_DECODE_DAY(combined_data);
            status->usage_record[i].consumption = POWER_USAGE_DECODE_DATA(combined_data);
        }
    }
    nvs_close(handle);
    return ESP_OK;
}

void query_energy_usage_in_range(uint16_t start_day, uint16_t end_day) {
    for(int i=0; i < POWER_USAGE_STORAGE_SIZE; ++i) {
        if(energy_statistics.usage_record[i].day >= start_day && energy_statistics.usage_record[i].day <= end_day) {
            ESP_LOGI(TAG, "day = %d, power_consumption = %d", energy_statistics.usage_record[i].day, energy_statistics.usage_record[i].consumption);
        }
    }
}

esp_err_t energy_usage_storage_init() {
    nvs_handle_t handle;

    esp_err_t err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME, POWER_USAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to open NVS power_usage namespace, error: %d", err);
        return err;
    }

    // 电量统计数据
    uint32_t u_time = 1704081600;

    for(uint16_t i = 0; i < POWER_USAGE_STORAGE_SIZE; ++i) {
        char key[32];
        snprintf(key, sizeof(key), "%s%u", POWER_USAGE_PREFIX, i);

        u_time+=86400;
        uint16_t random_number = (esp_random() % 901) + 100;
        int day = unix_timestamp_to_days(u_time);
        err = nvs_set_u32(handle, key, POWER_USAGE_ENCODE(day, random_number));
        energy_statistics.usage_record[i].day = day;
        energy_statistics.usage_record[i].consumption = random_number;
        ESP_LOGI(TAG, "saved power data = %d, %d", day, random_number);

//        err = nvs_set_u32(handle, key, 0);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error writing init power_usage to NVS, index = %d, err = %d", i, err);
        }
    }

    // 电量统计状态
    save_power_usage_status(&handle);

    err = nvs_commit(handle);
    if(err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to commit init power_usage, error: %d", err);
    }

    nvs_close(handle);

    return err;
}