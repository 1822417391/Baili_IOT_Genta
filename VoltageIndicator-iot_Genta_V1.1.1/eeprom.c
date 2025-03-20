//ʹ�ô��ڷ���EEPROM����
//���Թ���Ƶ��Ϊ11.0592MHz
#include "stc8h.h"
#include "intrins.h"
#define       FOSC                 11059200UL
#define       BRT                    (65536-(FOSC / 115200+2) / 4)
#define EEPROM_SECTOR_ADDR 0x0000 // �Ϸ�EEPROM��ַ
//��2������Ϊ����Keil������
                                                                                                                //�Զ�ʵ��������������
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
         IAP_CONTR= 0;//�ر�IAP����
         IAP_CMD= 0;//�������Ĵ���
         IAP_TRIG= 0;//��������Ĵ���
         IAP_ADDRH= 0x80;//����ַ���õ���IAP����
         IAP_ADDRL= 0;
}
char IapRead(int addr)
{
         chardat;
         IAP_CONTR= 0x80;//ʹ��IAP
         IAP_TPS= 11;//���õȴ�����11MHz
         IAP_CMD= 1;//����IAP������
         IAP_ADDRL= addr;//����IAP�͵�ַ
         IAP_ADDRH= addr >> 8;//����IAP�ߵ�ַ
         IAP_TRIG= 0x5a;//д��������(0x5a)
         IAP_TRIG= 0xa5;//д��������(0xa5)
         _nop_();
         dat= IAP_DATA;//��IAP����
         IapIdle();//�ر�IAP����
         returndat;
}
void IapProgram(int addr, char dat)
{
         IAP_CONTR= 0x80;//ʹ��IAP
         IAP_TPS= 11;//���õȴ�����11MHz
         IAP_CMD= 2;//����IAPд����
         IAP_ADDRL= addr;//����IAP�͵�ַ
         IAP_ADDRH= addr >> 8;//����IAP�ߵ�ַ
         IAP_DATA= dat;//дIAP����
         IAP_TRIG= 0x5a;//д��������(0x5a)
         IAP_TRIG= 0xa5;//д��������(0xa5)
         _nop_();
         IapIdle();//�ر�IAP����
}
void IapErase(int addr)
{
         IAP_CONTR= 0x80;//ʹ��IAP
         IAP_TPS= 11;//���õȴ�����11MHz
         IAP_CMD= 3;//����IAP��������
         IAP_ADDRL= addr;//����IAP�͵�ַ
         IAP_ADDRH= addr >> 8;//����IAP�ߵ�ַ
         IAP_TRIG= 0x5a;//д��������(0x5a)
         IAP_TRIG= 0xa5;//д��������(0xa5)
         _nop_();//
         IapIdle();//�ر�IAP����
}
void main()
{
         P_SW2|= 0x80;                                                                            //ʹ�ܷ���XFR
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
         IapErase(EEPROM_SECTOR_ADDR);  // �Ϸ�EEPROM��ַ
         UartSend(IapRead(EEPROM_SECTOR_ADDR));
         IapProgram(EEPROM_SECTOR_ADDR,0x12);
         UartSend(IapRead(EEPROM_SECTOR_ADDR));
         while(1);
}


/* **************************��ʽ2************************** */
 
/***********************************************************
*@fuction	:write_rom
*@brief		: ͨ����������ʹ�����ֱ�Ӵ���16λ����
*@param		:--u16 address�����ݴ���ĵ�ַ   u16 Data_Address����Ҫ����ı���
*@return	:void
*@author	:--xptx
*@date		:2022-11-19
***********************************************************/
 
// void write_rom(unsigned int address, unsigned int Data_Address) //16λ ����д��
// {
//     unsigned char x[2];
//     if(Data_Address != read_rom(address)) //���ݷ�������
//     {
//         x[0] = 0xff & Data_Address;  //���µ�8λ
//         x[1] = Data_Address >> 8;    //���¸�8λ
//         EEPROM_write_n(address, &x, 2); //д���µ�����
//     }
// }
 
// /***********************************************************
// *@fuction	:read_rom
// *@brief		:��ȡEEPROM�����16λ����
// *@param		:--u16 address�����ݴ�ŵ�ַ
// *@return	:����16λ����
// *@author	:--xptx
// *@date		:2022-11-20
// ***********************************************************/
 
// unsigned int read_rom(unsigned int address)
// {
//     unsigned char  xdata   dat[2];        //EEPROM��������
//     EEPROM_read_n(address, dat, 2);
//     return (dat[0] + (dat[1] << 8)); //���ض�ȡ��ֵ
//}