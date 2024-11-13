/**
 * @author kaiyin
 */

#ifndef IOT_SWITCH_SYSTEM_TIME_H
#define IOT_SWITCH_SYSTEM_TIME_H

#define TIMESTAMP_BASE_LINE 1704038400

void system_time_sync_task_create();
uint16_t unix_timestamp_to_days(uint32_t timestamp);
#endif //IOT_SWITCH_SYSTEM_TIME_H
