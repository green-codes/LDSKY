/*
 * ISR definitions
 * defines interrupts for handling keypad and screen updates
 */

#ifndef ISR_H
#define ISR_H

#include "devices.hpp"

/* ===== scheduled system interrupt handling ===== */

ISR(TIMER4_COMPA_vect) // keypad update
{
  Devices::keypad->ISRUpdate();
}

ISR(TIMER5_COMPA_vect) // screen and LED update (from screen buffer)
{
  // Devices::status_led->setActivityLED(true); // turn on activity LED

  Devices::lcd->ISRUpdate();        // update main display from buffer
  Devices::status_led->ISRUpdate(); // update LEDs from buffer

  // Devices::status_led->setActivityLED(false); // turn off activity LED
}

#endif // ISR_H
