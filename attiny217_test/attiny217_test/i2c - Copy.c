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

static int g_I2Ccount;

// 400kHz clock
//uint32_t const FREQUENCY = 400000L;  // Hardware I2C clock in Hz
//uint32_t const T_RISE = 300L;        // Rise time

// Choose these for 1MHz clock
uint32_t const FREQUENCY = 1000000L; // Hardware I2C clock in Hz
uint32_t const T_RISE = 120L;        // Rise time

void twi_init()
{
    uint32_t baud = (F_CPU / FREQUENCY - F_CPU / 1000 / 1000 * T_RISE / 1000 - 10) / 2;
    TWI0.MBAUD = (uint8_t)baud;
    TWI0.MCTRLA = TWI_ENABLE_bm | TWI_SMEN_bm | TWI_TIMEOUT_DISABLED_gc;          // Enable as master, no interrupts
    TWI0.MCTRLB = TWI_ACKACT_bm;
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
    while (!(TWI0.MSTATUS & TWI_WIF_bm));                               // Wait for write interrupt flag
    TWI0.MDATA = data;
    TWI0.MCTRLB |= TWI_MCMD_RECVTRANS_gc;                                // Do nothing
    return !(TWI0.MSTATUS & TWI_RXACK_bm);                              // Returns true if slave gave an ACK
}

bool twi_writeBytes(const uint8_t* data, const uint8_t dataLength)
{
    bool lastStatus = false;
    for (const uint8_t* curData = data; curData < data + dataLength; ++curData) {
        while (!(TWI0.MSTATUS & TWI_WIF_bm));
        TWI0.MDATA = *curData;
        TWI0.MCTRLB |= TWI_MCMD_RECVTRANS_gc;
    }
    
    lastStatus = TWI0.MSTATUS & TWI_RXACK_bm;
    if (lastStatus) return false;
    return !lastStatus;
}

// Start transmission by sending address
bool twi_start(uint8_t address, int readcount)
{
    bool read;
    if (readcount == 0) read = 0;                                       // Write
    else { g_I2Ccount = readcount; read = 1; }                          // Read
    TWI0.MADDR = address<<1 | read;                                     // Send START condition
    while (!(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm)));                // Wait for write or read interrupt flag
    if ((TWI0.MSTATUS & TWI_ARBLOST_bm)) return false;                  // Return false if arbitration lost or bus error
    return !(TWI0.MSTATUS & TWI_RXACK_bm);                              // Return true if slave gave an ACK
}

bool twi_restart(uint8_t address, int readcount)
{
    return twi_start(address, readcount);
}

void twi_stop()
{
    TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;                     // Send STOP
}

typedef enum {
    TWI_SUCCESS,
    TWI_BUSY,
    TWI_FAILURE_CLOSE
} twi_result_t;

//static bool g_bufferFree = true;
//static uint8_t* g_bufferPtr = 0;
//static size_t g_bufferLength = 0;
//
//twi_result_t twi_writeBytes(i2c_address_t address, void *data, size_t len)
//{
//	/* timeout is used to get out of twim_release, when there is no device connected to the bus*/
//	uint16_t timeout = 1000;
//
//	while (!twi_start(address, 0) && --timeout)
//		; // sit here until we get the bus..
//        
//	if (!timeout) return TWI_BUSY;
//    
//	g_bufferPtr = data;
//    g_bufferLength = len;
//    
//	//I2C_0_set_address_nack_callback(i2c_cb_restart_write, NULL); // NACK polling?
//
//
//	//if (!I2C_0_status.busy) {
//		//I2C_0_status.busy = true;
//
//		//I2C_0_status.state = I2C_SEND_ADR_WRITE;
//		//I2C_0_master_isr();
//
//		//I2C_0_poller();
//	//}
//    
//	timeout = 1000;
//	//while (I2C_BUSY == twi_close() && --timeout)
//    twi_stop();
//		; // sit here until finished.
//	if (!timeout)
//		return TWI_FAILURE_CLOSE;
//
//	return TWI_SUCCESS;
//}