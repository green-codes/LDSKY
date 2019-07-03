/*
 * higher level driver modules
 * Note: add device-specific libraries here
 */

#ifndef DEVICES_H
#define DEVICES_H

#include "base.h"
#include "system.hpp"

// hardware libs
#include <Key.h>
#include <Keypad.h> // parallel
#include "LedControl.h"

namespace Devices
{

/* ===== interrupt-based keypad class ===== */

class Keypad_I
{
private:
  Keypad *kpd; // instance of the basic non-blocking Keypad class
  char k;      // key buffer
  bool k_read; // set true to enable key buffer update
public:
  Keypad_I()
  {
    kpd = new Keypad(makeKeymap(KEYPAD_KEYS), KEYPAD_ROW_PINS,
                     KEYPAD_COL_PINS, KEYPAD_ROWS, KEYPAD_COLS);
    k = 0;
    k_read = true; // ready to be updated
  }
  // get the key stored in the key buffer and set ready for update
  // NOTE: should be called by the UI
  char getKeyEvent()
  {
    if (!k_read) // key has not been read by another program
    {
      k_read = true;
      return k;
    }
    else //  another program has already read the key (no new key event)
      return 0;
  }
  // update the key buffer
  // NOTE: should be called by a timer interrupt
  void ISRUpdate()
  {
    char k_temp = kpd->getKey();
    if (k_read && k_temp != 0) // key has been read, enable key update
    {                          // NOTE: only update if k != 0
      k = k_temp;              // register key event
      k_read = false;
    }
  }
};
Keypad_I *keypad; // pointer to a keypad instance

/* ===== display helper classes ===== */

class LC_Display
{
private:
  LedControl *lc;       // instance of the LedControl driver class
  typedef struct LC_BUF // struct for display buffers
  {
    bool update = true;                   // update flag
    bool flash = false;                   // flashing flag
    char c_buf[LC_ROW_LEN * 2] = {0};     // row buffer
    bool char_mask[LC_ROW_LEN] = {false}; // clear mask to allow update
    bool dot_buf[LC_ROW_LEN] = {false};   // set to display dots
  } LC_BUF;
  LC_BUF lc_buf[NUM_LC]; // main display buffer
  bool flash_toggle;     // screen flash control flag
  long last_millis;      // buffer for monitoring flash period

public:
  // TODO: allow the user to specify number of rows; need dynamic alloc
  LC_Display()
  {
    lc = new LedControl(LC_CS, NUM_LC);
    flash_toggle = true;
    last_millis = 0;
    for (int i = 0; i < NUM_LC; i++)
    {
      lc->shutdown(i, false);
      lc->setIntensity(i, LC_LUM);
      lc->clearDisplay(i);
      clear(i);
    }
  }

  // print a string to the specified position on display
  void printStr(int addr, char *buf, int offset, int len,
                bool *char_mask, bool *dot_buf)
  {
    len = (offset + len > LC_ROW_LEN) ? LC_ROW_LEN - offset : len;
    for (int i = 0; i < len; i++)
    {
      if (char_mask && char_mask[i])
        continue; // if mask set, skip updating current position
      int idx = LC_ROW_LEN - 1 - (i + offset);
      lc->setChar(addr, idx, buf[i], dot_buf ? dot_buf[i] : false);
      // NOTE: above -- setting dp to false does not prevent printing '.'
    }
  }
  // print a row from the display buffer
  void update(int addr)
  {
    printStr(addr, lc_buf[addr].c_buf, 0, LC_ROW_LEN,
             lc_buf[addr].char_mask, lc_buf[addr].dot_buf);
    if (lc_buf[addr].flash) // toggle display if flashing
      lc->shutdown(addr, flash_toggle);
  }
  // update the entire display
  // NOTE: call this from a screen update ISR
  void ISRUpdate()
  {
    // update main display from buffer
    for (int i = 0; i < NUM_LC; i++)
      if (lc_buf[i].update)
        update(i);
    // flip flash toggle
    long curr_millis = millis();
    if (curr_millis - last_millis > LC_FLASH_DELAY)
    {
      last_millis = curr_millis;
      flash_toggle = flash_toggle ? false : true;
    }
  }

  // clear the display buffer of a row
  void clear(int addr)
  {
    memset(lc_buf[addr].c_buf, ' ', LC_ROW_LEN);
    memset(lc_buf[addr].dot_buf, false, LC_ROW_LEN);
  }
  // clear all data rows
  void clearDataRows()
  {
    for (int i = 1; i < NUM_LC; i++)
      clear(i);
  }

  // write formatted numbers to display buffers
  void setDouble(int addr, double num)
  {
    // print the sign
    // NOTE: if num in (-1,1), put the dot directly after the sign
    lc_buf[addr].c_buf[0] = (num >= 0) ? ' ' : '-';
    lc_buf[addr].dot_buf[0] = (num < 1 && num > -1) ? true : false;
    num = num >= 0 ? num : -num; // flip to print the abs

    // print the rest of the number using a string buffer
    char buf[LC_ROW_LEN * 2];
    dtostrf(num, LC_ROW_LEN, LC_ROW_LEN - 1, buf);
    int digits_printed = 1, buf_i = 0;
    if (buf[0] == '0')
      buf_i += 2; // omit '0.' to get more precision
    while (digits_printed < LC_ROW_LEN)
    {
      bool print_dot = buf_i + 1 < strlen(buf) && buf[buf_i + 1] == '.';
      lc_buf[addr].dot_buf[digits_printed] = print_dot;
      lc_buf[addr].c_buf[digits_printed] = buf[buf_i];

      digits_printed += 1;
      buf_i += print_dot ? 2 : 1;
    }
  }
  void setInt(int addr, long num)
  {
    lc_buf[addr].c_buf[0] = (num >= 0) ? ' ' : '-';
    num = num >= 0 ? num : -num; // flip to print the abs
    snprintf(lc_buf[addr].c_buf + 1, LC_ROW_LEN, "%07ld", num);
  }
  void setUL(int addr, unsigned long num, bool hex)
  {
    snprintf(lc_buf[addr].c_buf, LC_ROW_LEN + 1,
             (hex ? "%08lx" : "%08lu"), num);
  }
  void setUL(int addr, uint16_t num)
  {
    setUL(addr, num, false);
  }

  // write buffer for the program, verb and noun display
  // NOTE: requires arrays of size 3
  void setPVN(int addr, int *pgm_buf)
  {
    for (int i = 0; i < 3; i++)
    {
      char buf[3];
      if (i > 0 && pgm_buf[i] == 0)
        sprintf(buf, "--");
      else
        sprintf(buf, "%02.2d", pgm_buf[i]);
      memcpy(lc_buf[addr].c_buf + i * 3, buf, 2);
    }
  }

  // set the flashing flag for a row
  void setFlash(int addr, bool enable)
  {
    if (enable)
      lc_buf[addr].flash = true;
    else
    {
      lc_buf[addr].flash = false;
      lc->shutdown(addr, false); // ensure display is on
    }
  }

  // freeze/unfreeze rows
  void setUpdate(int addr, bool update)
  {
    lc_buf[addr].update = update;
  }
  void setUpdateAll(bool update)
  {
    for (int i = 1; i < NUM_LC; i++)
      lc_buf[i].update = update;
  }
};
LC_Display *lcd; // LED Control Display :/

/* ===== status lights ===== */

class StatusDisplay
{
private:
  bool led_status_[NUM_LED] = {0};

public:
  StatusDisplay()
  {
    pinMode(LED_ACT, OUTPUT); // activity LED
    for (int i = 0; i < NUM_LED; i++)
    {
      pinMode(LED_PINS[i], OUTPUT);
      digitalWrite(LED_PINS[i], LOW);
    }
  }
  // set an individual LED
  void setStatus(int addr, bool enable)
  {
    if (addr < 0 || addr >= NUM_LED)
      return;
    led_status_[addr] = enable;
  }
  // clear all status LEDs
  void clear()
  {
    for (int i = 0; i < NUM_LED; i++)
      led_status_[i] = false;
  }
  // clear error LEDs
  void clearError()
  {
    led_status_[LED_PGER_P] = false;
    led_status_[LED_OPER_P] = false;
  }
  // call from ISR
  void ISRUpdate()
  {
    for (int i = 0; i < NUM_LED; i++)
      digitalWrite(LED_PINS[i], led_status_[i] ? HIGH : LOW);
  }
  void setActivityLED(bool enable)
  {
    if (enable)
      PORT_ON(LED_ACT_PORT, LED_ACT_PIN);
    else
      PORT_OFF(LED_ACT_PORT, LED_ACT_PIN);
  }
};
StatusDisplay *status_led;

}; // namespace Devices

#endif