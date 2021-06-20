/*
 * millis.c
 *
 * Created: 5/31/2021 1:04:05 PM
 *  Author: joe
 */ 

#include "millis_tcb.h"
#include "system.h"

#include <avr/interrupt.h>
#include <util/atomic.h>

volatile static uint32_t g_timerMillis;
static TCB_t* g_tcb;

void millis_init(TCB_t* tcb)
{
    // https://www.avrfreaks.net/forum/millis-and-micros-attiny-1-series
    // https://github.com/zkemble/millis/blob/master/arduino/millis/millis.cpp
    // https://www.avrfreaks.net/forum/attiny-412-setting-tcb-timer
    g_tcb = tcb;
    tcb->CCMP = F_CPU / 1000 / 2 - 1; // Overflow after 1 ms, /2 for CLKDIV2.
    tcb->INTCTRL = TCB_CAPT_bm; // Enable overflow interrupt
    tcb->CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
    tcb->CNT = 0;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		g_timerMillis = 0;
	}
    sei();
}

ISR(TCB0_INT_vect)
{
    g_timerMillis++;
    
    // Clear the interrupt flag (to reset TCA0.CNT)
    g_tcb->INTFLAGS = TCB_CAPT_bm;
}

uint32_t millis()
{
    //uint32_t t1;
    //volatile uint8_t* t2 = (volatile uint8_t*)&g_timerMillis;
    //for(;;) {
    //    t1 = g_timerMillis; // get time
    //    if ((uint8_t)t1 == *t2) break; // if byte0 same, good
    //}
    //return t1;
	uint32_t m;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		m = g_timerMillis;
	}
	return m;
}

void millis_restart()
{
    millis_init(g_tcb);
}