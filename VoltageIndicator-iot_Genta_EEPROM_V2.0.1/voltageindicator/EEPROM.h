#ifndef __EEPROM_H__
#define __EEPROM_H__

//EEPROM1-4K，8个扇区起始地址定义
#define SECTOR0 	0x0000
#define SECTOR1 	0x0200
#define SECTOR2 	0x0400
#define SECTOR3 	0x0600
#define SECTOR4 	0x0800
#define SECTOR5 	0x0A00
#define SECTOR6 	0x0C00
#define SECTOR7 	0x0E00

char IapRead(int addr);
void IapProgram(int addr, char dat);
void IapErase(int addr);
void IapIdle();
	
#endif