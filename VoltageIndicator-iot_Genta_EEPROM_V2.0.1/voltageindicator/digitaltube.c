#include "digitaltube.h"
#include <reg51.h>
#include "stdint.h"

// setting io mode
sfr P1M0 = 0x92;
sfr P1M1 = 0x91;

sbit Pin1 = P1^3;
sbit Pin2 = P1^4;
sbit Pin3 = P1^5;
sbit Pin4 = P1^6;
sbit Pin5 = P1^7;



unsigned int xdata Segment[3][11] = {
    {0x0000, 0x0006, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, // 百位
    {0x0770, 0x0420, 0x0741, 0x0661, 0x0431, 0x0271, 0x0371, 0x0460, 0x0771, 0x0671, 0x0000}, // 十位
    {0xE888, 0x8080, 0xD808, 0xD880, 0xB080, 0x7880, 0x7888, 0x8880, 0xF888, 0xF880, 0x0000}  // 个位
};

/********************************LED Setting functions***********************************/
void PIN1_IN() {
    P1M1 |= 0x08;
    P1M0 &= ~0x08;
}
void PIN2_IN() {
    P1M1 |= 0x10;
    P1M0 &= ~0x10;
}
void PIN3_IN() {
    P1M1 |= 0x20;
    P1M0 &= ~0x20;
}
void PIN4_IN() {
    P1M1 |= 0x40;
    P1M0 &= ~0x40;
}
void PIN5_IN() {
    P1M1 |= 0x80;
    P1M0 &= ~0x80;
}

void PIN1_L() {
    P1M1 &= ~0x08;
    P1M0 |= 0x08;
    Pin1 = 0;
}
void PIN2_L() {
    P1M1 &= ~0x10;
    P1M0 |= 0x10;
    Pin2 = 0;
}
void PIN3_L() {
    P1M1 &= ~0x20;
    P1M0 |= 0x20;
    Pin3 = 0;
}
void PIN4_L() {
    P1M1 &= ~0x40;
    P1M0 |= 0x40;
    Pin4 = 0;
}
//void PIN5_L() {
//    P1M1 &= ~0x80;
//    P1M0 |= 0x80;
//    Pin5 = 0;
//}

void PIN1_H() {
    P1M1 &= ~0x08;
    P1M0 |= 0x08;
    Pin1 = 1;
}
void PIN2_H() {
    P1M1 &= ~0x10;
    P1M0 |= 0x10;
    Pin2 = 1;
}
void PIN3_H() {
    P1M1 &= ~0x20;
    P1M0 |= 0x20;
    Pin3 = 1;
}
void PIN4_H() {
    P1M1 &= ~0x40;
    P1M0 |= 0x40;
    Pin4 = 1;
}
void PIN5_H() {
    P1M1 &= ~0x80;
    P1M0 |= 0x80;
    Pin5 = 1;
}

/********************************LED Setting functions***********************************/
/********************************熄灭所有数码管***********************************/
void Set_AllPin_INPUT(void)
{
    PIN1_IN();
    PIN2_IN();
    PIN3_IN();
    PIN4_IN();
    PIN5_IN();
}

void Display_Scan1(void)
{
    PIN1_L();//拉低Pin1
    if(display_sram&0x8000)
        PIN2_H();
    if(display_sram&0x4000)
        PIN3_H();
    if(display_sram&0x2000)
        PIN4_H();
    if(display_sram&0x1000)
        PIN5_H();
}
void Display_Scan2(void)
{
    PIN2_L();
    if(display_sram&0x0800)
        PIN1_H();
    if(display_sram&0x0400)
        PIN3_H();
    if(display_sram&0x0200)
        PIN4_H();
    if(display_sram&0x0100)
        PIN5_H();
}
void Display_Scan3(void)
{
    PIN3_L();
    if(display_sram&0x0080)
        PIN1_H();
    if(display_sram&0x0040)
        PIN2_H();
    if(display_sram&0x0020)
        PIN4_H();
    if(display_sram&0x0010)
        PIN5_H();
}
void Display_Scan4(void)
{
    PIN4_L();
    if(display_sram&0x0008)
        PIN1_H();
    if(display_sram&0x0004)
        PIN2_H();
    if(display_sram&0x0002)
        PIN3_H();
    if(display_sram&0x0001)
        PIN5_H();
}


/****************************显示函数***********************************/

// 放在定时器中断函数，5ms运行一次，扫描一轮需要4*5ms=20ms，
// 也就是50Hz的刷新频率，如果感觉闪烁，可以4ms运行。
void Display_tube(unsigned char num)
{
    unsigned char ge, shi, bai;
    bai = num / 100;
    shi = (num - (bai * 100)) / 10;
    ge = (num - (bai * 100) - (shi * 10));

    Set_AllPin_INPUT();//消影作用
    display_sram=0;
    display_sram=Segment[0][bai] | Segment[1][shi] | Segment[2][ge];//显示百位，十位，个位
    switch(case_cnt)
    {
      case 0x00:Display_Scan1();case_cnt++;break;
      case 0x01:Display_Scan2();case_cnt++;break;
      case 0x02:Display_Scan3();case_cnt++;break;
      case 0x03:Display_Scan4();case_cnt=0;break;
      default:case_cnt=0;break;
    }  
}