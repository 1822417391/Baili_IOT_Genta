#include "crc.h"

unsigned int MODBUS_CRC16(unsigned char *p, char n) {
	unsigned char i;
	unsigned int crc16;
	crc16 = 0xffff; //预置 16 位 CRC 寄存器为 0xffff（即全为 1）
	do
	{
		crc16 ^= (unsigned int)*p; //把 8 位数据与 16 位 CRC 寄存器的低位相异或，把结果放于 CRC 寄存器
		for(i=0; i<8; i++) //8 位数据
		{
			//if(crc16 & 1) crc16 = (crc16 >> 1) ^ 0xA001; //如果最低位为 0，把 CRC 寄存器的内容右移一位
			if(crc16 & 1) crc16 = (crc16 >> 1) ^ MODBUS_CRC16_POLY;
			//(朝低位)，用 0 填补最高位，
			//再异或多项式 0xA001
			else crc16 >>= 1; //如果最低位为 0，把 CRC 寄存器的内容右移一位
			//(朝低位)，用 0 填补最高位
		}
		p++;
	}while(--n != 0);
	return (crc16);
}