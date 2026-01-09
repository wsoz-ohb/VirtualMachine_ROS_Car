#ifndef __OLED_H__
#define __OLED_H__

#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2c.h"

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

#define OLED_FONT_SIZE_6X8 8
#define OLED_FONT_SIZE_8X16 16

#ifndef OLED_ADDR
#define OLED_ADDR 0x3C
#endif

#ifndef OLED_I2C_PORT
#define OLED_I2C_PORT I2C_NUM_0
#endif

#ifndef OLED_I2C_SDA_GPIO
#define OLED_I2C_SDA_GPIO GPIO_NUM_41
#endif

#ifndef OLED_I2C_SCL_GPIO
#define OLED_I2C_SCL_GPIO GPIO_NUM_42
#endif

#ifndef OLED_I2C_SPEED_HZ
#define OLED_I2C_SPEED_HZ 400000
#endif

void OLED_Write_cmd(uint8_t cmd);
void OLED_Write_data(uint8_t data);
void OLED_ShowPic(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t BMP[]);
void OLED_ShowHanzi(uint8_t x, uint8_t y, uint8_t no);
void OLED_ShowHzbig(uint8_t x, uint8_t y, uint8_t n);
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t accuracy, uint8_t fontsize);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t length, uint8_t fontsize);
void OLED_ShowStr(uint8_t x, uint8_t y, char *ch, uint8_t fontsize);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t ch, uint8_t fontsize);
void OLED_Allfill(void);
void OLED_Set_Position(uint8_t x, uint8_t y);
void OLED_Clear(void);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Init(void);

#endif /* __OLED_H__ */
