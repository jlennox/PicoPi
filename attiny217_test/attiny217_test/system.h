/*
 * system.h
 *
 * Created: 5/29/2021 12:01:30 PM
 *  Author: joe
 */ 


#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <avr/io.h>

#define F_CPU 20000000L
//3333333

typedef enum { true = 1, false = 0 } bool;
typedef uint8_t i2c_address_t;

#define PORTA_ADDR &PORTA
#define PORTB_ADDR &PORTB
#define PORTC_ADDR &PORTC

#define VPORTA_ADDR &VPORTA
#define VPORTB_ADDR &VPORTB
#define VPORTC_ADDR &VPORTC

#endif /* SYSTEM_H_ */