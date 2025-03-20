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
#include "EEPROM.h"

#define RVREADTIMEOUT 		200
#define CRCOUNT				2
#define ReadFailedLimit 	3

sbit LED = P1^2;
// Uart1 defines
#define BRT (65536 - MAIN_Fosc / 9600 / 4)


void delay_ms(unsigned int ms)
{
   unsigned int i;
   do
   {
       i = MAIN_Fosc / 13000;
       while (--i)
           ; // 14T per loop
   } while (--ms);
}

/********************************INIT Setting***********************************/
char 					FIRST_INIT;
unsigned int 			VOLTAGE_NUM;
unsigned int 			CALCULATE_COUNT;
unsigned long xdata		VCC_NUM;
unsigned int 			VCC_RESULT;
unsigned int 			VCC_READNUM;
char					LOW_POWER_MODE;
int 					SCCOUNTER;
int						SCTIMEOUT = 500;	//Õý³£5000£¬²âÊÔ500
int						WKCOUNTER;
char xdata 				BATRESP[8] = {0x29, 0x29, 0x01, 0x00, 0x01, 0x00, 0x00, 0x0D};
char 					sendingFlag2 = 0;
char 					sendingFlag1 = 0;
/********************************INIT Setting***********************************/

/********************************188 Setting***********************************/
unsigned int DISPLAY_NUM;
unsigned int TTL_DISPLAY_NUM;
unsigned char eepromTestData = 0x64;

//char DISPLAY_STATUS;
/********************************188 Setting***********************************/

/********************************FILP LED***********************************/
void FilpLED() {
	if(LOW_POWER_MODE == 1 || LOW_POWER_MODE == 2){
		LED = 1;
	}
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
char headok = 0;


void ReadBat(char size) {
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
		sendingFlag1 = 1;
	}
}

void Uart1Isr() interrupt 4
{
    int s;
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
				if(buffer1[2] == 0x04){
					SCCOUNTER = SCTIMEOUT - 100;
				}
				else if(FIRST_INIT == 0 ){
					FIRST_INIT = 1;
					CALCULATE_COUNT = 0;
				}
				//²Á³ý
				IapErase(SECTOR0);
				//delay_ms(10);
				//Ð´ÈëEEPROM
				for (s = 0; s < 4; s++) { // Ð´Èë4×Ö½Ú
					IapProgram(SECTOR0 + s, buffer1[2 + s]);
				}
				ReadBat(7);
				wptr1 = 0;
				headok = 0;
				return;
			}
		}
		else {
			if(wptr1 == 7) {
				ReadBat(8);
			}
			wptr1 = 0;
			headok = 0;
			return;
		}
		wptr1++;
		wptr1 &= 0x0f;
		SCCOUNTER = 0;
        WKCOUNTER = 0;
		LOW_POWER_MODE = 0;
	}
}
void Uart1Init()
{
   SCON = 0x50 ;
   T2L = BRT;
   T2H = BRT >> 8;  
   wptr1 = 0x00;
   busy1 = 0;
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
// char busy2;
// char wptr2;
// char xdata buffer2[16] = {0};

// char headok = 0;


// void Uart2Isr() interrupt 8
// {
// 	if (S2CON & 0x02)
// 	{
// 		S2CON &= ~0x02;
// 		busy2 = 0;
// 	}
// 	if (S2CON & 0x01)
// 	{
// 		S2CON &= ~0x01;
// 		buffer2[wptr2] = S2BUF;
// 		if(wptr2 == 0){
// 			if(buffer2[wptr2] != 0x29){
// 				wptr2 = 0;
// 				return;
// 			}
// 		}
// 		else if(wptr2 == 1 && buffer2[wptr2] == 0x29 && buffer2[wptr2 - 1 ] == 0x29) {
// 			headok = 1;
// 		}
// 		else if(headok == 1 && wptr2 < 7) {
// 			//readbyte ++;
// 			if(wptr2 == 6 && buffer2[wptr2] == 0x0D) {
// 				if(FIRST_INIT == 0 ){
// 					FIRST_INIT = 1;
// 					CALCULATE_COUNT = 0;
// 				}
// 				if(buffer2[2] == 0x04){
// 					SCCOUNTER = SCTIMEOUT - 100;
// 				}
// 				ReadBat(7);
// 				wptr2 = 0;
// 				headok = 0;
// 				return;
// 			}
// 		}
// 		else {
// 			if(wptr2 == 7) {
// 				ReadBat(8);
// 			}
// 			wptr2 = 0;
// 			headok = 0;
// 			return;
// 		}
// 		wptr2++;
// 		wptr2 &= 0x0f;
// 		SCCOUNTER = 0;
//         WKCOUNTER = 0;
// 		LOW_POWER_MODE = 0;
// 	}
// }
// void Uart2Init()
// {
//     S2CON = 0x10 ;
//     T2L = BRT;
//     T2H = BRT >> 8;  
//     wptr2 = 0x00;
//     busy2 = 0;
//     IE2 = 1;
//     EA = 1;
// }
// void Uart2Send(char dat)
// {
//     while (busy2);
//     busy2 = 1;
//     S2BUF = dat;
// }
// void Uart2SendStr(char *p, unsigned char size)
// {
// 	unsigned char i = 0;
//     while (i < size)
//     {
//         Uart2Send(p[i]);
// 		i++;
//     }
// }
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

    ADC_CONTR |= 0x40; // å¯åŠ¨ AD è½¬æ¢
	_nop_(); 
	_nop_(); 
    while (!(ADC_CONTR & 0x20));     // æŸ¥è¯¢ ADC å®Œæˆæ ‡å¿—
    ADC_CONTR &= ~0x20;              // æ¸…å®Œæˆæ ‡å¿?
    tmpres = (ADC_RES << 8) | ADC_RESL; // è¯»å– ADC ç»“æžœ

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
		//è½¬æ¢å›žåŽ»12vç”µåŽ‹
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
int LPCOUNTER;
void timer0Init()
{
    TH0 = 0x4c;
    TL0 = 0xf7;
}
void timer0Start()
{
    T0COUNTER = 0;
	RVCOUNTER = 0;		//åŽŸï¼šRVCOUNTER = RVREADTIMEOUT
	SCCOUNTER = 0;
    ET0 = 1;
    EA = 1;
    TR0 = 1;
}
void timer0_ISR() interrupt 1
{
    T0COUNTER ++;
	SCCOUNTER ++;
	LPCOUNTER ++;
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
        sendingFlag1 = 2;
    }
	
	if(LPCOUNTER > 25){
		LPCOUNTER = 0;
		if(LOW_POWER_MODE == 0){
			P32 = 1;
			P33 = 1;
		}
		if(LOW_POWER_MODE == 3){
			//P32 = 1;
            FIRST_INIT = 1;
		}
	}
	
	if(SCCOUNTER > SCTIMEOUT){
		if(LOW_POWER_MODE == 0 || LOW_POWER_MODE == 3 || LOW_POWER_MODE == 4){
			LOW_POWER_MODE = 1;
			P32 = 0;
			P33 = 0;
			LED = 1;
            RVCOUNTER = 0;
            P37 = 1;  //ç¡®ä¿å¤ªé˜³èƒ½æ¿å……ç”µæ‰“å¼€
            ADCStop();
			//DISPLAY_NUM = 100;
		}
		if(SCCOUNTER > (SCTIMEOUT+50)){
			SCCOUNTER = 0;
			TR0=0;
			WKTCH = 0xFF;
			WKTCL = 0xFE;
			LOW_POWER_MODE = 2;
		}

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


// todo MAIN
void main()
{
	char xdata str[30];
    int i;
	//int readD;	//test
	FIRST_INIT = 1;
	LOW_POWER_MODE = 0;
	WKCOUNTER = 0;
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

    // Uart2Init();
	Uart1Init();

    TMOD |= 0x00;
    timer0Init();
    timer0Start();

    //timer1Init();
    //timer1Start();

    //ReadVoltage();

    LED = 1;
    // uint8_t count = 0;
	
		
    while (1)
    {
		if(sendingFlag1 == 2){
			
			//IapProgram(SECTOR0, 0x64);
			//delay_ms(10);
        	// ¶ÁÈ¡²¢ÑéÖ¤Ð´ÈëµÄ×Ö·û´®
        	char readStr[5] = {0};
        	for (i = 0; i < 4; i++) {
         	   readStr[i] = IapRead(SECTOR0 + i)&0x00ff;
        	}
        	sprintf(str, "EEPROM: %s\r\n", readStr);
        	Uart1SendStr(str, strlen(str));
        	sendingFlag1 = 0;
			//FilpLED();
		}
		
		if(sendingFlag1 == 1){
			Uart1SendStr(BATRESP, 8);
			sendingFlag1 = 0;
		}
		
		if(LOW_POWER_MODE == 2){
			// //¶Á³ö
			// sprintf(str, "EEPROM: %u\r\n", (unsigned char)IapRead(SECTOR0));
			// Uart1SendStr(str, strlen(str));
			// delay_ms(10);
			PCON |= 0x02;
			WKCOUNTER ++;
            
            SCCOUNTER = (SCTIMEOUT - 100);
            LOW_POWER_MODE = 4;
            TR0 = 1;
            
			if(WKCOUNTER > 225){
				WKCOUNTER = 0;
				LOW_POWER_MODE = 3;
				SCCOUNTER = (SCTIMEOUT - 2000);
				//TR0 = 1;
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
}