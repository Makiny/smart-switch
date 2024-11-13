/**
 * @author kaiyin
 */

#ifndef IOT_SWITCH_SWITCH_CONTROL_H
#define IOT_SWITCH_SWITCH_CONTROL_H
#include <stdbool.h>

void switch_off();

esp_err_t switch_status_update(bool value);

#endif //IOT_SWITCH_SWITCH_CONTROL_H
