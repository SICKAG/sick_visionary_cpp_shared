//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "ControlSession.h"
#include "CoLaParameterWriter.h"

namespace visionary {

ControlSession::ControlSession(IProtocolHandler& ProtocolHandler) : m_ProtocolHandler(ProtocolHandler)
{
}

ControlSession::~ControlSession() = default;

CoLaCommand ControlSession::prepareRead(const std::string& varname)
{
  CoLaCommand cmd = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, varname.c_str()).build();
  return cmd;
}

CoLaCommand ControlSession::prepareWrite(const std::string& varname)
{
  CoLaCommand cmd = CoLaParameterWriter(CoLaCommandType::WRITE_VARIABLE, varname.c_str()).build();
  return cmd;
}

CoLaCommand ControlSession::prepareCall(const std::string& varname)
{
  CoLaCommand cmd = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, varname.c_str()).build();
  return cmd;
}

CoLaCommand ControlSession::send(const CoLaCommand& cmd)
{
  // ToDo: send command via CoLaProtocolHandler?
  // ProcolHandler needs to add e.g. header and checksum
  // Afterwards send to socket and receive the response.
  // return the response.
  return m_ProtocolHandler.send(cmd);
}

} // namespace visionary
