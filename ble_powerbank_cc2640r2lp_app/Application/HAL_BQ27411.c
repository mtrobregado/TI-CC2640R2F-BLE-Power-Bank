/******************************************************************************

 @file       HAL_BQ27411.c

 @brief This file contains the C API's to initialize and read/write the
        BQ27441 Fuel Gauge on the BOOSTXL-BATPAKMKII

 Maker/Author - Markel T. Robregado
 Date: June 15, 2018

*****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "HAL_BQ27441.h"
#include "HAL_I2C.h"

/*********************************************************************
 * @fn      BQ27441_initConfig
 *
 * @brief   Configures BQ27441 device properties.
 *
 * @param   None.
 *
 * @return  None.
 */
bool BQ27441_initConfig(void)
{
    short result = 0;
    char new_chksum = 0;
    char chksum = 0;

    // Start Steps to UNSEAL the BQ27441 Fuel Gauge
    BQ27441_write16(CONTROL, BQ27441_UNSEAL);
    BQ27441_write16(CONTROL, BQ27441_UNSEAL);
    // End Steps to UNSEAL the BQ27441 Fuel Gauge

    // Start Steps to verify DEVICE TYPE of BQ27441 Fuel Gauge
    if (!BQ27441_controlRead(DEVICE_TYPE, &result))
    {
        return 0;
    }

    /* Check if Device Type is expected */
    if (result != 0x0421)
    {
        return 0;
    }
    // End Steps to verify DEVICE TYPE of BQ27441 Fuel Gauge

    /* Start Steps to enter CONFIG UPDATE mode. */
    if (!BQ27441_control(SET_CFGUPDATE))
    {
        return 0;
    }

    result = 0;

    /* Check if CFGUPMODE bit is set in FLAGS */
    while(!(result & 0x0010))
    {
        if (!BQ27441_read16(FLAGS, &result))
        {
            return 0;
        }
    }

    /* End Steps to enter CONFIG UPDATE mode. */

    /* Start Steps to setup Block RAM Update. */
    /* Enable Block data memory control */
    if (!BQ27441_command(BLOCK_DATA_CONTROL, 0x00))
    {
        return 0;
    }

    /* Set the data class to be accessed */
    if (!BQ27441_command(DATA_CLASS, 0x52))
    {
        return 0;
    }

    /* Write the block offset loaction */
    if (!BQ27441_command(DATA_BLOCK, 0x00))
    {
        return 0;
    }
    /* End Steps to setup Block RAM Update. */

    delay(10000);

    do
    {

        /* Write new design capacity */
        if (!BQ27441_write16(0x4A, swapMSB_LSB(CONF_DESIGN_CAPACITY)))
        {
            return 0;
        }

        /* Write new design energy */
        if (!BQ27441_write16(0x4C, swapMSB_LSB(CONF_DESIGN_ENERGY)))
        {
            return 0;
        }

        /* Write new terminate voltage */
        if (!BQ27441_write16(0x50, swapMSB_LSB(CONF_TERMINATE_VOLTAGE)))
        {
            return 0;
        }

        /* Write new taper rate */
        if (!BQ27441_write16(0x5B, swapMSB_LSB(CONF_TAPER_RATE)))
        {
            return 0;
        }

        delay(10000);

        /* End Steps to compute the new Checksum. */

        new_chksum = computeBlockChecksum();

        delay(10000);

        /* Write new checksum */
        if (!BQ27441_command(BLOCK_DATA_CHECKSUM, new_chksum))
        {
            return 0;
        }

        delay(10000);

        /* Read Block Data Checksum */
        if (!BQ27441_readChecksum(&chksum))
        {
            return 0;
        }

        delay(10000);

    }
    while(new_chksum != chksum);
    /* End Steps to verify RAM Update completed correctly. */

    /* Start Steps to exit CONFIG Update Mode. */
    /* Clear CFGUPMODE bit */
    if (!BQ27441_control(SOFT_RESET))
    {
        return 0;
    }

    result = 0;
    /* Check if CFGUPMODE bit is cleared in FLAGS */
    while(result & 0x0010)
    {
        if (!BQ27441_read16(FLAGS, &result))
        {
            return 0;
        }
    }
    /* End Steps to exit CONFIG Update Mode. */

    BQ27441_control(BQ27441_SEAL);

    return 1;
}


/* Configures BQ27441 opconfig */
bool BQ27441_initOpConfig()
{
    short result = 0;

    char old_chksum = 0;
    char new_chksum = 0;
    //char tmp_chksum = 0;
    char chksum = 0;

    short old_opconfig= 0;
    short new_opconfig = 0x05F8;

    /* Instructs fuel gauge to enter CONFIG UPDATE mode. */
    if (!BQ27441_control(SET_CFGUPDATE))
    {
        return 0;
    }

    result = 0;

    /* Check if CFGUPMODE bit is set in FLAGS */
    while(!(result & 0x0010))
    {
        if (!BQ27441_read16(FLAGS, &result))
        {
            return 0;
        }
    }

    /* Enable Block data memory control */
    if (!BQ27441_command(BLOCK_DATA_CONTROL, 0x00))
    {
        return 0;
    }

    /* Set the data class to be accessed */
    if (!BQ27441_command(DATA_CLASS, 0x40))
    {
        return 0;
    }

    /* Write the block offset loaction */
    if (!BQ27441_command(DATA_BLOCK, 0x00))
    {
        return 0;
    }

    do
    {
        /* Read Block Data Checksum */
        if (!BQ27441_readChecksum(&old_chksum))
        {
            return 0;
        }

        /* Read old opconfig */
        if (!BQ27441_read16(0x40, &old_opconfig))
        {
            return 0;
        }

        /* Write new opconfig */
        if (!BQ27441_write16(0x40, swapMSB_LSB(new_opconfig)))
        {
            return 0;
        }

         /* Checksum calculation */
        new_chksum = computeBlockChecksum();

        /* Write new checksum */
        if (!BQ27441_command(BLOCK_DATA_CHECKSUM, new_chksum))
        {
            return 0;
        }

        /* Set the data class to be accessed */
        if (!BQ27441_command(DATA_CLASS, 0x52))
        {
            return 0;
        }

        /* Write the block offset loaction */
        if (!BQ27441_command(DATA_BLOCK, 0x00))
        {
            return 0;
        }

        /* Read Block Data Checksum */
        if (!BQ27441_readChecksum(&chksum))
        {
            return 0;
        }

    }
    while(new_chksum != chksum);

    /* Send SOFT_RESET control command */
    if (!BQ27441_control(SOFT_RESET))
    {
        return 0;
    }

    result = 0;
    /* Check if CFGUPMODE bit is cleared in FLAGS */
    while(result & 0x0010)
    {
        if (!BQ27441_read16(FLAGS, &result))
        {
            return 0;
        }
    }

    // Read the Operation Config
    short result16 = 0;
    if (!BQ27441_read16(OP_CONFIG, &result16))
    {
        return 0;
    }

    // Check if BIE is cleared in Operation Config
    if (result16 & 0x2000)
    {
        return 0;
    }

    return 1;
}

/* Send control subcommand */
bool BQ27441_control(short subcommand)
{

    /* Specify slave address for BQ27441 */
    I2C_setslave(BQ27441_SLAVE_ADDRESS);

    if (!I2C_write16(CONTROL, subcommand))
    {
        return 0;
    }

    return 1;
}


/* Send control subcommand then read from control command */
bool BQ27441_controlRead(short subcommand, short *result)
{
    if (!BQ27441_control(subcommand))
    {
        return 0;
    }

    if (!BQ27441_read16(CONTROL, result))
    {
        return 0;
    }

    return 1;
}

/* Send command */
bool BQ27441_command(unsigned char command, char data)
{
    /* Specify slave address for BQ27441 */
    I2C_setslave(BQ27441_SLAVE_ADDRESS);

    if (!I2C_write8(command, data))
    {
        return 0;
    }

    return 1;
}

/* Write word to address */
bool BQ27441_write16(short addr, short data)
{
    /* Specify slave address for BQ27441 */
    I2C_setslave(BQ27441_SLAVE_ADDRESS);

    if (!I2C_write16(addr, data))
    {
        return 0;
    }

    return 1;
}


/* Read from standard command*/
bool BQ27441_read16(short stdcommand, short *result)
{
    /* Specify slave address for BQ27441 */
    I2C_setslave(BQ27441_SLAVE_ADDRESS);

    if (!I2C_read16((unsigned char)stdcommand, result))
    {
        return 0;
    }

    return 1;
}


/* Read block data checksum */
bool BQ27441_readChecksum(char *result)
{

    /* Specify slave address for BQ27441 */
    I2C_setslave(BQ27441_SLAVE_ADDRESS);

    if (!I2C_read8(BLOCK_DATA_CHECKSUM, result))
    {
        return 0;
    }

    return 1;
}

// Read all 32 bytes of the loaded extended data and compute a
// checksum based on the values.
unsigned char computeBlockChecksum(void)
{
    char data[32];
    unsigned char csum = 0;
    int i = 0;

    I2C_read32(BLOCK_DATA, data);

    for (i=0; i<32; i++)
    {
        csum += data[i];
    }

    csum = 255 - csum;

    return csum;
}


/* Computes checksum for fuel gauge */
unsigned char computeCheckSum(unsigned char oldCheckSum, int oldData, int newData)
{
    unsigned char tmpCheckSum = 0xFF - oldCheckSum - ( unsigned char )oldData - ( unsigned char )( oldData >> 8 );
    unsigned char newCheckSum = 0xFF - (  tmpCheckSum + ( unsigned char )newData + ( unsigned char )( newData >> 8 ) );
    return newCheckSum;
}


/* Swaps the MSB and LSB of a word */
int swapMSB_LSB(int data)
{
    int tmp = ( unsigned char )data;
    tmp = tmp << 8;
    tmp += ( unsigned char )( data >> 8 );
    return tmp;
}

void delay(int x)
{
    int i = 0;

    for(i = 0; i < x; i++);
}

