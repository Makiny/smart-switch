
#include "stdlib.h"
#include "oledfont.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "oled.h"

static const char *TAG = "OLED";

uint8_t OLED_GRAM[144][8];

// I2C初始化
esp_err_t i2c_master_init()
{
    i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = I2C_MASTER_SDA_IO,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_io_num = I2C_MASTER_SCL_IO,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// I2C写入一个字节
esp_err_t i2c_write_byte(i2c_cmd_handle_t cmd, uint8_t data, bool ack_en)
{
    return i2c_master_write_byte(cmd, data, ack_en ? I2C_MASTER_ACK : I2C_MASTER_NACK);
}

// I2C发送命令
esp_err_t oled_master_write_cmd(uint8_t cmd)
{
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_write_byte(handle, OLED_ADDR | I2C_MASTER_WRITE, true);
    i2c_write_byte(handle, 0x00, true);  // 发送命令标志
    i2c_write_byte(handle, cmd, true);
    i2c_master_stop(handle);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, handle, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error sending command: 0x%X", cmd);
    }
    return ret;
}

// I2C发送数据
esp_err_t i2c_master_write_data(uint8_t data)
{
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_write_byte(handle, OLED_ADDR | I2C_MASTER_WRITE, true);
    i2c_write_byte(handle, 0x40, true);  // 发送数据标志
    i2c_write_byte(handle, data, true);
    i2c_master_stop(handle);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, handle, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error sending data: 0x%X", data);
    }
    return ret;
}

// 反显函数
void OLED_ColorTurn(uint8_t i)
{
    if (i == 0) {
        oled_master_write_cmd(0xA6); // 正常显示
    } else if (i == 1) {
        oled_master_write_cmd(0xA7); // 反色显示
    }
}

// 屏幕旋转180度
void OLED_DisplayTurn(uint8_t i)
{
    if (i == 0) {
        oled_master_write_cmd(0xC8); // 正常显示
        oled_master_write_cmd(0xA1);
    } else if (i == 1) {
        oled_master_write_cmd(0xC0); // 反转显示
        oled_master_write_cmd(0xA0);
    }
}

// 开启OLED显示
void OLED_DisplayOn(void)
{
    oled_master_write_cmd(0x8D); // 电荷泵使能
    oled_master_write_cmd(0x14); // 开启电荷泵
    oled_master_write_cmd(0xAF); // 点亮屏幕
}

// 关闭OLED显示
void OLED_DisplayOff(void)
{
    oled_master_write_cmd(0x8D); // 电荷泵使能
    oled_master_write_cmd(0x10); // 关闭电荷泵
    oled_master_write_cmd(0xAE); // 关闭屏幕
}

// 更新显存到OLED
void OLED_Refresh(void)
{
    for (uint8_t i = 0; i < 8; i++) {
        oled_master_write_cmd(0xb0 + i); // 设置行起始地址
        oled_master_write_cmd(0x00);     // 设置低列起始地址
        oled_master_write_cmd(0x10);     // 设置高列起始地址

        i2c_cmd_handle_t handle = i2c_cmd_link_create();
        i2c_master_start(handle);
        i2c_write_byte(handle, OLED_ADDR | I2C_MASTER_WRITE, true);
        i2c_write_byte(handle, 0x40, true); // 发送数据标志

        for (uint8_t n = 0; n < 128; n++) {
            i2c_master_write_byte(handle, OLED_GRAM[n][i], I2C_MASTER_ACK);
        }

        i2c_master_stop(handle);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, handle, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(handle);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error sending data for row: %d", i);
        }
    }
}

// 清屏函数
void OLED_Clear(void)
{
    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t n = 0; n < 128; n++) {
            OLED_GRAM[n][i] = 0; // 清除所有数据
        }
    }
    OLED_Refresh(); // 更新显示
}

//画点
//x:0~127
//y:0~63
//t:1 填充 0,清空
void OLED_DrawPoint(uint8_t x,uint8_t y,uint8_t t)
{
    uint8_t i,m,n;
    i=y/8;
    m=y%8;
    n=1<<m;
    if(t){OLED_GRAM[x][i]|=n;}
    else
    {
        OLED_GRAM[x][i]=~OLED_GRAM[x][i];
        OLED_GRAM[x][i]|=n;
        OLED_GRAM[x][i]=~OLED_GRAM[x][i];
    }
}

//画线
//x1,y1:起点坐标
//x2,y2:结束坐标
void OLED_DrawLine(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t mode)
{
    uint16_t t;
    int xerr=0,yerr=0,delta_x,delta_y,distance;
    int incx,incy,uRow,uCol;
    delta_x=x2-x1; //计算坐标增量
    delta_y=y2-y1;
    uRow=x1;//画线起点坐标
    uCol=y1;
    if(delta_x>0)incx=1; //设置单步方向
    else if (delta_x==0)incx=0;//垂直线
    else {incx=-1;delta_x=-delta_x;}
    if(delta_y>0)incy=1;
    else if (delta_y==0)incy=0;//水平线
    else {incy=-1;delta_y=-delta_x;}
    if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴
    else distance=delta_y;
    for(t=0;t<distance+1;t++)
    {
        OLED_DrawPoint(uRow,uCol,mode);//画点
        xerr+=delta_x;
        yerr+=delta_y;
        if(xerr>distance)
        {
            xerr-=distance;
            uRow+=incx;
        }
        if(yerr>distance)
        {
            yerr-=distance;
            uCol+=incy;
        }
    }
}
//x,y:圆心坐标
//r:圆的半径
void OLED_DrawCircle(uint8_t x,uint8_t y,uint8_t r)
{
    int a, b,num;
    a = 0;
    b = r;
    while(2 * b * b >= r * r)
    {
        OLED_DrawPoint(x + a, y - b,1);
        OLED_DrawPoint(x - a, y - b,1);
        OLED_DrawPoint(x - a, y + b,1);
        OLED_DrawPoint(x + a, y + b,1);

        OLED_DrawPoint(x + b, y + a,1);
        OLED_DrawPoint(x + b, y - a,1);
        OLED_DrawPoint(x - b, y - a,1);
        OLED_DrawPoint(x - b, y + a,1);

        a++;
        num = (a * a + b * b) - r*r;//计算画的点离圆心的距离
        if(num > 0)
        {
            b--;
            a--;
        }
    }
}



//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//size1:选择字体 6x8/6x12/8x16/12x24
//mode:0,反色显示;1,正常显示
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size1,uint8_t mode)
{
    uint8_t i,m,temp,size2,chr1;
    uint8_t x0=x,y0=y;
    if(size1==8)size2=6;
    else size2=(size1/8+((size1%8)?1:0))*(size1/2);  //得到字体一个字符对应点阵集所占的字节数
    chr1=chr-' ';  //计算偏移后的值
    for(i=0;i<size2;i++)
    {
        if(size1==8)
        {temp=asc2_0806[chr1][i];} //调用0806字体
        else if(size1==12)
        {temp=asc2_1206[chr1][i];} //调用1206字体
        else if(size1==16)
        {temp=asc2_1608[chr1][i];} //调用1608字体
        else if(size1==24)
        {temp=asc2_2412[chr1][i];} //调用2412字体
        else return;
        for(m=0;m<8;m++)
        {
            if(temp&0x01)OLED_DrawPoint(x,y,mode);
            else OLED_DrawPoint(x,y,!mode);
            temp>>=1;
            y++;
        }
        x++;
        if((size1!=8)&&((x-x0)==size1/2))
        {x=x0;y0=y0+8;}
        y=y0;
    }
}


//显示字符串
//x,y:起点坐标
//size1:字体大小
//*chr:字符串起始地址
//mode:0,反色显示;1,正常显示
void OLED_ShowString(uint8_t x,uint8_t y,char *chr,uint8_t size1,uint8_t mode)
{
    while((*chr>=' ')&&(*chr<='~'))//判断是不是非法字符!
    {
        OLED_ShowChar(x,y,*chr,size1,mode);
        if(size1==8)x+=6;
        else x+=size1/2;
        chr++;
    }
}

//m^n
uint32_t OLED_Pow(uint8_t m,uint8_t n)
{
    uint32_t result=1;
    while(n--)
    {
        result*=m;
    }
    return result;
}

//显示数字
//x,y :起点坐标
//num :要显示的数字
//len :数字的位数
//size:字体大小
//mode:0,反色显示;1,正常显示
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size1,uint8_t mode)
{
    uint8_t t,temp,m=0;
    if(size1==8)m=2;
    for(t=0;t<len;t++)
    {
        temp=(num/OLED_Pow(10,len-t-1))%10;
        if(temp==0)
        {
            OLED_ShowChar(x+(size1/2+m)*t,y,'0',size1,mode);
        }
        else
        {
            OLED_ShowChar(x+(size1/2+m)*t,y,temp+'0',size1,mode);
        }
    }
}

// 显示浮点数
// x, y : 起点坐标
// num : 要显示的浮点数
// size : 字体大小
// len : 小数点后有效位数
// mode : 0,反色显示;1,正常显示
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t len, uint8_t size, uint8_t mode)
{
    char buffer[32]; // 足够大的缓冲区来存储格式化后的字符串

    // 格式化浮点数为字符串，整数部分总是三位，小数点后保留三位
    snprintf(buffer, sizeof(buffer), "%07.3f", num);

    // 显示格式化后的字符串
    OLED_ShowString(x, y, buffer, size, mode);
}

//显示汉字
//x,y:起点坐标
//num:汉字对应的序号
//mode:0,反色显示;1,正常显示
void OLED_ShowChinese(uint8_t x,uint8_t y,uint8_t num,uint8_t size1,uint8_t mode)
{
    uint8_t m,temp;
    uint8_t x0=x,y0=y;
    uint16_t i,size3=(size1/8+((size1%8)?1:0))*size1;  //得到字体一个字符对应点阵集所占的字节数
    for(i=0;i<size3;i++)
    {
        if(size1==16)
        {temp=Hzk1[num][i];}//调用16*16字体
        else if(size1==24)
        {temp=Hzk2[num][i];}//调用24*24字体
        else if(size1==32)
        {temp=Hzk3[num][i];}//调用32*32字体
        else if(size1==64)
        {temp=Hzk4[num][i];}//调用64*64字体
        else return;
        for(m=0;m<8;m++)
        {
            if(temp&0x01)OLED_DrawPoint(x,y,mode);
            else OLED_DrawPoint(x,y,!mode);
            temp>>=1;
            y++;
        }
        x++;
        if((x-x0)==size1)
        {x=x0;y0=y0+8;}
        y=y0;
    }
}

//num 显示汉字的个数
//space 每一遍显示的间隔
//mode:0,反色显示;1,正常显示
void OLED_ScrollDisplay(uint8_t num,uint8_t space,uint8_t mode)
{
    uint8_t i,n,t=0,m=0,r;
    while(1)
    {
        if(m==0)
        {
            OLED_ShowChinese(128,24,t,16,mode); //写入一个汉字保存在OLED_GRAM[][]数组中
            t++;
        }
        if(t==num)
        {
            for(r=0;r<16*space;r++)      //显示间隔
            {
                for(i=1;i<144;i++)
                {
                    for(n=0;n<8;n++)
                    {
                        OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
                    }
                }
                OLED_Refresh();
            }
            t=0;
        }
        m++;
        if(m==16){m=0;}
        for(i=1;i<144;i++)   //实现左移
        {
            for(n=0;n<8;n++)
            {
                OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
            }
        }
        OLED_Refresh();
    }
}

//x,y：起点坐标
//sizex,sizey,图片长宽
//BMP[]：要写入的图片数组
//mode:0,反色显示;1,正常显示
void OLED_ShowPicture(uint8_t x,uint8_t y,uint8_t sizex,uint8_t sizey,uint8_t BMP[],uint8_t mode)
{
    uint16_t j=0;
    uint8_t i,n,temp,m;
    uint8_t x0=x,y0=y;
    sizey=sizey/8+((sizey%8)?1:0);
    for(n=0;n<sizey;n++)
    {
        for(i=0;i<sizex;i++)
        {
            temp=BMP[j];
            j++;
            for(m=0;m<8;m++)
            {
                if(temp&0x01)OLED_DrawPoint(x,y,mode);
                else OLED_DrawPoint(x,y,!mode);
                temp>>=1;
                y++;
            }
            x++;
            if((x-x0)==sizex)
            {
                x=x0;
                y0=y0+8;
            }
            y=y0;
        }
    }
}

// OLED的初始化
void OLED_Init(void)
{
    ESP_ERROR_CHECK(i2c_master_init());

    vTaskDelay(100 / portTICK_RATE_MS);

    oled_master_write_cmd(0xAE); // 关闭显示

    oled_master_write_cmd(0x20); // 设置内存寻址模式
    oled_master_write_cmd(0x10); // 地址模式设置为页寻址模式

    oled_master_write_cmd(0xB0); // 设置页起始地址
    oled_master_write_cmd(0xC8); // 设置COM输出扫描方向

    oled_master_write_cmd(0x00); // 设置低列地址
    oled_master_write_cmd(0x10); // 设置高列地址

    oled_master_write_cmd(0x40); // 设置起始行地址

    oled_master_write_cmd(0x81); // 设置对比度控制
    oled_master_write_cmd(0xFF); // 亮度为最大值

    oled_master_write_cmd(0xA1); // 设置段重映射
    oled_master_write_cmd(0xA6); // 正常显示

    oled_master_write_cmd(0xA8); // 设置多路复用率
    oled_master_write_cmd(0x3F); // 1/64占空比

    oled_master_write_cmd(0xA4); // 输出遵循RAM内容

    oled_master_write_cmd(0xD3); // 设置显示偏移
    oled_master_write_cmd(0x00); // 无偏移

    oled_master_write_cmd(0xD5); // 设置显示时钟分频比/振荡器频率
    oled_master_write_cmd(0xF0); // 设置分频比为最高

    oled_master_write_cmd(0xD9); // 设置预充电期
    oled_master_write_cmd(0x22); // 设置预充电期为22

    oled_master_write_cmd(0xDA); // 设置COM引脚硬件配置
    oled_master_write_cmd(0x12);

    oled_master_write_cmd(0xDB); // 设置VCOMH消隐选择
    oled_master_write_cmd(0x20); // 设置VCOMH为0.77xVcc

    oled_master_write_cmd(0x8D); // 电荷泵设置
    oled_master_write_cmd(0x14); // 使能电荷泵

    OLED_DisplayTurn(1);

    oled_master_write_cmd(0xAF); // 开启显示

    OLED_Clear(); // 清屏
}
