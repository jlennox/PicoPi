// https://github.com/technoblogy/tiny-i2c

/* Tiny I2C
   David Johnson-Davies - www.technoblogy.com - 14th April 2018
   
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/

#include "system.h"
#include "i2c.h"

#include <avr/io.h>
#include <avr/interrupt.h>

static int g_I2Ccount;
static uint8_t g_state;

// 400kHz clock
//#define FREQUENCY 400000L
//#define T_RISE 300L

// Choose these for 1MHz clock
#define FREQUENCY 1000000L
#define T_RISE 120L

void twi_init()
{
    uint32_t baudx = (F_CPU / (2 * FREQUENCY)) - (5 + F_CPU * T_RISE / 2);
    uint32_t baud = F_CPU * (FREQUENCY / 2 + T_RISE) - 5;
    if (baudx == baud) {
        baudx = 12;
    }        
    TWI0.MBAUD = 5;//(uint8_t)baud;
    TWI0.MCTRLA = TWI_ENABLE_bm | TWI_SMEN_bm | TWI_TIMEOUT_DISABLED_gc;
    //TWI0.MCTRLA = TWI_RIEN_bm | TWI_WIEN_bm | TWI_ENABLE_bm;
    //TWI0.MCTRLB = TWI_ACKACT_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

int8_t twi_scan()
{
    for (uint8_t x = 0; x < 128; ++x) {
        if (twi_start(x, 0)) {
            return x;
        }
    }

    return -1;
}

uint8_t twi_read()
{
    if (g_I2Ccount != 0) g_I2Ccount--;
    while (!(TWI0.MSTATUS & TWI_RIF_bm));                               // Wait for read interrupt flag
    uint8_t data = TWI0.MDATA;
    // Check slave sent ACK?
    if (g_I2Ccount != 0) TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;           // ACK = more bytes to read
    else TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_RECVTRANS_gc;           // Send NAK
    return data;
}

uint8_t twi_readLast()
{
    g_I2Ccount = 0;
    return twi_read();
}

bool twi_write(uint8_t data)
{
    while (!(TWI0.MSTATUS & TWI_WIF_bm)); // Wait for write interrupt flag
    g_state = 0;
    TWI0.MDATA = data;
    //TWI0.MCTRLB |= TWI_MCMD_RECVTRANS_gc;
    //while (!g_state);
    return !(TWI0.MSTATUS & TWI_RXACK_bm); // Returns true if slave gave an ACK
} 

static uint8_t* writeData = (uint8_t*)1;
static uint8_t* writeDataEnd = (uint8_t*)0;

static inline void twi_writeNextByte()
{
    if (writeData < writeDataEnd) {
        TWI0.MDATA = *writeData;
        ++writeData;
    } else {
        g_state = 1;
    }
}

//bool twi_writeBytes(const uint8_t* data, const uint8_t dataLength)
//{
//    //while (!(TWI0.MSTATUS & TWI_WIF_bm));
//    bool lastStatus = false;
//    writeData = (uint8_t*)data;
//    writeDataEnd = writeData + dataLength;
//    //for (const uint8_t* curData = data; curData < data + dataLength; ++curData) {
//    //    g_state = 0;
//    //    while (!(TWI0.MSTATUS & TWI_WIF_bm));
//    //    TWI0.MDATA = *curData;
//    //    //TWI0.MCTRLB |= TWI_MCMD_RECVTRANS_gc;
//    //    while (!g_state);
//    //}
//    
//    g_state = 0;
//    twi_writeNextByte();
//    
//    while (!g_state);
//    
//    lastStatus = TWI0.MSTATUS & TWI_RXACK_bm;
//    if (lastStatus) return false;
//    return !lastStatus;
//}

bool twi_writeBytes(const uint8_t* data, const uint8_t dataLength)
{
    bool lastStatus = false;
    for (const uint8_t* curData = data; curData < data + dataLength; ++curData) {
        while (!(TWI0.MSTATUS & TWI_WIF_bm));
        TWI0.MDATA = *curData;
        
        lastStatus = TWI0.MSTATUS & TWI_RXACK_bm;
        if (lastStatus) return false;
    }

    return !lastStatus;
}

bool twi_start(uint8_t address, int readcount)
{
    bool read;
    if (readcount == 0) read = 0;
    else { g_I2Ccount = readcount; read = 1; }
    TWI0.MADDR = address<<1 | read;
    while (!(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm)));
    if ((TWI0.MSTATUS & TWI_ARBLOST_bm)) return false;
    return !(TWI0.MSTATUS & TWI_RXACK_bm);
}

bool twi_restart(uint8_t address, int readcount)
{
    return twi_start(address, readcount);
}

void twi_stop()
{
    TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;
}

ISR(TWI0_TWIM_vect) {
    if (TWI0.MSTATUS & TWI_WIF_bm) {
        twi_writeNextByte();
    }    
}