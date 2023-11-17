//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

namespace visionary {

// Available CoLa command types.
namespace CoLaCommandType {
enum Enum
{
  NETWORK_ERROR = -2,
  UNKNOWN       = -1,
  READ_VARIABLE,
  READ_VARIABLE_RESPONSE,
  WRITE_VARIABLE,
  WRITE_VARIABLE_RESPONSE,
  METHOD_INVOCATION,
  METHOD_RETURN_VALUE,
  COLA_ERROR
};
}

} // namespace visionary
