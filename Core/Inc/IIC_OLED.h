/*
 * @Author: LO_StartNet
 * @Date: 2022-08-15 13:14:01
 * @LastEditors: LO_StartNet
 * @LastEditTime: 2022-08-19 00:42:59
 * @Description: 使用IIC驱动128*64OLED
 * 
 * Copyright (c) 2022 by LO_StartNet, All Rights Reserved. 
 */

#ifndef _IIC_OLED_H_
#define _IIC_OLED_H_

#include "main.h"
/*************************************************************************/
/**
 * 使用说明：
 * 1，调用OLED_Init()初始化
 * 2，调用其他函数实现相应功能
 * Init干了什么参考注释
 * 参考datasheet使用
 * delay中time变量根据使用单片机进行修改
*/
/*****************************USER_DEFINE***********************************/

#define OLED_ADDRESS     0x78

#define OLED_SDA_Port GPIOA
#define OLED_SDA_Pin GPIO_PIN_9

#define OLED_SCL_Port GPIOA
#define OLED_SCL_Pin GPIO_PIN_10

/************************************************************************/

#define OLED_SDA_1() OLED_SDA_Port ->BSRR = OLED_SDA_Pin
#define OLED_SDA_0() OLED_SDA_Port ->BSRR = (uint32_t)OLED_SDA_Pin << 16U

#define OLED_SCL_1() OLED_SCL_Port ->BSRR = OLED_SCL_Pin
#define OLED_SCL_0() OLED_SCL_Port ->BSRR = (uint32_t)OLED_SCL_Pin << 16U

#define OLED_READ() ((OLED_SDA_Port->IDR & OLED_SDA_Pin) != (uint32_t)GPIO_PIN_RESET)

/****************************************************************/

typedef enum
{
    OLED_ROLL_5=0x00,
    OLED_ROLL_64=0x01,
    OLED_ROLL_128=0x02,
    OLED_ROLL_256=0x03,
    OLED_ROLL_3=0x04,
    OLED_ROLL_4=0x05,
    OLED_ROLL_25=0x06,
    OLED_ROLL_2=0x07,
}OLED_ROLL_SPEED;//滚动速度选择，越小越快

typedef enum{
    OLED_ROLL_LEFT,
    OLED_ROLL_RIGHT,
}OLED_ROLL_DIR;

/****************************************************************/
uint8_t OLED_Init(void);//初始化函数
void OLED_SetBright(uint8_t lingt);//亮度调节函数
void OELD_StartShow(void);//开启显示函数
void OLED_StopShow(void);//关闭显示函数
void OLED_ShowALL(uint8_t ch);//全显示函数
void OLED_RollBack(void);//反转显示函数
void OLED_HoriChange(void);//左右反转显示,好像没啥用
void OLED_VertChange(void);//上下反转显示
void OLED_ClearAll(void);//清屏函数
void OLED_ClearLine(uint8_t line);//清除一行函数
void OLED_Clear(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2);//清屏函数
void OLED_StopRoll(void);//停止滚动
void OLED_StartHoriRoll(OLED_ROLL_DIR dir,OLED_ROLL_SPEED speed,uint8_t Spage,uint8_t Epage);//水平滚动
void OLED_StartHoriAndVertRoll(OLED_ROLL_DIR dir,OLED_ROLL_SPEED speed,uint8_t Spage,uint8_t Epage,uint8_t vert);//水平和垂直滚动
void OLED_SetVertRollPage(uint8_t Dpage,uint8_t Rpage);//设置垂直滚动区域,经测试好像没啥用

void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t Size,uint8_t mode);//显示字符
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t* chr,uint8_t Size,uint8_t mode);//显示字符串
void OLED_Printf(uint8_t x,uint8_t y,uint8_t Size,uint8_t mode,const char *__format, ...);//向屏幕printf
void OLED_ShowChinese(uint8_t x,uint8_t y,uint8_t mode,uint8_t no);//显示汉字
void OLED_ShowBmp(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[]);//显示图片

uint8_t OLED_Test(void);//测试函数

#endif
