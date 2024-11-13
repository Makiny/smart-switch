/**
 * @author kaiyin
 */

#ifndef IOT_SWITCH_NTC_H
#define IOT_SWITCH_NTC_H

#include <math.h>
#include "driver/adc.h"

#define SERIES_RESISTOR 10000.0F
#define BETA 3540.0F
#define ROOM_TEMP 298.15F
#define NOMINAL_RESISTANCE 10000.0F
#define VCC 3.3F

void ntc_init();

void ntc_read_temperature(float *temperature_c);

#endif //IOT_SWITCH_NTC_H
