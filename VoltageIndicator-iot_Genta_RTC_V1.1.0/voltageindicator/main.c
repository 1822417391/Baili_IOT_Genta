// main.c

#include <stc8h.h>
#include <intrins.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdint.h"
#include "common.h"
#include "romhandler.h"
// #include "RTCController.h"
#include "crc.h"

#define RVREADTIMEOUT 		20
#define CRCOUNT				2
#define ReadFailedLimit 	3



sbit LED = P1^2;
// Uart1 defines
#define BRT (65536 - MAIN_Fosc / 9600 / 4)


// void delay_ms(unsigned int ms)
// {
//    unsigned int i;
//    do
//    {
//        i = MAIN_Fosc / 13000;
//        while (--i)
//            ; // 14T per loop
//    } while (--ms);
// }

/********************************INIT Setting***********************************/
// 定时器计数器
char T1COUNTER = 0;
unsigned char T1MSGCOUNTER = 0;

// 系统状态标志
char xdata FIRST_INIT;          // 首次初始化标志
unsigned int VOLTAGE_NUM;       // 电压数值
unsigned int CALCULATE_COUNT;   // 计算计数器
char LOW_POWER_MODE;            // 低功耗模式标志

// 超时计数器
int SCCOUNTER;     // 休眠计数器
int SCTIMEOUT;     // 休眠超时
int WKTIMEOUT;     // 唤醒超时
int WKCOUNTER;     // 唤醒计数器

// 通信协议数据包
char xdata BATRESP[9] = {0x29, 0x29, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0D};
char xdata SLEEPRESP[8] = {0x29, 0x29, 0x04, 0x00, 0x00, 0x30, 0x5B, 0x0D};

//手动模式数据包
char xdata manual_resp[9] = {0x29, 0x29, 0x0B, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0D};

// 其他系统变量
char sendingFlag2 = 0;          // 发送标志
int *F32K;                      // 32K时钟指针
int xdata WAKEUPTIMERCOUNT;     // 唤醒定时器计数
char RTC_INT_Flag;              // RTC中断标志
char xdata DAYS[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // 每月天数
unsigned long xdata DATETIME;   // 系统时间
char xdata HARTBEATMSG[26];     // 心跳消息
int xdata TMPSCTIMEOUT;         // 临时休眠超时
int xdata TMPWKTIMEOUT;         // 临时唤醒超时
char xdata TMPWKTCH;            // 唤醒定时器高字节
char xdata TMPWKTCL;            // 唤醒定时器低字节

// 在INIT Setting区域添加变量
extern char xdata WAKE_MODE = WAKE_INTERVAL;  // 当前唤醒模式
extern char xdata LAST_WAKE_MIN = -1;         // 上次唤醒的分钟数

// 新增手动模式标志位 RTC_V1.1.0
char xdata MANUAL_MODE_ENABLED = 0;  // 手动模式状态 0-关闭 1-开启

/*(Genta New) 唤醒指令实例
 *  间隔唤醒(现有功能)
    29 29 07 00 03 00 02 58 [CRC] 0D            29 29 07 00 03 00 02 58 3B D2 0D
    - 模式: 0x00(间隔)
    - 时间: 0x0258(600秒=10分钟)
 *
 *  整点唤醒(新增)
    29 29 07 00 01 01 [CRC] 0D                  29 29 07 00 01 01 9B 00 0D
    - 模式: 0x01(整点)
 *
 *  半点唤醒(新增)
    29 29 07 00 01 02 [CRC] 0D                  29 29 07 00 01 02 DB 01 0D
    - 模式: 0x02(半点)
 *
 *  每15分钟唤醒(新增)
    29 29 07 00 01 03 [CRC] 0D                  29 29 07 00 01 03 1A C1 0D
    - 模式: 0x03(15分钟)
*/

/*=============== RTC_V1.1.0代码修改说明 ================
1. 新增手动控制模式标志位及相关处理逻辑
2. 在UART协议中新增两个指令：
   - 开启手动模式：0x29 0x29 0x0B [数据长度] [CRC] 0x0D
   - 关闭手动模式：0x29 0x29 0x0C [数据长度] [CRC] 0x0D
   - 代码带CRC在最后两行
3. 修改定时任务和低功耗逻辑，确保手动模式优先级
===========================================*/

/********************************INIT Setting***********************************/

/********************************188 Setting***********************************/
unsigned int DISPLAY_NUM;
//char DISPLAY_STATUS;
/********************************188 Setting***********************************/


/********************************Gate Setting***********************************/

// 水阀状态变量
unsigned long xdata GATE_START_TIME;  // 水阀开始时间
int xdata GATE_WORK_DURATION;         // 水阀工作持续时间
int xdata GATE_WORK_ENABLE;           // 水阀工作使能
char xdata GATE_ACTIVATING;           // 水阀激活标志
char xdata GATE_STATUS = -1;          // 水阀状态(-1:未知, 0:关闭, 1:打开)
int xdata GATE_WORK_COUNTDOWN = 0;    // 水阀工作倒计时

// 关闭水阀函数
void OPENGATE() {
    if(P13 == 0 && P14 == 0) {  // 确保水阀引脚当前处于非激活状态
        // 设置10秒后复位水阀引脚
        GATE_ACTIVATING = T1COUNTER + 10 > 20 ? (T1COUNTER + 10) - 20 : T1COUNTER + 10;
        P13 = 1;  // 设置P1.3高电平
        P14 = 0;  // 设置P1.4低电平
        GATE_STATUS = 1;  // 更新水阀状态为打开
    }
}

// 关闭水阀函数
void CLOSEGATE() {
    if(P13 == 0 && P14 == 0) {  // 确保水阀引脚当前处于非激活状态
        // 设置10秒后复位水阀引脚
        GATE_ACTIVATING = T1COUNTER + 10 > 20 ? (T1COUNTER + 10) - 20 : T1COUNTER + 10;
        P13 = 0;  // 设置P1.3低电平
        P14 = 1;  // 设置P1.4高电平
        GATE_STATUS = 0;  // 更新水阀状态为关闭
    }
}

// 复位水阀引脚
void RESETGATE() {
    P13 = 0;  // 复位P1.3
    P14 = 0;  // 复位P1.4
}

// 水阀启动检查器
char doGateStartChecker(char early) {
    return GateStartChecker(DATETIME, GATE_START_TIME - early,
                          GATE_WORK_DURATION, GATE_WORK_ENABLE,
                          GATE_STATUS, GATE_WORK_COUNTDOWN);
}
/********************************Gate Setting***********************************/


/********************************FILP LED***********************************/
// void FilpLED() {
// 	if(LOW_POWER_MODE == 1 || LOW_POWER_MODE == 2){
// 		LED = 1;
// 	}
// 	if (LED)
// 	{
// 		LED = 0;
// 	}
// 	else
// 	{
// 		LED = 1;
// 	}
// }
/********************************FILP LED***********************************/
void GetWakeupTimerCount() {
    unsigned int tmpf32k;

    tmpf32k = *F32K & 0xffff;  // 读取32K时钟值
    WAKEUPTIMERCOUNT = ((35675 * 10) / 16) - 1;  // 计算唤醒定时器计数值

    // 设置唤醒定时器低字节和高字节
    TMPWKTCL = WAKEUPTIMERCOUNT & 0x00ff;
    TMPWKTCH = 0;
    TMPWKTCH += (WAKEUPTIMERCOUNT >> 8) & 0x00ff;
    TMPWKTCH += 0x80;  // 设置唤醒定时器使能位
}

void ReadBat() {
    unsigned int crc;
    BATRESP[5] = DISPLAY_NUM;  // 设置电池电量值

    // 计算CRC校验
    crc = MODBUS_CRC16(BATRESP, 6);
    BATRESP[6] = crc & 0x00ff;       // CRC低字节
    BATRESP[7] = (crc >> 8) & 0x00ff; // CRC高字节

    sendingFlag2 = 1;  // 设置发送标志
}

void LoadConfig() {
    char tmpDatetime[4];

    // 从ROM读取水阀工作使能设置
    IapGet2byteData(IapWorkEnable2bAddr, &GATE_WORK_ENABLE);

    // 读取水阀开始时间
    IapReadWithSize(IapStartDateTime4bAddr, 4, tmpDatetime);
    GATE_START_TIME = tmpDatetime[0] & 0xff;
    GATE_START_TIME <<= 8;
    GATE_START_TIME += tmpDatetime[1] & 0xff;
    GATE_START_TIME <<= 8;
    GATE_START_TIME += tmpDatetime[2] & 0xff;
    GATE_START_TIME <<= 8;
    GATE_START_TIME += tmpDatetime[3] & 0xff;

    // 读取水阀工作持续时间
    IapGet2byteData(IapWorkDuration2bAddr, &GATE_WORK_DURATION);

    // 读取休眠和唤醒超时设置
    IapGet2byteData(IapSleepTimeout2bAddr, &TMPSCTIMEOUT);
    IapGet2byteData(IapWakeupInterval2bAddr, &TMPWKTIMEOUT);

    // 计算实际超时值
    if(TMPSCTIMEOUT >= 60) {
        SCTIMEOUT = (TMPSCTIMEOUT * 100) / 6;
    }
    if(TMPWKTIMEOUT >= 60) {
        WKTIMEOUT = TMPWKTIMEOUT / 10;
    }

    // 重置计数器
    SCCOUNTER = 0;
    WKCOUNTER = 0;
}
/********************************UART1 Setting***********************************/
//char busy1;
//char wptr1;
//char xdata buffer1[16];
//char sendingFlag1 = 0;
//char headok = 0;

//void Uart1Isr() interrupt 4
//{
//	if (TI)
//	{
//		TI = 0;
//		busy1 = 0;
//	}
//	if (RI)
//	{
//		RI = 0;
////		buffer1[wptr1++] = SBUF;
////		wptr1 &= 0x0f;
//
//		buffer1[wptr1] = SBUF;
//		if(wptr1 == 0){
//			if(buffer1[wptr1] != 0x29){
//				wptr1 = 0;
//				return;
//			}
//		}
//		else if(wptr1 == 1 && buffer1[wptr1] == 0x29 && buffer1[wptr1 - 1 ] == 0x29) {
//			headok = 1;
//		}
//		else if(headok == 1 && wptr1 < 7) {
//			//readbyte ++;
//			if(wptr1 == 6 && buffer1[wptr1] == 0x0D) {
//				if(FIRST_INIT == 0 ){
//					FIRST_INIT = 1;
//					CALCULATE_COUNT = 0;
//				}
//				ReadBat(buffer1, 7);
//				wptr1 = 0;
//				headok = 0;
//				return;
//			}
//		}
//		else {
//			if(wptr1 == 7) {
//				ReadBat(buffer1, 8);
//			}
//			wptr1 = 0;
//			headok = 0;
//			return;
//		}
//		wptr1++;
//		wptr1 &= 0x0f;
//		SCCOUNTER = 0;
//		LOW_POWER_MODE = 0;
//	}
//}
//void Uart1Init()
//{
//    SCON = 0x50 ;
//    T2L = BRT;
//    T2H = BRT >> 8;
//    wptr1 = 0x00;
//    busy1 = 0;
//    ES = 1;
//    EA = 1;
//}
//void Uart1Send(char dat)
//{
//    while (busy1);
//    busy1 = 1;
//    SBUF = dat;
//}
//void Uart1SendStr(char *p, unsigned char size)
//{
//	unsigned char i = 0;
//    while (i < size)
//    {
//        Uart1Send(p[i]);
//		i++;
//    }
//}
/********************************UART1 Setting***********************************/

/********************************UART2 Setting***********************************/
// UART2状态变量
char busy2;                  // 发送忙标志
char wptr2;                  // 写指针
char xdata buffer2[16] = {0}; // 接收缓冲区
char headok = 0;             // 头校验通过标志
unsigned int xdata crcData;  // CRC数据
unsigned int xdata crcRes;   // CRC结果

// UART2中断服务程序
void Uart2Isr() interrupt 8 {
    if (S2CON & 0x02) {  // 发送完成中断
        S2CON &= ~0x02;
        busy2 = 0;
    }
    if (S2CON & 0x01) {  // 接收中断
        S2CON &= ~0x01;

        // 重置休眠计数器
        SCCOUNTER = 0;
        LOW_POWER_MODE = 0;

        // 存储接收到的数据
        buffer2[wptr2] = S2BUF;

        // 协议解析
        if(wptr2 == 0) {  // 检查第一个字节
            if(buffer2[wptr2] != 0x29) {  // 不是协议头
                wptr2 = 0;
                return;
            }
        }
        else if(wptr2 == 1 && buffer2[wptr2] == 0x29 && buffer2[wptr2 - 1] == 0x29) {
            headok = 1;  // 协议头校验通过
        }
        else if(headok == 1 && wptr2 < 16) {
            if(buffer2[wptr2] == 0x0D) {  // 收到结束标志
                // 计算CRC校验
                crcData = 0;
                crcData += buffer2[wptr2 - 1] & 0xff;
                crcData = crcData << 8;
                crcData += buffer2[wptr2 - 2] & 0xff;
                crcRes = MODBUS_CRC16(buffer2, wptr2 - 2);

                if(crcData == crcRes) {  // CRC校验通过
                    switch(buffer2[2]) {  // 根据指令类型处理
                        case 1: {  // 查询电池电量
                            if(FIRST_INIT == 0) {
                                FIRST_INIT = 1;
                                CALCULATE_COUNT = 0;
                            }
                            ReadBat();
                        } break;

                        case 4: {  // 进入低功耗模式
                            SCCOUNTER = SCTIMEOUT - 100;
                            sendingFlag2 = 2;
                        } break;

                        case 5: {  // 门控控制
                            if(buffer2[5] == 0x01) {
                                OPENGATE();
                            }
                            else if(buffer2[5] == 0x02) {
                                CLOSEGATE();
                            }
                        } break;

                        case 6: {  // 设置时钟
                            DATETIME = buffer2[5] & 0xff;
                            DATETIME <<= 8;
                            DATETIME += buffer2[6] & 0xff;
                            DATETIME <<= 8;
                            DATETIME += buffer2[7] & 0xff;
                            DATETIME <<= 8;
                            DATETIME += buffer2[8] & 0xff;
                            sendingFlag2 = 3;
                        } break;

                        case 7: {  // 设置唤醒和休眠选项
                            // 指令格式: 0x29 0x29 0x07 [LEN_H] [LEN_L] [MODE] [TIME_H] [TIME_L] [CRC] 0x0D
                            // MODE: 0=间隔,1=整点,2=半点,3=15分钟
                            // TIME: 仅MODE=0时有效(间隔秒数)
                            setWakeupSleepOptions(buffer2, 5);  //擦除烧写
                            LoadConfig();
                            sendingFlag2 = 3;
                        } break;

                        case 8: {  // 设置工作选项
                            setWorkingOptions(buffer2, 5);  //擦除烧写
                            LoadConfig();
                            sendingFlag2 = 3;
                        } break;

                        /* 新增手动模式控制指令 */
                        case 0x0B:{     // 开启手动模式
                            MANUAL_MODE_ENABLED = 1;
                            SCCOUNTER = 0;  // 重置休眠计数器
                            LOW_POWER_MODE = 0; // 退出低功耗
                            sendingFlag2 = 4; // 设置手动模式响应标志
                        } break;

                        case 0x0C:{     // 关闭手动模式
                            MANUAL_MODE_ENABLED = 0;
                            sendingFlag2 = 4; // 设置手动模式响应标志
                        } break;
                    }
                }
                wptr2 = 0;
                headok = 0;
                return;
            }
        }
        else {
            wptr2 = 0;
            headok = 0;
            return;
        }
        wptr2++;
        wptr2 &= 0x0f;  // 循环缓冲区
    }
}

// UART2初始化
void Uart2Init() {
    S2CON = 0x10;  // 设置串口模式
    T2L = BRT;     // 设置波特率低字节
    T2H = BRT >> 8; // 设置波特率高字节
    wptr2 = 0x00;  // 初始化写指针
    busy2 = 0;     // 初始化忙标志
    IE2 = 1;       // 使能UART2中断
    EA = 1;        // 全局中断使能
}

// UART2发送单个字符
void Uart2Send(char dat) {
    while (busy2);  // 等待发送完成
    busy2 = 1;      // 设置忙标志
    S2BUF = dat;    // 发送数据
}

// UART2发送字符串
void Uart2SendStr(char *p, unsigned char size) {
    unsigned char i = 0;
    while (i < size) {
        Uart2Send(p[i]);  // 逐个发送字符
        i++;
    }
}
/********************************UART2 Setting***********************************/

/********************************ADC Setting***********************************/
//unsigned int *BGV ;
unsigned int VCC;  // 电源电压
unsigned int RES;  // ADC结果

// 启动ADC
void ADCStart() {
    ADCCFG = 0x2f;      // 配置ADC
    ADC_CONTR = 0x8E;   // 使能ADC
    ADCTIM &= 0xE0;     // 清除采样时间设置
    ADCTIM |= 0x0F;     // 设置采样时间
}

// 停止ADC
void ADCStop() {
    ADC_CONTR &= ~0x80; // 禁用ADC
}

// 读取ADC值
int ADCRead() {
    int tmpres;

    ADC_CONTR |= 0x40;  // 启动AD转换
    _nop_();
    _nop_();
    while (!(ADC_CONTR & 0x20));  // 等待转换完成
    ADC_CONTR &= ~0x20;           // 清除完成标志
    tmpres = (ADC_RES << 8) | ADC_RESL;  // 读取ADC结果

    return tmpres;
}

// 计算电池百分比
void batteryPrecentage2(int Voltage) {
    char ischarge;
    float tmp = Voltage * 1.0f / 1000.0f;
    int tmpInt = 0;

    CALCULATE_COUNT++;
    ischarge = 0;

    // 根据初始化状态和计算次数处理电压值
    if(FIRST_INIT == 1) {
        if(CALCULATE_COUNT > 2) {
            FIRST_INIT = 0;
            CALCULATE_COUNT = 0;
            tmpInt = VOLTAGE_NUM;
            DISPLAY_NUM = tmpInt;
            return;
        }
    }
    else if(FIRST_INIT == 2) {
        if(CALCULATE_COUNT > CRCOUNT) {
            FIRST_INIT = 0;
            CALCULATE_COUNT = 0;
            tmpInt = VOLTAGE_NUM / (CRCOUNT - 1);
            if(tmpInt > DISPLAY_NUM || DISPLAY_NUM > 100) {
                DISPLAY_NUM = tmpInt;
            } else if(tmpInt < DISPLAY_NUM && DISPLAY_NUM - tmpInt > 5) {
                DISPLAY_NUM = tmpInt;
            }
            return;
        }
    }
    else {
        if(CALCULATE_COUNT > 21) {
            CALCULATE_COUNT = 0;
            tmpInt = (VOLTAGE_NUM / 20);
            if(tmpInt > DISPLAY_NUM && tmpInt - DISPLAY_NUM > 5) {
                DISPLAY_NUM = tmpInt;
            }
            if(tmpInt < DISPLAY_NUM) {
                DISPLAY_NUM = tmpInt;
            }
            return;
        }
    }

    if(CALCULATE_COUNT == 1) {
        VOLTAGE_NUM = 0;
        return;
    }

    // 根据电压值计算电池百分比
    if(Voltage >= 4100) {
        if(ischarge == 0) {
            VOLTAGE_NUM += 100;
        } else {
            if(Voltage >= 4120) {
                VOLTAGE_NUM += 100;
            } else {
                VOLTAGE_NUM += 95;
            }
        }
    }
    else if(Voltage >= 3960 && Voltage < 4100) {
        VOLTAGE_NUM += (unsigned int)((1 - ((tmp - 4.14f) / -0.8f )) * 100.0f);
    }
    else if (Voltage >= 3220 && Voltage < 3960) {
        VOLTAGE_NUM += (unsigned int)((1 - ((tmp - 4.193f) / -1.043f)) * 100.0f);
    }
    else if (Voltage >= 3020 && Voltage < 3220) {
        VOLTAGE_NUM += 5;
    }
    else if (Voltage < 3020) {
        VOLTAGE_NUM += 1;
    }
}

// 读取电压
void ReadVoltage(char update) {
    int i;
    int tmpvcc;
    unsigned int vs;

    vs = 0;
    RES = 0;
    VCC = 0;

    ADCStart();
    ADCRead();  // 丢弃第一次读数
    ADCRead();  // 丢弃第二次读数

    // 读取8次并取平均值
    for(i=0; i < 8; i++) {
        RES += ADCRead();
    }
    RES >>= 3;

    // 计算电压值
    tmpvcc = (int)(5030L * RES / 1024L);
    if(update == 1) {
        // 转换为12V电压
        vs = (unsigned int)((tmpvcc * (25L + 10L)) / 10L);
        VCC = vs / 3;
        batteryPrecentage2(VCC);  // 计算电池百分比
    }

    ADCStop();
}

/********************************Timer0 Setting***********************************/
int T0COUNTER;   // 定时器0计数器
int RVCOUNTER;   // 电压读取计数器
int LPCOUNTER;   // 低功耗计数器

// 定时器0初始化
void timer0Init() {
    TH0 = 0x27;  // 设置定时器高字节
    TL0 = 0xff;  // 设置定时器低字节
}

// 启动定时器0
void timer0Start() {
    T0COUNTER = 0;
    RVCOUNTER = 0;
    SCCOUNTER = 0;
    ET0 = 1;    // 使能定时器0中断
    EA = 1;     // 全局中断使能
    TR0 = 1;    // 启动定时器0
}

// 定时器0中断服务程序
void timer0_ISR() interrupt 1 {
    T0COUNTER++;
    SCCOUNTER++;
    LPCOUNTER++;

    // 每50次中断(约50ms)执行一次
    if(T0COUNTER > 50) {
        T0COUNTER = 0;
        RVCOUNTER++;

        // 首次初始化处理
        if(FIRST_INIT == 1) {
            RVCOUNTER = RVREADTIMEOUT;
        }

        // 电压读取逻辑
        if(RVCOUNTER == RVREADTIMEOUT) {
            FIRST_INIT = 2;
            CALCULATE_COUNT = 0;
            ReadVoltage(1);  // 读取电压并更新
        }
        else if(RVCOUNTER > RVREADTIMEOUT && RVCOUNTER <= RVREADTIMEOUT + CRCOUNT) {
            ReadVoltage(1);  // 读取电压并更新
        }
        else if(RVCOUNTER > RVREADTIMEOUT + CRCOUNT) {
            RVCOUNTER = 0;
        }

        // 低功耗处理
        if(LPCOUNTER > 25) {
            LPCOUNTER = 0;
            if(LOW_POWER_MODE == 0) {
                P32 = 1;  // 设置P3.2高电平
                P33 = 1;  // 设置P3.3高电平
            }
            if(LOW_POWER_MODE == 3) {
                P32 = 1;  // 设置P3.2高电平
            }
        }

        // 门控打开时禁用休眠(RTC_V1.1.0注释)
        // if(GATE_STATUS == 1) {
        //     SCCOUNTER = 0;
        // }

        /* 修改休眠计数器逻辑 */
        if(!MANUAL_MODE_ENABLED) { // 仅非手动模式增加计数器
            SCCOUNTER++;
        }
        else {
            SCCOUNTER = 0; // 手动模式下始终保持活跃
        }

        // 修改休眠超时处理（原：(SCCOUNTER > SCTIMEOUT)）
        if(SCCOUNTER > SCTIMEOUT && !MANUAL_MODE_ENABLED) {
            if(LOW_POWER_MODE == 0 || LOW_POWER_MODE == 3) {
                LOW_POWER_MODE = 1;
                P32 = 0;  // 设置P3.2低电平
                P33 = 0;  // 设置P3.3低电平
                LED = 1;  // 关闭LED
                RVCOUNTER = 0;
                ADCStop();  // 停止ADC
            }

            if(SCCOUNTER > (SCTIMEOUT+25)) {
                SCCOUNTER = 0;
                TR0 = 0;  // 停止定时器0
                TR1 = 0;  // 停止定时器1
                WKTCL = TMPWKTCL;  // 设置唤醒定时器低字节
                WKTCH = TMPWKTCH;  // 设置唤醒定时器高字节
                LOW_POWER_MODE = 2;  // 进入深度低功耗模式
            }
        }
    }
}

/**
 * Genta_New
 * @brief 检查是否需要唤醒(根据当前模式)
 * @param currentTime 当前时间(时间戳)
 * @return 1:需要唤醒, 0:不需要
 */
char checkWakeupTime(long currentTime) {
    // 从时间戳中提取分钟数(0-59)
    char current_min = (currentTime / 60) % 60;

    // 如果分钟数未变化，不需要重复唤醒
    if(current_min == LAST_WAKE_MIN) {
        return 0;
    }

    // 根据当前模式检查唤醒条件
    switch(WAKE_MODE) {
        case WAKE_INTERVAL:  // 间隔唤醒(保持原有逻辑)
            return (WKCOUNTER >= WKTIMEOUT);

        case WAKE_HOURLY:    // 整点唤醒(00分)
            if(current_min == 0) {
                LAST_WAKE_MIN = current_min;
                return 1;
            }
            break;

        case WAKE_HALF_HOUR: // 半点唤醒(30分)
            if(current_min == 30) {
                LAST_WAKE_MIN = current_min;
                return 1;
            }
            break;

        case WAKE_QUARTER_HOUR: // 每15分钟唤醒(00,15,30,45分)
            if(current_min % 15 == 0) {
                LAST_WAKE_MIN = current_min;
                return 1;
            }
            break;
    }

    return 0;
}



/********************************Timer1 Setting***********************************/

// 定时器1初始化
void timer1Init() {
    TH1 = 0x54;  // 设置定时器高字节(50ms)
    TL1 = 0xA3;  // 设置定时器低字节
}

// 启动定时器1
void timer1Start() {
    ET1 = 1;  // 使能定时器1中断
    EA = 1;   // 全局中断使能
    TR1 = 1;  // 启动定时器1
}

// 定时器1中断服务程序
void timer1_ISR() interrupt 3 {
    T1COUNTER++;
    T1MSGCOUNTER++;

    // 每20次中断(约1秒)执行一次
    if(T1COUNTER > 20) {
        T1COUNTER = 0;
        WDT_CONTR = 0x37;  // 重置看门狗
        DATETIME++;        // 系统时间递增

        if(LOW_POWER_MODE == 0 || LOW_POWER_MODE == 3) {
            RTC_INT_Flag = 1;  // 设置RTC中断标志
        }
    }

    // 门控激活时间检查
    if(T1COUNTER == GATE_ACTIVATING) {
        GATE_ACTIVATING = -1;
        RESETGATE();  // 复位门控引脚
    }

    // 每200次中断(约10秒)发送一次心跳消息
    if(T1MSGCOUNTER > 200) {
        T1MSGCOUNTER = 0;
        sendingFlag2 = 3;  // 设置心跳发送标志
    }
}

/********************************Timer1 Setting***********************************/


// todo MAIN
void main() {
    char datetimeCal;   //新增
    char gateStartCheckRes;
    int tmpCRC;
    char xdata str[30];

    // 校准掉电唤醒定时器计数(10秒唤醒一次)
    F32K = (int idata *)0x0f8;
    GetWakeupTimerCount();

    // 启动看门狗
    WDT_CONTR = 0x27;

    // 系统初始化
    FIRST_INIT = 1;
    LOW_POWER_MODE = 0;
    WKCOUNTER = 0;
    WKTIMEOUT = 18;
    SCTIMEOUT = 3000;  // 正式刷机改回5000
    RTC_INT_Flag = 0;
    datetimeCal = 0;

    // 开启额外RAM
    AUXR |= 0x17;
    AUXR &= ~0x02;

    // 设置P37上拉电阻
    P3PU &= 0x7F;

    // 设置引脚模式
    P1M1 = 0x00;
    P1M0 |= 0x04;  // P1.2(LED)输出
    P1M1 &= 0xF7;
    P1M0 |= 0x08;  // P1.3输出
    P1M1 &= 0xEF;
    P1M0 |= 0x10;  // P1.4输出
    P3M1 &= 0xFB;
    P3M0 |= 0x04;  // P3.2输出
    P3M1 &= 0xF7;
    P3M0 |= 0x08;  // P3.3输出
    P3M1 |= 0x40;
    P3M0 &= 0xBF;  // P3.6高阻输入

    // 初始化引脚状态
    P32 = 1;
    P33 = 1;

    // 从ROM加载配置
    LoadConfig();

    // 初始化显示数值
    DISPLAY_NUM = 0xbc;
    VOLTAGE_NUM = 0;
    CALCULATE_COUNT = 0;

    // 初始化UART2
    Uart2Init();

    // 初始化定时器
    TMOD |= 0x00;
    timer0Init();
    timer0Start();
    timer1Init();
    timer1Start();

    // 初始化系统时间和水阀状态
    DATETIME = 1744287444;
    RESETGATE();
    CLOSEGATE();

    // 主循环
    while (1) {
        // 处理发送标志
        if(sendingFlag2 == 1) {  // 发送电池响应
            Uart2SendStr(BATRESP, 9);
            sendingFlag2 = 0;
        }
        if(sendingFlag2 == 2) {  // 发送休眠响应
            Uart2SendStr(SLEEPRESP, 8);
            sendingFlag2 = 0;
        }

        if(sendingFlag2 == 3) {  // 发送心跳消息
            // 构建心跳消息
            HARTBEATMSG[0] = 0x29;
            HARTBEATMSG[1] = 0x29;
            HARTBEATMSG[2] = 0x0A;  // 指令编号
            HARTBEATMSG[3] = 0x00;  // 数据长度高字节
            HARTBEATMSG[4] = 0x10;  // 数据长度低字节
            longToByteArray(DATETIME, HARTBEATMSG, 5);  // 当前时间
            HARTBEATMSG[9] = GATE_STATUS & 0xff;  // 阀门状态
            HARTBEATMSG[10] = GATE_WORK_ENABLE & 0xff;  // 任务开关
            longToByteArray(GATE_START_TIME, HARTBEATMSG, 11);  // 任务开始时间
            HARTBEATMSG[15] = (GATE_WORK_DURATION >> 8) & 0xff;  // 任务持续时间高字节
            HARTBEATMSG[16] = (char)(GATE_WORK_DURATION & 0x00ff) & 0xff;  // 任务持续时间低字节
            HARTBEATMSG[17] = (TMPWKTIMEOUT >> 8) & 0xff;  // 休眠唤醒间隔高字节
            HARTBEATMSG[18] = (char)(TMPWKTIMEOUT & 0x00ff) & 0xff;  // 休眠唤醒间隔低字节
            HARTBEATMSG[19] = (TMPSCTIMEOUT >> 8) & 0xff;  // 休眠空虚等待时间高字节
            HARTBEATMSG[20] = (char)(TMPSCTIMEOUT & 0x00ff) & 0xff;  // 休眠空虚等待时间低字节
            HARTBEATMSG[21] = (GATE_WORK_COUNTDOWN >> 8) & 0xff;  // 倒计时高字节
            HARTBEATMSG[22] = (char)(GATE_WORK_COUNTDOWN & 0x00ff) & 0xff;  // 倒计时低字节

            // Genta_New 增加心跳中现十当前唤醒模式
            HARTBEATMSG[23] = WAKE_MODE;  // 当前唤醒模式
            HARTBEATMSG[24] = MANUAL_MODE_ENABLED; // 新增手动模式状态位    手动模式状态 0-关闭 1-开启

            // 计算CRC(CRC高低位没反转)
            tmpCRC = MODBUS_CRC16(HARTBEATMSG, 25);
            HARTBEATMSG[25] = (tmpCRC >> 8) & 0xff;  // CRC高字节
            HARTBEATMSG[26] = (char)(tmpCRC & 0x00ff) & 0xff;  // CRC低字节
            HARTBEATMSG[27] = 0x0D;  // 结束符

            Uart2SendStr(HARTBEATMSG, 28);  // 发送心跳消息
            sendingFlag2 = 0;
        }
        /* 新增手动模式响应处理 */
        if(sendingFlag2 == 4) {
            unsigned int crc;
            manual_resp[5] = MANUAL_MODE_ENABLED;  // 更新模式状态
            crc = MODBUS_CRC16(manual_resp, 6);
            manual_resp[6] = crc & 0xFF;
            manual_resp[7] = (crc >> 8) & 0xFF;
            Uart2SendStr(manual_resp, 9);
            sendingFlag2 = 0;
        }

        // 低功耗模式处理   Genta_New
        if(LOW_POWER_MODE == 2) {
            PCON |= 0x02;  // MCU进入掉电模式
            WKCOUNTER++;
            WDT_CONTR = 0x37;  // 重置看门狗
            DATETIME += 10;    // 系统时间递增(唤醒间隔周期10s)
			datetimeCal ++;
			if(datetimeCal > 5) {
				datetimeCal = 0;
				DATETIME = DATETIME + 2;
			}
            // // 检查门控启动条件(提前20秒)
            // if(doGateStartChecker(20) == 1) {
            //     WKCOUNTER = 0;
            //     SCCOUNTER = 0;
            //     LOW_POWER_MODE = 0;
            //     LPCOUNTER = 25;
            //     T1COUNTER = 0;
            //     T1MSGCOUNTER = 0;
            //     TR0 = 1;  // 启动定时器0
            //     TR1 = 1;  // 启动定时器1
            // }

            // 替换原有的WKCOUNTER检查
            if(checkWakeupTime(DATETIME)) {
                WKCOUNTER = 0;
                SCCOUNTER = 0;
                LOW_POWER_MODE = 0;
                LPCOUNTER = 25;
                T1COUNTER = 0;
                T1MSGCOUNTER = 0;
                TR0 = 1;  // 启动定时器0
                TR1 = 1;  // 启动定时器1
            }

            // 唤醒超时处理
            if(WKCOUNTER >= WKTIMEOUT) {
                WKCOUNTER = 0;
                LOW_POWER_MODE = 3;
                SCCOUNTER = (SCTIMEOUT / 2);
                TR0 = 1;  // 启动定时器0
                TR1 = 1;  // 启动定时器1
            }
        }

        // RTC中断处理
            if(RTC_INT_Flag == 1) {
                RTC_INT_Flag = 0;
                /* 仅在非手动模式执行自动控制 */
                if(!MANUAL_MODE_ENABLED) {
                // 水阀倒计时处理
                if(GATE_WORK_COUNTDOWN > 0) {
                    GATE_WORK_COUNTDOWN--;
                }

                // 检查水阀启动条件
                gateStartCheckRes = doGateStartChecker(0);
                if(gateStartCheckRes != 0) {
                    SCCOUNTER = 0;
                    LOW_POWER_MODE = 0;
                }

                // 根据检查结果处理水阀
                switch(gateStartCheckRes) {
                    case 1: {  // 需要开水阀
                        GATE_WORK_COUNTDOWN = GATE_WORK_DURATION;
                        OPENGATE();
                        sendingFlag2 = 3;  // 发送心跳消息
                    } break;

                    case 3: {  // 需要关水阀
                        CLOSEGATE();
                        if(DATETIME - (GATE_START_TIME) < 600) {
                            GATE_START_TIME -= 600;
                        }
                        sendingFlag2 = 3;  // 发送心跳消息
                    } break;
                }
            }
        }
    }
}
/*******************************工作机制说明*******************************
1. 指令协议：
   - 开启手动：29 29 0B 00 00 [CRC16] 0D           29 29 0B 00 00 00 58 0d
   - 关闭手动：29 29 0C 00 00 [CRC16] 0D           29 29 0C 00 00 B1 99 0d
***********************************************************************/
