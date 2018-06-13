/******************************************************************************

 @file       HAL_I2C.c

 @brief This file contains the I2C C function prototypes

 Maker/Author - Markel T. Robregado
 Date: June 15, 2018

*****************************************************************************/

#ifndef HAL_I2C_H_
#define HAL_I2C_H_

#include <stdbool.h>

void I2C_Init(void);
bool I2C_write8(unsigned char pointer, unsigned char writeByte);
bool I2C_write16(unsigned char pointer, unsigned short writeWord);
bool I2C_read8(unsigned char pointer, char *result);
bool I2C_read16(unsigned char pointer, short *result);
bool I2C_read32(unsigned char pointer, char * result);
void I2C_setslave(unsigned short slaveAdr);


#endif /* HAL_I2C_H_ */
