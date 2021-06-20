/*
 * i2c.h
 *
 * Created: 5/29/2021 11:59:17 AM
 *  Author: joe
 */ 


#ifndef I2C_H_
#define I2C_H_

#include <avr/io.h>

void twi_init();
int8_t twi_scan();
uint8_t twi_read();
uint8_t twi_readLast();
bool twi_write(uint8_t data);
bool twi_writeBytes(const uint8_t* data, const uint8_t dataLength);
bool twi_start(uint8_t address, int readcount);
bool twi_restart(uint8_t address, int readcount);
void twi_stop();

#endif /* I2C_H_ */