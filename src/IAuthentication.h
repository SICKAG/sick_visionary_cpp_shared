//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once
#include <string>

namespace visionary {

class IAuthentication
{
public:
  /// Available CoLa user levels.
  enum class UserLevel : int8_t
  {
    RUN               = 0,
    OPERATOR          = 1,
    MAINTENANCE       = 2,
    AUTHORIZED_CLIENT = 3,
    SERVICE           = 4
  };

  virtual ~IAuthentication() = default;

  virtual bool login(UserLevel userLevel, const std::string& password) = 0;
  virtual bool logout()                                                = 0;
};

} // namespace visionary
