#include "EEPROM.h"
#include <stc8h.h>
#include <intrins.h>

void IapIdle()
{
	IAP_CONTR = 0; 						// 关闭 IAP 功能
	IAP_CMD = 0; 							// 清除命令寄存器
	IAP_TRIG = 0; 						// 清除触发寄存器
	//IAP_ADDRE = 0x00;					// 清零最高地址寄存器
	IAP_ADDRH = 0x80;					// 清零高地址寄存器
	IAP_ADDRL = 0x00;					// 清零低地址寄存器
}

char IapRead(int addr)
{
	char dat;
	IAP_CONTR = 0x80; 				// 使能 IAP
	IAP_TPS = 12; 						// 设置等待参数 11MHz
	IAP_CMD = 1; 							// 设置 IAP 读命令
	IAP_ADDRH = addr >> 8; 			// 设置 IAP 高地址
	IAP_ADDRL = addr; 				// 设置 IAP 低地址
	//IAP_ADDRE = addr >> 16;		// 设置 IAP 最 高地址
	IAP_TRIG = 0x5a; 					// 写触发命令 (0x5a)
	IAP_TRIG = 0xa5; 					// 写触发命令 (0xa5)
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	dat = IAP_DATA; 					// 读 IAP 数据
	IapIdle(); 								// 关闭 IAP 功能
	return dat;
}
void IapProgram(int addr, char dat)
{
	IAP_CONTR = 0x80; 				// 使能 IAP
	IAP_TPS = 12; 						// 设置等待参数 11MHz
	IAP_CMD = 2;							// 设置 IAP 写命令
	IAP_ADDRH = addr >> 8; 			// 设置 IAP 高地址
	IAP_ADDRL = addr; 				// 设置 IAP 低地址
	//IAP_ADDRE = addr >> 16; 	// 设置 IAP 最 高地址
	IAP_DATA = dat; 					// 写 IAP 数据
	IAP_TRIG = 0x5a; 					// 写触发命令 (0x5a)
	IAP_TRIG = 0xa5; 					// 写触发命令 (0xa5)
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	IapIdle();								// 关闭 IAP 功能
}
void IapErase(int addr)
{
	IAP_CONTR = 0x80; 				// 使能 IAP
	IAP_TPS = 12; 						// 设置等待参数 11MHz
	IAP_CMD = 3; 							// 设置 IAP 擦除命令
	IAP_ADDRL = addr; 				// 设置 IAP 低地址
	IAP_ADDRH = addr >> 8; 		// 设置 IAP 高地址
	//IAP_ADDRE = addr >> 16;		// 设置 IAP 最 高地址
	IAP_TRIG = 0x5a; 					// 写触发命令 (0x5a)
	IAP_TRIG = 0xa5; 					// 写触发命令 (0xa5)
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	IapIdle(); 								// 关闭 IAP 功能
}

//-------------EEPROM写操作，在第10扇区，第0字节，写入0x10-----------
//IapProgram(SECTOR10,0x10);

	
//-------------EEPROM读操作，读取第3扇区，第20字节-------------------
//	IapRead(SECTOR3 + 20);
	
	
//-------------EEPROM擦除操作，擦除第3扇区---------------------------
//	IapErase(SECTOR3);	

//-------------修改第10扇区，第0字节数据-----------------------------
//	IapErase(SECTOR10);					// 先擦除第10扇区						
//	IapProgram(SECTOR10,0x11);	// 在第10扇区，第0字节，写入0x11