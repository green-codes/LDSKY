/*
 * VERB definitions
 * References the Apollo AGC Verb list
 */

#ifndef VERB_H
#define VERB_H

#include "system.hpp"
#include "comm.hpp"
#include "devices.hpp"
#include "sysutils.hpp"

namespace Verbs
{

/* ===== VERB implementations ===== */
// NOTE: remember to add implementations to the init list!
// NOTE: verb return values: use SysUtils::SysManager::verb_status_t

// 16: display time elapsed since LDSKY bootup (h:m:s)
int verb_16(int *p_stage, void **pp_data)
{
  if (*p_stage == 0)
  {
    Devices::lcd->clearDataRows();
    *p_stage = 1;
  }
  long s = millis() / 1000;
  Devices::lcd->setInt(1, s / 3600);
  Devices::lcd->setInt(2, (s / 60) % 60);
  Devices::lcd->setInt(3, s % 60);
  delay(50); // don't work too hard for a clock...
  return SysUtils::SysManager::V_RUN;
}

// 27: display memory location
// verb 01:hex; 02:dec
struct verb_27_data
{
  unsigned long addr;
  bool hex;
};
int verb_27(int *p_stage, void **pp_data)
{
  if (*p_stage == 0)
  {
    *pp_data = calloc(1, sizeof(verb_27_data));
    verb_27_data *d = (verb_27_data *)*pp_data;
    // get mode
    if (SysUtils::sys->get_noun() == 1)
      d->hex = true;
    else if (SysUtils::sys->get_noun() == 2)
      d->hex = false;
    else
      return SysUtils::SysManager::V_OPR_ERR;
    // init input window
    SysUtils::sys->input_window_open(&(d->addr), 1, 0, LC_ROW_LEN,
                                     SysUtils::InputWindow::IW_UL, false);
    *p_stage = 1;
    return SysUtils::SysManager::V_RUN;
  }
  if (*p_stage == 1)
  {
    Devices::lcd->setUpdateAll(false);
    if (SysUtils::sys->iw_->status_ == SysUtils::InputWindow::IW_COMPLETE)
    {
      SysUtils::sys->input_window_close();
      Devices::lcd->setUpdateAll(true);
      *p_stage = 2;
      return SysUtils::SysManager::V_RUN; // go to next stage
    }
    else if (SysUtils::sys->iw_->status_ == SysUtils::InputWindow::IW_EXIT)
    {
      SysUtils::sys->input_window_close();
      Devices::lcd->setUpdateAll(true);
      return SysUtils::SysManager::V_COMPLETE; // user exit, stop verb
    }
    else
      return SysUtils::SysManager::V_RUN; // wait
  }
  if (*p_stage == 2)
  {
    verb_27_data *d = (verb_27_data *)*pp_data;
    uint32_t *disp_ptr = (uint32_t *)d->addr;
    Devices::lcd->setUL(1, disp_ptr[0], d->hex);
    Devices::lcd->setUL(2, disp_ptr[1], d->hex);
    Devices::lcd->setUL(3, disp_ptr[2], d->hex);
    return SysUtils::SysManager::V_RUN;
  }
  return SysUtils::SysManager::V_COMPLETE; // how'd we get here?
}

// 36: update information from kRPC
int verb_36(int *p_stage, void **pp_data)
{
  using namespace Comm;
  krpc_SpaceCenter_ActiveVessel(conn, vessel);
  krpc_SpaceCenter_Vessel_Control(conn, control, vessel);
  // TODO

  // TODO: update all displays

  return SysUtils::SysManager::V_COMPLETE;
}

// 37: set up the selected program for launch in the next ISR
// NOTE: verb 37 always kills the old program
int verb_37(int *p_stage, void **pp_data)
{
  if (0 == SysUtils::sys->set_program(SysUtils::sys->get_noun(), true))
    return SysUtils::SysManager::V_COMPLETE;
  else // program invalid, operator error
    return SysUtils::SysManager::V_OPR_ERR;
}

// 69: hard-reset system
int verb_69(int *p_stage, void **pp_data)
{
  System::reset_system();
  return SysUtils::SysManager::V_COMPLETE;
}

// stage the current vessel
int verb_99(int *p_stage, void **pp_data)
{
  using namespace Comm;
  krpc_SpaceCenter_Control_ActivateNextStage(conn, vessel, control);
  return SysUtils::SysManager::V_COMPLETE;
}

// verb dict initializer
void init_verbs()
{
  SysUtils::sys->register_verb(0, NULL, false);
  SysUtils::sys->register_verb(16, &verb_16, false);
  SysUtils::sys->register_verb(27, &verb_27, true);
  SysUtils::sys->register_verb(36, &verb_36, false);
  SysUtils::sys->register_verb(37, &verb_37, true);
  SysUtils::sys->register_verb(69, &verb_69, false);
  SysUtils::sys->register_verb(99, &verb_99, false);
}

} // namespace Verbs

#endif // VERB_H
