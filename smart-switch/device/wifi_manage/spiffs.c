/**
 * @author kaiyin
 * @date 2024/7/24
 */

#include "esp_log.h"
#include "esp_spiffs.h"


static const char *TAG = "spiffs";

void mount_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount or format filesystem");
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

void unmount_spiffs(void)
{
    // 卸载文件系统
    esp_err_t ret = esp_vfs_spiffs_unregister(NULL);

    // 检查卸载是否成功
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SPIFFS (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS unmounted successfully");
    }
}