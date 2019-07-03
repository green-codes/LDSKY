/*
 * Communication with kRPC
 * rewrite (releveant parts of) the kRPC C-nano interface in OOP
 */

#ifndef COMM_H
#define COMM_H

#include "base.h"
#include "system.hpp"
#include <HardwareSerial.h>

// kRPC libs
#include <krpc.h>
#include <krpc/services/krpc.h>
#include <krpc/services/space_center.h>
#include <MechJeb.h>

namespace Comm
{

// kRPC objects
HardwareSerial *conn;
krpc_SpaceCenter_Control_t control;
krpc_SpaceCenter_Vessel_t vessel;
krpc_SpaceCenter_Flight_t flight;
krpc_SpaceCenter_Orbit_t orbit;
krpc_MechJeb_AscentAutopilot_t mj_ascent;

} // namespace Comm

#endif // COMM_H
