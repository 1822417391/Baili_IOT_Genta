//使用串口发送EEPROM数据
//测试工作频率为11.0592MHz
#include "stc8h.h"
#include "intrins.h"
#define       FOSC                 11059200UL
#define       BRT                    (65536-(FOSC / 115200+2) / 4)
#define EEPROM_SECTOR_ADDR 0x0000 // 合法EEPROM地址
//加2操作是为了让Keil编译器
                                                                                                                //自动实现四舍五入运算
void UartInit()
{
         SCON= 0x5a;
         T2L= BRT;
         T2H= BRT >> 8;
         AUXR= 0x15;
}
void UartSend(char dat)
{
         while(!TI);
         TI= 0;
         SBUF= dat;
}
void IapIdle()
{
         IAP_CONTR= 0;//关闭IAP功能
         IAP_CMD= 0;//清除命令寄存器
         IAP_TRIG= 0;//清除触发寄存器
         IAP_ADDRH= 0x80;//将地址设置到非IAP区域
         IAP_ADDRL= 0;
}
char IapRead(int addr)
{
         chardat;
         IAP_CONTR= 0x80;//使能IAP
         IAP_TPS= 11;//设置等待参数11MHz
         IAP_CMD= 1;//设置IAP读命令
         IAP_ADDRL= addr;//设置IAP低地址
         IAP_ADDRH= addr >> 8;//设置IAP高地址
         IAP_TRIG= 0x5a;//写触发命令(0x5a)
         IAP_TRIG= 0xa5;//写触发命令(0xa5)
         _nop_();
         dat= IAP_DATA;//读IAP数据
         IapIdle();//关闭IAP功能
         returndat;
}
void IapProgram(int addr, char dat)
{
         IAP_CONTR= 0x80;//使能IAP
         IAP_TPS= 11;//设置等待参数11MHz
         IAP_CMD= 2;//设置IAP写命令
         IAP_ADDRL= addr;//设置IAP低地址
         IAP_ADDRH= addr >> 8;//设置IAP高地址
         IAP_DATA= dat;//写IAP数据
         IAP_TRIG= 0x5a;//写触发命令(0x5a)
         IAP_TRIG= 0xa5;//写触发命令(0xa5)
         _nop_();
         IapIdle();//关闭IAP功能
}
void IapErase(int addr)
{
         IAP_CONTR= 0x80;//使能IAP
         IAP_TPS= 11;//设置等待参数11MHz
         IAP_CMD= 3;//设置IAP擦除命令
         IAP_ADDRL= addr;//设置IAP低地址
         IAP_ADDRH= addr >> 8;//设置IAP高地址
         IAP_TRIG= 0x5a;//写触发命令(0x5a)
         IAP_TRIG= 0xa5;//写触发命令(0xa5)
         _nop_();//
         IapIdle();//关闭IAP功能
}
void main()
{
         P_SW2|= 0x80;                                                                            //使能访问XFR
         P0M0= 0x00;
         P0M1= 0x00;
         P1M0= 0x00;
         P1M1= 0x00;
         P2M0= 0x00;
         P2M1= 0x00;
         P3M0= 0x00;
         P3M1= 0x00;
         P4M0= 0x00;
         P4M1= 0x00;
         P5M0= 0x00;
         P5M1= 0x00;
         UartInit();
         IapErase(EEPROM_SECTOR_ADDR);  // 合法EEPROM地址
         UartSend(IapRead(EEPROM_SECTOR_ADDR));
         IapProgram(EEPROM_SECTOR_ADDR,0x12);
         UartSend(IapRead(EEPROM_SECTOR_ADDR));
         while(1);
}


/* **************************方式2************************** */
 
/***********************************************************
*@fuction	:write_rom
*@brief		: 通过函数处理，使其可以直接存入16位数据
*@param		:--u16 address：数据存入的地址   u16 Data_Address：需要存入的变量
*@return	:void
*@author	:--xptx
*@date		:2022-11-19
***********************************************************/
 
// void write_rom(unsigned int address, unsigned int Data_Address) //16位 数据写入
// {
//     unsigned char x[2];
//     if(Data_Address != read_rom(address)) //数据发生更改
//     {
//         x[0] = 0xff & Data_Address;  //留下低8位
//         x[1] = Data_Address >> 8;    //留下高8位
//         EEPROM_write_n(address, &x, 2); //写入新的数据
//     }
// }
 
// /***********************************************************
// *@fuction	:read_rom
// *@brief		:读取EEPROM里面的16位数据
// *@param		:--u16 address：数据存放地址
// *@return	:返回16位数据
// *@author	:--xptx
// *@date		:2022-11-20
// ***********************************************************/
 
// unsigned int read_rom(unsigned int address)
// {
//     unsigned char  xdata   dat[2];        //EEPROM操作缓冲
//     EEPROM_read_n(address, dat, 2);
//     return (dat[0] + (dat[1] << 8)); //返回读取的值
//}