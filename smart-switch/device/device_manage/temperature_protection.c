/**
 * @author kaiyin
 */

#include <esp_log.h>
#include "device.h"

#define WINDOW_SIZE 5

static uint8_t temp_window[WINDOW_SIZE];
static uint16_t window_sum = 0;
static uint8_t sample_index = 0;

void temperature_protection(const float curr_temp) {

    window_sum -= temp_window[sample_index];
    window_sum += (uint8_t)curr_temp;
    temp_window[sample_index] = (uint8_t)curr_temp;
    sample_index = (sample_index + 1) % WINDOW_SIZE;

    uint8_t average_temp = window_sum / WINDOW_SIZE;

    if(!device_config.temperature_protection) {
        return;
    }

    if (!device_status.in_temperature_protection && average_temp > device_config.temperature_protection_threshold) {
//    if (!device_status.in_temperature_protection && average_temp > 35) {
        device_status.in_temperature_protection = true;

        scb_event_ctx_t scb_event_ctx;
        scb_event_ctx.event = SCB_EVENT_TEMPERATURE_PROTECTION;
        device_send_event(scb_event_ctx);
    } else if(device_status.in_temperature_protection && average_temp < device_config.temperature_protection_lift_threshold) {
//    } else if(device_status.in_temperature_protection && average_temp < 30) {
        device_status.in_temperature_protection = false;

        scb_event_ctx_t scb_event_ctx;
        scb_event_ctx.event = SCB_EVENT_TEMPERATURE_PROTECTION_LIFT;
        device_send_event(scb_event_ctx);
    }
}