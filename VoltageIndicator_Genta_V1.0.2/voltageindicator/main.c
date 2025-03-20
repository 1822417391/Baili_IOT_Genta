// main.c

#include <stc8h.h>
#include <intrins.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdint.h"
#include "common.h"
#include "digitaltube.h"

#define RVREADTIMEOUT 		200
#define CRCOUNT				6
#define ReadFailedLimit 	3

//Low_Power_Mode defines
#define TOTAL_WAKEUPS 37  // 需要唤醒 37 次，每次 16 秒，总计约 600 秒
#define TIMEOUT_MS 120000            // 2 分钟 = 120,000 毫秒

bit uart1_received = 0; //uart1接收标志位
sbit LED = P1^2;
// Uart1 defines
#define BRT (65536 - MAIN_Fosc / 9600 / 4)


//void delay_ms(unsigned int ms)
//{
//    unsigned int i;
//    do
//    {
//        i = MAIN_Fosc / 13000;
//        while (--i)
//            ; // 14T per loop
//    } while (--ms);
//}

/********************************INIT Setting***********************************/
char 					FIRST_INIT;
unsigned int 			VOLTAGE_NUM;
unsigned int 			CALCULATE_COUNT;
unsigned long xdata		VCC_NUM;
unsigned int 			VCC_RESULT;
unsigned int 			VCC_READNUM;
/********************************INIT Setting***********************************/

/********************************LOW_POWER_MODE Setting***********************************/
unsigned int wakeup_count = 0;	//低功耗循环计时器
unsigned long timer_count = 0;	//闲时2分钟计时器
unsigned int LOW_POWER_MODE;	//0-唤醒模式 1-断开太阳能的低功耗模式 2-太阳能低功耗模式
/********************************188 Setting***********************************/
unsigned int DISPLAY_NUM;
unsigned int TTL_DISPLAY_NUM;
//char DISPLAY_STATUS;
/********************************188 Setting***********************************/

/********************************FILP LED***********************************/
void FilpLED() {
	if (LED)
	{
		LED = 0;
	}
	else
	{
		LED = 1;
	}
}

/********************************FILP LED***********************************/

/********************************UART1 Setting***********************************/
char busy1;
char wptr1;
char xdata buffer1[16];
char sendingFlag1 = 0;
char headok = 0;
char xdata BATRESP[8] = {0x29, 0x29, 0x01, 0x00, 0x01, 0x00, 0x00, 0x0D};

void ReadBat(char *buf, char size) {
	char i;
	char xor;
		
	if(size == 7){
		i = 3;
		xor = 0;

		BATRESP[5] = DISPLAY_NUM;
		
		while(i < 6) {
			xor = xor ^ BATRESP[i];
			i++;
		}
		BATRESP[6] = xor;
		FilpLED();
		sendingFlag1 = 2;
	}
}

void Uart1Isr() interrupt 4
{
	if (TI)
	{
		TI = 0;
		busy1 = 0;
	}
	if (RI)
	{
		RI = 0;
//		buffer1[wptr1++] = SBUF;
//		wptr1 &= 0x0f;
		uart1_received = 1;  // 设置串口接收标志

		buffer1[wptr1] = SBUF;
		if(wptr1 == 0){
			if(buffer1[wptr1] != 0x29){
				wptr1 = 0;
				return;
			}
		}
		else if(wptr1 == 1 && buffer1[wptr1] == 0x29 && buffer1[wptr1 - 1 ] == 0x29) {
			headok = 1;
		}
		else if(headok == 1 && wptr1 < 7) {
			//readbyte ++;
			if(wptr1 == 6 && buffer1[wptr1] == 0x0D) {
				ReadBat(buffer1, 7);
				wptr1 = 0;
				headok = 0;
				return;
			}
		}
		else {
			if(wptr1 == 7) {
				ReadBat(buffer1, 8);
			}
			wptr1 = 0;
			headok = 0;
			return;
		}
		wptr1++;
		wptr1 &= 0x0f;
		
	}
}
void Uart1Init()
{
    SCON = 0x50 ;
    T2L = BRT;
    T2H = BRT >> 8;  
    wptr1 = 0x00;
    busy1 = 0;
	TR1 = 1;            // 启动 Timer1
    ES = 1;
    EA = 1;
}
void Uart1Send(char dat)
{
    while (busy1);
    busy1 = 1;
    SBUF = dat;
}
void Uart1SendStr(char *p, unsigned char size)
{
	unsigned char i = 0;
    while (i < size)
    {
        Uart1Send(p[i]);
		i++;
    }
}
/********************************UART1 Setting***********************************/

/********************************UART2 Setting***********************************/
//char busy2;
//char wptr2;
//char xdata buffer2[16] = {0};
//char sendingFlag2 = 0;
////char ReadOK = 0;
////char ReadFailedLimit = 3;
//char headok = 0;
//void ReadBat(char *buf, char size)
//{
//	char i = 3;
//	char xor = 0;
//	
//	while(i < size - 2) {
//		xor = xor ^ buf[i];
//		i++;
//	}
//	if(xor == buf[size - 2]){
//		if(size == 7){
//			FilpLED();
//			sendingFlag2 = 2;
//		}
//	}
//}

//void Uart2Isr() interrupt 8
//{
//	if (S2CON & 0x02)
//	{
//		S2CON &= ~0x02;
//		busy2 = 0;
//	}
//	if (S2CON & 0x01)
//	{
//		S2CON &= ~0x01;
//		buffer2[wptr2] = S2BUF;
//		if(wptr2 == 0){
//			if(buffer2[wptr2] != 0x29){
//				wptr2 = 0;
//				return;
//			}
//		}
//		else if(wptr2 == 1 && buffer2[wptr2] == 0x29 && buffer2[wptr2 - 1 ] == 0x29) {
//			headok = 1;
//		}
//		else if(headok == 1 && wptr2 < 7) {
//			//readbyte ++;
//			if(wptr2 == 6 && buffer2[wptr2] == 0x0D) {
//				ReadBat(buffer2, 7);
//				wptr2 = 0;
//				headok = 0;
//				return;
//			}
//		}
//		else {
//			if(wptr2 == 7) {
//				ReadBat(buffer2, 8);
//			}
//			wptr2 = 0;
//			headok = 0;
//			return;
//		}
//		wptr2++;
//		wptr2 &= 0x0f;
//	}
//}
//void Uart2Init()
//{
//    S2CON = 0x10 ;
//    T2L = BRT;
//    T2H = BRT >> 8;  
//    wptr2 = 0x00;
//    busy2 = 0;
//    IE2 = 1;
//    EA = 1;
//}
//void Uart2Send(char dat)
//{
//    while (busy2);
//    busy2 = 1;
//    S2BUF = dat;
//}
//void Uart2SendBat(char *p, unsigned char size)
//{
//	unsigned char i = 0;
//    while (i < size)
//    {
//        Uart2Send(p[i]);
//		i++;
//    }
//}
/********************************UART2 Setting***********************************/

/********************************ADC Setting***********************************/
//unsigned int *BGV ;
unsigned int VCC;
unsigned int RES;

char FULL = 100;

void ADCStart()
{
    ADCCFG = 0x2f;
    ADC_CONTR = 0x8E;
	//Set ADC sample time
	ADCTIM &= 0xE0;
	ADCTIM |= 0x0F;
}

void ADCStop()
{
    ADC_CONTR &= ~0x80;
}

int ADCRead()
{
    int tmpres;

    ADC_CONTR |= 0x40; // 启动 AD 转换
	_nop_(); 
	_nop_(); 
    while (!(ADC_CONTR & 0x20));     // 查询 ADC 完成标志
    ADC_CONTR &= ~0x20;              // 清完成标�?
    tmpres = (ADC_RES << 8) | ADC_RESL; // 读取 ADC 结果

    return tmpres;
}

void batteryPrecentage2(int Voltage) {
	char ischarge;
	float tmp = Voltage * 1.0f / 1000.0f;
	int tmpInt = 0;
	
	VCC_READNUM = (tmp * 1000);
	
	CALCULATE_COUNT ++;
	ischarge = 0;
	
	if(FIRST_INIT == 1) {
		if(CALCULATE_COUNT > 2){
			FIRST_INIT = 0;
			CALCULATE_COUNT = 0;
			tmpInt = VOLTAGE_NUM;
			VCC_RESULT = VCC_NUM & 0xffff;
			//VCC_RESULT = VCC_RESULT / 10;
			DISPLAY_NUM = tmpInt;
//			sendingFlag = 1; 
			return ;
		}
	}
	else if(FIRST_INIT == 2) {
		//if(CALCULATE_COUNT > 4 ){
		if(CALCULATE_COUNT > CRCOUNT ){
			FIRST_INIT = 0;
			CALCULATE_COUNT = 0;
			tmpInt = VOLTAGE_NUM / (CRCOUNT - 1);
			if(tmpInt > DISPLAY_NUM || DISPLAY_NUM > 100) {
				//DISPLAY_NUM = tmpInt;
				DISPLAY_NUM = tmpInt;
				VCC_RESULT = VCC_NUM /  (CRCOUNT - 1);
				//VCC_RESULT = VCC_RESULT / 10;
			} else if(tmpInt < DISPLAY_NUM && DISPLAY_NUM - tmpInt > 5) {
				DISPLAY_NUM = tmpInt;
				VCC_RESULT = VCC_NUM /  (CRCOUNT - 1);
				//VCC_RESULT = VCC_RESULT / 10;
			}
			return ;
		}
	}
	else {
		if(CALCULATE_COUNT > 21){
			CALCULATE_COUNT = 0;
			tmpInt = (VOLTAGE_NUM / 20);
			if(tmpInt > DISPLAY_NUM && tmpInt - DISPLAY_NUM > 5) {
				//DISPLAY_NUM = tmpInt;
				DISPLAY_NUM = tmpInt;
				VCC_RESULT = VCC_NUM / 20;
				//VCC_RESULT = VCC_RESULT / 10;
			}
			if(tmpInt < DISPLAY_NUM ) {
				DISPLAY_NUM = tmpInt;
				VCC_RESULT = VCC_NUM / 20;
				//VCC_RESULT = VCC_RESULT / 10;
			}
			//VOLTAGE_NUM = 0;
			//sendingFlag = 1;
			return ;
		}
	}
	if(CALCULATE_COUNT == 1){
		VOLTAGE_NUM = 0;
		VCC_NUM = 0;
		return;
		
	}
	VCC_NUM += (Voltage & 0xffff);
	if(Voltage >= 4100){
		if(ischarge == 0){
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
		//y=-2.152x+4.48
		VOLTAGE_NUM += (unsigned int)((1 - ((tmp - 4.14f) / -0.8f )) * 100.0f);
	} 
	else if (Voltage >= 3220 && Voltage < 3960) {
		//y=-0.943x + 4.193
		VOLTAGE_NUM += (unsigned int)((1 - ((tmp - 4.193f) / -1.043f)) * 100.0f);
	} 
	else if  (Voltage >= 3020 && Voltage < 3220){
		//y=-4.373x + 7.443
		VOLTAGE_NUM += 5;
	} 
	else if (Voltage < 3020) {
		VOLTAGE_NUM += 1;
	}
}



void ReadVoltage(char update) {
	int i;
	int tmpvcc;
	unsigned int vs;
	
	vs = 0;
    RES = 0;
    VCC = 0;
	
    ADCStart();
    ADCRead();
    ADCRead();
    for(i=0; i < 8; i++) {
        RES += ADCRead();
    }
    RES >>= 3;
		
	tmpvcc = (int)(5030L * RES / 1024L);
	if(update == 1) {
		//转换回去12v电压
		vs = (unsigned int)((tmpvcc * (25L + 10L)) / 10L);
		VCC = vs / 3;
		batteryPrecentage2(VCC);
	}
	//DISPLAY_NUM=VCC/100;
    ADCStop();
}

/********************************ADC Setting***********************************/

/********************************Timer0 Setting***********************************/
int T0COUNTER;
int RVCOUNTER;
int SCCOUNTER;
void timer0Init()
{
	AUXR |= 0x80;       // Timer0 使用 1T 模式（Fosc）
    TMOD &= 0xF0;       // 设置 Timer0 为 16 位自动重载模式
    TH0 = 0x4c;
    TL0 = 0xf7;
}
void timer0Start()
{
    T0COUNTER = 0;
	RVCOUNTER = 0;
    ET0 = 1;
    EA = 1;
    TR0 = 1;
}
void timer0_ISR() interrupt 1
{
    T0COUNTER ++;
	timer_count += 4;      // 计时器,满足2分钟进入睡眠
	//T0Counter > 200
    if(T0COUNTER > 50) {
		//char tmpP32;
        TR0 = 0;
        T0COUNTER = 0;
        RVCOUNTER ++ ;

		if(FIRST_INIT == 1) {
			RVCOUNTER = RVREADTIMEOUT;
		}
		if(RVCOUNTER == RVREADTIMEOUT){
			//RVCOUNTER = 0;
			//P3PU &= 0x7F;
			P37 = 0;
			FIRST_INIT = 2;
			CALCULATE_COUNT = 0;
			ReadVoltage(1);
		}
		else if(RVCOUNTER > RVREADTIMEOUT && RVCOUNTER <= RVREADTIMEOUT + CRCOUNT) {
			ReadVoltage(1);
		} 
		else if(RVCOUNTER > RVREADTIMEOUT + CRCOUNT) {
			RVCOUNTER = 0;
			//P3PU |= 0x80;
			P37 = 1;
		} else {
			//ReadVoltage(0);
		}
		
		TR0 = 1;
		FilpLED();
		//sendingFlag1 = 1;
    }

}
/********************************Timer0 Setting***********************************/

/********************************Timer1 Setting***********************************/
//int T1COUNTER;
//char xdata BATQUERY[9] = {0x29, 0x29, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0D};
//void timer1Init()
//{
//    TH1 = 0xEE;
//    TL1 = 0x19;
//}
//void timer1Start()
//{
//    ET1 = 1;
//    EA = 1;
//    TR1 = 1;
//}
//void timer1_ISR() interrupt 3
//{	
//    char tmp;
//	char tmpP37;
//	unsigned int num = 0;
//	tmp = P32;
//	tmpP37 = P37;
//	if(FIRST_INIT == 1 || tmp != 0) {
//		num = DISPLAY_NUM;
//	}
//	else if(DISPLAY_STATUS == 0) {
//		num = DISPLAY_NUM;
//	} else {
//		num = TTL_DISPLAY_NUM;
//	}
//	
//	
//	if(tmp != 0 && tmpP37 == 1) {
//		T1COUNTER++;
//		if(T1COUNTER < 200) {		
//			Display_tube(num);
//		} else if(T1COUNTER >= 200 && T1COUNTER < 400) {
//			Set_AllPin_INPUT();
//			//T1COUNTER = 0;
//		} else {
//			T1COUNTER = 0;
//		}
//	} else {
//		Display_tube(num);
//		T1COUNTER = 0;
//	}
//	//Display_tube(1);

//}
/********************************Timer1 Setting***********************************/

/********************************LOW_POWER_MODE Setting***********************************/
void LowPowerModeInit(){
    // 初始化掉电唤醒定时器
    IRC32KCR = 0x80;  // 启用内部 32KHz 低速 IRC
    _nop_();
    _nop_(); 
    _nop_();
    WKTCL = 0xF8;    // 低 8 位：248
    WKTCH = 0x7F;    // 高 7 位：127，组合为 32760（16.38 秒）
    WKTCH |= 0x80;   // 使能掉电唤醒定时器
}

void LowPowerProc(){
    TR0 = 0;          // 停止 Timer0
    TR1 = 0;          // 停止 Timer1
	ES = 0;
	ADCStop();        // 停止 ADC
    P32 = 0;
	P33 = 0;
	P37 = 1;
	LED = 0;
	PCON |= 0x02; //进入有太阳能充电低功耗模式
}

void SolarPowerProc(){
    TR0 = 1;          // 开启 Timer0
    TR1 = 0;          // 停止 Timer1
	ES = 0;
    ADCStart();        // 停止 ADC
	P32 = 0;
	P33 = 0;
	P37 = 0;
	LED = 0;
	// PCON |= 0x02;//进入断太阳能低功耗模式
	}

// todo MAIN
void main()
{
	char xdata str[30];
	
	FIRST_INIT = 1;
	//DISPLAY_STATUS = 0;
		
    //open extra ram
	AUXR |= 0x17;
    AUXR &= ~0x02;
	
	//Setting P37 pull up resistor
	P3PU &= 0x7F;

    //Setting LED pin
    P1M1 = 0x00;
    P1M0 |= 0x04;
	//P3.2 set output,M1=0, M0=1
	P3M1 &= 0xFB;
	P3M0 |= 0x04;
	//P3.3 set output,M1=0, M0=1
	P3M1 &= 0xF7;
	P3M0 |= 0x08;
	//P3.6 High resistance input: M1=1, M0=0
	P3M1 |= 0x40;
	P3M0 &= 0xBF;
	//P3.7 open drain M1=1, M0=1, dual M1=0, M0=0
	P3M1 |= 0x80;
	P3M0 |= 0x80;
	//P3M1 &= 0x7F;
	//P3M0 &= 0x7F;
	
	P37 = 1;
	
	P32 = 1;
	
	P33 = 1;

//    BGV = (idata int *)0xef;
	
	VCC_NUM = 0;
	VCC_RESULT = 0;
    DISPLAY_NUM = 0xbc;
	TTL_DISPLAY_NUM = 0xbc;
	VOLTAGE_NUM = 0;
	CALCULATE_COUNT = 0;

    //Uart2Init();
	Uart1Init();

    TMOD |= 0x00;
    timer0Init();
    timer0Start();
	LowPowerModeInit();
    //timer1Init();
    //timer1Start();

    //ReadVoltage();

    LED = 1;
    // uint8_t count = 0;
	
		
    while (1)
    {
		if(sendingFlag1 == 1){
			int tmpP32;
			int tmpP37;
			sendingFlag1 = 0;
			tmpP32 = P32;
			tmpP37 = P37;
			
			sprintf(str, "2.5.x_1.0.8,%d,%d,%d,%d,%d\r\n\0", VCC, VCC_READNUM, DISPLAY_NUM, tmpP37, RVCOUNTER & 0x00ff);
			Uart1SendStr(str, strlen(str));
			//FilpLED();
		}
		
		if(sendingFlag1 == 2){
			Uart1SendStr(BATRESP, 8);
			sendingFlag1 = 0;
		}
		
		//低功耗逻辑判断
		// 等待 2 分钟无串口数据，进入有太阳能低功耗模式
        if (timer_count >= TIMEOUT_MS && !uart1_received) {
            while (wakeup_count < TOTAL_WAKEUPS) {
                LowPowerProc();    // 2分钟进入有太阳能低功耗模式

                // 唤醒后检查原因
                if (uart1_received) {        // 如果是串口唤醒
                    timer_count = 0;        // 重置计时器
                    uart1_received = 0;      // 清除串口标志
                    wakeup_count = 0;       // 重置掉电唤醒计数器
                    break;                  // 退出掉电循环，重新等待 2 分钟
                } else {                    // 如果是掉电唤醒定时器触发
                    wakeup_count++;         // 继续累加掉电唤醒次数
                }
            }

            // 完成 10 分钟休眠后执行任务
            if (wakeup_count >= TOTAL_WAKEUPS) {
                SolarPowerProc();	// 进入太阳能休眠模式
                sprintf(str, "2.5.x_1.0.8,%d,%d,%d,%d,%d\r\n\0", VCC, VCC_READNUM, DISPLAY_NUM, P37, RVCOUNTER & 0x00ff);
                Uart1SendStr(str, strlen(str));	//发出当前剩余电量数据
				_nop_(); 			//等待串口发送完毕
				_nop_(); 
				_nop_(); 
				_nop_();
                wakeup_count = 0;           // 重置计数器，继续下一次 10 分钟休眠
                timer_count = 0;            // 重置 2 分钟计时器
            }
        }

        // 如果收到串口数据，重置计时器
        if (uart1_received) {
            timer_count = 0;    // 重置计时器
            uart1_received = 0;  // 清除串口接收标志
        }
    }


//		if(T0COUNTER <100){
//			sprintf(str, "%d %d %d %d\r\n\0", DISPLAY_NUM, VCC, *BGV, RES);
//			UartSendStr(str);
//			delay_ms(5000);
//		} else {
//				delay_ms(500);
//		}		  	
    }
