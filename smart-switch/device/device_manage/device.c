/**
 * @author kaiyin
 * @date 2024/8/10
 */

#include "device.h"

#include <string.h>
#include <esp_log.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "energy_statistics.h"
#include "switch_control.h"

static const char *TAG = "device";

// 定义键名
const config_keys_t nvs_dev_conf_key = {
        .sw_control_mode = "ctrl_mode",
        .sw_delay_time = "sw_d_time",
        .power_restore = "pwr_rst",
        .power_protection = "pwr_prot",
        .power_protection_threshold = "pwr_thr",
        .temperature_protection = "temp_prot",
        .temperature_protection_threshold = "temp_thr",
        .temperature_protection_lift_threshold = "temp_lift_thr"
};

const status_keys_t nvs_dev_state_key = {
        .init_start = "init_start",
        .today_power_usage_day = "tpu_day",
        .today_power_usage_index = "tpu_index",
        .today_power_usage_consumption = "tpu_con",
        .daily_on_duration = "daily_dur",
        .daily_switch_count = "sw_count",
        .power_off_count = "pwr_off",
        .reboot_count = "reboot",
        .temperature_protection_count = "temp_count",
        .power_protection_count = "pwr_count",
        .switch_state_on_power_off = "sw_state"
};

device_config_t device_config = {
    .power_restore = 0,
    .power_protection = 1,
    .power_protection_threshold = 2300,
    .temperature_protection = 1,
    .temperature_protection_threshold = 70,
    .temperature_protection_lift_threshold = 50,
    .switch_control.delay_time = 5,
    .switch_control.mode = TOGGLE_MODE,
    .switch_control.status = false,
};

device_status_t device_status = {
        .init_start = 0,
        .time_init_synced = false,
        .daily_on_duration = 0,
        .daily_switch_count = 0,
        .power_off_count = 0,
        .reboot_count = 0,
        .temperature_protection_count = 0,
        .power_protection_count = 0,
        .in_power_protection = false,
        .in_temperature_protection = false,
        .temperature = 0.0f,
        .switch_state_on_power_off = 0,
        .power_data = {0.f, 0.f, 0.f, 0.f},
        .runtime = 0
};

device_t device = {
        .power_sensor = NULL,
};

device_info_t device_info = {
        .model = DEVICE_MODEL,
        .hw_v = DEVICE_HARDWARE_VERSION,
        .fw_v = DEVICE_SOFTWARE_VERSION,
        .name = DEVICE_NAME,
        .sn = "10972678583",
        .mft = "kaiyin",
};

/**
 * 读取设备配置
 * @param config
 * @return
 */
static esp_err_t read_device_config(device_config_t* config) {
    esp_err_t err;
    nvs_handle_t handle;

    err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME,DEVICE_CONFIG_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to open NVS device_config namespace, error: %d", err);
        return err;
    }

    // 读取开关模式
    err = nvs_get_i8(handle, nvs_dev_conf_key.sw_control_mode, (int8_t*)(&config->switch_control.mode));
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read control_mode, error: %d", err);
    }

    err = nvs_get_u16(handle, nvs_dev_conf_key.sw_delay_time, (uint16_t*)(&config->switch_control.delay_time));
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read control_mode, error: %d", err);
    }

    err = nvs_get_u8(handle, nvs_dev_conf_key.power_restore, &config->power_restore);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read power_restore, error: %d", err);
    }

    err = nvs_get_u8(handle, nvs_dev_conf_key.power_protection, &config->power_protection);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read power_protection, error: %d", err);
    }

    err = nvs_get_u16(handle, nvs_dev_conf_key.power_protection_threshold, &config->power_protection_threshold);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read power_protection_threshold, error: %d", err);
    }

    err = nvs_get_u8(handle, nvs_dev_conf_key.temperature_protection, &config->temperature_protection);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read temperature_protection, error: %d", err);
    }

    err = nvs_get_u8(handle, nvs_dev_conf_key.temperature_protection_threshold, &config->temperature_protection_threshold);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read temperature_protection_threshold, error: %d", err);
    }

    nvs_close(handle);

    return err;
}

/**
 * 保存设备配置
 * @param config
 * @return
 */
static esp_err_t save_device_config(device_config_t* config) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME,DEVICE_CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to open NVS config namespace, error: %d", err);
        return err;
    }

    err = nvs_set_i8(handle, nvs_dev_conf_key.sw_control_mode, config->switch_control.mode);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save control_mode, error: %d", err);
    }
    err = nvs_set_u16(handle, nvs_dev_conf_key.sw_delay_time, config->switch_control.delay_time);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save control_mode, error: %d", err);
    }

    err = nvs_set_u8(handle, nvs_dev_conf_key.power_restore, config->power_restore);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save power_restore, error: %d", err);
    }

    err = nvs_set_u8(handle, nvs_dev_conf_key.power_protection, config->power_protection);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save power_protection, error: %d", err);
    }

    err = nvs_set_u16(handle, nvs_dev_conf_key.power_protection_threshold, config->power_protection_threshold);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save power_protection_threshold, error: %d", err);
    }

    err = nvs_set_u8(handle, nvs_dev_conf_key.temperature_protection, config->temperature_protection);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save temperature_protection, error: %d", err);
    }

    err = nvs_set_u8(handle, nvs_dev_conf_key.temperature_protection_threshold, config->temperature_protection_threshold);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save temperature_protection_threshold, error: %d", err);
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to commit device_config, error: %d", err);
    }

    nvs_close(handle);

    return ESP_OK;
}

esp_err_t save_device_config_increment(device_config_t* config) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME,DEVICE_CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to open NVS config namespace, error: %d", err);
        return err;
    }

    if(config->switch_control.mode != device_config.switch_control.mode) {
        ESP_LOGI(TAG, "save switch_control.mode = %d", config->switch_control.mode);
        device_config.switch_control.mode = config->switch_control.mode;
        err = nvs_set_i8(handle, nvs_dev_conf_key.sw_control_mode, config->switch_control.mode);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to save control_mode, error: %d", err);
        }
    }

    if(config->switch_control.delay_time != device_config.switch_control.delay_time) {
        ESP_LOGI(TAG, "save switch_control.delay_time = %d", config->switch_control.delay_time);
        device_config.switch_control.delay_time = config->switch_control.delay_time;
        err = nvs_set_u16(handle, nvs_dev_conf_key.sw_delay_time, config->switch_control.delay_time);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to save control_mode, error: %d", err);
        }
    }

    if(config->power_restore != device_config.power_restore) {
        ESP_LOGI(TAG, "save power_restore = %d", config->power_restore);
        device_config.power_restore = config->power_restore;
        err = nvs_set_u8(handle, nvs_dev_conf_key.power_restore, config->power_restore);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to save power_restore, error: %d", err);
        }
    }

    if(config->power_protection != device_config.power_protection) {
        ESP_LOGI(TAG, "save power_protection = %d", config->power_protection);
        device_config.power_protection = config->power_protection;
        err = nvs_set_u8(handle, nvs_dev_conf_key.power_protection, config->power_protection);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to save power_protection, error: %d", err);
        }
    }

    if(config->power_protection_threshold != device_config.power_protection_threshold) {
        ESP_LOGI(TAG, "save power_protection_threshold = %d", config->power_protection_threshold);
        device_config.power_protection_threshold = config->power_protection_threshold;
        err = nvs_set_u16(handle, nvs_dev_conf_key.power_protection_threshold, config->power_protection_threshold);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to save power_protection_threshold, error: %d", err);
        }
    }

    if(config->temperature_protection != device_config.temperature_protection) {
        ESP_LOGI(TAG, "save temperature_protection = %d", config->temperature_protection);
        device_config.temperature_protection = config->temperature_protection;
        err = nvs_set_u8(handle, nvs_dev_conf_key.temperature_protection, config->temperature_protection);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to save temperature_protection, error: %d", err);
        }
    }

    if(config->temperature_protection_threshold != device_config.temperature_protection_threshold) {
        ESP_LOGI(TAG, "save temperature_protection_threshold = %d", config->temperature_protection_threshold);
        device_config.temperature_protection_threshold = config->temperature_protection_threshold;
        err = nvs_set_u8(handle, nvs_dev_conf_key.temperature_protection_threshold,
                         config->temperature_protection_threshold);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to save temperature_protection_threshold, error: %d", err);
        }
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to commit device_config, error: %d", err);
    }

    nvs_close(handle);

    return ESP_OK;
}

/**
 * 读取设备状态
 * @param status
 * @return
 */
static esp_err_t read_device_status(device_status_t* status) {
    nvs_handle_t handle;

    esp_err_t err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME,DEVICE_STATUS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_get_u8(handle, nvs_dev_state_key.init_start, &status->init_start);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read init_start, error: %d", err);
    }

    err = nvs_get_u32(handle, nvs_dev_state_key.daily_on_duration, &status->daily_on_duration);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read daily_switch_count, error: %d", err);
    }

    err = nvs_get_u16(handle, nvs_dev_state_key.daily_switch_count, &status->daily_switch_count);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read daily_switch_count, error: %d", err);
    }

    err = nvs_get_u16(handle, nvs_dev_state_key.power_off_count, &status->power_off_count);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read power_off_count, error: %d", err);
    }

    err = nvs_get_u16(handle, nvs_dev_state_key.reboot_count, &status->reboot_count);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read reboot_count, error: %d", err);
    }

    err = nvs_get_u16(handle, nvs_dev_state_key.temperature_protection_count, &status->temperature_protection_count);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read temperature_protection_count, error: %d", err);
    }

    err = nvs_get_u16(handle, nvs_dev_state_key.power_protection_count, &status->power_protection_count);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read power_protection_count, error: %d", err);
    }

    err = nvs_get_u8(handle, nvs_dev_state_key.switch_state_on_power_off, &status->switch_state_on_power_off);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to read switch_state_on_power_off, error: %d", err);
    }

    nvs_close(handle);

    return err;
}

/**
 * 保存设备状态
 * @param status
 * @return
 */
static esp_err_t save_device_status(device_status_t* status) {
    esp_err_t err;
    nvs_handle_t handle;

    err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME,DEVICE_STATUS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to open NVS status namespace, error: %d", err);
        return err;
    }

    err = nvs_set_u8(handle, nvs_dev_state_key.init_start, status->init_start);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save init_start, error: %d", err);
    }

    err = nvs_set_u16(handle, nvs_dev_state_key.daily_switch_count, status->daily_switch_count);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save daily_switch_count, error: %d", err);
    }

    err = nvs_set_u32(handle, nvs_dev_state_key.daily_on_duration, status->daily_on_duration);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save daily_on_duration, error: %d", err);
    }

    err = nvs_set_u16(handle, nvs_dev_state_key.power_off_count, status->power_off_count);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save power_off_count, error: %d", err);
    }

    err = nvs_set_u16(handle, nvs_dev_state_key.reboot_count, status->reboot_count);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save reboot_count, error: %d", err);
    }

    err = nvs_set_u16(handle, nvs_dev_state_key.temperature_protection_count, status->temperature_protection_count);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save temperature_protection_count, error: %d", err);
    }

    err = nvs_set_u16(handle, nvs_dev_state_key.power_protection_count, status->power_protection_count);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save power_protection_count, error: %d", err);
    }

    err = nvs_set_u8(handle, nvs_dev_state_key.switch_state_on_power_off, status->switch_state_on_power_off);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to save switch_state_on_power_off, error: %d", err);
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to commit device_status, error: %d", err);
    }

    nvs_close(handle);

    return err;
}

esp_err_t read_u8_on_readwrite_mode(const char* namespace_name, const char* key, uint8_t* data) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME, namespace_name, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_get_u8(handle, key, data);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to read u8, error: %d", err);
        }
        nvs_close(handle);
    }
    return err;
}

esp_err_t save_u8(const char* namespace_name, const char* key, uint8_t data) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(DEVICE_NVS_PARTITION_NAME,namespace_name, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_set_u8(handle, key, data);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to save u8, error: %d", err);
        }
        err = nvs_commit(handle);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to commit save_u8, error: %d", err);
        }
        nvs_close(handle);
    }
    return err;
}

esp_err_t device_param_init(const float sensor_value) {
    energy_statistics.today_usage.sensor_init_value = sensor_value;

    esp_err_t err = nvs_flash_init_partition(DEVICE_NVS_PARTITION_NAME);
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase_partition(DEVICE_NVS_PARTITION_NAME));
        nvs_flash_init_partition(DEVICE_NVS_PARTITION_NAME);
    }

    read_u8_on_readwrite_mode(DEVICE_STATUS_NAMESPACE, nvs_dev_state_key.init_start, &device_status.init_start);
    if(DEVICE_INIT_STATUS_CODE != device_status.init_start) {
        ESP_LOGI(TAG, "device param not init, read init_start = %d , target init_start = %d", DEVICE_INIT_STATUS_CODE, device_status.init_start);
        device_status.init_start = DEVICE_INIT_STATUS_CODE;
        save_device_status(&device_status);
        save_device_config(&device_config);
        energy_usage_storage_init();
    } else {
        ESP_LOGI(TAG, "device param has init");
        read_device_status(&device_status);
        read_device_config(&device_config);
        read_all_energy_usage(&energy_statistics);

//        for(int i=0; i < POWER_USAGE_STORAGE_SIZE; ++i) {
//            ESP_LOGI(TAG, "power data day = %d, consumption = %d", energy_statistics.usage_record[i].day, energy_statistics.usage_record[i].consumption);
//        }
    }

    ESP_LOGI(TAG, "device_status.init_start                                     = %d", device_status.init_start);
    ESP_LOGI(TAG, "device_status.time_init_synced                               = %d", device_status.time_init_synced);
    ESP_LOGI(TAG, "energy_statistics.today_power_usage.day                      = %d", energy_statistics.today_usage.day);
    ESP_LOGI(TAG, "energy_statistics.today_power_usage.current_storage_index    = %d", energy_statistics.today_usage.current_storage_index);
    ESP_LOGI(TAG, "energy_statistics.today_power_usage.sensor_init_value        = %f", energy_statistics.today_usage.sensor_init_value);
    ESP_LOGI(TAG, "device_status.daily_on_duration                              = %d", device_status.daily_on_duration);
    ESP_LOGI(TAG, "device_status.daily_switch_count                             = %d", device_status.daily_switch_count);
    ESP_LOGI(TAG, "device_status.power_off_count                                = %d", device_status.power_off_count);
    ESP_LOGI(TAG, "device_status.reboot_count                                   = %d", device_status.reboot_count);
    ESP_LOGI(TAG, "device_status.temperature_protection_count                   = %d", device_status.temperature_protection_count);
    ESP_LOGI(TAG, "device_status.power_protection_count                         = %d", device_status.power_protection_count);
    ESP_LOGI(TAG, "device_config.switch_state_on_power_off                      = %d", device_status.switch_state_on_power_off);


    ESP_LOGI(TAG, "device_config.power_restore                     = %d", device_config.power_restore);
    ESP_LOGI(TAG, "device_config.power_protection                  = %d", device_config.power_protection);
    ESP_LOGI(TAG, "device_config.power_protection_threshold        = %d", device_config.power_protection_threshold);
    ESP_LOGI(TAG, "device_config.temperature_protection            = %d", device_config.temperature_protection);
    ESP_LOGI(TAG, "device_config.temperature_protection_threshold  = %d", device_config.temperature_protection_threshold);
    ESP_LOGI(TAG, "switch_control.mode                             = %d", device_config.switch_control.mode);
    ESP_LOGI(TAG, "switch_control.delay_time                       = %d", device_config.switch_control.delay_time);



    return ESP_OK;
}

esp_err_t device_param_reset() {
    uint8_t init_start = 0;
    read_u8_on_readwrite_mode(DEVICE_STATUS_NAMESPACE, nvs_dev_state_key.init_start, &init_start);
    if(init_start != DEVICE_INIT_STATUS_CODE) {
        return ESP_OK;
    }
    esp_err_t err = save_u8(DEVICE_STATUS_NAMESPACE, nvs_dev_state_key.init_start, 0);
    if(err != ESP_OK) {
        ESP_LOGI(TAG, "device param reset failed");
    }
    return err;
}