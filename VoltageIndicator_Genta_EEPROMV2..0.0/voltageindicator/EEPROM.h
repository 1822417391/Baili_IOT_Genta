#ifndef __EEPROM_H__
#define __EEPROM_H__

//EEPROM1-4K，8个扇区起始地址定义
#define SECTOR0 	0x000000
#define SECTOR1 	0x000200
#define SECTOR2 	0x000400
#define SECTOR3 	0x000600
#define SECTOR4 	0x000800
#define SECTOR5 	0x000A00
#define SECTOR6 	0x000C00
#define SECTOR7 	0x000E00

char IapRead(unsigned long addr);
void IapProgram(unsigned long addr, char dat);
void IapErase(unsigned long addr);
void IapIdle();
	
#endif