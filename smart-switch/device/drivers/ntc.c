/**
 * @author kaiyin
 */
#include "freertos/FreeRTOS.h"
#include <freertos/timers.h>
#include <esp_log.h>
#include "ntc.h"

// 设置ADC
void ntc_init() {
    // 配置ADC宽度
    adc1_config_width(ADC_WIDTH_BIT_DEFAULT);
    // 配置ADC通道，IO3对应ADC1_CHANNEL_0
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);
}

// 读取温度
void ntc_read_temperature(float *temperature_c) {
// 读取 ADC 值
    int adcValue = adc1_get_raw(ADC1_CHANNEL_3);

// 计算电压
    float voltage = adcValue * (VCC / 4095.0f);

// 计算 NTC 电阻值
    float ntcResistance = voltage * SERIES_RESISTOR /(VCC - voltage);

// 计算温度
    float temperatureK = (BETA * ROOM_TEMP) / (BETA + ROOM_TEMP * log(ntcResistance / NOMINAL_RESISTANCE));
    temperatureK =  temperatureK - 273.15f;
    *temperature_c = temperatureK;
}