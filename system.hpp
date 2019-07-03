/*
 * System core
 */

#ifndef SYSTEM_H
#define SYSTEM_H

// C/C++ standard libs
#include <math.h>
#include <stdarg.h> // variable arguments
#include <stdlib.h>
#include <string.h>

#include "base.h"

#define PORT_ON(port, pin) port |= (1 << pin)
#define PORT_OFF(port, pin) port &= ~(1 << pin)
#define PORT_TOGGLE(port, pin) port ^= (1 << pin)

namespace System
{

/*===== global vars =====*/

// pointer to config struct
CT_Config *conf = NULL; // to be init'd from EEPROM or new instance

// display buffers
// LC_BUF lc_buf[NUM_LC];    // main display buffer
// bool flash_toggle = true; // screen flash control flag (don't modify)
// long last_millis = 0;     // buffer for monitoring flash period

// flags
// TODO

/* ===== system functions ===== */

/*
 * Setup Timer5 (Mega2560) for scheduled interrupts
 */
void timer_init()
{
  cli();

  // make sure Timer/Counter is not disabled by poewr saving
  PRR1 = PRR1 & ~(_BV(PRTIM5));
  PRR1 = PRR1 & ~(_BV(PRTIM4));

  // put Timers into CTC mode (WGM5[3:0] = 0b0100), compare to OCR5A
  TCCR5A = TCCR5A & ~(_BV(WGM51) | _BV(WGM50));
  TCCR5B = (TCCR5B & ~_BV(WGM53)) | _BV(WGM52);
  TCCR4A = TCCR4A & ~(_BV(WGM41) | _BV(WGM40));
  TCCR4B = (TCCR4B & ~_BV(WGM43)) | _BV(WGM42);

  // set prescalers
  TCCR5B = ((TCCR5B | _BV(CS52)) & ~_BV(CS51)) | _BV(CS50);  //1024
  TCCR4B = ((TCCR4B | _BV(CS42)) & ~_BV(CS41)) & ~_BV(CS40); // 256

  // set output compare register
  // NOTE: counter increments 1 per 1024 CPU cycle
  OCR5A = F_CPU / (1024 * INT_FREQ_1);
  OCR4A = F_CPU / (256 * INT_FREQ_2);

  // enable Timer5 compare match interrupt A
  TIMSK5 = TIMSK5 | _BV(OCIE5A);
  TIMSK4 = TIMSK4 | _BV(OCIE4A);

  sei();
}
void timer_sleep() // NOTE: Mega2560 specific
{
  PRR1 = PRR1 | _BV(PRTIM5);
  PRR1 = PRR1 | _BV(PRTIM4);
}
void timer_wake() // NOTE: Mega2560 specific
{
  PRR1 = PRR1 & ~(_BV(PRTIM5));
  PRR1 = PRR1 & ~(_BV(PRTIM4));
}

// reading from / saving to persistent configs in flash
void read_config()
{
  // TODO
}
void write_config()
{
  // TODO
}

void handle_exi() // handle external interrupt
{
  // TODO
}

void reset_system()
{
  // NOTE: the AVR watchdog timer, once enabled, forces hard system reset
  //       after the watchdog timer is triggered.
  wdt_enable(WDTO_15MS);
}

} // namespace System

#endif // SYSTEM_H