/**
 * @author kaiyin
 */

#ifndef IOT_SWITCH_HA_SWITCH_H
#define IOT_SWITCH_HA_SWITCH_H

void hap_switch_status_update(bool is_on);
void hap_device_active_update(bool is_active);

void hap_switch_task_create();

#endif //IOT_SWITCH_HA_SWITCH_H
