//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "AuthenticationLegacy.h"
#include "CoLaParameterReader.h"
#include "CoLaParameterWriter.h"

namespace visionary {

AuthenticationLegacy::AuthenticationLegacy(VisionaryControl& vctrl) : m_VisionaryControl(vctrl)
{
}

AuthenticationLegacy::~AuthenticationLegacy() = default;

bool AuthenticationLegacy::login(UserLevel userLevel, const std::string& password)
{
  CoLaCommand loginCommand = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "SetAccessMode")
                               .parameterSInt(static_cast<int8_t>(userLevel))
                               .parameterPasswordMD5(password)
                               .build();
  CoLaCommand loginResponse = m_VisionaryControl.sendCommand(loginCommand);

  if (loginResponse.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(loginResponse).readBool();
  }
  return false;
}

bool AuthenticationLegacy::logout()
{
  CoLaCommand runCommand  = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "Run").build();
  CoLaCommand runResponse = m_VisionaryControl.sendCommand(runCommand);

  if (runResponse.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(runResponse).readBool();
  }
  return false;
}

} // namespace visionary
