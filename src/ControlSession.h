//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once
#include "CoLaCommand.h"
#include "IProtocolHandler.h"
#include <string>

namespace visionary {

class ControlSession
{
public:
  ControlSession(IProtocolHandler& ProtocolHandler);
  virtual ~ControlSession();

  // void login(IAuthentication::UserLevel userLevel, const std::string& password);
  // void logout();

  CoLaCommand prepareRead(const std::string& varname);
  CoLaCommand prepareWrite(const std::string& varname);
  CoLaCommand prepareCall(const std::string& varname);

  CoLaCommand send(const CoLaCommand& cmd);

private:
  IProtocolHandler& m_ProtocolHandler;
};

} // namespace visionary
