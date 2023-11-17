//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include "CoLaCommand.h"
#include <cstdint>

namespace visionary {

class IProtocolHandler
{
public:
  virtual ~IProtocolHandler()                                      = default;
  virtual bool        openSession(uint8_t sessionTimeout /*secs*/) = 0;
  virtual void        closeSession()                               = 0;
  virtual CoLaCommand send(CoLaCommand cmd)                        = 0;
};

} // namespace visionary
