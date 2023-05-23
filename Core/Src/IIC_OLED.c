/*
 * @Author: LO_StartNet
 * @Date: 2022-08-15 13:13:47
 * @LastEditors: LO_StartNet
 * @LastEditTime: 2022-08-19 00:43:32
 * @Description: 使用IIC驱动128*64OLED
 * 
 * Copyright (c) 2022 by LO_StartNet, All Rights Reserved. 
 */
#include "IIC_OLED.h"
#include "OLEDFonts.h"
#include "string.h"
#include "stdarg.h"
#include "stdio.h"
#define TX_BUF_LEN 255 
uint8_t TxBuf[TX_BUF_LEN];  /* 发送缓冲区*/
/**********************************************************/

#define OLED_CMD 0x00 //发送命令指示
#define OLED_DATA 0x40 //发送数据指示

#define OLED_CMD_SetContrast 0x81 //设置对比度命令头，后面紧接一字节00h-FFh

#define OLED_CMD_StartShow 0xAF //开启显示指令
#define OLED_CMD_StopShow 0xAE //关闭显示指令

#define OLED_CMD_ChargePumpH 0x8D //开启电荷泵命令头
#define OLED_CMD_ChargePumpEN 0x14 //开始电荷泵
#define OLED_CMD_ChargePumpDis 0x10 //关闭电荷泵

#define OLED_CMD_EntireDisplayOn 0xA5 //忽略数据全点亮
#define OLED_CMD_EntireDisplayOff 0xA4 //输出显示数据

#define OLED_CMD_RollBackShowOff 0xA6 //1亮0灭
#define OLED_CMD_RollBackShowOn 0xA7 //0亮1灭

//Addressing Accessing
//设置寻址模式和地址指令
#define OLED_CMD_SetAddMode 0x20 //设置内存地址模式的命令头
#define OLED_CMD_AddressHori 0x00 //水平寻址模式
#define OLED_CMD_AddressVert 0x01 //垂直寻址模式
#define OLED_CMD_AddressPage 0x02 //页寻址模式(默认)
//页寻址模式下
#define OLED_CMD_SetPage 0xB0 //设置页地址，使用时更改后三位， 对应0-7页--页寻址模式(page addressing)
#define OLED_CMD_SetLineL 0x00 //设置列低四位，使用时使用低四位--页地址模式(page addressing)
#define OLED_CMD_SetLineH 0x10 //设置列高四位，使用时使用低四位--页地址模式(page addressing)
//水平和垂直地址模式下
#define OLED_CMD_SetPageAddress 0x22 //水平和垂直模式下设置起始页和终止页地址,后接两个字节，先为起始地址后为终止地址，范围为0~7，默认为0-7
#define OLED_CMD_SetColuAddress 0x21 //水平和垂直模式下设置起始列和终止列，后接两字节，起始+终止，范围0~127，默认0~127

//Hardware Configuration
 #define OLED_CMD_SetDisplayStartLine 0x40 //低六位决定起始行，40h-7Fh设置起始行0-63，默认0;会设置RAM第几行映射到COM0
//左右反置
 #define OLED_CMD_SetSegRemap_0 0xA0 //列地址0映射到SEG0(默认)
 #define OLED_CMD_SetSegRemap_1 0xA1 //列地址0映射到SEG1
//上下反置
 #define OLED_CMD_SetScanDirNOR 0xC0 //COM0-COM[N-1],N为复用率(默认)
 #define OLED_CMD_SetScanDirREM 0xC8 //COM[N-1]-COM0

 #define OLED_CMD_SetMultiRatio 0xA8 //设置复用率，后接复用率0x0F~0x3F对应1-64,默认111111b(64);用来设置使用了用几行

 #define OLED_CMD_SetDisplayOffset 0xD3 //设置垂直偏移行数，后接具体偏移行数(0-63),默认0

 #define OLED_CMD_SetCOMPin 0xDA //行引脚硬件配置命令头，

 //Timing Driving Scheme
 #define OLED_CMD_SetClock 0xD5 //设置时钟分频数。振荡器频率的命令头，后接具体数：A[7:4]震荡频率=A[7:4](默认1000b),A[3:0]分频数=A[3:0]+1(默认0000b)
 #define OLED_CMD_SetPreCharge 0xD9 //设置预充电周期命令头，后接周期数(默认2)
 #define OLED_CMD_SetVcomh 0xDB //设置Vcomh输出命令头,后接00h-0.65Vcc;10h-0.77Vcc(默认);30h-0.83Vcc
 #define OLED_CMD_NOP 0xE3 //空操作

 //Scrolling Command Table
 #define OLED_CMD_ENScroll 0x2F //启动26H/27H/29H/2AH指令配置的滚动,例如2AH,2FH
 #define OLED_CMD_DISscroll 0x2E //停止26H/27H/29H/2AH指令配置的滚动

 #define OLED_CMD_RightRoll 0x26 //向右滚动的指令头

 #define OLED_CMD_LeftRoll 0x27 //向左滚动指令头

 #define OLED_CMD_VeRightRoll 0x29 //垂直和向右滚动指令头

 #define OLED_CMD_VeLeftRoll 0x2A //垂直和向左滚动指令头

 #define OELD_CMD_SetVerollArea 0xA3 //设置垂直滚动区域


/***********************基础函数*****************************/

/**
 * @name: OLED_Delay
 * @description: 延时函数
 * @return {*}NONE
 */
static void OLED_Delay()
{
    uint16_t time=20;
    volatile uint16_t t=0;
    for(;time>0;time--) t++;
}

/**
 * @name: IIC_Start
 * @description: CPU发起I2C总线启动信号
 * @return {*}
 */
static void IIC_Start()
{
    //SCL和SDA拉高的情况下，SDA拉低发送开始信号
    OLED_SCL_1();
    OLED_SDA_1();
    OLED_Delay();
    OLED_SDA_0();
    OLED_Delay();
    OLED_SCL_0();
    
}

/**
 * @name: IIC_Stop
 * @description: CPU发起I2C总线停止信号
 * @return {*}
 */
static void IIC_Stop()
{
    //SCL拉高时，SDA从低变高，发送停止信号
    OLED_SDA_0();
    OLED_SCL_1();
    OLED_Delay();
    OLED_SDA_1();
}

/**
 * @name: IIC_SendByte
 * @description: CPU向I2C总线设备发送8bit数据,不带S和P信号
 * @param {uint8_t} byte 等待发送的字节
 * @return {*}
 */
static void IIC_SendByte(uint8_t byte)
{
    //初始状态SCL为0，当SCL低，改变SDA；当SCL高，发送SDA值
    for(uint8_t i=0;i<8;i++)//高位先发送
    {
        if(byte & 0x80) OLED_SDA_1();
        else OLED_SDA_0();
				
        OLED_SCL_1();
        OLED_Delay();
        byte <<=1;
        OLED_SCL_0();
        OLED_Delay();

        if(i==7) OLED_SDA_1();//空闲时SDA高电平
        
    }
}

/**
 * @name: IIC_WaitAck
 * @description: CPU产生一个时钟，并读取器件的ACK应答信号
 * @return {uint8_t}返回0表示正确应答，1表示无器件响应
 */
static uint8_t IIC_WaitAck()
{
    //初态SCL为0，SDA无所谓。先拉高SDA（即释放SDA），再将SCL拉高，读取SDA值，SDA为0为应答，否则非应答
    uint8_t rec;

    OLED_SDA_1();
    OLED_SCL_1();
    OLED_Delay();
    if(OLED_READ()) rec=1;
    else rec=0;
    OLED_SCL_0();

    return rec;
}




/***************************基础函数1**********************************/

/**
 * @name: OLED_Check
 * @description: 检测I2C总线设备，CPU向发送设备地址，然后读取设备应答来判断该设备是否存在
 * @return {uint8_t}1:OK 0:ERROR
 */
static uint8_t OLED_Check()
{
    uint8_t re;
    IIC_Start();
    IIC_SendByte(OLED_ADDRESS);
    re=IIC_WaitAck();
    IIC_Stop();

    return !re;
} 



/**
 * @name: OLED_SendCmd
 * @description: 向OLED发送一串命令，不带开始和结束信号
 * @param {uint8_t} cmd 要发送的命令
 * @return {uint8_t} 发送成功返回1:发送失败返回0
 */
static uint8_t OLED_SendCmd(uint8_t cmd)
{
    IIC_Start();

    IIC_SendByte(OLED_ADDRESS);
    if(IIC_WaitAck()) return 0;
    IIC_SendByte(OLED_CMD);
    if(IIC_WaitAck()) return 0;//没有收到回应即炸了

    IIC_SendByte(cmd);
    if(IIC_WaitAck()) return 0;

    IIC_Stop();
    return 1;
}

/**
 * @name: OLED_SendData
 * @description: 向OLED发送一串数据，不带开始和结束信号
 * @param {uint8_t} data 要发送的数据
 * @return {uint8_t}发送成功返回1:发送失败返回0
 */
static uint8_t OLED_SendData(uint8_t data)
{
    IIC_Start();

    IIC_SendByte(OLED_ADDRESS);
    if(IIC_WaitAck()) return 0;
    IIC_SendByte(OLED_DATA);
    if(IIC_WaitAck()) return 0;//没有收到回应即炸了

    IIC_SendByte(data);
    if(IIC_WaitAck()) return 0;

    IIC_Stop();
    return 1;
}


/***************************基础函数2**********************************/

/**
 * @name: OLED_SetPgaeAddress
 * @description: 设置为页寻址模式
 * @return {*}
 */
static void OLED_SetPgaeAddress()
{
    OLED_SendCmd(OLED_CMD_SetAddMode);
    OLED_SendCmd(OLED_CMD_AddressPage);
}

/**
 * @name: OLED_SetPos
 * @description: 设置起始位置的函数,datasheet上说只有页寻址可以用，但实际上三种模式都可以使用
 * @param {uint8_t} x 列地址,0-127
 * @param {uint8_t} y 页地址,0-7
 * @return {*}NONE
 */
static void OLED_SetPos(uint8_t x,uint8_t y)
{
    OLED_SendCmd(OLED_CMD_SetPage+y);

    OLED_SendCmd((x & 0xF0) >> 4 | OLED_CMD_SetLineH);//设置列高四位
    OLED_SendCmd((x & 0x0F) | OLED_CMD_SetLineL);//设置列低四位
}


/**
 * @name: OLED_SetHoriAddress
 * @description: 寻址模式切换为水平寻址模式，默认扫描范围,列范围0-127,页范围0-7
 * @return {*}
 */
static void OLED_SetHoriAddress()
{
    OLED_SendCmd(OLED_CMD_SetAddMode);
    OLED_SendCmd(OLED_CMD_AddressHori);
}

/**
 * @name: OLED_SetVertAddress
 * @description: 寻址模式切换为垂直寻址模式，默认扫描范围,列范围0-127,页范围0-7
 * @return {*}
 */
static void OLED_SetVertAddress()
{
    OLED_SendCmd(OLED_CMD_SetAddMode);
    OLED_SendCmd(OLED_CMD_AddressVert);
}

/**
 * @name: OLED_SetLineAndColu
 * @description: 水平扫描模式和垂直扫描模式下设置扫描起始页地址和列地址,列范围0-127,页范围0-7
 * @param {uint8_t} Scolu 起始列地址
 * @param {uint8_t} Ecolu 结束列地址
 * @param {uint8_t} Sline 起始页地址
 * @param {uint8_t} Eline 结束页地址
 * @return {*}
 */
static void OLED_SetLineAndColu(uint8_t Scolu,uint8_t Ecolu,uint8_t Sline,uint8_t Eline)
{
    OLED_SendCmd(OLED_CMD_SetColuAddress);//设置起始列和终止列
    OLED_SendCmd(Scolu);OLED_SendCmd(Ecolu);
    OLED_SendCmd(OLED_CMD_SetPageAddress);//设置起始页和终止页
    OLED_SendCmd(Sline);OLED_SendCmd(Eline);
}
/****************************用户函数1********************************/


uint8_t OLED_Init()
{

    OLED_SendCmd(OLED_CMD_StopShow);//关闭显示

    OLED_SendCmd(OLED_CMD_SetMultiRatio);OLED_SendCmd(0x3F);//设置使用64行
    OLED_SendCmd(OLED_CMD_SetDisplayOffset);OLED_SendCmd(0x00);//设置垂直偏移0行
    OLED_SendCmd(OLED_CMD_SetSegRemap_1);//不左右反转
    OLED_SendCmd(OLED_CMD_SetScanDirREM);//不上下反转
    OLED_SendCmd(OLED_CMD_SetCOMPin);OLED_SendCmd(0x12);//COM引脚不反置
    OLED_SendCmd(OLED_CMD_SetDisplayStartLine);//设置起始显示行为0
    OLED_SendCmd(OLED_CMD_SetAddMode);OLED_SendCmd(OLED_CMD_AddressHori);//设置水平寻址模式

    OLED_SendCmd(OLED_CMD_SetContrast);OLED_SendCmd(0x7F);//设置127级对比度
    OLED_SendCmd(OLED_CMD_EntireDisplayOff);//关闭全显示
    OLED_SendCmd(OLED_CMD_RollBackShowOff);//关闭反转显示
    OLED_SendCmd(OLED_CMD_SetVcomh);OLED_SendCmd(0x30);// set Vcomh
    OLED_SendCmd(OLED_CMD_SetPreCharge);OLED_SendCmd(0xF1);// Set Pre-Charge Period
    OLED_SendCmd(OLED_CMD_SetClock);OLED_SendCmd(0x80);//设置振荡器频率为默认值
    OLED_SendCmd(OLED_CMD_ChargePumpH);OLED_SendCmd(OLED_CMD_ChargePumpEN);//开启电荷泵

    OLED_SendCmd(OLED_CMD_DISscroll);//停止滚动

    OLED_ClearAll();

    OLED_SendCmd(OLED_CMD_StartShow);//开启显示

    return OLED_Check();
}


/**
 * @name: OLED_SetBright
 * @description: 设置屏幕对比度
 * @param {uint8_t} lingt 对比度0-256
 * @return {*}
 */
void OLED_SetBright(uint8_t lingt)
{
    OLED_SendCmd(OLED_CMD_SetContrast);//调节亮度指令
    OLED_SendCmd(lingt);
}

/**
 * @name: OELD_StartShow
 * @description: 开启显示
 * @return {*}
 */
void OELD_StartShow()
{
    OLED_SendCmd(OLED_CMD_StartShow);
}

/**
 * @name: OLED_StopShow
 * @description: 关闭显示
 * @return {*}
 */
void OLED_StopShow()
{
    OLED_SendCmd(OLED_CMD_StopShow);
}

/**
 * @name: OLED_ShowALL
 * @description: 亮屏,全部点亮
 * @param {uint8_t} ch 1:开启全点亮 0:关闭全点亮
 * @return {*}
 */
void OLED_ShowALL(uint8_t ch)
{
    if(ch==1) OLED_SendCmd(OLED_CMD_EntireDisplayOn);
    else OLED_SendCmd(OLED_CMD_EntireDisplayOff);
}

/**
 * @name: OLED_RollBack
 * @description: 反转显示
 * @return {*}
 */
void OLED_RollBack()
{
    static uint8_t now=0;
    if(now==0)
    {
        OLED_SendCmd(OLED_CMD_RollBackShowOn);
        now=1;
    }
    else 
    {
        OLED_SendCmd(OLED_CMD_RollBackShowOff);
        now=0;
    }
}

/**
 * @name: OLED_HoriChange
 * @description: 改变左右反转反向,经过测试好像没啥用
 * @return {*}
 */
void OLED_HoriChange()
{
    static uint8_t now=0;
    if(now==0)
    { 
        OLED_SendCmd(OLED_CMD_SetSegRemap_0);//左右反转
        now=1;
    }
    else 
    {
        OLED_SendCmd(OLED_CMD_SetSegRemap_1);//关闭
        now=0;
    }
}

/**
 * @name: OLED_VertChange
 * @description: 改变垂直反转显示
 * @return {*}
 */
void OLED_VertChange()
{
    static uint8_t now=0;
    if(now==0)
    {
        OLED_SendCmd(OLED_CMD_SetScanDirNOR);//上下反转
        now=1;
    }
    else{
        OLED_SendCmd(OLED_CMD_SetScanDirREM);
        now=0;
    }
}

/**
 * @name: OLED_ClearAll
 * @description: 清屏操作
 * @return {*}
 */
void OLED_ClearAll()
{
    //水平寻址模式
    for(uint8_t i=0;i<8;i++)
        for(uint8_t j=0;j<128;j++)
        {
            OLED_SendData(0x00);
        }
}

/**
 * @name: OLED_Clear
 * @description: 清除指定区域
 * @param {uint8_t} x1 起始x坐标
 * @param {uint8_t} y1 起始y坐标
 * @param {uint8_t} x2 结束x坐标
 * @param {uint8_t} y2 结束y坐标
 * @return {*}
 */
void OLED_Clear(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2){
    uint8_t i,j;
    for(i=y1;i<=y2;i++)
    {
        OLED_SetPos(x1,i);
        for(j=x1;j<=x2;j++)
        {
            OLED_SendData(0x00);
        }
    }
}

/**
 * @name: OLED_ClearLine
 * @description: 清除指定行
 * @param {uint8_t} line 行数
 * @return {*}
 */
void OLED_ClearLine(uint8_t line)
{
    OLED_SetPos(0,line);
    for(uint8_t i=0;i<128;i++) OLED_SendData(0x00);
}

/**
 * @name: OLED_StopRoll()
 * @description: 停止滚动
 * @return {*}
 */
void OLED_StopRoll()
{
    OLED_SendCmd(OLED_CMD_DISscroll);
}

/**
 * @name: OLED_StartHoriRoll
 * @description: 开始水平滚动指令，page:0-7,Spage必须小于Epage
 * @param {OLED_ROLL_DIR} dir 滚动反向
 * @param {OLED_ROLL_SPEED} speed 滚动速度
 * @param {uint8_t} Spage 起始滚动页
 * @param {uint8_t} Epage 结束滚动页
 * @return {*}
 */
void OLED_StartHoriRoll(OLED_ROLL_DIR dir,OLED_ROLL_SPEED speed,uint8_t Spage,uint8_t Epage)
{
    
    if(dir==OLED_ROLL_LEFT) OLED_SendCmd(OLED_CMD_LeftRoll);
    else OLED_SendCmd(OLED_CMD_RightRoll);
    OLED_SendCmd(0x00);
    OLED_SendCmd(Spage);
    OLED_SendCmd(speed);
    OLED_SendCmd(Epage);
    OLED_SendCmd(0x00);
    OLED_SendCmd(0xFF);
    OLED_SendCmd(OLED_CMD_ENScroll);
}

/**
 * @name: OLED_StartHoriAndVertRoll
 * @description: 开始水平和垂直滚动指令，page:0-7,Spage必须小于Epage，vert:0-63
 * @param {OLED_ROLL_DIR} dir 滚动反向
 * @param {OLED_ROLL_SPEED} speed 滚动速度
 * @param {uint8_t} Spage 起始滚动页
 * @param {uint8_t} Epage 结束滚动页
 * @param {uint8_t} vert 垂直偏移量0-63,0为无垂直滚动
 * @return {*}
 */
void OLED_StartHoriAndVertRoll(OLED_ROLL_DIR dir,OLED_ROLL_SPEED speed,uint8_t Spage,uint8_t Epage,uint8_t vert)
{
    
    if(dir==OLED_ROLL_LEFT) OLED_SendCmd(OLED_CMD_VeLeftRoll);
    else OLED_SendCmd(OLED_CMD_VeRightRoll);
    OLED_SendCmd(0x00);
    OLED_SendCmd(Spage);
    OLED_SendCmd(speed);
    OLED_SendCmd(Epage);
    OLED_SendCmd(vert);
    OLED_SendCmd(OLED_CMD_ENScroll);
}

/**
 * @name: OLED_SetVertRollPage
 * @description: 设置垂直滚动区域,经测试好像没啥用
 * @param {uint8_t} Dpage 设置顶部固定区域行数
 * @param {uint8_t} Rpage 设置末尾滚动区域行数
 * @return {*}
 */
void OLED_SetVertRollPage(uint8_t Dpage,uint8_t Rpage)
{
    OLED_SendCmd(OELD_CMD_SetVerollArea);
    OLED_SendCmd(Dpage);
    OLED_SendCmd(Rpage);
}


/***************************用户函数2**********************************/

/**
 * @name: OLED_ShowChar
 * @description: 在指定位置显示一个字符
 * @param {uint8_t} x 0-127
 * @param {uint8_t} y 0-7
 * @param {uint8_t} chr 要显示的字符
 * @param {uint8_t} Size 字体选择-16/8
 * @param {uint8_t} mode 0,反白显示;1,正常显示
 * @return {*}NONE
 */
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t Size,uint8_t mode)
{
    uint8_t c=0;

    if(chr>= ' ') c=chr-' ';//得到偏移后的值，字模中第一个为‘ ’
    else c=chr+16;

    if(x>127)//列数超限，转到下一行
    {
        x=0;
        y=y+2;
    }
    if(Size==16)//8*16,相当于画一个8*16的图
    {
        OLED_SetPos(x,y);
        for(uint8_t i=0;i<8;i++) OLED_SendData(mode==1 ? F8X16[c*16+i]:~F8X16[c*16+i]);
        OLED_SetPos(x,y+1);
        for(uint8_t i=0;i<8;i++) OLED_SendData(mode==1 ? F8X16[c*16+i+8]:~F8X16[c*16+i+8]);
    }
    else //6*8.相当于画一个6*8的图
    {
        OLED_SetPos(x,y);
        for(uint8_t i=0;i<6;i++) OLED_SendData(mode==1 ? F6x8[c*6+i]:~F6x8[c*6+i]);
    }
}

/**
 * @name: OLED_ShowString
 * @description: 显示字符串
 * @param {uint8_t} x 0-127
 * @param {uint8_t} y 0-7
 * @param {uint8_t*} chr 要显示的字符串
 * @param {uint8_t} Size 字体选择-16/8
 * @param {uint8_t} mode 0,反白显示;1,正常显示
 * @return {*}
 */
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t* chr,uint8_t Size,uint8_t mode)
{
    uint8_t i=0;
    while (chr[i]!='\0')
    {
        OLED_ShowChar(x,y,chr[i],Size,mode);
        x+=8;
        if(x>120)//转到下一行
        {
            x=0;
            y+= (Size==16 ? 2:1);
        }
        i++;
    }
    
}

/**
 * @name: OLED_Printf
 * @description: 向屏幕打印一串字符串
 * @param {uint8_t} x 0-127
 * @param {uint8_t} y 0-7
 * @param {uint8_t} Size 字体选择-16/8
 * @param {uint8_t} mode 0,反白显示;1,正常显示
 * @param {char} *__format
 * @return {*}
 */
void OLED_Printf(uint8_t x,uint8_t y,uint8_t Size,uint8_t mode,const char *__format, ...)
{
    va_list ap;
  va_start(ap, __format);
  
  /* 清空发送缓冲区 */
  memset(TxBuf, 0x0, TX_BUF_LEN);
  
  /* 填充发送缓冲区 */
  vsnprintf((char*)TxBuf, TX_BUF_LEN, (const char *)__format, ap);
  va_end(ap);
  
  OLED_ShowString(x,y,TxBuf,Size,mode);
}

/**
 * @name: OLED_ShowChinese
 * @description: 显示一个16x16汉字
 * @param {uint8_t} x 0-127
 * @param {uint8_t} y 0-7
 * @param {uint8_t} mode 0,反白显示;1,正常显示
 * @param {uint8_t} no 汉字编号
 * @return {*}
 */
void OLED_ShowChinese(uint8_t x,uint8_t y,uint8_t mode,uint8_t no)
{
    OLED_SetPos(x,y);
    for(uint8_t i=0;i<16;i++)
    {
        OLED_SendData(mode==1 ? Hzk[no*2][i]:~Hzk[no*2][i]);
    }
    OLED_SetPos(x,y+1);
    for(uint8_t i=0;i<16;i++)
    {
        OLED_SendData(mode==1 ? Hzk[no*2+1][i]:~Hzk[no*2+1][i]);
    }
}

/**
 * @name: OLED_ShowBmp
 * @description: 显示一张图片，x:0-127,y:0-7
 * @param {unsigned char} x0 起始横坐标
 * @param {unsigned char} y0 起始纵坐标
 * @param {unsigned char} x1 末尾横坐标
 * @param {unsigned char} y1 末尾纵坐标
 * @param {unsigned char} BMP 储存图片的数组
 * @return {*}
 */
void OLED_ShowBmp(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[])
{
    uint32_t i=0;
    uint8_t x,y;

    for(y=y0;y<=y1;y++)
    {
        OLED_SetPos(x0,y);
        for(x=x0;x<=x1;x++)
        {
            OLED_SendData(BMP[i++]);
        }
    }
}

uint8_t OLED_Test()
{
    static int now=0;
    //  if(now==0) now=1;
    //  else now=0;
    if(now==0){
    OLED_ShowALL(1);
    OLED_ClearAll();
    HAL_Delay(1000);
    OLED_ShowALL(0);
    
    OLED_SetPos(0,0);OLED_SendData(0xFF);OLED_SendData(0xFF);OLED_SendData(0xFF);OLED_SendData(0xFF);
    OLED_SetPos(0,7);OLED_SendData(0xFF);OLED_SendData(0xFF);OLED_SendData(0xFF);OLED_SendData(0xFF);
    OLED_SetPos(123,0);OLED_SendData(0xFF);OLED_SendData(0xFF);OLED_SendData(0xFF);OLED_SendData(0xFF);
    OLED_SetPos(123,7);OLED_SendData(0xFF);OLED_SendData(0xFF);OLED_SendData(0xFF);OLED_SendData(0xFF);


    OLED_ShowChar(86,3,'9',16,1);
    OLED_ShowString(10,0,"HELLO_World",16,1);
    OLED_Printf(10,2,16,0,"%d:%d",2,6);
    OLED_ShowChinese(0,5,1,0);OLED_ShowChinese(16,5,1,1);OLED_ShowChinese(32,5,1,2);OLED_ShowChinese(48,5,1,3);
    HAL_Delay(1000);
    OLED_ShowBmp(0,0,127,7,bmp);
    HAL_Delay(1000);
    OLED_StartHoriRoll(OLED_ROLL_RIGHT,OLED_ROLL_3,0,7);
    HAL_Delay(1000);
    OLED_StartHoriAndVertRoll(OLED_ROLL_RIGHT,OLED_ROLL_4,0,7,1);
    HAL_Delay(1000);
    OLED_StopRoll();
    OLED_ShowBmp(0,0,127,7,bmp);
    HAL_Delay(1000);
    OLED_RollBack();
    HAL_Delay(1000);
    OLED_RollBack();
    //OLED_HoriChange();
    HAL_Delay(1000);
    OLED_VertChange();
    HAL_Delay(1000);
    //OLED_HoriChange();
    OLED_VertChange();
    HAL_Delay(5000);
    for(int i=0;i<256;i++)
    {
        OLED_SetBright(i);
        HAL_Delay(100);
    }

    now=1;
    }
    
	return 1;
    
}

