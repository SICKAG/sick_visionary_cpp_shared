//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "CoLaCommandType.h"
#include "CoLaError.h"

namespace visionary {

class CoLaCommand
{
public:
  using ByteBuffer = std::vector<std::uint8_t>;

  CoLaCommand();
  ~CoLaCommand();

  /// Construct a new CoLaCommand
  /// from the given data buffer
  CoLaCommand(const ByteBuffer& buffer);

  /// Get the binary data buffer.
  const ByteBuffer& getBuffer() const;

  /// Get the type of command.
  CoLaCommandType::Enum getType() const;

  /// Get the name of command.
  const char* getName() const;

  /// Get offset in bytes to where first parameter starts.
  std::size_t getParameterOffset() const;

  /// Get error.
  CoLaError::Enum getError() const;

  /// Create a command for network errors.
  static CoLaCommand errorCommand();

  /// Create a command for network errors.
  static CoLaCommand networkErrorCommand();

private:
  ByteBuffer            m_buffer;
  CoLaCommandType::Enum m_type;
  std::string           m_name;
  std::size_t           m_parameterOffset;
  CoLaError::Enum       m_error;

  /// Construct a new CoLaCommand with the given command type, error, and name, but without any data.
  CoLaCommand(CoLaCommandType::Enum commandType, CoLaError::Enum error, const std::string& name);

  /// Decode a CoLa command from a given buffer
  bool fromBuffer(const ByteBuffer& buffer);
};

} // namespace visionary
