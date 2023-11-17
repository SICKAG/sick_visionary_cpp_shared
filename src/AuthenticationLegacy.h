//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once
#include "VisionaryControl.h"

namespace visionary {

class AuthenticationLegacy : public IAuthentication
{
public:
  explicit AuthenticationLegacy(VisionaryControl& vctrl);
  ~AuthenticationLegacy() override;

  bool login(UserLevel userLevel, const std::string& password) override;
  bool logout() override;

private:
  VisionaryControl& m_VisionaryControl;
};

} // namespace visionary
