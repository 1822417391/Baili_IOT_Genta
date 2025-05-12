#ifndef CRC_H
#define CRC_H

#define MODBUS_CRC16_POLY 0xA001


unsigned int MODBUS_CRC16(unsigned char *p, char n);

