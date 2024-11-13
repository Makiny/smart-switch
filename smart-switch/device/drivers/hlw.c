
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <hal/uart_types.h>
#include <driver/uart.h>
#include <hal/gpio_types.h>
#include <freertos/task.h>
#include <memory.h>
#include "hlw.h"
#include "oled.h"
#include "ntc.h"

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_10)

static const int RX_BUF_SIZE = 256;
static QueueHandle_t uart0_queue;

uint8_t k=0;
uint16_t old_reg=0;

static uint8_t* data_tmp = NULL;
static power_data_t latest_data;
static sensor_status_t energy_meter_uart_init(void) {

    const uart_config_t uart_config = {
            .baud_rate = 4800,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 24, &uart0_queue, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    data_tmp = (uint8_t*) malloc(RX_BUF_SIZE);

    return SENSOR_OK;
}

static sensor_status_t energy_meter_uart_read_data(power_data_t *data) {
    // Copy the latest data to the provided data structure
    if (data != NULL) {
        memcpy(data, &latest_data, sizeof(power_data_t));
        return SENSOR_OK;
    }
    return SENSOR_ERROR;
}

static void energy_meter_uart_deinit(void) {
    uart_driver_delete(UART_NUM_1);
    free(data_tmp);
    data_tmp = NULL;
}

static int hlw_parse_data(const uint8_t* reg_data, power_data_t * data)
{
    int ret = HLW_OK;
    // 检查是否为HLW数据
    if(reg_data[1]!=0x5A)
    {
        ret = HLW_ERR_INVALID_DATA;
        return ret;
    }

    // 校验和
    int check_sum = 0;
    for(int i=2; i<23; ++i)
    {
        check_sum += reg_data[i];
    }
    if((check_sum & 0xFF) != reg_data[23]) {
        ret = HLW_ERR_INVALID_CHECK_SUM;
        return ret;
    }

    // 芯片误差修正功能失效
    if(reg_data[0] == 0xAA)
    {
        ret = HLW_ERR_CORRECTION_FAILURE;
        return ret;
    }

    uint32_t VP_REG=0,CP_REG=0,PP_REG=0,PF_COUNT=0,PF=0;
    uint32_t V_REG=0, C_REG=0, P_REG=0;

    // 电压参数寄存器
    VP_REG = reg_data[2]<<16 | reg_data[3]<<8 | reg_data[4];
    // 电压寄存器
    if((reg_data[0]&0xF8) != 0xF8)
    {
        V_REG = reg_data[5]<<16 | reg_data[6]<<8 | reg_data[7];
    }
    // 计算电压值
    data->voltage = V_REG==0 ? 0 : ((float)VP_REG/(float)V_REG)*VOLTAGE_COEFFICIENT;

    // 计算电流参数寄存器
    CP_REG = reg_data[8]<<16 | reg_data[9]<<8 | reg_data[10];
    // 计算电流寄存器
    if((reg_data[0]&0XF4) != 0xF4)
    {
        C_REG = reg_data[11]<<16 | reg_data[12]<<8 | reg_data[13];
    }
    // 计算电流值
    data->current = C_REG==0 ? 0 : ((CP_REG*100.0)/C_REG)/100.0;

    // 计算功率参数寄存
    PP_REG = reg_data[14]<<16 | reg_data[15]<<8 | reg_data[16];
    // 计算功率寄存器
    if((reg_data[0]&0XF2) != 0xF2) {
        P_REG = reg_data[17] << 16 | reg_data[18] << 8 | reg_data[19];
    }
    //计算有效功率
    data->power = P_REG==0 ? 0 : ((float)PP_REG/(float)P_REG)*VOLTAGE_COEFFICIENT*CURRENT_COEFFICIENT;

    // 判断数据更新寄存器最高位有没有翻转
    if((reg_data[20]&0x80) != old_reg)
    {
        ++k;
        old_reg = reg_data[20] & 0x80;
    }
    // 计算已用电量脉冲数
    PF=(k<<16) | (reg_data[21]<<8) | reg_data[22];
    // 计算1度电对应的脉冲数量
    PF_COUNT=((100000 * 3600) / (PP_REG * VOLTAGE_COEFFICIENT * CURRENT_COEFFICIENT)) * 10000.f;
    // 计算已用电量
    data->power_consumption = (((float)PF*10000.f)/(float)PF_COUNT)/10000.f;

    return ret;
}


static void energy_meter_uart_event_task(void *pvParameters) {
    uart_event_t event;
    for(;;) {
        if(xQueueReceive(uart0_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    memset(data_tmp, 0, RX_BUF_SIZE);
                    uart_read_bytes(UART_NUM_1, data_tmp, event.size, portMAX_DELAY);
                    power_data_t power_data;
                    int res = hlw_parse_data(data_tmp, &power_data);
                    if (res == ESP_OK) {
                        latest_data.voltage = power_data.voltage;
                        latest_data.current = power_data.current;
                        latest_data.power = power_data.power;
                        latest_data.power_consumption = power_data.power_consumption;
                    }
                    break;
                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    uart_flush_input(UART_NUM_1);
                    xQueueReset(uart0_queue);
                    break;
                default:
                    break;
            }
        }
    }
}

static void energy_meter_uart_start_reading(void) {
    xTaskCreate(energy_meter_uart_event_task, "energy_meter_uart_event_task", 2048, NULL, 10, NULL);
}

power_sensor_t* get_hlw8032_driver(void) {
    static power_sensor_t energy_meter_uart_driver = {
            .init = energy_meter_uart_init,
            .read_data = energy_meter_uart_read_data,
            .deinit = energy_meter_uart_deinit,
            .start_reading = energy_meter_uart_start_reading
    };
    return &energy_meter_uart_driver;
}
