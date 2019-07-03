/*
 * DSKY project main file
 */

#include "sysutils.hpp"
#include "verbs.hpp"
#include "programs.hpp"
#include "isr.hpp"

void setup()
{
  MCUSR = 0; // clear watchdog timer flags

  if (SERIAL_ENABLE) // initialize serial interface
    Serial.begin(SERIAL_RATE);
  else // prepare to set up kRPC connection
  {
    Comm::conn = (HardwareSerial *)&Serial;
    krpc_connection_config_t config;
    config.speed = 115200;
    config.config = SERIAL_8N1;
    krpc_open(&Comm::conn, &config);
    krpc_connect(Comm::conn, "LDSKY");
  }

  // init keypad
  Devices::keypad = new Devices::Keypad_I;

  // init MAX7219 displays
  Devices::lcd = new Devices::LC_Display;

  // init status lights
  Devices::status_led = new Devices::StatusDisplay;

  // init sys
  SysUtils::sys = new SysUtils::SysManager;

  // init verbs and programs
  Verbs::init_verbs();
  Programs::init_programs();

  // TODO: read saved system state from EEPROM if so configured

  // setup scheduled interrupts for system updates
  System::timer_init();
}

void loop()
{

  SysUtils::sys->update();

  // Devices::lcd->setUL(1, 1234567890L, true);

  // delay(400);
}
