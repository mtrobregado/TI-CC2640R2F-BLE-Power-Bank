/******************************************************************************

 @file       HAL_I2C.c

 @brief This file contains the I2C C API's

 Maker/Author - Markel T. Robregado
 Date: June 15, 2018

*****************************************************************************/

#include <stdint.h>
#include <stddef.h>
//#include <unistd.h>
#include <stdbool.h>


/* Driver Header files */
#include <ti/drivers/I2C.h>

/* Example/Board Header files */
#include "Board.h"
#include "HAL_I2C.h"

uint8_t         txBuffer[3];
uint8_t         rxBuffer[2];
I2C_Handle      i2c;
I2C_Params      i2cParams;
I2C_Transaction i2cTransaction;

/*****************************************************************************
 * @brief  Configures I2C
 * @param  none
 * @return none
 ******************************************************************************/
void I2C_Init(void)
{
    I2C_init();

    /* Create I2C for usage */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c = I2C_open(Board_I2C0, &i2cParams);
    if (i2c == NULL)
    {
        while (1);
    }

    return;
}

/*****************************************************************************
 * @brief  Writes data to the sensor
 * @param  pointer  Address of register you want to modify
 * @param  writeByte Data to be written to the specified register
 * @return none
 ******************************************************************************/
bool I2C_write8 (unsigned char pointer, unsigned char writeByte)
{
    txBuffer[0] = pointer;
    txBuffer[1] = writeByte;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 2;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if(!(I2C_transfer(i2c, &i2cTransaction)))
    {
        return 0;
    }

    return 1;
}

/*****************************************************************************
 * @brief  Writes data to the sensor
 * @param  pointer  Address of register you want to modify
 * @param  writeWord Data to be written to the specified register
 * @return none
 ******************************************************************************/
bool I2C_write16 (unsigned char pointer, unsigned short writeWord)
{
    txBuffer[0] = pointer;
    txBuffer[1] = writeWord & 0Xff;
    txBuffer[2] = writeWord >> 8;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 3;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if(!(I2C_transfer(i2c, &i2cTransaction)))
    {
        return 0;
    }

    return 1;
}

bool I2C_read8(unsigned char pointer, char * result)
{
    txBuffer[0] = pointer;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = result;
    i2cTransaction.readCount = 1;

    if(!(I2C_transfer(i2c, &i2cTransaction)))
    {
        return 0;
    }

    return 1;
}

bool I2C_read32(unsigned char pointer, char * result)
{
    txBuffer[0] = pointer;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = result;
    i2cTransaction.readCount = 32;

    if(!(I2C_transfer(i2c, &i2cTransaction)))
    {
        return 0;
    }

    return 1;
}

/***************************************************************************//**
 * @brief  Reads data from the sensor
 * @param  pointer Address of register to read from
 * @return Register contents
 ******************************************************************************/
bool I2C_read16(unsigned char pointer, short * result)
{

    txBuffer[0] = pointer;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 2;

    if(!(I2C_transfer(i2c, &i2cTransaction)))
    {
        return 0;
    }

    *result = (rxBuffer[1] << 8) | rxBuffer[0];

    return 1;

}

void I2C_setslave(unsigned short slaveAdr)
{
    /* Specify slave address for I2C */
    i2cTransaction.slaveAddress = slaveAdr;
    return;
}
