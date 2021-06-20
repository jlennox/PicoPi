/*
 * millis.h
 *
 * Created: 5/31/2021 1:04:14 PM
 *  Author: joe
 */ 


#ifndef MILLIS_H_
#define MILLIS_H_

#include <avr/io.h>

void millis_init(TCB_t* tcb);
uint32_t millis();
void millis_restart();

#endif /* MILLIS_H_ */