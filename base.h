/*
 * Definitions and configs
 */

#ifndef BASE_H
#define BASE_H

// Arduino libs
#include <Arduino.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include <avr/io.h>

#include "data.h"

/*===== global configs =====*/
#define DEBUG 1
#define SERIAL_ENABLE 1
#define SERIAL_RATE 115200
#define RESET_EEPROM 0
#define RESET_CONF 0

/* ===== System timer ===== */
#define INT_FREQ_1 4  // screen update frequency, in Hz
#define INT_FREQ_2 20 // keyboard update frequency

/* ===== Misc pin configs ===== */
#define TONE_PIN 53

/* ===== LED configs ===== */
#define LED_ACT LED_BUILTIN
#define LED_ACT_PORT PORTB
#define LED_ACT_PIN PORTB7
#define NUM_LED 8
// #define LED_ACT PB7 // comp activity (using low level toggle)
// #define LED_WAIT PJ1 // waiting for input (TODO: using low level toggle)
#define LED_UPLK 1  // TODO: uplink
#define LED_KYRL 40 // TODO: key release
#define LED_PGER 41 // TODO: program error
#define LED_OPER 42 // TODO: operator error
#define LED_STBY 43 // TODO: standby
#define LED_HI_G 44 // TODO: high-G alarm
#define LED_HEAT 45 // TODO: heat alarm
#define LED_CHUT 46 // TODO: chute deployed
const byte LED_PINS[NUM_LED] =
    {LED_UPLK,
     LED_KYRL,
     LED_PGER,
     LED_OPER,
     LED_STBY,
     LED_HI_G,
     LED_HEAT,
     LED_CHUT};
enum LED_PINS_P
{
  LED_UPLK_P,
  LED_KYRL_P,
  LED_PGER_P,
  LED_OPER_P,
  LED_STBY_P,
  LED_HI_G_P,
  LED_HEAT_P,
  LED_CHUT_P
};

/* ===== display configs ===== */
#define DEFAULT_DELAY_TIME 1000
#define LC_LUM 1 // default luminosity
#define NUM_LC 4
// #define LC_DIN 12
#define LC_CS 10
// #define LC_CLK 11
#define LC_ROW_LEN 8
#define LC_FLASH_DELAY 400 // in milliseconds

/* ===== Tone configs ===== */
#define TONE_PIN 53
#define TONE_KEY_FREQ 440
#define TONE_KEY_DELAY 10
#define TONE_CONFIRM_FREQ 880

/* ===== keypad configs ===== */
const byte KEYPAD_ROWS = 4,
           KEYPAD_COLS = 4;
const char KEYPAD_KEYS[KEYPAD_ROWS][KEYPAD_COLS] = { // Define the Keymap
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'.', '0', '-', 'D'}};
const byte KEYPAD_ROW_PINS[KEYPAD_ROWS] = {A11, A10, A9, A8};   // TODO
const byte KEYPAD_COL_PINS[KEYPAD_COLS] = {A15, A14, A13, A12}; // TODO

/* ===== sysutils configs ===== */
// input key bindings
#define M_UP_KEY 'A'
#define M_DOWN_KEY 'B'
#define M_ENTR_KEY 'C'
#define M_EXIT_KEY 'D'
// sysmanager
#define PVN_PGM 0
#define PVN_VERB 1
#define PVN_NOUN 2
#define ERR_NULL 0
#define ERR_PGM -1
#define ERR_OPR -2

/* ===== data structure definitions ===== */

typedef struct CT_Config
{
  // system configs
  bool tone_en = 1;
  bool splash = 0;
  bool fancy = 0;
  int fancy_delay = 50; // in milliseconds
  // app configs
  int placeholder;
} CT_Config;
const uint16_t CONFIG_ADDRESS = 0x0;
const int CONFIG_LEN = sizeof(CT_Config);

#endif // BASE_H
