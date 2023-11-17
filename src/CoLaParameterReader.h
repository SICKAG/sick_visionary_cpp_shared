//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include "CoLaCommand.h"
#include <cstdint>
#include <string>

namespace visionary {

/// <summary>
/// Class for reading data from a <see cref="CoLaCommand" />.
/// </summary>
class CoLaParameterReader
{
private:
  void        checkSize(size_t pos, size_t size);
  CoLaCommand m_command;
  size_t      m_currentPosition;

public:
  CoLaParameterReader(CoLaCommand command);
  ~CoLaParameterReader();

  /// <summary>
  /// Rewind the position to the first parameter.
  /// </summary>
  void rewind();

  /// <summary>
  /// Read a signed short int (8 bit, range [-128, 127]) and advances position by 1 byte.
  /// Throws an std::out_of_range exception if CoLaCommand is invalid
  /// </summary>
  int8_t readSInt();

  /// <summary>
  /// Read a unsigned short int (8 bit, range [0, 255]) and advances position by 1 byte.
  /// Throws an std::out_of_range exception if CoLaCommand is invalid
  /// </summary>
  uint8_t readUSInt();

  /// <summary>
  /// Read a signed int (16 bit, range [-32768, 32767]) and advances position by 2 bytes.
  /// Throws an std::out_of_range exception if CoLaCommand is invalid
  /// </summary>
  int16_t readInt();

  /// <summary>
  /// Read a unsigned int (16 bit, range [0, 65535]) and advances position by 2 bytes.
  /// Throws an std::out_of_range exception if CoLaCommand is invalid
  /// </summary>
  uint16_t readUInt();

  /// <summary>
  /// Read a signed double int (32 bit) and advances position by 4 bytes.
  /// Throws an std::out_of_range exception if CoLaCommand is invalid
  /// </summary>
  int32_t readDInt();

  /// <summary>
  /// Read a unsigned int (32 bit) and advances position by 4 bytes.
  /// Throws an std::out_of_range exception if CoLaCommand is invalid
  /// </summary>
  uint32_t readUDInt();

  /// <summary>
  /// Read a IEEE-754 single precision (32 bit) and advances position by 4 bytes.
  /// Throws an std::out_of_range exception if CoLaCommand is invalid
  /// </summary>
  float readReal();

  /// <summary>
  /// Read a IEEE-754 double precision (64 bit) and advances position by 8 bytes.
  /// Throws an std::out_of_range exception if CoLaCommand is invalid
  /// </summary>
  double readLReal();

  /// <summary>
  /// Read a boolean and advance the position by 1 byte.
  /// Throws an std::out_of_range exception if CoLaCommand is invalid
  /// </summary>
  bool readBool();

  /// <summary>
  /// Read a flex string, and advance position according to string size.
  /// Throws an std::out_of_range exception if CoLaCommand is invalid
  /// </summary>
  std::string readFlexString();
};

} // namespace visionary
