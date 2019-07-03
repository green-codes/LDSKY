/*
 * Programs
 */

#ifndef PROGRAMS_H
#define PROGRAMS_H

#include "sysutils.hpp"
#include "verbs.hpp"

namespace Programs
{

/*===== programs implementations =====*/
// NOTE: programs should correspond to kRPC.MechJeb autopilot functions;
//       they can update the display/status buffers, call verbs, etc;
//       use the stage information in pgm_dict to manage execution
// NOTE: remember to add implementations to the init list!

// NOTE: program main function template: int pgm(int *p_stage, void* data_ptr)
//       the program is responsible for allocating its own persistent storage
// NOTE: the program MUST initialize persistent storage to keep track of its
//       stage; it also must NOT allocate memory elsewhere from data_ptr

//

// playground program
int program_01(int *p_stage, void **data_ptr)
{
  if (*p_stage == 0)
  {
    *p_stage = 1;
    *data_ptr = calloc(1, sizeof(int));
    return SysUtils::SysManager::P_RUN;
  }
  if (*p_stage == 1)
  {
    // request verb/noun switch
    SysUtils::sys->request_vn(0, 0, true);
    *p_stage = 2;
  }
  if (*p_stage == 2)
  {
    // if request accepted, proceed to next stage
    if (SysUtils::sys->keyrel_req == SysUtils::SysManager::KYRL_ACC)
    {
      *p_stage = 3;
      return SysUtils::SysManager::P_RUN;
    }
    // if rejected throw program error
    else if (SysUtils::sys->keyrel_req == SysUtils::SysManager::KYRL_REJ)
      return SysUtils::SysManager::P_PGM_ERR;
    else // wait
      return SysUtils::SysManager::P_RUN;
  }
  if (*p_stage == 3)
  {
    *((int *)*data_ptr) += 1;
    if (0 == SysUtils::sys->get_verb()) // avoid conflict with verbs
      Devices::lcd->setInt(1, *((int *)*data_ptr));
    if (0 != SysUtils::sys->get_verb())
      SysUtils::sys->request_vn(0, 0);
    return SysUtils::SysManager::P_RUN;
  }
  return SysUtils::SysManager::P_COMPLETE; // ???
}

void init_programs()
{
  SysUtils::sys->register_program(0, NULL);
  SysUtils::sys->register_program(1, &program_01);
}

} // namespace Programs

#endif // PROGRAMS_H
