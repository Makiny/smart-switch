/**
 * @author kaiyin
 */

#include <esp_log.h>
#include "device.h"

#define WINDOW_SIZE 5

static uint32_t power_window[WINDOW_SIZE];
static uint64_t window_sum = 0;
static uint8_t sample_index = 0;

bool power_protection_check(const float curr_power) {

    window_sum -= power_window[sample_index];
    window_sum += (uint64_t)curr_power;
    power_window[sample_index] = (uint64_t)curr_power;
    sample_index = (sample_index + 1) % WINDOW_SIZE;

    uint32_t average_power = window_sum / WINDOW_SIZE;

    if (device_config.power_protection && average_power > device_config.power_protection_threshold) {
//    if (average_power > 30) {
       return true;
    }

    return false;
}