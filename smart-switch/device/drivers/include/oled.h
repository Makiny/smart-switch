#ifndef IOT_SWITCH_OLED_H
#define IOT_SWITCH_OLED_H
#include "stdint.h"

//-----------------OLED端口定义----------------

#define I2C_MASTER_NUM I2C_NUM_0  // 使用I2C0
#define I2C_MASTER_SCL_IO 19      // I2C SCL引脚
#define I2C_MASTER_SDA_IO 18      // I2C SDA引脚
#define I2C_MASTER_FREQ_HZ 200000 // I2C频率
#define OLED_ADDR 0x78            // OLED I2C地址

void OLED_ClearPoint(uint8_t x,uint8_t y);
void OLED_ColorTurn(uint8_t i);
void OLED_DisplayTurn(uint8_t i);

void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_DrawPoint(uint8_t x,uint8_t y,uint8_t t);
void OLED_DrawLine(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t mode);
void OLED_DrawCircle(uint8_t x,uint8_t y,uint8_t r);
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size1,uint8_t mode);
void OLED_ShowChar6x8(uint8_t x,uint8_t y,uint8_t chr,uint8_t mode);
void OLED_ShowString(uint8_t x,uint8_t y,char *chr,uint8_t size1,uint8_t mode);
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size1,uint8_t mode);
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t len, uint8_t size, uint8_t mode);
void OLED_ShowChinese(uint8_t x,uint8_t y,uint8_t num,uint8_t size1,uint8_t mode);
void OLED_ScrollDisplay(uint8_t num,uint8_t space,uint8_t mode);
void OLED_ShowPicture(uint8_t x,uint8_t y,uint8_t sizex,uint8_t sizey,uint8_t BMP[],uint8_t mode);
void OLED_Init(void);

#endif //IOT_SWITCH_OLED_H
